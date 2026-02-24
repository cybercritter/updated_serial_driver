#ifndef SERIAL_DRIVER_SERIAL_DRIVER_H
#define SERIAL_DRIVER_SERIAL_DRIVER_H

/**
 * @file serial_driver.h
 * @brief Public API for the software serial TX queue.
 */

#include <stddef.h>
#include <stdint.h>

#include "registers.h"

/** Opaque serial-driver descriptor returned by @ref serial_port_init. */
typedef uint32_t serial_descriptor_t;

/** Sentinel for an invalid descriptor. */
#define SERIAL_DESCRIPTOR_INVALID ((serial_descriptor_t)0U)

/**
 * @brief Serial driver status/error codes.
 *
 * Values are aligned with @ref uart_error_t for compatibility with existing
 * UART-level error handling.
 */
typedef enum {
  /** Operation completed successfully. */
  SERIAL_DRIVER_OK = UART_ERROR_NONE,
  /** One or more function arguments are invalid. */
  SERIAL_DRIVER_ERROR_INVALID_ARG = UART_ERROR_INVALID_ARG,
  /** Driver has not been initialized. */
  SERIAL_DRIVER_ERROR_NOT_INITIALIZED = UART_ERROR_NOT_INITIALIZED,
  /** UART was configured, but not in serial mode. */
  SERIAL_DRIVER_ERROR_NOT_CONFIGURED = UART_ERROR_NOT_CONFIGURED,
  /** TX queue has no free space for additional bytes. */
  SERIAL_DRIVER_ERROR_TX_FULL = UART_ERROR_FIFO_FULL,
  /** TX queue has no bytes available to read. */
  SERIAL_DRIVER_ERROR_TX_EMPTY = UART_ERROR_FIFO_EMPTY
} serial_driver_error_t;

/**
 * @brief Initialize common serial-driver state shared across UART ports.
 *
 * Must be called before @ref serial_port_init.
 *
 * @return @ref SERIAL_DRIVER_OK on success.
 */
serial_driver_error_t serial_driver_common_init(void);

/**
 * @brief Initialize one UART port instance and return its descriptor.
 *
 * @param uart_device UART device associated with this serial descriptor.
 * @param tx_buffer Storage buffer used as circular queue when mode is serial.
 * @param tx_capacity Capacity of @p tx_buffer in bytes (must be >= 5 when mode
 * is serial).
 * @param mode Configuration mode for this UART (serial or discrete).
 * @return Valid serial descriptor on success, @ref SERIAL_DESCRIPTOR_INVALID on
 * failure.
 */
serial_descriptor_t serial_port_init(uart_device_t *uart_device,
                                     uint8_t *tx_buffer, size_t tx_capacity,
                                     uart_port_mode_t mode);

/**
 * @brief Queue one 32-bit value for transmit.
 *
 * @param descriptor Initialized serial descriptor.
 * @param value 32-bit value to enqueue.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t serial_driver_write_u32(serial_descriptor_t descriptor,
                                              uint32_t value);

/**
 * @brief Read and remove the next queued 32-bit TX value.
 *
 * @param descriptor Initialized serial descriptor.
 * @param out_value Output pointer for the dequeued 32-bit value.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t serial_driver_read_next_tx_u32(
    serial_descriptor_t descriptor, uint32_t *out_value);

/**
 * @brief Return number of queued TX bytes currently pending.
 *
 * @param descriptor Serial descriptor.
 * @return Number of queued bytes, or 0 if descriptor is invalid/uninitialized.
 */
size_t serial_driver_pending_tx(serial_descriptor_t descriptor);

/**
 * @brief Resolve descriptor to associated UART device.
 *
 * @param descriptor Serial descriptor.
 * @return Mapped UART device pointer, or NULL if descriptor is invalid.
 */
uart_device_t *serial_driver_get_uart_device(serial_descriptor_t descriptor);

#endif
