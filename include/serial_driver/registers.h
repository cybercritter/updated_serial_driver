#ifndef SERIAL_DRIVER_REGISTERS_H
#define SERIAL_DRIVER_REGISTERS_H

/**
 * @file registers.h
 * @brief Register and device definitions for 16550-compatible UARTs.
 */

#include <stddef.h>
#include <stdint.h>

/** Capacity of each software TX/RX circular buffer in bytes. */
#define UART_CIRCULAR_BUFFER_SIZE 256U
/** Number of UART device slots tracked in @ref uart_devices. */
#define UART_DEVICE_COUNT 4U

/**
 * @brief Common UART and FIFO error/status codes.
 */
typedef enum {
  /** Operation completed successfully. */
  UART_ERROR_NONE = 0,

  /** One or more function arguments are invalid. */
  UART_ERROR_INVALID_ARG,
  /** Device or subsystem has not been initialized. */
  UART_ERROR_NOT_INITIALIZED,
  /** Device exists but required configuration was not applied. */
  UART_ERROR_NOT_CONFIGURED,
  /** Requested UART device identifier/name was not found. */
  UART_ERROR_DEVICE_NOT_FOUND,
  /** FIFO/circular buffer cannot accept more bytes. */
  UART_ERROR_FIFO_FULL,
  /** FIFO/circular buffer has no bytes available. */
  UART_ERROR_FIFO_EMPTY,
  /** A write exceeded available FIFO/circular buffer space. */
  UART_ERROR_FIFO_OVERFLOW,
  /** A read/remove was attempted on an empty FIFO/circular buffer. */
  UART_ERROR_FIFO_UNDERFLOW,
  /** Operation timed out before completion. */
  UART_ERROR_TIMEOUT,
  /** UART detected a parity error on received data. */
  UART_ERROR_PARITY,
  /** UART detected a framing error on received data. */
  UART_ERROR_FRAMING,
  /** UART receive overrun occurred. */
  UART_ERROR_OVERRUN,
  /** Non-specific hardware-level failure. */
  UART_ERROR_HARDWARE_FAULT
} uart_error_t;

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
 * @brief Generic circular buffer state used for UART TX and RX queues.
 */
typedef struct {
  /** Backing storage for buffered bytes. */
  uint8_t data[UART_CIRCULAR_BUFFER_SIZE];
  /** Index where next byte is written. */
  size_t head;
  /** Index where next byte is read. */
  size_t tail;
  /** Number of valid bytes currently in the buffer. */
  size_t count;
} uart_circular_buffer_t;

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
  /** Software transmit queue. */
  uart_circular_buffer_t tx_circular_buffer;
  /** Software receive queue. */
  uart_circular_buffer_t rx_circular_buffer;
} uart_device_t;

/** Global table of UART devices managed by the driver. */
extern uart_device_t uart_devices[UART_DEVICE_COUNT];

#endif
