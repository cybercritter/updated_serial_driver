#ifndef SERIAL_DRIVER_QUEUE_H
#define SERIAL_DRIVER_QUEUE_H

/**
 * @file queue.h
 * @brief Simple byte-oriented circular queue utilities.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "serial_driver/errors.h"

/** Fixed queue storage size in bytes (1.2K). */
#define SERIAL_QUEUE_FIXED_SIZE_BYTES 1200U

/**
 * @brief Circular queue for byte storage.
 */
typedef struct {
  /** Fixed-size backing storage buffer. */
  uint8_t buffer[SERIAL_QUEUE_FIXED_SIZE_BYTES];
  /** Write index for next pushed byte. */
  size_t head;
  /** Read index for next popped byte. */
  size_t tail;
  /** Number of bytes currently queued. */
  size_t count;
  /** Set to true once queue has been initialized. */
  bool initialized;
} serial_queue_t;

/**
 * @brief Initialize a queue with built-in fixed storage.
 *
 * @param queue Queue context to initialize.
 * @return @ref UART_ERROR_NONE on success, otherwise an error code.
 */
uart_error_t serial_queue_init(serial_queue_t *queue);

/**
 * @brief Push one byte into the queue.
 *
 * @param queue Initialized queue context.
 * @param value Byte to enqueue.
 * @return @ref UART_ERROR_NONE on success, otherwise an error code.
 */
uart_error_t serial_queue_push(serial_queue_t *queue, uint8_t value);

/**
 * @brief Pop one byte from the queue.
 *
 * @param queue Initialized queue context.
 * @param out_value Output pointer for popped byte.
 * @return @ref UART_ERROR_NONE on success, otherwise an error code.
 */
uart_error_t serial_queue_pop(serial_queue_t *queue, uint8_t *out_value);

/**
 * @brief Return current queued byte count.
 *
 * @param queue Queue context.
 * @return Number of bytes currently queued, or 0 for invalid queue.
 */
size_t serial_queue_size(const serial_queue_t *queue);

/**
 * @brief Return whether queue has no queued bytes.
 *
 * @param queue Queue context.
 * @return true if queue is empty or invalid; otherwise false.
 */
bool serial_queue_is_empty(const serial_queue_t *queue);

/**
 * @brief Return whether queue cannot accept more bytes.
 *
 * @param queue Queue context.
 * @return true if queue is full; otherwise false.
 */
bool serial_queue_is_full(const serial_queue_t *queue);

#endif
