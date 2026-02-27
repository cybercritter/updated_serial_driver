#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "device_driver/device_driver.h"
#include "device_driver/device_driver_internal.h"

#define TEST_PORT_SERIAL SERIAL_PORT_6
#define TEST_PORT_DISCRETE SERIAL_PORT_5
#define UART_MCR_DISCRETE_LINE_ENABLE_BIT (1U << 0U)
#define UART_MCR_LOOPBACK_ENABLE_BIT (1U << 4U)

static bool g_loopback_enabled = false;
static uart16550_registers_t g_mock_registers[UART_DEVICE_COUNT];

static void loopback_enable(void) { g_loopback_enabled = true; }

static void loopback_disable(void) { g_loopback_enabled = false; }

static uart_error_t map_mock_uart_registers(size_t port_index,
                                            uart_device_t *uart_device,
                                            void *context) {
  uart16550_registers_t *register_blocks =
      (uart16550_registers_t *)context;

  if (uart_device == NULL || register_blocks == NULL ||
      port_index >= UART_DEVICE_COUNT) {
    return UART_ERROR_INVALID_ARG;
  }

  uart_device->registers = &register_blocks[port_index];
  uart_device->device_name = "mock-uart";
  uart_device->uart_base_address = (uintptr_t)&register_blocks[port_index];

  return UART_ERROR_NONE;
}

static int configure_mock_uart_registers(void) {
  size_t i = 0U;
  for (i = 0U; i < UART_DEVICE_COUNT; ++i) {
    memset(&g_mock_registers[i], 0, sizeof(g_mock_registers[i]));
  }

  return serial_driver_hw_set_mapper(map_mock_uart_registers,
                                     g_mock_registers) == UART_ERROR_NONE
             ? 0
             : 1;
}

static int run_port6_serial_roundtrip(void) {
  serial_descriptor_t serial_descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint8_t tx_data[4] = {0x11U, 0x22U, 0x33U, 0x44U};
  uint8_t rx_data[sizeof(tx_data)] = {0U};
  size_t bytes_written = 0U;
  size_t bytes_transmitted = 0U;
  size_t bytes_received = 0U;
  size_t bytes_read = 0U;
  size_t fifo_index = 0U;
  uart_device_t *serial_device = NULL;

  serial_descriptor = serial_port_init(TEST_PORT_SERIAL, UART_PORT_MODE_SERIAL);
  if (serial_descriptor == SERIAL_DESCRIPTOR_INVALID) {
    fprintf(stderr, "Failed to register port 6 in serial mode.\n");
    return 1;
  }
  serial_device = serial_driver_get_uart_device(serial_descriptor);
  if (serial_device == NULL || serial_device->registers == NULL) {
    fprintf(stderr, "Port 6 serial device/register mapping is invalid.\n");
    return 1;
  }

  if (serial_driver_write(serial_descriptor, tx_data, sizeof(tx_data),
                          &bytes_written) != SERIAL_DRIVER_OK ||
      bytes_written != sizeof(tx_data)) {
    fprintf(stderr, "Failed to queue TX data for port 6.\n");
    return 1;
  }

  if (serial_driver_transmit_to_device_fifo(serial_descriptor, sizeof(tx_data),
                                            &bytes_transmitted) !=
          SERIAL_DRIVER_OK ||
      bytes_transmitted != sizeof(tx_data)) {
    fprintf(stderr, "Failed to transmit TX queue to device FIFO on port 6.\n");
    return 1;
  }

  fifo_index = serial_driver_descriptor_index(serial_descriptor);

  if (!g_loopback_enabled) {
    fprintf(stderr, "Loopback is disabled for port 6 data test.\n");
    return 1;
  }
  serial_device->registers->mcr |= UART_MCR_LOOPBACK_ENABLE_BIT;

  while (!serial_driver_byte_fifo_is_empty(
      &uart_fifo_map.write_fifos[fifo_index])) {
    serial_driver_byte_fifo_push(
        &uart_fifo_map.read_fifos[fifo_index],
        serial_driver_byte_fifo_pop(&uart_fifo_map.write_fifos[fifo_index]));
  }

  serial_device->registers->mcr &= (uint8_t)(~UART_MCR_LOOPBACK_ENABLE_BIT);

  if (serial_driver_receive_from_device_fifo(serial_descriptor, sizeof(tx_data),
                                             &bytes_received) !=
          SERIAL_DRIVER_OK ||
      bytes_received != sizeof(tx_data)) {
    fprintf(stderr, "Failed to receive RX data from device FIFO on port 6.\n");
    return 1;
  }

  if (serial_driver_read(serial_descriptor, rx_data, sizeof(rx_data), &bytes_read) !=
          SERIAL_DRIVER_OK ||
      bytes_read != sizeof(rx_data) || memcmp(tx_data, rx_data, sizeof(tx_data)) != 0) {
    fprintf(stderr, "Roundtrip RX data mismatch on port 6.\n");
    return 1;
  }

  printf("Port 6 serial roundtrip succeeded.\n");
  return 0;
}

static int run_port5_discrete_toggle(void) {
  serial_descriptor_t discrete_descriptor = SERIAL_DESCRIPTOR_INVALID;
  uart_device_t *discrete_device = NULL;

  discrete_descriptor =
      serial_port_init(TEST_PORT_DISCRETE, UART_PORT_MODE_DISCRETE);
  if (discrete_descriptor == SERIAL_DESCRIPTOR_INVALID) {
    fprintf(stderr, "Failed to register port 5 in discrete mode.\n");
    return 1;
  }

  discrete_device = serial_driver_get_uart_device(discrete_descriptor);
  if (discrete_device == NULL || discrete_device->registers == NULL) {
    fprintf(stderr, "Port 5 discrete device/register mapping is invalid.\n");
    return 1;
  }

  discrete_device->registers->mcr |= UART_MCR_DISCRETE_LINE_ENABLE_BIT;
  if ((discrete_device->registers->mcr & UART_MCR_DISCRETE_LINE_ENABLE_BIT) == 0U) {
    fprintf(stderr, "Failed to enable discrete line on port 5.\n");
    return 1;
  }

  discrete_device->registers->mcr &= (uint8_t)(~UART_MCR_DISCRETE_LINE_ENABLE_BIT);
  if ((discrete_device->registers->mcr & UART_MCR_DISCRETE_LINE_ENABLE_BIT) != 0U) {
    fprintf(stderr, "Failed to disable discrete line on port 5.\n");
    return 1;
  }

  printf("Port 5 discrete line enable/disable succeeded.\n");
  return 0;
}

int main(void) {
  if (serial_driver_common_init() != SERIAL_DRIVER_OK) {
    fprintf(stderr, "serial_driver_common_init failed.\n");
    return 1;
  }

  if (configure_mock_uart_registers() != 0) {
    return 1;
  }
  loopback_enable();
  if (run_port6_serial_roundtrip() != 0) {
    return 1;
  }
  loopback_disable();
  if (run_port5_discrete_toggle() != 0) {
    return 1;
  }

  printf("device_driver_test_main passed.\n");
  return 0;
}
