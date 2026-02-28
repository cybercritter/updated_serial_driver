#ifndef SERIAL_DRIVER_SERIAL_DRIVER_H
#define SERIAL_DRIVER_SERIAL_DRIVER_H

/**
 * @file device_driver.h
 * @brief Public API for the software serial TX queue.
 */

#include <stddef.h>
#include <stdint.h>

#include "hw_abstraction.h"
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
typedef enum
{
    /** Operation completed successfully. */
    SERIAL_DRIVER_OK = UART_ERROR_NONE,
    /** One or more function arguments are invalid. */
    SERIAL_DRIVER_ERROR_INVALID_ARG = UART_ERROR_INVALID_ARG,
    /** Driver has not been initialized. */
    SERIAL_DRIVER_ERROR_NOT_INITIALIZED = UART_ERROR_NOT_INITIALIZED,
    /** UART was configured, but not in serial mode. */
    SERIAL_DRIVER_ERROR_NOT_CONFIGURED = UART_ERROR_NOT_CONFIGURED,
    /** TX queue has no free space for additional bytes. */
    SERIAL_DRIVER_ERROR_TX_FULL = UART_ERROR_FIFO_QUEUE_FULL,
    /** TX queue has no bytes available to read. */
    SERIAL_DRIVER_ERROR_TX_EMPTY = UART_ERROR_FIFO_QUEUE_EMPTY,
    /** RX queue has no free space for additional words. */
    SERIAL_DRIVER_ERROR_RX_FULL = UART_ERROR_FIFO_QUEUE_FULL,
    /** RX queue has no words available to read. */
    SERIAL_DRIVER_ERROR_RX_EMPTY = UART_ERROR_FIFO_QUEUE_EMPTY,
    /** Invalid serial port. */
    SERIAL_DRIVER_ERROR_INVALID_PORT = UART_ERROR_INVALID_ARG,
} serial_driver_error_t;

/**
 * @brief UART port identifiers for serial descriptor allocation.
 */
typedef enum SERIAL_PORTS
{
    SERIAL_PORT_0,
    SERIAL_PORT_1,
    SERIAL_PORT_2,
    SERIAL_PORT_3,
    SERIAL_PORT_4,
    SERIAL_PORT_5,
    SERIAL_PORT_6,
    SERIAL_PORT_7,
} serial_ports_t;

/**
 * @brief Initialize one UART port instance and return its descriptor.
 *
 * @param port UART port used to resolve the device from @ref uart_devices.
 * @param mode Configuration mode for this UART (serial or discrete).
 * @return Valid serial descriptor on success, @ref SERIAL_DESCRIPTOR_INVALID on
 * failure.
 */
serial_descriptor_t serial_port_init(serial_ports_t port,
                                     uart_port_mode_t mode);

/**
 * @brief Queue transmit bytes from a user buffer.
 *
 * Bytes are accepted in-order and later transmitted to the device TX FIFO by
 * polling/transmit APIs.
 *
 * @param descriptor Initialized serial descriptor.
 * @param data Input byte buffer.
 * @param length Number of bytes to queue.
 * @param out_bytes_written Output number of bytes accepted.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t serial_driver_write(serial_descriptor_t descriptor,
                                          const uint8_t *data, size_t length,
                                          size_t *out_bytes_written);

/**
 * @brief Read received bytes into a user buffer.
 *
 * @param descriptor Initialized serial descriptor.
 * @param data Output byte buffer.
 * @param length Maximum number of bytes to read.
 * @param out_bytes_read Output number of bytes read.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t serial_driver_read(serial_descriptor_t descriptor,
                                         uint8_t *data, size_t length,
                                         size_t *out_bytes_read);

/**
 * @brief Enable UART local loopback for a serial descriptor.
 *
 * @param descriptor Serial descriptor.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t
serial_driver_enable_loopback(serial_descriptor_t descriptor);

/**
 * @brief Disable UART local loopback for a serial descriptor.
 *
 * @param descriptor Serial descriptor.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t
serial_driver_disable_loopback(serial_descriptor_t descriptor);

/**
 * @brief Assert the discrete control line bit (#RTS) for a discrete
 * descriptor.
 *
 * @param descriptor Serial descriptor.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t
serial_driver_enable_discrete(serial_descriptor_t descriptor);

/**
 * @brief Deassert the discrete control line bit (#RTS) for a discrete
 * descriptor.
 *
 * @param descriptor Serial descriptor.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t
serial_driver_disable_discrete(serial_descriptor_t descriptor);

/**
 * @brief Poll one serial port: drain TX first, then service RX.
 *
 * @param descriptor Serial descriptor.
 * @param max_tx_bytes Maximum TX bytes to write this call.
 * @param max_rx_bytes Maximum RX bytes to read this call.
 * @param out_tx_bytes_transmitted Output number of TX bytes written.
 * @param out_rx_bytes_received Output number of RX bytes consumed.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t serial_driver_poll(serial_descriptor_t descriptor,
                                         size_t max_tx_bytes,
                                         size_t max_rx_bytes,
                                         size_t *out_tx_bytes_transmitted,
                                         size_t *out_rx_bytes_received);

#endif
