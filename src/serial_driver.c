#include "serial_driver/serial_driver.h"

#include <stdbool.h>

typedef struct {
    uart_device_t *uart_device;
    uart_port_mode_t mode;
    uint8_t *tx_buffer;
    size_t tx_capacity;
    size_t tx_head;
    size_t tx_tail;
    bool initialized;
} serial_descriptor_entry_t;

static serial_descriptor_entry_t serial_descriptor_map[UART_DEVICE_COUNT] = {0};
static const size_t SERIAL_DRIVER_WORD_BYTES = 4U;
static bool serial_driver_common_initialized = false;

static serial_descriptor_entry_t *get_entry(serial_descriptor_t descriptor)
{
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

static size_t next_index(const serial_descriptor_entry_t *entry, size_t current)
{
    return (current + 1U) % entry->tx_capacity;
}

static size_t pending_bytes(const serial_descriptor_entry_t *entry)
{
    if (entry->tx_head >= entry->tx_tail) {
        return entry->tx_head - entry->tx_tail;
    }

    return entry->tx_capacity - (entry->tx_tail - entry->tx_head);
}

serial_driver_error_t serial_driver_common_init(void)
{
    size_t index = 0U;

    serial_descriptor_map[0].initialized = false;
    for (index = 0U; index < UART_DEVICE_COUNT; ++index) {
        serial_descriptor_map[index].uart_device = NULL;
        serial_descriptor_map[index].mode = UART_PORT_MODE_DISCRETE;
        serial_descriptor_map[index].tx_buffer = NULL;
        serial_descriptor_map[index].tx_capacity = 0U;
        serial_descriptor_map[index].tx_head = 0U;
        serial_descriptor_map[index].tx_tail = 0U;
        serial_descriptor_map[index].initialized = false;

        uart_devices[index].configured = false;
        uart_devices[index].port_mode = UART_PORT_MODE_DISCRETE;
    }

    for (index = 0U; index < UART_FIFO_UART_COUNT; ++index) {
        (void)serial_queue_init(&uart_fifo_map.write_fifos[index]);
        (void)serial_queue_init(&uart_fifo_map.read_fifos[index]);
    }

    serial_driver_common_initialized = true;
    return SERIAL_DRIVER_OK;
}

serial_descriptor_t serial_port_init(
    uart_device_t *uart_device,
    uint8_t *tx_buffer,
    size_t tx_capacity,
    uart_port_mode_t mode)
{
    size_t index = 0U;

    if (!serial_driver_common_initialized) {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    if (uart_device == NULL) {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    if (mode != UART_PORT_MODE_SERIAL && mode != UART_PORT_MODE_DISCRETE) {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    if (mode == UART_PORT_MODE_SERIAL &&
        (tx_buffer == NULL || tx_capacity < 5U)) {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    for (index = 0U; index < UART_DEVICE_COUNT; ++index) {
        if (!serial_descriptor_map[index].initialized) {
            serial_descriptor_map[index].uart_device = uart_device;
            serial_descriptor_map[index].mode = mode;
            serial_descriptor_map[index].tx_buffer =
                (mode == UART_PORT_MODE_SERIAL) ? tx_buffer : NULL;
            serial_descriptor_map[index].tx_capacity =
                (mode == UART_PORT_MODE_SERIAL) ? tx_capacity : 0U;
            serial_descriptor_map[index].tx_head = 0U;
            serial_descriptor_map[index].tx_tail = 0U;
            serial_descriptor_map[index].initialized = true;

            uart_device->port_mode = mode;
            uart_device->configured = true;
            return (serial_descriptor_t)(index + 1U);
        }
    }

    return SERIAL_DESCRIPTOR_INVALID;
}

serial_driver_error_t serial_driver_write_u32(
    serial_descriptor_t descriptor,
    uint32_t value)
{
    serial_descriptor_entry_t *entry = NULL;
    size_t free_bytes = 0U;
    size_t i = 0U;

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

    free_bytes = (entry->tx_capacity - 1U) - pending_bytes(entry);
    if (free_bytes < SERIAL_DRIVER_WORD_BYTES) {
        return SERIAL_DRIVER_ERROR_TX_FULL;
    }

    for (i = 0U; i < SERIAL_DRIVER_WORD_BYTES; ++i) {
        entry->tx_buffer[entry->tx_head] = (uint8_t)((value >> (8U * i)) & 0xFFU);
        entry->tx_head = next_index(entry, entry->tx_head);
    }
    return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_read_next_tx_u32(
    serial_descriptor_t descriptor,
    uint32_t *out_value)
{
    serial_descriptor_entry_t *entry = NULL;
    size_t i = 0U;
    uint32_t value = 0U;

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

    if (pending_bytes(entry) < SERIAL_DRIVER_WORD_BYTES) {
        return SERIAL_DRIVER_ERROR_TX_EMPTY;
    }

    for (i = 0U; i < SERIAL_DRIVER_WORD_BYTES; ++i) {
        value |= ((uint32_t)entry->tx_buffer[entry->tx_tail]) << (8U * i);
        entry->tx_tail = next_index(entry, entry->tx_tail);
    }

    *out_value = value;
    return SERIAL_DRIVER_OK;
}

size_t serial_driver_pending_tx(serial_descriptor_t descriptor)
{
    serial_descriptor_entry_t *entry = NULL;

    if (!serial_driver_common_initialized) {
        return 0U;
    }

    entry = get_entry(descriptor);
    if (entry == NULL) {
        return 0U;
    }

    return pending_bytes(entry);
}

uart_device_t *serial_driver_get_uart_device(serial_descriptor_t descriptor)
{
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
