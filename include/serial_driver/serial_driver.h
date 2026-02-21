#ifndef SERIAL_DRIVER_SERIAL_DRIVER_H
#define SERIAL_DRIVER_SERIAL_DRIVER_H

/**
 * @file serial_driver.h
 * @brief Public API for the software serial TX queue.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Status codes returned by serial driver queue operations.
 */
typedef enum {
    /** Operation completed successfully. */
    SERIAL_DRIVER_OK = 0,
    /** Invalid function argument or invalid driver state. */
    SERIAL_DRIVER_INVALID_ARG,
    /** TX queue cannot accept additional bytes. */
    SERIAL_DRIVER_BUFFER_FULL,
    /** TX queue has no data available to read. */
    SERIAL_DRIVER_BUFFER_EMPTY
} serial_driver_status_t;

/**
 * @brief Software TX circular buffer state.
 */
typedef struct {
    /** Backing storage for queued TX bytes. */
    uint8_t *tx_buffer;
    /** Size of @ref tx_buffer in bytes. */
    size_t tx_capacity;
    /** Write index for the next queued byte. */
    size_t tx_head;
    /** Read index for the next byte to transmit. */
    size_t tx_tail;
    /** Tracks whether @ref serial_driver_init has completed successfully. */
    bool initialized;
} serial_driver_t;

/**
 * @brief Initialize a serial driver TX queue with caller-provided storage.
 *
 * @param driver Driver context to initialize.
 * @param tx_buffer Storage buffer used as circular queue.
 * @param tx_capacity Capacity of @p tx_buffer (must be >= 2).
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_status_t serial_driver_init(
    serial_driver_t *driver,
    uint8_t *tx_buffer,
    size_t tx_capacity);

/**
 * @brief Queue one byte for transmit.
 *
 * @param driver Initialized driver context.
 * @param byte Byte to enqueue.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_status_t serial_driver_write_byte(
    serial_driver_t *driver,
    uint8_t byte);

/**
 * @brief Read and remove the next queued TX byte.
 *
 * @param driver Initialized driver context.
 * @param out_byte Output pointer for the dequeued byte.
 * @return @ref SERIAL_DRIVER_OK on success, otherwise an error code.
 */
serial_driver_status_t serial_driver_read_next_tx(
    serial_driver_t *driver,
    uint8_t *out_byte);

/**
 * @brief Return number of queued TX bytes currently pending.
 *
 * @param driver Driver context.
 * @return Number of queued bytes, or 0 if the driver is invalid/uninitialized.
 */
size_t serial_driver_pending_tx(const serial_driver_t *driver);

#endif
