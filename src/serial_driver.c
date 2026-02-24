#include "serial_driver/serial_driver.h"

#include <stdbool.h>

typedef struct SerialDescriptorEntry {
  uart_device_t *uart_device;
  uart_port_mode_t mode;
  uint32_t tx_input_staged_word;
  size_t tx_input_staged_word_bytes;
  uint32_t staged_word;
  size_t staged_word_bytes;
  uint32_t rx_staged_word;
  size_t rx_staged_word_bytes;
  uint32_t rx_output_staged_word;
  size_t rx_output_staged_word_bytes;
  bool initialized;
} serial_descriptor_entry_t;

static serial_descriptor_entry_t serial_descriptor_map[UART_DEVICE_COUNT] = {0};
static bool serial_driver_common_initialized = false;

static serial_descriptor_entry_t *get_entry(serial_descriptor_t descriptor) {
  size_t index = 0U;

  if (descriptor == SERIAL_DESCRIPTOR_INVALID) {
    return NULL;
  }

  index = (size_t)(descriptor - 1U);
  if (index >= UART_DEVICE_COUNT) {
    return NULL;
  }

  if (!serial_descriptor_map[index].initialized) {
    return NULL;
  }

  return &serial_descriptor_map[index];
}

static size_t descriptor_index(serial_descriptor_t descriptor) {
  return (size_t)(descriptor - 1U);
}

static void byte_fifo_reset(uart_byte_fifo_t *fifo) {
  fifo->head = 0U;
  fifo->tail = 0U;
  fifo->count = 0U;
}

static bool byte_fifo_is_full(const uart_byte_fifo_t *fifo) {
  return fifo->count == UART_DEVICE_FIFO_SIZE_BYTES;
}

static void byte_fifo_push(uart_byte_fifo_t *fifo, uint8_t byte) {
  fifo->data[fifo->head] = byte;
  fifo->head = (fifo->head + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
  fifo->count += 1U;
}

static bool byte_fifo_is_empty(const uart_byte_fifo_t *fifo) {
  return fifo->count == 0U;
}

static uint8_t byte_fifo_pop(uart_byte_fifo_t *fifo) {
  uint8_t byte = fifo->data[fifo->tail];
  fifo->tail = (fifo->tail + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
  fifo->count -= 1U;
  return byte;
}

static size_t available_rx_bytes(const serial_descriptor_entry_t *entry) {
  return (serial_queue_size(&entry->uart_device->rx_queue) * sizeof(uint32_t)) +
         entry->rx_output_staged_word_bytes + entry->rx_staged_word_bytes;
}

serial_driver_error_t serial_driver_common_init(void) {
  size_t index = 0U;

  serial_descriptor_map[0].initialized = false;
  for (index = 0U; index < UART_DEVICE_COUNT; ++index) {
    serial_descriptor_map[index].uart_device = NULL;
    serial_descriptor_map[index].mode = UART_PORT_MODE_DISCRETE;
    serial_descriptor_map[index].tx_input_staged_word = 0U;
    serial_descriptor_map[index].tx_input_staged_word_bytes = 0U;
    serial_descriptor_map[index].staged_word = 0U;
    serial_descriptor_map[index].staged_word_bytes = 0U;
    serial_descriptor_map[index].rx_staged_word = 0U;
    serial_descriptor_map[index].rx_staged_word_bytes = 0U;
    serial_descriptor_map[index].rx_output_staged_word = 0U;
    serial_descriptor_map[index].rx_output_staged_word_bytes = 0U;
    serial_descriptor_map[index].initialized = false;

    uart_devices[index].configured = false;
    uart_devices[index].port_mode = UART_PORT_MODE_DISCRETE;
  }

  for (index = 0U; index < UART_FIFO_UART_COUNT; ++index) {
    byte_fifo_reset(&uart_fifo_map.write_fifos[index]);
    byte_fifo_reset(&uart_fifo_map.read_fifos[index]);
  }

  serial_driver_common_initialized = true;
  return SERIAL_DRIVER_OK;
}

serial_descriptor_t serial_port_init(serial_ports_t port, uart_port_mode_t mode) {
  size_t index = 0U;
  uart_device_t *uart_device = NULL;

  if (!serial_driver_common_initialized) {
    return SERIAL_DESCRIPTOR_INVALID;
  }

  if ((size_t)port >= UART_DEVICE_COUNT) {
    return SERIAL_DESCRIPTOR_INVALID;
  }
  uart_device = &uart_devices[(size_t)port];

  if (mode != UART_PORT_MODE_SERIAL && mode != UART_PORT_MODE_DISCRETE) {
    return SERIAL_DESCRIPTOR_INVALID;
  }

  for (index = 0U; index < UART_DEVICE_COUNT; ++index) {
    if (serial_descriptor_map[index].initialized &&
        serial_descriptor_map[index].uart_device == uart_device) {
      return (serial_descriptor_t)(index + 1U);
    }
  }

  for (index = 0U; index < UART_DEVICE_COUNT; ++index) {
    if (!serial_descriptor_map[index].initialized) {
      serial_descriptor_map[index].uart_device = uart_device;
      serial_descriptor_map[index].mode = mode;
      serial_descriptor_map[index].tx_input_staged_word = 0U;
      serial_descriptor_map[index].tx_input_staged_word_bytes = 0U;
      serial_descriptor_map[index].staged_word = 0U;
      serial_descriptor_map[index].staged_word_bytes = 0U;
      serial_descriptor_map[index].rx_staged_word = 0U;
      serial_descriptor_map[index].rx_staged_word_bytes = 0U;
      serial_descriptor_map[index].rx_output_staged_word = 0U;
      serial_descriptor_map[index].rx_output_staged_word_bytes = 0U;
      serial_descriptor_map[index].initialized = true;

      if (mode == UART_PORT_MODE_SERIAL) {
        (void)serial_queue_init(&uart_device->tx_queue);
        (void)serial_queue_init(&uart_device->rx_queue);
      }

      uart_device->port_mode = mode;
      uart_device->configured = true;
      return (serial_descriptor_t)(index + 1U);
    }
  }

  return SERIAL_DESCRIPTOR_INVALID;
}

serial_driver_error_t serial_driver_write_u32(serial_descriptor_t descriptor,
                                              uint32_t value) {
  serial_descriptor_entry_t *entry = NULL;
  uart_error_t queue_error = UART_ERROR_NONE;

  if (!serial_driver_common_initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  entry = get_entry(descriptor);
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

  entry = get_entry(descriptor);
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
  if (bytes_written < length) {
    return SERIAL_DRIVER_ERROR_TX_FULL;
  }
  return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_transmit_to_device_fifo(
    serial_descriptor_t descriptor, size_t max_bytes,
    size_t *out_bytes_transmitted) {
  serial_descriptor_entry_t *entry = NULL;
  uart_byte_fifo_t *fifo = NULL;
  size_t bytes_transmitted = 0U;
  uint32_t word = 0U;
  uart_error_t queue_error = UART_ERROR_NONE;
  size_t fifo_index = 0U;

  if (out_bytes_transmitted == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  *out_bytes_transmitted = 0U;

  if (!serial_driver_common_initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  entry = get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }
  if (entry->mode != UART_PORT_MODE_SERIAL) {
    return SERIAL_DRIVER_ERROR_NOT_CONFIGURED;
  }

  fifo_index = descriptor_index(descriptor);
  if (fifo_index >= UART_FIFO_UART_COUNT) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  fifo = &uart_fifo_map.write_fifos[fifo_index];

  while (bytes_transmitted < max_bytes && !byte_fifo_is_full(fifo)) {
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

    byte_fifo_push(fifo, (uint8_t)(entry->staged_word & 0xFFU));
    entry->staged_word >>= 8U;
    entry->staged_word_bytes -= 1U;
    bytes_transmitted += 1U;
  }

  *out_bytes_transmitted = bytes_transmitted;
  return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_receive_from_device_fifo(
    serial_descriptor_t descriptor, size_t max_bytes,
    size_t *out_bytes_received) {
  serial_descriptor_entry_t *entry = NULL;
  uart_byte_fifo_t *fifo = NULL;
  uart_error_t queue_error = UART_ERROR_NONE;
  size_t bytes_received = 0U;
  size_t fifo_index = 0U;
  uint8_t byte = 0U;

  if (out_bytes_received == NULL) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  *out_bytes_received = 0U;

  if (!serial_driver_common_initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  entry = get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }
  if (entry->mode != UART_PORT_MODE_SERIAL) {
    return SERIAL_DRIVER_ERROR_NOT_CONFIGURED;
  }

  fifo_index = descriptor_index(descriptor);
  if (fifo_index >= UART_FIFO_UART_COUNT) {
    return SERIAL_DRIVER_ERROR_INVALID_ARG;
  }
  fifo = &uart_fifo_map.read_fifos[fifo_index];

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

    if (byte_fifo_is_empty(fifo)) {
      break;
    }

    byte = byte_fifo_pop(fifo);
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

  status = serial_driver_transmit_to_device_fifo(descriptor, max_tx_bytes,
                                                 out_tx_bytes_transmitted);
  if (status != SERIAL_DRIVER_OK) {
    return status;
  }

  entry = get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
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

  entry = get_entry(descriptor);
  if (entry == NULL) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }
  if (entry->mode != UART_PORT_MODE_SERIAL) {
    return SERIAL_DRIVER_ERROR_NOT_CONFIGURED;
  }
  if (!entry->uart_device->rx_queue.initialized) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  if (available_rx_bytes(entry) < sizeof(uint32_t)) {
    return SERIAL_DRIVER_ERROR_RX_EMPTY;
  }

  if (serial_driver_read(descriptor, data, sizeof(data), &bytes_read) !=
          SERIAL_DRIVER_OK ||
      bytes_read != sizeof(uint32_t)) {
    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
  }

  *out_value = ((uint32_t)data[0]) | (((uint32_t)data[1]) << 8U) |
               (((uint32_t)data[2]) << 16U) | (((uint32_t)data[3]) << 24U);
  return SERIAL_DRIVER_OK;
}

size_t serial_driver_pending_rx(serial_descriptor_t descriptor) {
  serial_descriptor_entry_t *entry = NULL;

  if (!serial_driver_common_initialized) {
    return 0U;
  }

  entry = get_entry(descriptor);
  if (entry == NULL) {
    return 0U;
  }

  return serial_queue_size(&entry->uart_device->rx_queue);
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

  entry = get_entry(descriptor);
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

  entry = get_entry(descriptor);
  if (entry == NULL) {
    return 0U;
  }
  pending = serial_queue_size(&entry->uart_device->tx_queue);
  if (entry->tx_input_staged_word_bytes == sizeof(uint32_t)) {
    pending += 1U;
  }
  return pending;
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

  entry = get_entry(descriptor);
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

uart_device_t *serial_driver_get_uart_device(serial_descriptor_t descriptor) {
  serial_descriptor_entry_t *entry = NULL;

  if (!serial_driver_common_initialized) {
    return NULL;
  }

  entry = get_entry(descriptor);
  if (entry == NULL) {
    return NULL;
  }

  return entry->uart_device;
}
