#ifndef SERIAL_DRIVER_INTERNAL_H
#define SERIAL_DRIVER_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "device_driver/device_driver.h"

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

extern serial_descriptor_entry_t serial_descriptor_map[UART_DEVICE_COUNT];
extern bool serial_driver_common_initialized;

serial_descriptor_entry_t *
serial_driver_get_entry(serial_descriptor_t descriptor);
size_t serial_driver_descriptor_index(serial_descriptor_t descriptor);

void serial_driver_byte_fifo_reset(uart_byte_fifo_t *fifo);
bool serial_driver_byte_fifo_is_full(const uart_byte_fifo_t *fifo);
void serial_driver_byte_fifo_push(uart_byte_fifo_t *fifo, uint8_t byte);
bool serial_driver_byte_fifo_is_empty(const uart_byte_fifo_t *fifo);
uint8_t serial_driver_byte_fifo_pop(uart_byte_fifo_t *fifo);

size_t serial_driver_available_rx_bytes(const serial_descriptor_entry_t *entry);
uart_error_t serial_driver_hw_map_uart(size_t port_index,
                                       uart_device_t *uart_device);

#endif
