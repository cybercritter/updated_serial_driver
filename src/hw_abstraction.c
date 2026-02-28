#include "device_driver/hw_abstraction.h"

#include <stddef.h>
#include <stdint.h>

static xr17c358_channel_register_map_t default_register_blocks[UART_DEVICE_COUNT] = {0};
static const char *const default_device_names[UART_DEVICE_COUNT] = {
    "uart0", "uart1", "uart2", "uart3",
    "uart4", "uart5", "uart6", "uart7"};

static uart_error_t serial_driver_default_hw_map(size_t port_index,
                                                 uart_device_t *uart_device,
                                                 void *context)
{
    (void)context;

    if (uart_device == NULL || port_index >= UART_DEVICE_COUNT)
    {
        return UART_ERROR_INVALID_ARG;
    }

    if (uart_device->registers == NULL)
    {
        if (uart_device->uart_base_address != (uintptr_t)0U)
        {
            uart_device->registers =
                (xr17c358_channel_register_map_t *)uart_device->uart_base_address;
        }
        else
        {
            uart_device->registers = &default_register_blocks[port_index];
            uart_device->uart_base_address =
                (uintptr_t)&default_register_blocks[port_index];
        }
    }

    if (uart_device->device_name == NULL)
    {
        uart_device->device_name = default_device_names[port_index];
    }

    return UART_ERROR_NONE;
}

static serial_driver_hw_map_fn serial_driver_hw_mapper =
    serial_driver_default_hw_map;
static void *serial_driver_hw_mapper_context = NULL;

uart_error_t serial_driver_hw_map_uart(size_t port_index,
                                       uart_device_t *uart_device)
{
    return serial_driver_hw_mapper(port_index, uart_device,
                                   serial_driver_hw_mapper_context);
}

uart_error_t serial_driver_hw_set_mapper(serial_driver_hw_map_fn mapper,
                                         void *context)
{
    if (mapper == NULL)
    {
        return UART_ERROR_INVALID_ARG;
    }

    serial_driver_hw_mapper = mapper;
    serial_driver_hw_mapper_context = context;
    return UART_ERROR_NONE;
}

void serial_driver_hw_reset_mapper(void)
{
    serial_driver_hw_mapper = serial_driver_default_hw_map;
    serial_driver_hw_mapper_context = NULL;
}
