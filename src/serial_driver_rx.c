#include "serial_driver/serial_driver_internal.h"

serial_driver_error_t serial_driver_receive_from_device_fifo(
    serial_descriptor_t descriptor, size_t max_bytes,
    size_t *out_bytes_received) {
  serial_descriptor_entry_t *entry = NULL;
  uart_byte_fifo_t *fifo = NULL;
  uart_error_t queue_error = UART_ERROR_NONE;
  size_t bytes_received = 0U;
  uint8_t byte = 0U;

  if (out_bytes_received == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  *out_bytes_received = 0U;

  if (!serial_driver_common_initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  entry = serial_driver_get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }
  if (entry->mode != UART_PORT_MODE_SERIAL) {
    return SERIAL_DRIVER_ERROR_NOT_CONFIGURED;
  }

  fifo = &uart_fifo_map.read_fifos[serial_driver_descriptor_index(descriptor)];

  while (bytes_received < max_bytes) {
    if (entry->rx_staged_word_bytes == sizeof(uint32_t)) {
      queue_error = serial_queue_push(&entry->uart_device->rx_queue,
                                      entry->rx_staged_word);
      if (queue_error == UART_ERROR_FIFO_FULL) {
        break;
      }
      if (queue_error != UART_ERROR_NONE) {
        return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
      }

      entry->rx_staged_word = 0U;
      entry->rx_staged_word_bytes = 0U;
      continue;
    }

    if (serial_driver_byte_fifo_is_empty(fifo)) {
      break;
    }

    byte = serial_driver_byte_fifo_pop(fifo);
    entry->rx_staged_word |= ((uint32_t)byte)
                             << (8U * entry->rx_staged_word_bytes);
    entry->rx_staged_word_bytes += 1U;
    bytes_received += 1U;
  }

  if (entry->rx_staged_word_bytes == sizeof(uint32_t)) {
    queue_error =
        serial_queue_push(&entry->uart_device->rx_queue, entry->rx_staged_word);
    if (queue_error == UART_ERROR_NONE) {
      entry->rx_staged_word = 0U;
      entry->rx_staged_word_bytes = 0U;
    } else if (queue_error != UART_ERROR_FIFO_FULL) {
      return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
    }
  }

  *out_bytes_received = bytes_received;
  return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_poll(serial_descriptor_t descriptor,
                                         size_t max_tx_bytes,
                                         size_t max_rx_bytes,
                                         size_t *out_tx_bytes_transmitted,
                                         size_t *out_rx_bytes_received) {
  serial_descriptor_entry_t *entry = NULL;
  serial_driver_error_t status = SERIAL_DRIVER_OK;

  if (out_tx_bytes_transmitted == NULL || out_rx_bytes_received == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }

  *out_tx_bytes_transmitted = 0U;
  *out_rx_bytes_received = 0U;

  entry = serial_driver_get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  status = serial_driver_transmit_to_device_fifo(descriptor, max_tx_bytes,
                                                 out_tx_bytes_transmitted);
  if (status != SERIAL_DRIVER_OK) {
    return status;
  }

  if (entry->staged_word_bytes != 0U ||
      serial_queue_size(&entry->uart_device->tx_queue) != 0U) {
    return SERIAL_DRIVER_OK;
  }

  status = serial_driver_receive_from_device_fifo(descriptor, max_rx_bytes,
                                                  out_rx_bytes_received);
  if (status != SERIAL_DRIVER_OK) {
    return status;
  }

  return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_read(serial_descriptor_t descriptor,
                                         uint8_t *data, size_t length,
                                         size_t *out_bytes_read) {
  serial_descriptor_entry_t *entry = NULL;
  size_t bytes_read = 0U;
  uint32_t word = 0U;
  uart_error_t queue_error = UART_ERROR_NONE;

  if (out_bytes_read == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  *out_bytes_read = 0U;

  if (length > 0U && data == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }

  if (!serial_driver_common_initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  entry = serial_driver_get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }
  if (entry->mode != UART_PORT_MODE_SERIAL) {
    return SERIAL_DRIVER_ERROR_NOT_CONFIGURED;
  }

  while (bytes_read < length) {
    if (entry->rx_output_staged_word_bytes > 0U) {
      data[bytes_read] = (uint8_t)(entry->rx_output_staged_word & 0xFFU);
      entry->rx_output_staged_word >>= 8U;
      entry->rx_output_staged_word_bytes -= 1U;
      bytes_read += 1U;
      continue;
    }

    queue_error = serial_queue_pop(&entry->uart_device->rx_queue, &word);
    if (queue_error == UART_ERROR_NONE) {
      entry->rx_output_staged_word = word;
      entry->rx_output_staged_word_bytes = sizeof(uint32_t);
      continue;
    }
    if (queue_error == UART_ERROR_FIFO_EMPTY) {
      if (entry->rx_staged_word_bytes > 0U) {
        data[bytes_read] = (uint8_t)(entry->rx_staged_word & 0xFFU);
        entry->rx_staged_word >>= 8U;
        entry->rx_staged_word_bytes -= 1U;
        bytes_read += 1U;
        continue;
      }
      break;
    }
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  *out_bytes_read = bytes_read;
  if (bytes_read == 0U && length > 0U) {
    return SERIAL_DRIVER_ERROR_RX_EMPTY;
  }
  return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_read_u32(serial_descriptor_t descriptor,
                                             uint32_t *out_value) {
  serial_descriptor_entry_t *entry = NULL;
  uint8_t data[sizeof(uint32_t)] = {0U};
  size_t bytes_read = 0U;

  if (out_value == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }

  if (!serial_driver_common_initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  entry = serial_driver_get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }
  if (entry->mode != UART_PORT_MODE_SERIAL) {
    return SERIAL_DRIVER_ERROR_NOT_CONFIGURED;
  }
  if (!entry->uart_device->rx_queue.initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  if (serial_driver_available_rx_bytes(entry) < sizeof(uint32_t)) {
    return SERIAL_DRIVER_ERROR_RX_EMPTY;
  }

  (void)serial_driver_read(descriptor, data, sizeof(data), &bytes_read);

  *out_value = ((uint32_t)data[0]) | (((uint32_t)data[1]) << 8U) |
               (((uint32_t)data[2]) << 16U) | (((uint32_t)data[3]) << 24U);
  return SERIAL_DRIVER_OK;
}

size_t serial_driver_pending_rx(serial_descriptor_t descriptor) {
  serial_descriptor_entry_t *entry = NULL;

  if (!serial_driver_common_initialized) {
    return 0U;
  }

  entry = serial_driver_get_entry(descriptor);
  if (entry == NULL) {
    return 0U;
  }

  return serial_queue_size(&entry->uart_device->rx_queue);
}
