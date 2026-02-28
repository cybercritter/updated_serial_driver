#include "device_driver/device_driver.h"
#include "device_driver/device_driver_internal.h"

serial_descriptor_t serial_port_init(serial_ports_t port, uart_port_mode_t mode)
{
    size_t index = 0U;
    uart_device_t *uart_device = NULL;
    uart_error_t hw_map_error = UART_ERROR_NONE;

    if (!serial_driver_common_initialized)
    {
        if (serial_driver_common_init() != SERIAL_DRIVER_OK) /* LCOV_EXCL_BR_LINE */
        {
            return SERIAL_DESCRIPTOR_INVALID; /* LCOV_EXCL_LINE */
        }
    }

    if ((size_t)port >= UART_DEVICE_COUNT)
    {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    if (mode != UART_PORT_MODE_SERIAL && mode != UART_PORT_MODE_DISCRETE)
    {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    uart_device = &uart_devices[(size_t)port];

    for (index = 0U; index < UART_DEVICE_COUNT; ++index)
    {
        if (serial_descriptor_map[index].initialized &&
            serial_descriptor_map[index].uart_device == uart_device)
        {
            return (serial_descriptor_t)(index + 1U);
        }
    }

    hw_map_error = serial_driver_hw_map_uart((size_t)port, uart_device);
    if (hw_map_error != UART_ERROR_NONE || uart_device->registers == NULL)
    {
        return SERIAL_DESCRIPTOR_INVALID;
    }

    for (index = 0U; index < UART_DEVICE_COUNT; ++index) /* LCOV_EXCL_BR_LINE */
    {
        if (!serial_descriptor_map[index].initialized)
        {
            serial_descriptor_map[index].uart_device = uart_device;
            serial_descriptor_map[index].port_index = (uint32_t)port;
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
                if (serial_queue_init(&uart_device->tx_queue) != /* LCOV_EXCL_BR_LINE */
                        UART_ERROR_NONE || /* LCOV_EXCL_BR_LINE */
                    serial_queue_init(&uart_device->rx_queue) != /* LCOV_EXCL_BR_LINE */
                        UART_ERROR_NONE)
                {
                    serial_descriptor_map[index].initialized = false; /* LCOV_EXCL_LINE */
                    return SERIAL_DESCRIPTOR_INVALID;                  /* LCOV_EXCL_LINE */
                }
            }

            uart_device->port_mode = mode;
            uart_device->configured = true;
            return (serial_descriptor_t)(index + 1U);
        }
    }

    return SERIAL_DESCRIPTOR_INVALID; /* LCOV_EXCL_LINE */
}

serial_driver_error_t serial_driver_write(serial_descriptor_t descriptor,
                                          const uint8_t *data, size_t length,
                                          size_t *out_bytes_written)
{
    serial_descriptor_entry_t *entry = NULL;
    size_t bytes_written = 0U;
    uart_error_t queue_error = UART_ERROR_NONE;
    serial_driver_error_t status = SERIAL_DRIVER_OK;

    if (out_bytes_written == NULL)
    {
        return SERIAL_DRIVER_ERROR_INVALID_ARG;
    }
    *out_bytes_written = 0U;

    if (length > 0U && data == NULL)
    {
        return SERIAL_DRIVER_ERROR_INVALID_ARG;
    }

    status =
        serial_driver_get_mode_entry(descriptor, UART_PORT_MODE_SERIAL, &entry);
    if (status != SERIAL_DRIVER_OK)
    {
        return status;
    }

    while (bytes_written < length)
    {
        if (entry->tx_input_staged_word_bytes == sizeof(uint32_t))
        {
            queue_error = serial_queue_push(&entry->uart_device->tx_queue,
                                            entry->tx_input_staged_word);
            if (queue_error == UART_ERROR_FIFO_QUEUE_FULL)
            {
                break;
            }
            if (queue_error != UART_ERROR_NONE)
            {
                return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
            }

            entry->tx_input_staged_word = 0U;
            entry->tx_input_staged_word_bytes = 0U;
            continue;
        }

        entry->tx_input_staged_word |=
            ((uint32_t)data[bytes_written])
            << (8U * entry->tx_input_staged_word_bytes);
        entry->tx_input_staged_word_bytes += 1U;
        bytes_written += 1U;
    }

    if (entry->tx_input_staged_word_bytes == sizeof(uint32_t))
    {
        queue_error = serial_queue_push(&entry->uart_device->tx_queue,
                                        entry->tx_input_staged_word);
        if (queue_error == UART_ERROR_NONE)
        {
            entry->tx_input_staged_word = 0U;
            entry->tx_input_staged_word_bytes = 0U;
        }
        else if (queue_error == UART_ERROR_FIFO_QUEUE_FULL)
        {
            if (bytes_written < length)
            {
                *out_bytes_written = bytes_written;
                return SERIAL_DRIVER_ERROR_TX_FULL;
            }
        }
        else
        {
            return SERIAL_DRIVER_ERROR_NOT_INITIALIZED;
        }
    }

    *out_bytes_written = bytes_written;

    return (bytes_written == length) ? SERIAL_DRIVER_OK
                                     : SERIAL_DRIVER_ERROR_TX_FULL; /* LCOV_EXCL_BR_LINE */
}

serial_driver_error_t serial_driver_read(serial_descriptor_t descriptor,
                                         uint8_t *data, size_t length,
                                         size_t *out_bytes_read)
{
    serial_descriptor_entry_t *entry = NULL;
    size_t bytes_read = 0U;
    uint32_t word = 0U;
    uart_error_t queue_error = UART_ERROR_NONE;
    serial_driver_error_t status = SERIAL_DRIVER_OK;

    if (out_bytes_read == NULL)
    {
        return SERIAL_DRIVER_ERROR_INVALID_ARG;
    }
    *out_bytes_read = 0U;

    if (length > 0U && data == NULL)
    {
        return SERIAL_DRIVER_ERROR_INVALID_ARG;
    }

    status =
        serial_driver_get_mode_entry(descriptor, UART_PORT_MODE_SERIAL, &entry);
    if (status != SERIAL_DRIVER_OK)
    {
        return status;
    }

    while (bytes_read < length)
    {
        if (entry->rx_output_staged_word_bytes > 0U)
        {
            data[bytes_read] = (uint8_t)(entry->rx_output_staged_word & 0xFFU);
            entry->rx_output_staged_word >>= 8U;
            entry->rx_output_staged_word_bytes -= 1U;
            bytes_read += 1U;
            continue;
        }

        queue_error = serial_queue_pop(&entry->uart_device->rx_queue, &word);
        if (queue_error == UART_ERROR_NONE)
        {
            entry->rx_output_staged_word = word;
            entry->rx_output_staged_word_bytes = sizeof(uint32_t);
            continue;
        }

        if (queue_error == UART_ERROR_FIFO_QUEUE_EMPTY)
        {
            if (entry->rx_staged_word_bytes > 0U)
            {
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
    if (bytes_read == 0U && length > 0U)
    {
        return SERIAL_DRIVER_ERROR_RX_EMPTY;
    }

    return SERIAL_DRIVER_OK;
}

serial_driver_error_t serial_driver_poll(serial_descriptor_t descriptor,
                                         size_t max_tx_bytes,
                                         size_t max_rx_bytes,
                                         size_t *out_tx_bytes_transmitted,
                                         size_t *out_rx_bytes_received)
{
    serial_descriptor_entry_t *entry = NULL;
    serial_driver_error_t status = SERIAL_DRIVER_OK;

    if (out_tx_bytes_transmitted == NULL || out_rx_bytes_received == NULL)
    {
        return SERIAL_DRIVER_ERROR_INVALID_ARG;
    }

    *out_tx_bytes_transmitted = 0U;
    *out_rx_bytes_received = 0U;

    status =
        serial_driver_get_mode_entry(descriptor, UART_PORT_MODE_SERIAL, &entry);
    if (status != SERIAL_DRIVER_OK)
    {
        return status;
    }

    status = serial_driver_transmit_to_device_fifo(entry, max_tx_bytes,
                                                   out_tx_bytes_transmitted);
    if (status != SERIAL_DRIVER_OK)
    {
        return status;
    }

    if (entry->staged_word_bytes != 0U ||
        entry->tx_input_staged_word_bytes != 0U || /* LCOV_EXCL_BR_LINE */
        serial_queue_size(&entry->uart_device->tx_queue) != 0U)
    {
        return SERIAL_DRIVER_OK;
    }

    return serial_driver_receive_from_device_fifo(entry, max_rx_bytes,
                                                  out_rx_bytes_received);
}

static serial_driver_error_t
serial_driver_set_mcr_bit(serial_descriptor_t descriptor, uart_port_mode_t mode,
                          uint8_t bit_mask, bool enable)
{
    serial_descriptor_entry_t *entry = NULL;
    volatile uint8_t *mcr = NULL;
    serial_driver_error_t status = SERIAL_DRIVER_OK;

    status = serial_driver_get_mode_entry(descriptor, mode, &entry);
    if (status != SERIAL_DRIVER_OK)
    {
        return status;
    }

    mcr = &entry->uart_device->registers->uart.mcr;
    if (enable)
    {
        *mcr |= bit_mask;
    }
    else
    {
        *mcr &= (uint8_t)(~bit_mask);
    }

    return SERIAL_DRIVER_OK;
}

serial_driver_error_t
serial_driver_enable_loopback(serial_descriptor_t descriptor)
{
    return serial_driver_set_mcr_bit(descriptor, UART_PORT_MODE_SERIAL,
                                     UART_MCR_LOOPBACK_BIT, true);
}

serial_driver_error_t
serial_driver_disable_loopback(serial_descriptor_t descriptor)
{
    return serial_driver_set_mcr_bit(descriptor, UART_PORT_MODE_SERIAL,
                                     UART_MCR_LOOPBACK_BIT, false);
}

serial_driver_error_t
serial_driver_enable_discrete(serial_descriptor_t descriptor)
{
    return serial_driver_set_mcr_bit(descriptor, UART_PORT_MODE_DISCRETE,
                                     UART_MCR_DISCRETE_LINE_BIT, true);
}

serial_driver_error_t
serial_driver_disable_discrete(serial_descriptor_t descriptor)
{
    return serial_driver_set_mcr_bit(descriptor, UART_PORT_MODE_DISCRETE,
                                     UART_MCR_DISCRETE_LINE_BIT, false);
}
