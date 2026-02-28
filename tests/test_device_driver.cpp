#include <array>
#include <cstdint>
#include <cstring>

extern "C"
{
#include "device_driver/device_driver.h"
}

#include <gtest/gtest.h>

namespace
{

xr17c358_channel_register_map_t g_test_registers[UART_DEVICE_COUNT];

uart_error_t TestMapper(size_t port_index, uart_device_t *uart_device, void *context)
{
    (void)context;

    if (uart_device == nullptr || port_index >= UART_DEVICE_COUNT)
    {
        return UART_ERROR_INVALID_ARG;
    }

    std::memset(&g_test_registers[port_index], 0,
                sizeof(g_test_registers[port_index]));

    uart_device->registers = &g_test_registers[port_index];
    uart_device->uart_base_address =
        reinterpret_cast<uintptr_t>(&g_test_registers[port_index]);
    uart_device->device_name = "test-uart";

    return UART_ERROR_NONE;
}

void ResetFifo(uart_byte_fifo_t *fifo)
{
    fifo->head = 0U;
    fifo->tail = 0U;
    fifo->count = 0U;
}

bool FifoIsEmpty(const uart_byte_fifo_t *fifo)
{
    return fifo->count == 0U;
}

bool FifoIsFull(const uart_byte_fifo_t *fifo)
{
    return fifo->count == UART_DEVICE_FIFO_SIZE_BYTES;
}

void FifoPush(uart_byte_fifo_t *fifo, uint8_t value)
{
    fifo->data[fifo->head] = value;
    fifo->head = (fifo->head + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count += 1U;
}

uint8_t FifoPop(uart_byte_fifo_t *fifo)
{
    const uint8_t value = fifo->data[fifo->tail];
    fifo->tail = (fifo->tail + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
    fifo->count -= 1U;
    return value;
}

size_t MoveWriteToRead(size_t port_index)
{
    size_t moved = 0U;
    uart_byte_fifo_t *write_fifo = &uart_fifo_map.write_fifos[port_index];
    uart_byte_fifo_t *read_fifo = &uart_fifo_map.read_fifos[port_index];

    while (!FifoIsEmpty(write_fifo) && !FifoIsFull(read_fifo))
    {
        FifoPush(read_fifo, FifoPop(write_fifo));
        moved += 1U;
    }

    return moved;
}

} // namespace

class SerialDriverApiTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ASSERT_EQ(serial_driver_hw_set_mapper(TestMapper, nullptr),
                  UART_ERROR_NONE);
    }

    void TearDown() override
    {
        serial_driver_hw_reset_mapper();
    }
};

TEST_F(SerialDriverApiTest, PortInitRejectsInvalidPortAndMode)
{
    EXPECT_EQ(serial_port_init(static_cast<serial_ports_t>(UART_DEVICE_COUNT),
                               UART_PORT_MODE_SERIAL),
              SERIAL_DESCRIPTOR_INVALID);

    EXPECT_EQ(serial_port_init(SERIAL_PORT_0, static_cast<uart_port_mode_t>(99)),
              SERIAL_DESCRIPTOR_INVALID);
}

TEST_F(SerialDriverApiTest, SerialRoundTripViaPollAndReadWrite)
{
    constexpr size_t kPort = SERIAL_PORT_0;
    const std::array<uint8_t, 6> payload{{0x10U, 0x20U, 0x30U, 0x40U, 0x50U,
                                          0x60U}};
    std::array<uint8_t, payload.size()> received{{0U}};
    size_t bytes_written = 0U;
    size_t tx_bytes = 0U;
    size_t rx_bytes = 0U;
    size_t bytes_read = 0U;

    ResetFifo(&uart_fifo_map.write_fifos[kPort]);
    ResetFifo(&uart_fifo_map.read_fifos[kPort]);

    const serial_descriptor_t descriptor =
        serial_port_init(static_cast<serial_ports_t>(kPort),
                         UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

    ASSERT_EQ(serial_driver_enable_loopback(descriptor), SERIAL_DRIVER_OK);
    ASSERT_NE((uart_devices[kPort].registers->uart.mcr & UART_MCR_LOOPBACK_BIT),
              0U);

    ASSERT_EQ(serial_driver_write(descriptor, payload.data(), payload.size(),
                                  &bytes_written),
              SERIAL_DRIVER_OK);
    ASSERT_EQ(bytes_written, payload.size());

    ASSERT_EQ(serial_driver_poll(descriptor, payload.size(), 0U, &tx_bytes,
                                 &rx_bytes),
              SERIAL_DRIVER_OK);
    ASSERT_EQ(tx_bytes, payload.size());
    ASSERT_EQ(rx_bytes, 0U);

    ASSERT_EQ(MoveWriteToRead(kPort), payload.size());

    ASSERT_EQ(serial_driver_poll(descriptor, 0U, payload.size(), &tx_bytes,
                                 &rx_bytes),
              SERIAL_DRIVER_OK);
    ASSERT_EQ(tx_bytes, 0U);
    ASSERT_EQ(rx_bytes, payload.size());

    ASSERT_EQ(serial_driver_read(descriptor, received.data(), received.size(),
                                 &bytes_read),
              SERIAL_DRIVER_OK);
    ASSERT_EQ(bytes_read, received.size());
    EXPECT_EQ(received, payload);

    ASSERT_EQ(serial_driver_disable_loopback(descriptor), SERIAL_DRIVER_OK);
    ASSERT_EQ((uart_devices[kPort].registers->uart.mcr & UART_MCR_LOOPBACK_BIT),
              0U);
}

TEST_F(SerialDriverApiTest, DiscretePortAllowsOnlyDiscreteControl)
{
    constexpr size_t kPort = SERIAL_PORT_1;

    const serial_descriptor_t descriptor =
        serial_port_init(static_cast<serial_ports_t>(kPort),
                         UART_PORT_MODE_DISCRETE);
    ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

    EXPECT_EQ(serial_driver_enable_loopback(descriptor),
              SERIAL_DRIVER_ERROR_NOT_CONFIGURED);

    ASSERT_EQ(serial_driver_enable_discrete(descriptor), SERIAL_DRIVER_OK);
    ASSERT_NE((uart_devices[kPort].registers->uart.mcr & UART_MCR_DISCRETE_LINE_BIT),
              0U);

    ASSERT_EQ(serial_driver_disable_discrete(descriptor), SERIAL_DRIVER_OK);
    ASSERT_EQ((uart_devices[kPort].registers->uart.mcr & UART_MCR_DISCRETE_LINE_BIT),
              0U);
}

TEST_F(SerialDriverApiTest, SerialPortRejectsDiscreteControl)
{
    constexpr size_t kPort = SERIAL_PORT_2;

    const serial_descriptor_t descriptor =
        serial_port_init(static_cast<serial_ports_t>(kPort),
                         UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

    EXPECT_EQ(serial_driver_enable_discrete(descriptor),
              SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
}

TEST_F(SerialDriverApiTest, ReadWriteAndPollValidateArguments)
{
    constexpr size_t kPort = SERIAL_PORT_3;
    const std::array<uint8_t, 1> payload{{0xABU}};
    uint8_t out_byte = 0U;
    size_t bytes = 0U;
    size_t tx_bytes = 0U;
    size_t rx_bytes = 0U;

    const serial_descriptor_t descriptor =
        serial_port_init(static_cast<serial_ports_t>(kPort),
                         UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

    EXPECT_EQ(serial_driver_write(descriptor, nullptr, 1U, &bytes),
              SERIAL_DRIVER_ERROR_INVALID_ARG);
    EXPECT_EQ(serial_driver_write(descriptor, payload.data(), payload.size(),
                                  nullptr),
              SERIAL_DRIVER_ERROR_INVALID_ARG);

    EXPECT_EQ(serial_driver_read(descriptor, nullptr, 1U, &bytes),
              SERIAL_DRIVER_ERROR_INVALID_ARG);
    EXPECT_EQ(serial_driver_read(descriptor, &out_byte, 1U, nullptr),
              SERIAL_DRIVER_ERROR_INVALID_ARG);

    EXPECT_EQ(serial_driver_poll(descriptor, 1U, 1U, nullptr, &rx_bytes),
              SERIAL_DRIVER_ERROR_INVALID_ARG);
    EXPECT_EQ(serial_driver_poll(descriptor, 1U, 1U, &tx_bytes, nullptr),
              SERIAL_DRIVER_ERROR_INVALID_ARG);
}

TEST_F(SerialDriverApiTest, PollMovesReadFifoDataToReadApi)
{
    constexpr size_t kPort = SERIAL_PORT_4;
    const std::array<uint8_t, 3> payload{{0xDEU, 0xADU, 0xBEU}};
    std::array<uint8_t, payload.size()> received{{0U}};
    size_t tx_bytes = 0U;
    size_t rx_bytes = 0U;
    size_t bytes_read = 0U;

    ResetFifo(&uart_fifo_map.read_fifos[kPort]);

    const serial_descriptor_t descriptor =
        serial_port_init(static_cast<serial_ports_t>(kPort),
                         UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

    for (const uint8_t value : payload)
    {
        FifoPush(&uart_fifo_map.read_fifos[kPort], value);
    }

    ASSERT_EQ(serial_driver_poll(descriptor, 0U, payload.size(), &tx_bytes,
                                 &rx_bytes),
              SERIAL_DRIVER_OK);
    ASSERT_EQ(tx_bytes, 0U);
    ASSERT_EQ(rx_bytes, payload.size());

    ASSERT_EQ(serial_driver_read(descriptor, received.data(), received.size(),
                                 &bytes_read),
              SERIAL_DRIVER_OK);
    ASSERT_EQ(bytes_read, received.size());
    EXPECT_EQ(received, payload);
}

TEST_F(SerialDriverApiTest, InvalidDescriptorReturnsNotInitialized)
{
    const std::array<uint8_t, 1> payload{{0x11U}};
    uint8_t out_byte = 0U;
    size_t bytes = 0U;
    size_t tx_bytes = 0U;
    size_t rx_bytes = 0U;

    EXPECT_EQ(serial_driver_write(SERIAL_DESCRIPTOR_INVALID, payload.data(),
                                  payload.size(), &bytes),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    EXPECT_EQ(serial_driver_read(SERIAL_DESCRIPTOR_INVALID, &out_byte, 1U,
                                 &bytes),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    EXPECT_EQ(serial_driver_poll(SERIAL_DESCRIPTOR_INVALID, 1U, 1U, &tx_bytes,
                                 &rx_bytes),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    EXPECT_EQ(serial_driver_enable_loopback(SERIAL_DESCRIPTOR_INVALID),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
    EXPECT_EQ(serial_driver_enable_discrete(SERIAL_DESCRIPTOR_INVALID),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
}
