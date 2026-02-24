#include "serial_driver/queue.h"

static size_t queue_next_index(const serial_queue_t *queue, size_t current)
{
    (void)queue;
    return (current + 1U) % SERIAL_QUEUE_FIXED_SIZE_BYTES;
}

uart_error_t serial_queue_init(serial_queue_t *queue)
{
    if (queue == NULL) {
        return UART_ERROR_INVALID_ARG;
    }

    queue->head = 0U;
    queue->tail = 0U;
    queue->count = 0U;
    queue->initialized = true;
    return UART_ERROR_NONE;
}

uart_error_t serial_queue_push(serial_queue_t *queue, uint8_t value)
{
    size_t next_head = 0U;

    if (queue == NULL || !queue->initialized) {
        return UART_ERROR_NOT_INITIALIZED;
    }

    if (queue->count == SERIAL_QUEUE_FIXED_SIZE_BYTES) {
        return UART_ERROR_FIFO_FULL;
    }

    next_head = queue_next_index(queue, queue->head);
    queue->buffer[queue->head] = value;
    queue->head = next_head;
    queue->count += 1U;
    return UART_ERROR_NONE;
}

uart_error_t serial_queue_pop(serial_queue_t *queue, uint8_t *out_value)
{
    if (queue == NULL || out_value == NULL) {
        return UART_ERROR_INVALID_ARG;
    }

    if (!queue->initialized) {
        return UART_ERROR_NOT_INITIALIZED;
    }

    if (queue->count == 0U) {
        return UART_ERROR_FIFO_EMPTY;
    }

    *out_value = queue->buffer[queue->tail];
    queue->tail = queue_next_index(queue, queue->tail);
    queue->count -= 1U;
    return UART_ERROR_NONE;
}

size_t serial_queue_size(const serial_queue_t *queue)
{
    if (queue == NULL || !queue->initialized) {
        return 0U;
    }

    return queue->count;
}

bool serial_queue_is_empty(const serial_queue_t *queue)
{
    return serial_queue_size(queue) == 0U;
}

bool serial_queue_is_full(const serial_queue_t *queue)
{
    if (queue == NULL || !queue->initialized) {
        return false;
    }

    return queue->count == SERIAL_QUEUE_FIXED_SIZE_BYTES;
}
