#ifndef SERIAL_DRIVER_HW_ABSTRACTION_H
#define SERIAL_DRIVER_HW_ABSTRACTION_H

/**
 * @file hw_abstraction.h
 * @brief Platform-specific UART register mapping hooks.
 */

#include <stddef.h>

#include "device_driver/registers.h"

/**
 * @brief Callback used to map one UART device to platform registers.
 *
 * The callback should populate at minimum @p uart_device->registers and may
 * also set fields such as @p uart_device->uart_base_address or
 * @p uart_device->device_name.
 *
 * @param port_index UART port index in range [0, UART_DEVICE_COUNT).
 * @param uart_device UART device entry associated with @p port_index.
 * @param context Caller-provided context passed at registration time.
 * @return @ref UART_ERROR_NONE on success, otherwise an error code.
 */
typedef uart_error_t (*serial_driver_hw_map_fn)(size_t port_index,
                                                uart_device_t *uart_device,
                                                void *context);

/**
 * @brief Register a platform-specific UART mapping callback.
 *
 * @param mapper Mapping callback to use for future port initialization.
 * @param context Opaque callback context.
 * @return @ref UART_ERROR_NONE on success, otherwise an error code.
 */
uart_error_t serial_driver_hw_set_mapper(serial_driver_hw_map_fn mapper,
                                         void *context);

/**
 * @brief Restore the built-in default UART mapping callback.
 */
void serial_driver_hw_reset_mapper(void);

#endif
