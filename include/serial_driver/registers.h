#ifndef SERIAL_DRIVER_REGISTERS_H
#define SERIAL_DRIVER_REGISTERS_H

/**
 * @file registers.h
 * @brief Register and device definitions for 16550-compatible UARTs.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "serial_driver/errors.h"
#include "serial_driver/queue.h"

/** Number of UART device slots tracked in @ref uart_devices. */
#define UART_DEVICE_COUNT 4U
/** Number of UARTs represented in the read/write FIFO map. */
#define UART_FIFO_UART_COUNT 8U

/**
 * @brief UART data register view at offset 0.
 *
 * Register alias depends on DLAB bit in LCR:
 * - DLAB=0: RBR (read) / THR (write)
 * - DLAB=1: DLL
 */
typedef union {
  volatile uint8_t rbr;
  volatile uint8_t thr;
  volatile uint8_t dll;
} uart16550_data_reg_t;

/**
 * @brief UART interrupt/divisor-high register view at offset 1.
 *
 * Register alias depends on DLAB bit in LCR:
 * - DLAB=0: IER
 * - DLAB=1: DLM
 */
typedef union {
  volatile uint8_t ier;
  volatile uint8_t dlm;
} uart16550_interrupt_reg_t;

/**
 * @brief UART FIFO/interrupt-ID register view at offset 2.
 *
 * - Read: IIR
 * - Write: FCR
 */
typedef union {
  volatile uint8_t iir;
  volatile uint8_t fcr;
} uart16550_fifo_reg_t;

/**
 * @brief Memory-mapped layout for a 16550-compatible UART.
 */
typedef struct {
  /** Offset 0x00: RBR/THR/DLL depending on access and DLAB. */
  uart16550_data_reg_t data;
  /** Offset 0x01: IER/DLM depending on DLAB. */
  uart16550_interrupt_reg_t interrupt_enable;
  /** Offset 0x02: IIR (read) / FCR (write). */
  uart16550_fifo_reg_t fifo_control;
  /** Offset 0x03: Line Control Register. */
  volatile uint8_t lcr;
  /** Offset 0x04: Modem Control Register. */
  volatile uint8_t mcr;
  /** Offset 0x05: Line Status Register. */
  volatile uint8_t lsr;
  /** Offset 0x06: Modem Status Register. */
  volatile uint8_t msr;
  /** Offset 0x07: Scratch Register. */
  volatile uint8_t scr;
} uart16550_registers_t;

/**
 * @brief Operating mode for a UART device slot.
 */
typedef enum {
  /** UART is configured for queued serial TX/RX behavior. */
  UART_PORT_MODE_SERIAL = 0,
  /** UART is configured for discrete/non-serial behavior. */
  UART_PORT_MODE_DISCRETE
} uart_port_mode_t;

/**
 * @brief Read and write FIFO sets for multiple UARTs.
 */
typedef struct {
  /** Per-UART transmit FIFOs. */
  serial_queue_t write_fifos[UART_FIFO_UART_COUNT];
  /** Per-UART receive FIFOs. */
  serial_queue_t read_fifos[UART_FIFO_UART_COUNT];
} uart_fifo_map_t;

/**
 * @brief Descriptor for one UART instance managed by the driver.
 */
typedef struct {
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
