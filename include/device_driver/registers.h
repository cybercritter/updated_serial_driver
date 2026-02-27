#ifndef SERIAL_DRIVER_REGISTERS_H
#define SERIAL_DRIVER_REGISTERS_H

/**
 * @file registers.h
 * @brief UART device-slot and FIFO definitions.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "device_driver/errors.h"
#include "device_driver/queue.h"
#include "device_driver/register_map.h"

/** Number of UART device slots tracked in @ref uart_devices. */
#define UART_DEVICE_COUNT 8U
/** Number of UARTs represented in the read/write FIFO map. */
#define UART_FIFO_UART_COUNT 8U
/** Hardware/device FIFO capacity in bytes. */
#define UART_DEVICE_FIFO_SIZE_BYTES 255U

/**
 * @brief Operating mode for a UART device slot.
 */
typedef enum PORT_MODE {
  /** UART is configured for queued serial TX/RX behavior. */
  UART_PORT_MODE_SERIAL = 0,
  /** UART is configured for discrete/non-serial behavior. */
  UART_PORT_MODE_DISCRETE
} uart_port_mode_t;

/**
 * @brief Read and write FIFO sets for multiple UARTs.
 */
typedef struct UARTByteFifo {
  /** Fixed-size byte storage for the FIFO. */
  uint8_t data[UART_DEVICE_FIFO_SIZE_BYTES];
  /** Index where next byte will be written. */
  size_t head;
  /** Index where next byte will be read. */
  size_t tail;
  /** Number of bytes currently stored. */
  size_t count;
} uart_byte_fifo_t;

/**
 * @brief Read and write FIFO sets for multiple UARTs.
 */
typedef struct UARTFifoMap {
  /** Per-UART transmit FIFOs. */
  uart_byte_fifo_t write_fifos[UART_FIFO_UART_COUNT];
  /** Per-UART receive FIFOs. */
  uart_byte_fifo_t read_fifos[UART_FIFO_UART_COUNT];
} uart_fifo_map_t;

/**
 * @brief Descriptor for one UART instance managed by the driver.
 */
typedef struct UARTDevice {
  /** Pointer to memory-mapped UART register block. */
  uart16550_registers_t *registers;
  /** Human-readable device name (for logs/config selection). */
  const char *device_name;
  /** Base address used to map/register this UART. */
  uintptr_t uart_base_address;
  /** Active mode for this UART slot. */
  uart_port_mode_t port_mode;
  /** True once this UART slot has been configured. */
  bool configured;
  /** Software transmit queue. */
  serial_queue_t tx_queue;
  /** Software receive queue. */
  serial_queue_t rx_queue;
} uart_device_t;

/** Global table of UART devices managed by the driver. */
extern uart_device_t uart_devices[UART_DEVICE_COUNT];
/** Global read/write FIFO map for 8 UART channels. */
extern uart_fifo_map_t uart_fifo_map;

#endif
