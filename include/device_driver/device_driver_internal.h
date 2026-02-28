#ifndef SERIAL_DRIVER_INTERNAL_H
#define SERIAL_DRIVER_INTERNAL_H
#include "device_driver/device_driver.h"

/*
 * Internal helpers were intentionally removed from the exported interface.
 * Use the public API in device_driver.h.
 */
typedef struct SerialDescriptorEntry
{
    uart_device_t *uart_device;
    uint32_t port_index;
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

extern uart_error_t serial_driver_hw_map_uart(size_t port_index,
                                              uart_device_t *uart_device);

static serial_descriptor_entry_t *
serial_driver_get_entry(serial_descriptor_t descriptor)
{
    size_t index = 0U;

    if (descriptor == SERIAL_DESCRIPTOR_INVALID)
    {
        return NULL;
    }

    index = (size_t)(descriptor - 1U);
    if (index >= UART_DEVICE_COUNT)
    {
        return NULL;
    }

    if (!serial_descriptor_map[index].initialized)
    {
        return NULL;
    }

    return &serial_descriptor_map[index];
}

static void serial_driver_byte_fifo_reset(uart_byte_fifo_t *fifo)
{
    if (fifo == NULL)
    {
        return;
    }

    fifo->head = 0U;
    fifo->tail = 0U;
    fifo->count = 0U;
}

static bool serial_driver_byte_fifo_is_full(const uart_byte_fifo_t *fifo)
{
    return fifo != NULL && fifo->count == UART_DEVICE_FIFO_SIZE_BYTES;
}

static void serial_driver_byte_fifo_push(uart_byte_fifo_t *fifo, uint8_t byte)
{
    if (fifo == NULL)
    {
        return;
    }

    fifo->data[fifo->head] = byte;
    fifo->head = (fifo->head + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count += 1U;
}

static bool serial_driver_byte_fifo_is_empty(const uart_byte_fifo_t *fifo)
{
    return fifo == NULL || fifo->count == 0U; /* LCOV_EXCL_BR_LINE */
}

static uint8_t serial_driver_byte_fifo_pop(uart_byte_fifo_t *fifo)
{
    uint8_t byte = 0U;

    if (fifo == NULL)
    {
        return 0U;
    }

    byte = fifo->data[fifo->tail];
    fifo->tail = (fifo->tail + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count -= 1U;
    return byte;
}

static serial_driver_error_t serial_driver_common_init(void)
{
    size_t index = 0U;

    for (index = 0U; index < UART_DEVICE_COUNT; ++index)
    {
        serial_descriptor_map[index].uart_device = NULL;
        serial_descriptor_map[index].port_index = UART_DEVICE_COUNT;
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

    for (index = 0U; index < UART_FIFO_UART_COUNT; ++index)
    {
        serial_driver_byte_fifo_reset(&uart_fifo_map.write_fifos[index]);
        serial_driver_byte_fifo_reset(&uart_fifo_map.read_fifos[index]);
    }

    serial_driver_common_initialized = true;
    return SERIAL_DRIVER_OK;
}

static serial_driver_error_t
serial_driver_get_mode_entry(serial_descriptor_t descriptor,
                             uart_port_mode_t mode,
                             serial_descriptor_entry_t **out_entry)
{
    serial_descriptor_entry_t *entry = NULL;

    if (!serial_driver_common_initialized)
    {
        return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
    }

    entry = serial_driver_get_entry(descriptor);
    if (entry == NULL)
    {
        return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
    }

    if (entry->mode != mode)
    {
        return SERIAL_DRIVER_ERROR_NOT_CONFIGURED;
    }

    if (entry->uart_device == NULL || entry->uart_device->registers == NULL)
    {
        return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
    }

    *out_entry = entry;
    return SERIAL_DRIVER_OK;
}

static serial_driver_error_t
serial_driver_transmit_to_device_fifo(serial_descriptor_entry_t *entry,
                                      size_t max_bytes,
                                      size_t *out_bytes_transmitted)
{
    uart_byte_fifo_t *fifo = NULL;
    size_t fifo_index = 0U;
    size_t bytes_transmitted = 0U;
    uint32_t word = 0U;
    uart_error_t queue_error = UART_ERROR_NONE;

    if (out_bytes_transmitted == NULL)
    {
        return SERIAL_DRIVER_ERROR_INVALID_ARG;
    }
    *out_bytes_transmitted = 0U;

    fifo_index = (size_t)entry->port_index;
    fifo = &uart_fifo_map.write_fifos[fifo_index];

    while (bytes_transmitted < max_bytes &&
           !serial_driver_byte_fifo_is_full(fifo)) /* LCOV_EXCL_BR_LINE */
    {
        if (entry->staged_word_bytes == 0U)
        {
            if (serial_queue_size(&entry->uart_device->tx_queue) == 0U && /* LCOV_EXCL_BR_LINE */
                entry->tx_input_staged_word_bytes > 0U)
            {
                entry->staged_word = entry->tx_input_staged_word;
                entry->staged_word_bytes = entry->tx_input_staged_word_bytes;
                entry->tx_input_staged_word = 0U;
                entry->tx_input_staged_word_bytes = 0U;
            }
            else
            {
                queue_error =
                    serial_queue_pop(&entry->uart_device->tx_queue, &word); /* LCOV_EXCL_BR_LINE */
                if (queue_error == UART_ERROR_FIFO_QUEUE_EMPTY) /* LCOV_EXCL_BR_LINE */
                {
                    break;
                }
                if (queue_error != UART_ERROR_NONE)
                {
                    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
                }

                entry->staged_word = word;
                entry->staged_word_bytes = sizeof(uint32_t);
            }
        }

        serial_driver_byte_fifo_push(fifo,
                                     (uint8_t)(entry->staged_word & 0xFFU));
        entry->staged_word >>= 8U;
        entry->staged_word_bytes -= 1U;
        bytes_transmitted += 1U;
    }

    *out_bytes_transmitted = bytes_transmitted;
    return SERIAL_DRIVER_OK;
}

static serial_driver_error_t
serial_driver_flush_rx_staged_word(serial_descriptor_entry_t *entry,
                                   bool *out_queue_full)
{
    uart_error_t queue_error = UART_ERROR_NONE;

    *out_queue_full = false;

    queue_error =
        serial_queue_push(&entry->uart_device->rx_queue, entry->rx_staged_word);

    if (queue_error == UART_ERROR_NONE)
    {
        entry->rx_staged_word = 0U;
        entry->rx_staged_word_bytes = 0U;
        return SERIAL_DRIVER_OK;
    }

    if (queue_error == UART_ERROR_FIFO_QUEUE_FULL)
    {
        *out_queue_full = true;
        return SERIAL_DRIVER_OK;
    }

    return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
}

static serial_driver_error_t
serial_driver_receive_from_device_fifo(serial_descriptor_entry_t *entry,
                                       size_t max_bytes,
                                       size_t *out_bytes_received)
{
    uart_byte_fifo_t *fifo = NULL;
    size_t fifo_index = 0U;
    size_t bytes_received = 0U;
    uint8_t byte = 0U;
    bool queue_full = false;
    serial_driver_error_t status = SERIAL_DRIVER_OK;

    if (out_bytes_received == NULL)
    {
        return SERIAL_DRIVER_ERROR_INVALID_ARG;
    }
    *out_bytes_received = 0U;

    fifo_index = (size_t)entry->port_index;
    fifo = &uart_fifo_map.read_fifos[fifo_index];

    while (bytes_received < max_bytes)
    {
        if (entry->rx_staged_word_bytes == sizeof(uint32_t))
        {
            status = serial_driver_flush_rx_staged_word(entry, &queue_full); /* LCOV_EXCL_BR_LINE */
            if (status != SERIAL_DRIVER_OK)
            {
                *out_bytes_received = bytes_received;
                return status;
            }
            if (queue_full)
            {
                break;
            }
            continue;
        }

        if (serial_driver_byte_fifo_is_empty(fifo))
        {
            break;
        }

        byte = serial_driver_byte_fifo_pop(fifo);
        entry->rx_staged_word |= ((uint32_t)byte)
                                 << (8U * entry->rx_staged_word_bytes);
        entry->rx_staged_word_bytes += 1U;
        bytes_received += 1U;
    }

    if (entry->rx_staged_word_bytes == sizeof(uint32_t))
    {
        status = serial_driver_flush_rx_staged_word(entry, &queue_full); /* LCOV_EXCL_BR_LINE */
        if (status != SERIAL_DRIVER_OK)
        {
            *out_bytes_received = bytes_received;
            return status;
        }
    }

    *out_bytes_received = bytes_received;
    return SERIAL_DRIVER_OK;
}
#endif
