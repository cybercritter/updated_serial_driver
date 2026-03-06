#ifndef SERIAL_DRIVER_SERIAL_DRIVER_H
#define SERIAL_DRIVER_SERIAL_DRIVER_H

/**
 * @file device_driver.h
 * @brief Public API for UART descriptor initialization, serial I/O, and
 * polling.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include "registers.h"

    /** Opaque serial-driver descriptor returned by @ref serial_port_init. */
    typedef uint32_t serial_descriptor_t;

/** Sentinel for an invalid descriptor. */
#define SERIAL_DESCRIPTOR_INVALID ((serial_descriptor_t)0U)

/** Frame delimiter byte used by escape encoding. */
#define SERIAL_DRIVER_ESCAPE_FRAME_FLAG 0x7EU
/** Escape introducer byte used by escape encoding. */
#define SERIAL_DRIVER_ESCAPE_BYTE 0x7DU
/** XOR mask applied to escaped payload bytes. */
#define SERIAL_DRIVER_ESCAPE_XOR 0x20U

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
     * @return Valid serial descriptor on success, @ref
     * SERIAL_DESCRIPTOR_INVALID on failure.
     */
    serial_descriptor_t serial_port_init(serial_ports_t port,
                                         uart_port_mode_t mode);

    /**
     * @brief Queue transmit bytes from a user buffer.
     *
     * Bytes are accepted in-order and later transmitted to the device TX FIFO
     * by polling/transmit APIs.
     *
     * @param descriptor Initialized serial descriptor.
     * @param data Input byte buffer.
     * @param length Number of bytes to queue.
     * @param out_bytes_written Output number of bytes accepted.
     * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
     */
    serial_driver_error_t serial_driver_write(serial_descriptor_t descriptor,
                                              const uint8_t *data,
                                              size_t length,
                                              size_t *out_bytes_written);

    /**
     * @brief Encode one payload into an escaped frame.
     *
     * Output format:
     * - leading @ref SERIAL_DRIVER_ESCAPE_FRAME_FLAG
     * - payload bytes with special-byte escaping
     * - trailing @ref SERIAL_DRIVER_ESCAPE_FRAME_FLAG
     *
     * Bytes equal to @ref SERIAL_DRIVER_ESCAPE_FRAME_FLAG or
     * @ref SERIAL_DRIVER_ESCAPE_BYTE are emitted as two bytes:
     * escape byte followed by payload byte XORed with
     * @ref SERIAL_DRIVER_ESCAPE_XOR.
     *
     * @param data Raw payload bytes.
     * @param length Number of payload bytes.
     * @param out_encoded Destination buffer for escaped frame bytes.
     * @param out_encoded_capacity Capacity of @p out_encoded in bytes.
     * @param out_encoded_length Output escaped frame length on success.
     * @return @ref SERIAL_DRIVER_OK on success.
     * @return @ref SERIAL_DRIVER_ERROR_TX_FULL when destination capacity is too
     * small.
     * @return @ref SERIAL_DRIVER_ERROR_INVALID_ARG for invalid parameters.
     */
    serial_driver_error_t serial_driver_escape_encode(
        const uint8_t *data, size_t length, uint8_t *out_encoded,
        size_t out_encoded_capacity, size_t *out_encoded_length);

    /**
     * @brief Decode one escaped frame into raw payload bytes.
     *
     * Input must contain exactly one frame delimited by leading and trailing
     * @ref SERIAL_DRIVER_ESCAPE_FRAME_FLAG.
     *
     * @param encoded Escaped frame bytes.
     * @param encoded_length Number of bytes in @p encoded.
     * @param out_decoded Destination buffer for decoded payload.
     * @param out_decoded_capacity Capacity of @p out_decoded in bytes.
     * @param out_decoded_length Output decoded payload length on success.
     * @return @ref SERIAL_DRIVER_OK on success.
     * @return @ref SERIAL_DRIVER_ERROR_RX_FULL when destination capacity is too
     * small.
     * @return @ref SERIAL_DRIVER_ERROR_INVALID_ARG for invalid parameters or
     * malformed frame data.
     */
    serial_driver_error_t serial_driver_escape_decode(
        const uint8_t *encoded, size_t encoded_length, uint8_t *out_decoded,
        size_t out_decoded_capacity, size_t *out_decoded_length);

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
     * @brief Assert the discrete control line bit (RTS) for a discrete
     * descriptor.
     *
     * @param descriptor Serial descriptor.
     * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
     */
    serial_driver_error_t
    serial_driver_enable_discrete(serial_descriptor_t descriptor);

    /**
     * @brief De-assert the discrete control line bit (RTS) for a discrete
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

#ifdef __cplusplus
}
#endif

#endif
