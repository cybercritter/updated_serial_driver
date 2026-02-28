#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "device_driver/device_driver.h"

#define TEST_PORT_SERIAL SERIAL_PORT_6
#define TEST_PORT_DISCRETE SERIAL_PORT_7

static xr17c358_channel_register_map_t g_mock_registers[UART_DEVICE_COUNT];

static uart_error_t map_mock_uart_registers(size_t port_index,
                                            uart_device_t *uart_device,
                                            void *context)
{
    xr17c358_channel_register_map_t *register_blocks =
        (xr17c358_channel_register_map_t *)context;

    if (uart_device == NULL || register_blocks == NULL ||
        port_index >= UART_DEVICE_COUNT)
    {
        return UART_ERROR_INVALID_ARG;
    }

    uart_device->registers = &register_blocks[port_index];
    uart_device->device_name = "mock-uart";
    uart_device->uart_base_address = (uintptr_t)&register_blocks[port_index];

    return UART_ERROR_NONE;
}

static int configure_mock_uart_registers(void)
{
    size_t i = 0U;

    for (i = 0U; i < UART_DEVICE_COUNT; ++i)
    {
        memset(&g_mock_registers[i], 0, sizeof(g_mock_registers[i]));
        uart_fifo_map.write_fifos[i].head = 0U;
        uart_fifo_map.write_fifos[i].tail = 0U;
        uart_fifo_map.write_fifos[i].count = 0U;
        uart_fifo_map.read_fifos[i].head = 0U;
        uart_fifo_map.read_fifos[i].tail = 0U;
        uart_fifo_map.read_fifos[i].count = 0U;
    }

    return serial_driver_hw_set_mapper(map_mock_uart_registers,
                                       g_mock_registers) == UART_ERROR_NONE
               ? 0
               : 1;
}

static bool fifo_is_empty(const uart_byte_fifo_t *fifo)
{
    return fifo->count == 0U;
}

static bool fifo_is_full(const uart_byte_fifo_t *fifo)
{
    return fifo->count == UART_DEVICE_FIFO_SIZE_BYTES;
}

static uint8_t fifo_pop(uart_byte_fifo_t *fifo)
{
    uint8_t value = fifo->data[fifo->tail];
    fifo->tail = (fifo->tail + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count -= 1U;
    return value;
}

static void fifo_push(uart_byte_fifo_t *fifo, uint8_t value)
{
    fifo->data[fifo->head] = value;
    fifo->head = (fifo->head + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count += 1U;
}

static int run_port6_serial_roundtrip(void)
{
    serial_descriptor_t serial_descriptor = SERIAL_DESCRIPTOR_INVALID;
    uint8_t tx_data[4] = {0x11U, 0x22U, 0x33U, 0x44U};
    uint8_t rx_data[sizeof(tx_data)] = {0U};
    size_t bytes_written = 0U;
    size_t bytes_transmitted = 0U;
    size_t bytes_received = 0U;
    size_t bytes_read = 0U;

    serial_descriptor = serial_port_init(TEST_PORT_SERIAL, UART_PORT_MODE_SERIAL);
    if (serial_descriptor == SERIAL_DESCRIPTOR_INVALID)
    {
        fprintf(stderr, "Failed to register serial port.\n");
        return 1;
    }

    if (serial_driver_enable_loopback(serial_descriptor) != SERIAL_DRIVER_OK)
    {
        fprintf(stderr, "Failed to enable loopback.\n");
        return 1;
    }

    if ((uart_devices[TEST_PORT_SERIAL].registers->uart.mcr &
         UART_MCR_LOOPBACK_BIT) == 0U)
    {
        fprintf(stderr, "Loopback bit did not assert.\n");
        return 1;
    }

    if (serial_driver_write(serial_descriptor, tx_data, sizeof(tx_data),
                            &bytes_written) != SERIAL_DRIVER_OK ||
        bytes_written != sizeof(tx_data))
    {
        fprintf(stderr, "Failed to queue TX data.\n");
        return 1;
    }

    if (serial_driver_poll(serial_descriptor, sizeof(tx_data), 0U,
                           &bytes_transmitted,
                           &bytes_received) != SERIAL_DRIVER_OK ||
        bytes_transmitted != sizeof(tx_data) || bytes_received != 0U)
    {
        fprintf(stderr, "Failed to transmit queued bytes.\n");
        return 1;
    }

    while (!fifo_is_empty(&uart_fifo_map.write_fifos[TEST_PORT_SERIAL]) &&
           !fifo_is_full(&uart_fifo_map.read_fifos[TEST_PORT_SERIAL]))
    {
        fifo_push(&uart_fifo_map.read_fifos[TEST_PORT_SERIAL],
                  fifo_pop(&uart_fifo_map.write_fifos[TEST_PORT_SERIAL]));
    }

    if (serial_driver_poll(serial_descriptor, 0U, sizeof(tx_data),
                           &bytes_transmitted,
                           &bytes_received) != SERIAL_DRIVER_OK ||
        bytes_transmitted != 0U || bytes_received != sizeof(tx_data))
    {
        fprintf(stderr, "Failed to move RX bytes into read queue.\n");
        return 1;
    }

    if (serial_driver_read(serial_descriptor, rx_data, sizeof(rx_data),
                           &bytes_read) != SERIAL_DRIVER_OK ||
        bytes_read != sizeof(rx_data) ||
        memcmp(tx_data, rx_data, sizeof(tx_data)) != 0)
    {
        fprintf(stderr, "Roundtrip RX data mismatch.\n");
        return 1;
    }

    if (serial_driver_disable_loopback(serial_descriptor) != SERIAL_DRIVER_OK)
    {
        fprintf(stderr, "Failed to disable loopback.\n");
        return 1;
    }

    if ((uart_devices[TEST_PORT_SERIAL].registers->uart.mcr &
         UART_MCR_LOOPBACK_BIT) != 0U)
    {
        fprintf(stderr, "Loopback bit did not clear.\n");
        return 1;
    }

    printf("Serial roundtrip succeeded.\n");
    return 0;
}

static int run_discrete_toggle(void)
{
    serial_descriptor_t discrete_descriptor = SERIAL_DESCRIPTOR_INVALID;

    discrete_descriptor =
        serial_port_init(TEST_PORT_DISCRETE, UART_PORT_MODE_DISCRETE);
    if (discrete_descriptor == SERIAL_DESCRIPTOR_INVALID)
    {
        fprintf(stderr, "Failed to register discrete port.\n");
        return 1;
    }

    if (serial_driver_enable_discrete(discrete_descriptor) != SERIAL_DRIVER_OK)
    {
        fprintf(stderr, "Failed to enable discrete line.\n");
        return 1;
    }

    if ((uart_devices[TEST_PORT_DISCRETE].registers->uart.mcr &
         UART_MCR_DISCRETE_LINE_BIT) == 0U)
    {
        fprintf(stderr, "Failed to assert discrete line bit.\n");
        return 1;
    }

    if (serial_driver_disable_discrete(discrete_descriptor) != SERIAL_DRIVER_OK)
    {
        fprintf(stderr, "Failed to disable discrete line.\n");
        return 1;
    }

    if ((uart_devices[TEST_PORT_DISCRETE].registers->uart.mcr &
         UART_MCR_DISCRETE_LINE_BIT) != 0U)
    {
        fprintf(stderr, "Failed to clear discrete line bit.\n");
        return 1;
    }

    printf("Discrete line toggle succeeded.\n");
    return 0;
}

int main(void)
{
    if (configure_mock_uart_registers() != 0)
    {
        fprintf(stderr, "Failed to configure mock UART mapper.\n");
        return 1;
    }

    if (run_port6_serial_roundtrip() != 0)
    {
        return 1;
    }

    if (run_discrete_toggle() != 0)
    {
        return 1;
    }

    serial_driver_hw_reset_mapper();

    printf("device_driver_test_main passed.\n");
    return 0;
}
