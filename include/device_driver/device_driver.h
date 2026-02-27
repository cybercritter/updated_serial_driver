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
serial_driver_error_t
serial_driver_read_next_tx_u32(serial_descriptor_t descriptor,
                               uint32_t *out_value);

/**
 * @brief Transmit queued data bytes into the device TX FIFO.
 *
 * Converts queued 32-bit words to bytes (LSB first) and writes into the
 * per-device FIFO up to @p max_bytes or until FIFO/data limits are reached.
 *
 * @param descriptor Serial descriptor.
 * @param max_bytes Maximum number of bytes to transmit this call.
 * @param out_bytes_transmitted Output number of bytes actually transmitted.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t
serial_driver_transmit_to_device_fifo(serial_descriptor_t descriptor,
                                      size_t max_bytes,
                                      size_t *out_bytes_transmitted);

/**
 * @brief Poll bytes from the device RX FIFO into the software RX queue.
 *
 * Collects incoming bytes from the per-device RX FIFO and assembles 32-bit
 * words (LSB first) into the RX queue.
 *
 * @param descriptor Serial descriptor.
 * @param max_bytes Maximum number of bytes to poll this call.
 * @param out_bytes_received Output number of bytes actually consumed.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t
serial_driver_receive_from_device_fifo(serial_descriptor_t descriptor,
                                       size_t max_bytes,
                                       size_t *out_bytes_received);

/**
 * @brief Poll one serial port: drain TX first, then service RX.
 *
 * This call transmits up to @p max_tx_bytes from the software TX queue into
 * the device TX FIFO. RX polling is performed only when TX is fully drained
 * (no queued TX words and no staged TX bytes remain), then up to
 * @p max_rx_bytes are consumed from the device RX FIFO into the software RX
 * queue.
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

/**
 * @brief Read received bytes into a user buffer.
 *
 * Reads bytes from the software RX path in-order.
 *
 * @param descriptor Serial descriptor.
 * @param data Output byte buffer.
 * @param length Maximum number of bytes to read.
 * @param out_bytes_read Output number of bytes read.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t serial_driver_read(serial_descriptor_t descriptor,
                                         uint8_t *data, size_t length,
                                         size_t *out_bytes_read);

/**
 * @brief Read and remove the next queued 32-bit RX value.
 *
 * @param descriptor Serial descriptor.
 * @param out_value Output pointer for the dequeued 32-bit value.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_error_t serial_driver_read_u32(serial_descriptor_t descriptor,
                                             uint32_t *out_value);

/**
 * @brief Return number of queued RX words currently pending.
 *
 * @param descriptor Serial descriptor.
 * @return Number of queued RX words, or 0 if descriptor is invalid.
 */
size_t serial_driver_pending_rx(serial_descriptor_t descriptor);

/**
 * @brief Return number of queued TX words currently pending.
 *
 * @param descriptor Serial descriptor.
 * @return Number of queued words, or 0 if descriptor is invalid/uninitialized.
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
