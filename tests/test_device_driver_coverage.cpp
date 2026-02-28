#include <array>
#include <cstdint>
#include <cstring>

extern "C"
{
#include "device_driver/device_driver.h"

uart_error_t serial_driver_hw_map_uart(size_t port_index,
                                       uart_device_t *uart_device);
}

#include <gtest/gtest.h>

namespace
{

xr17c358_channel_register_map_t g_registers[UART_DEVICE_COUNT];

uart_error_t CoverageMapper(size_t port_index, uart_device_t *uart_device,
                            void *context)
{
    (void)context;

    if (uart_device == nullptr || port_index >= UART_DEVICE_COUNT)
    {
        return UART_ERROR_INVALID_ARG;
    }

    std::memset(&g_registers[port_index], 0, sizeof(g_registers[port_index]));
    uart_device->registers = &g_registers[port_index];
    uart_device->uart_base_address =
        reinterpret_cast<uintptr_t>(&g_registers[port_index]);
    uart_device->device_name = "coverage-uart";
    return UART_ERROR_NONE;
}

uart_error_t FailingMapper(size_t port_index, uart_device_t *uart_device,
                           void *context)
{
    (void)port_index;
    (void)uart_device;
    (void)context;
    return UART_ERROR_DEVICE_NOT_FOUND;
}

uart_error_t NoRegistersMapper(size_t port_index, uart_device_t *uart_device,
                               void *context)
{
    (void)context;
    if (uart_device == nullptr || port_index >= UART_DEVICE_COUNT)
    {
        return UART_ERROR_INVALID_ARG;
    }

    uart_device->registers = nullptr;
    return UART_ERROR_NONE;
}

void ResetByteFifo(uart_byte_fifo_t *fifo)
{
    fifo->head = 0U;
    fifo->tail = 0U;
    fifo->count = 0U;
}

void FillQueue(serial_queue_t *queue)
{
    ASSERT_EQ(serial_queue_init(queue), UART_ERROR_NONE);
    for (size_t i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_WORDS; ++i)
    {
        ASSERT_EQ(serial_queue_push(queue, static_cast<uint32_t>(i)),
                  UART_ERROR_NONE);
    }
}

void PushReadByte(size_t port_index, uint8_t value)
{
    uart_byte_fifo_t *fifo = &uart_fifo_map.read_fifos[port_index];
    fifo->data[fifo->head] = value;
    fifo->head = (fifo->head + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count += 1U;
}

} // namespace

TEST(SerialDriverCoverageApiTest, ExerciseCoverageBranches)
{
    ASSERT_EQ(serial_driver_hw_set_mapper(CoverageMapper, nullptr),
              UART_ERROR_NONE);

    const serial_descriptor_t descriptor0 =
        serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor0, SERIAL_DESCRIPTOR_INVALID);
    EXPECT_EQ(serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL),
              descriptor0);

    ASSERT_EQ(serial_driver_hw_set_mapper(FailingMapper, nullptr),
              UART_ERROR_NONE);
    EXPECT_EQ(serial_port_init(SERIAL_PORT_1, UART_PORT_MODE_SERIAL),
              SERIAL_DESCRIPTOR_INVALID);

    ASSERT_EQ(serial_driver_hw_set_mapper(NoRegistersMapper, nullptr),
              UART_ERROR_NONE);
    EXPECT_EQ(serial_port_init(SERIAL_PORT_2, UART_PORT_MODE_SERIAL),
              SERIAL_DESCRIPTOR_INVALID);

    ASSERT_EQ(serial_driver_hw_set_mapper(CoverageMapper, nullptr),
              UART_ERROR_NONE);

    const serial_descriptor_t descriptor3 =
        serial_port_init(SERIAL_PORT_3, UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor3, SERIAL_DESCRIPTOR_INVALID);
    FillQueue(&uart_devices[SERIAL_PORT_3].tx_queue);

    const std::array<uint8_t, 5> five_bytes{{1U, 2U, 3U, 4U, 5U}};
    size_t bytes_written = 0U;
    EXPECT_EQ(serial_driver_write(descriptor3, five_bytes.data(),
                                  five_bytes.size(), &bytes_written),
              SERIAL_DRIVER_ERROR_TX_FULL);
    EXPECT_EQ(bytes_written, 4U);

    const serial_descriptor_t descriptor4 =
        serial_port_init(SERIAL_PORT_4, UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor4, SERIAL_DESCRIPTOR_INVALID);
    FillQueue(&uart_devices[SERIAL_PORT_4].tx_queue);
    const std::array<uint8_t, 4> four_bytes{{9U, 8U, 7U, 6U}};
    EXPECT_EQ(serial_driver_write(descriptor4, four_bytes.data(),
                                  four_bytes.size(), &bytes_written),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(bytes_written, four_bytes.size());

    const serial_descriptor_t descriptor5 =
        serial_port_init(SERIAL_PORT_5, UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor5, SERIAL_DESCRIPTOR_INVALID);
    uart_devices[SERIAL_PORT_5].tx_queue.initialized = false;
    EXPECT_EQ(serial_driver_write(descriptor5, five_bytes.data(),
                                  five_bytes.size(), &bytes_written),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    const serial_descriptor_t descriptor6 =
        serial_port_init(SERIAL_PORT_6, UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor6, SERIAL_DESCRIPTOR_INVALID);
    EXPECT_EQ(serial_driver_write(descriptor6, four_bytes.data(),
                                  four_bytes.size(), &bytes_written),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(bytes_written, four_bytes.size());

    const serial_descriptor_t descriptor7 =
        serial_port_init(SERIAL_PORT_7, UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor7, SERIAL_DESCRIPTOR_INVALID);
    uart_devices[SERIAL_PORT_7].tx_queue.initialized = false;
    EXPECT_EQ(serial_driver_write(descriptor7, four_bytes.data(),
                                  four_bytes.size(), &bytes_written),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    uint8_t out_byte = 0U;
    size_t bytes_read = 0U;
    EXPECT_EQ(serial_driver_write(descriptor6, nullptr, 0U, &bytes_written),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(bytes_written, 0U);

    EXPECT_EQ(serial_driver_read(descriptor6, nullptr, 0U, &bytes_read),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(bytes_read, 0U);

    EXPECT_EQ(serial_driver_read(descriptor6, &out_byte, 1U, &bytes_read),
              SERIAL_DRIVER_ERROR_RX_EMPTY);
    EXPECT_EQ(bytes_read, 0U);

    uart_devices[SERIAL_PORT_6].rx_queue.initialized = false;
    EXPECT_EQ(serial_driver_read(descriptor6, &out_byte, 1U, &bytes_read),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    uart_devices[SERIAL_PORT_6].tx_queue.initialized = false;
    size_t tx_bytes = 0U;
    size_t rx_bytes = 0U;
    EXPECT_EQ(serial_driver_poll(descriptor6, 1U, 1U, &tx_bytes, &rx_bytes),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    ASSERT_EQ(serial_driver_write(descriptor0, five_bytes.data(),
                                  five_bytes.size(), &bytes_written),
              SERIAL_DRIVER_OK);
    ResetByteFifo(&uart_fifo_map.read_fifos[SERIAL_PORT_0]);
    PushReadByte(SERIAL_PORT_0, 0xABU);

    EXPECT_EQ(serial_driver_poll(descriptor0, 1U, 4U, &tx_bytes, &rx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 1U);
    EXPECT_EQ(rx_bytes, 0U);
    EXPECT_EQ(uart_fifo_map.read_fifos[SERIAL_PORT_0].count, 1U);

    const serial_descriptor_t descriptor1 =
        serial_port_init(SERIAL_PORT_1, UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor1, SERIAL_DESCRIPTOR_INVALID);
    const std::array<uint8_t, 1> one_byte{{0x5AU}};
    ASSERT_EQ(serial_driver_write(descriptor1, one_byte.data(), one_byte.size(),
                                  &bytes_written),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(serial_driver_poll(descriptor1, 0U, 0U, &tx_bytes, &rx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 0U);
    EXPECT_EQ(rx_bytes, 0U);

    serial_driver_hw_reset_mapper();

    EXPECT_EQ(serial_driver_hw_set_mapper(nullptr, nullptr),
              UART_ERROR_INVALID_ARG);

    uart_device_t device = {};
    EXPECT_EQ(serial_driver_hw_map_uart(UART_DEVICE_COUNT, &device),
              UART_ERROR_INVALID_ARG);
    EXPECT_EQ(serial_driver_hw_map_uart(0U, nullptr), UART_ERROR_INVALID_ARG);

    uart_device_t base_mapped = {};
    base_mapped.uart_base_address =
        reinterpret_cast<uintptr_t>(&g_registers[SERIAL_PORT_0]);
    EXPECT_EQ(serial_driver_hw_map_uart(SERIAL_PORT_0, &base_mapped),
              UART_ERROR_NONE);
    EXPECT_EQ(base_mapped.registers, &g_registers[SERIAL_PORT_0]);
    EXPECT_STREQ(base_mapped.device_name, "uart0");

    uart_device_t default_mapped = {};
    EXPECT_EQ(serial_driver_hw_map_uart(SERIAL_PORT_1, &default_mapped),
              UART_ERROR_NONE);
    ASSERT_NE(default_mapped.registers, nullptr);
    EXPECT_EQ(default_mapped.uart_base_address,
              reinterpret_cast<uintptr_t>(default_mapped.registers));
    EXPECT_STREQ(default_mapped.device_name, "uart1");

    uart_device_t already_mapped = {};
    already_mapped.registers = &g_registers[SERIAL_PORT_0];
    already_mapped.uart_base_address =
        reinterpret_cast<uintptr_t>(&g_registers[SERIAL_PORT_0]);
    already_mapped.device_name = "already-set";
    EXPECT_EQ(serial_driver_hw_map_uart(SERIAL_PORT_0, &already_mapped),
              UART_ERROR_NONE);
    EXPECT_EQ(already_mapped.registers, &g_registers[SERIAL_PORT_0]);
    EXPECT_STREQ(already_mapped.device_name, "already-set");
}
