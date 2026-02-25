#ifndef SERIAL_DRIVER_ERRORS_H
#define SERIAL_DRIVER_ERRORS_H

/**
 * @file errors.h
 * @brief Common UART and FIFO error/status codes.
 */

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
    UART_ERROR_FIFO_QUEUE_FULL,
    /** FIFO/circular buffer has no bytes available. */
    UART_ERROR_FIFO_QUEUE_EMPTY,
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

#endif
