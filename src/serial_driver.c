#include "serial_driver/serial_driver.h"

static size_t next_index(const serial_driver_t *driver, size_t current)
{
    return (current + 1U) % driver->tx_capacity;
}

serial_driver_status_t serial_driver_init(
    serial_driver_t *driver,
    uint8_t *tx_buffer,
    size_t tx_capacity)
{
    if (driver == NULL || tx_buffer == NULL || tx_capacity < 2U) {
        return SERIAL_DRIVER_INVALID_ARG;
    }

    driver->tx_buffer = tx_buffer;
    driver->tx_capacity = tx_capacity;
    driver->tx_head = 0U;
    driver->tx_tail = 0U;
    driver->initialized = true;
    return SERIAL_DRIVER_OK;
}

serial_driver_status_t serial_driver_write_byte(
    serial_driver_t *driver,
    uint8_t byte)
{
    size_t next_head = 0U;

    if (driver == NULL || !driver->initialized) {
        return SERIAL_DRIVER_INVALID_ARG;
    }

    next_head = next_index(driver, driver->tx_head);
    if (next_head == driver->tx_tail) {
        return SERIAL_DRIVER_BUFFER_FULL;
    }

    driver->tx_buffer[driver->tx_head] = byte;
    driver->tx_head = next_head;
    return SERIAL_DRIVER_OK;
}

serial_driver_status_t serial_driver_read_next_tx(
    serial_driver_t *driver,
    uint8_t *out_byte)
{
    if (driver == NULL || out_byte == NULL || !driver->initialized) {
        return SERIAL_DRIVER_INVALID_ARG;
    }

    if (driver->tx_head == driver->tx_tail) {
        return SERIAL_DRIVER_BUFFER_EMPTY;
    }

    *out_byte = driver->tx_buffer[driver->tx_tail];
    driver->tx_tail = next_index(driver, driver->tx_tail);
    return SERIAL_DRIVER_OK;
}

size_t serial_driver_pending_tx(const serial_driver_t *driver)
{
    if (driver == NULL || !driver->initialized) {
        return 0U;
    }

    if (driver->tx_head >= driver->tx_tail) {
        return driver->tx_head - driver->tx_tail;
    }

    return driver->tx_capacity - (driver->tx_tail - driver->tx_head);
}
