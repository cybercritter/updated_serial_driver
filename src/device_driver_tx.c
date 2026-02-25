#include "device_driver/device_driver_internal.h"

serial_driver_error_t serial_driver_write_u32(serial_descriptor_t descriptor,
                                              uint32_t value) {
  serial_descriptor_entry_t *entry = NULL;
  uart_error_t queue_error = UART_ERROR_NONE;

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

  queue_error = serial_queue_push(&entry->uart_device->tx_queue, value);
  if (queue_error == UART_ERROR_NONE) {
    return SERIAL_DRIVER_OK;
  }
  if (queue_error == UART_ERROR_FIFO_FULL) {
    return SERIAL_DRIVER_ERROR_TX_FULL;
  }
  return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
}

serial_driver_error_t serial_driver_write(serial_descriptor_t descriptor,
                                          const uint8_t *data, size_t length,
                                          size_t *out_bytes_written) {
  serial_descriptor_entry_t *entry = NULL;
  size_t bytes_written = 0U;
  uart_error_t queue_error = UART_ERROR_NONE;

  if (out_bytes_written == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  *out_bytes_written = 0U;

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

  while (bytes_written < length) {
    if (entry->tx_input_staged_word_bytes == sizeof(uint32_t)) {
      queue_error = serial_queue_push(&entry->uart_device->tx_queue,
                                      entry->tx_input_staged_word);
      if (queue_error == UART_ERROR_FIFO_FULL) {
        break;
      }
      if (queue_error != UART_ERROR_NONE) {
        return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
      }

      entry->tx_input_staged_word = 0U;
      entry->tx_input_staged_word_bytes = 0U;
      continue;
    }

    entry->tx_input_staged_word |= ((uint32_t)data[bytes_written])
                                   << (8U * entry->tx_input_staged_word_bytes);
    entry->tx_input_staged_word_bytes += 1U;
    bytes_written += 1U;
  }

  if (entry->tx_input_staged_word_bytes == sizeof(uint32_t)) {
    queue_error = serial_queue_push(&entry->uart_device->tx_queue,
                                    entry->tx_input_staged_word);
    if (queue_error == UART_ERROR_NONE) {
      entry->tx_input_staged_word = 0U;
      entry->tx_input_staged_word_bytes = 0U;
    } else if (queue_error == UART_ERROR_FIFO_FULL) {
      if (bytes_written < length) {
        *out_bytes_written = bytes_written;
        return SERIAL_DRIVER_ERROR_TX_FULL;
      }
    } else {
      return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
    }
  }

  *out_bytes_written = bytes_written;
  return (bytes_written == length) ? SERIAL_DRIVER_OK
                                   : SERIAL_DRIVER_ERROR_TX_FULL;
}

serial_driver_error_t serial_driver_transmit_to_device_fifo(
    serial_descriptor_t descriptor, size_t max_bytes,
    size_t *out_bytes_transmitted) {
  serial_descriptor_entry_t *entry = NULL;
  uart_byte_fifo_t *fifo = NULL;
  size_t bytes_transmitted = 0U;
  uint32_t word = 0U;
  uart_error_t queue_error = UART_ERROR_NONE;

  if (out_bytes_transmitted == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  *out_bytes_transmitted = 0U;

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

  fifo = &uart_fifo_map.write_fifos[serial_driver_descriptor_index(descriptor)];

  while (bytes_transmitted < max_bytes && !serial_driver_byte_fifo_is_full(fifo)) {
    if (entry->staged_word_bytes == 0U) {
      if (serial_queue_size(&entry->uart_device->tx_queue) == 0U &&
          entry->tx_input_staged_word_bytes > 0U) {
        entry->staged_word = entry->tx_input_staged_word;
        entry->staged_word_bytes = entry->tx_input_staged_word_bytes;
        entry->tx_input_staged_word = 0U;
        entry->tx_input_staged_word_bytes = 0U;
      } else {
        queue_error = serial_queue_pop(&entry->uart_device->tx_queue, &word);
        if (queue_error == UART_ERROR_FIFO_EMPTY) {
          break;
        }
        if (queue_error != UART_ERROR_NONE) {
          return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
        }

        entry->staged_word = word;
        entry->staged_word_bytes = sizeof(uint32_t);
      }
    }

    serial_driver_byte_fifo_push(fifo, (uint8_t)(entry->staged_word & 0xFFU));
    entry->staged_word >>= 8U;
    entry->staged_word_bytes -= 1U;
    bytes_transmitted += 1U;
  }

  *out_bytes_transmitted = bytes_transmitted;
  return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_read_next_tx_u32(
    serial_descriptor_t descriptor, uint32_t *out_value) {
  serial_descriptor_entry_t *entry = NULL;
  uart_error_t queue_error = UART_ERROR_NONE;

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

  if (serial_queue_size(&entry->uart_device->tx_queue) == 0U &&
      entry->tx_input_staged_word_bytes == sizeof(uint32_t)) {
    *out_value = entry->tx_input_staged_word;
    entry->tx_input_staged_word = 0U;
    entry->tx_input_staged_word_bytes = 0U;
    return SERIAL_DRIVER_OK;
  }

  queue_error = serial_queue_pop(&entry->uart_device->tx_queue, out_value);
  if (queue_error == UART_ERROR_NONE) {
    return SERIAL_DRIVER_OK;
  }
  if (queue_error == UART_ERROR_FIFO_EMPTY) {
    return SERIAL_DRIVER_ERROR_TX_EMPTY;
  }
  return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
}

size_t serial_driver_pending_tx(serial_descriptor_t descriptor) {
  serial_descriptor_entry_t *entry = NULL;
  size_t pending = 0U;

  if (!serial_driver_common_initialized) {
    return 0U;
  }

  entry = serial_driver_get_entry(descriptor);
  if (entry == NULL) {
    return 0U;
  }
  pending = serial_queue_size(&entry->uart_device->tx_queue);
  if (entry->tx_input_staged_word_bytes == sizeof(uint32_t)) {
    pending += 1U;
  }
  return pending;
}
