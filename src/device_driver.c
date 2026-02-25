#include "device_driver/device_driver_internal.h"

serial_descriptor_entry_t serial_descriptor_map[UART_DEVICE_COUNT] = {0};
bool serial_driver_common_initialized = false;

serial_descriptor_entry_t *
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

size_t serial_driver_descriptor_index(serial_descriptor_t descriptor)
{
    return (size_t)(descriptor - 1U);
}

void serial_driver_byte_fifo_reset(uart_byte_fifo_t *fifo)
{
    fifo->head = 0U;
    fifo->tail = 0U;
    fifo->count = 0U;
}

bool serial_driver_byte_fifo_is_full(const uart_byte_fifo_t *fifo)
{
    return fifo->count == UART_DEVICE_FIFO_SIZE_BYTES;
}

void serial_driver_byte_fifo_push(uart_byte_fifo_t *fifo, uint8_t byte)
{
    fifo->data[fifo->head] = byte;
    fifo->head = (fifo->head + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count += 1U;
}

bool serial_driver_byte_fifo_is_empty(const uart_byte_fifo_t *fifo)
{
    return fifo->count == 0U;
}

uint8_t serial_driver_byte_fifo_pop(uart_byte_fifo_t *fifo)
{
    uint8_t byte = fifo->data[fifo->tail];
    fifo->tail = (fifo->tail + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count -= 1U;
    return byte;
}

size_t serial_driver_available_rx_bytes(const serial_descriptor_entry_t *entry)
{
    return (serial_queue_size(&entry->uart_device->rx_queue) *
            sizeof(uint32_t)) +
           entry->rx_output_staged_word_bytes + entry->rx_staged_word_bytes;
}

serial_driver_error_t serial_driver_common_init(void)
{
    size_t index = 0U;

    for (index = 0U; index < UART_DEVICE_COUNT; ++index)
    {
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

    for (index = 0U; index < UART_FIFO_UART_COUNT; ++index)
    {
        serial_driver_byte_fifo_reset(&uart_fifo_map.write_fifos[index]);
        serial_driver_byte_fifo_reset(&uart_fifo_map.read_fifos[index]);
    }

    serial_driver_common_initialized = true;
    return SERIAL_DRIVER_OK;
}

serial_descriptor_t serial_port_init(serial_ports_t port, uart_port_mode_t mode)
{
    size_t index = 0U;
    uart_device_t *uart_device = NULL;

    if (!serial_driver_common_initialized)
    {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    if ((size_t)port >= UART_DEVICE_COUNT)
    {
        return SERIAL_DESCRIPTOR_INVALID;
    }
    uart_device = &uart_devices[(size_t)port];

    if (mode != UART_PORT_MODE_SERIAL && mode != UART_PORT_MODE_DISCRETE)
    {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    for (index = 0U; index < UART_DEVICE_COUNT; ++index)
    {
        if (serial_descriptor_map[index].initialized &&
            serial_descriptor_map[index].uart_device == uart_device)
        {
            return (serial_descriptor_t)(index + 1U);
        }
    }

    for (index = 0U; index < UART_DEVICE_COUNT; ++index)
    {
        if (!serial_descriptor_map[index].initialized)
        {
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

            if (mode == UART_PORT_MODE_SERIAL)
            {
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

uart_device_t *serial_driver_get_uart_device(serial_descriptor_t descriptor)
{
    serial_descriptor_entry_t *entry = NULL;

    if (!serial_driver_common_initialized)
    {
        return NULL;
    }

    entry = serial_driver_get_entry(descriptor);
    if (entry == NULL)
    {
        return NULL;
    }

    return entry->uart_device;
}
