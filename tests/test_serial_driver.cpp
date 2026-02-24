#include <array>
#include <cstdint>
#include <string>

extern "C" {
#include "serial_driver/serial_driver.h"
}

#include <gtest/gtest.h>

class SerialDriverPortTest : public ::testing::TestWithParam<size_t> {
 protected:
  void SetUp() override {
    ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  }

  uart_device_t* PortDevice() { return &devices_[GetParam()]; }

  std::array<uart_device_t, UART_FIFO_UART_COUNT> devices_{};
};

TEST(SerialDriverTest, InvalidDescriptorPathsAreReported) {
  uint32_t value = 0U;

  EXPECT_EQ(serial_driver_write_u32(SERIAL_DESCRIPTOR_INVALID, 0x12345678U),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(SERIAL_DESCRIPTOR_INVALID, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(SERIAL_DESCRIPTOR_INVALID, nullptr),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_driver_pending_tx(SERIAL_DESCRIPTOR_INVALID), 0U);
  EXPECT_EQ(serial_driver_get_uart_device(SERIAL_DESCRIPTOR_INVALID), nullptr);

  EXPECT_EQ(serial_driver_write_u32(1U, 0x0U),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(1U, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_pending_tx(1U), 0U);
  EXPECT_EQ(serial_driver_get_uart_device(1U), nullptr);

  EXPECT_EQ(serial_driver_write_u32(UART_DEVICE_COUNT + 1U, 0xCAFEBABEU),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
}

TEST(SerialDriverTest, InitRejectsInvalidArguments) {
  std::array<uint8_t, 16> buffer{};
  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_port_init(nullptr, buffer.data(), buffer.size(),
                             UART_PORT_MODE_SERIAL),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST(SerialDriverTest, PortInitRequiresCommonInit) {
  uart_device_t device = {};
  std::array<uint8_t, 16> buffer{};

  EXPECT_EQ(serial_port_init(&device, buffer.data(), buffer.size(),
                             UART_PORT_MODE_SERIAL),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST_P(SerialDriverPortTest, InitRejectsInvalidArguments) {
  std::array<uint8_t, 16> buffer{};

  EXPECT_EQ(
      serial_port_init(PortDevice(), nullptr, buffer.size(),
                         UART_PORT_MODE_SERIAL),
      SERIAL_DESCRIPTOR_INVALID);
  EXPECT_EQ(
      serial_port_init(PortDevice(), buffer.data(), 1U, UART_PORT_MODE_SERIAL),
      SERIAL_DESCRIPTOR_INVALID);
  EXPECT_EQ(serial_port_init(PortDevice(), buffer.data(), buffer.size(),
                               static_cast<uart_port_mode_t>(99)),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST_P(SerialDriverPortTest, WriteAndReadMaintainsOrder) {
  std::array<uint8_t, 16> buffer{};
  serial_descriptor_t descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;

  descriptor = serial_port_init(PortDevice(), buffer.data(), buffer.size(),
                                  UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  ASSERT_EQ(serial_driver_get_uart_device(descriptor), PortDevice());

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x11223344U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0xAABBCCDDU), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x01020304U), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_driver_pending_tx(descriptor), 12U);

  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x11223344U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0xAABBCCDDU);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x01020304U);
  EXPECT_EQ(serial_driver_pending_tx(descriptor), 0U);
}

TEST_P(SerialDriverPortTest, FullAndEmptyStatesAreReported) {
  std::array<uint8_t, 9> buffer{};
  serial_descriptor_t descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;

  descriptor = serial_port_init(PortDevice(), buffer.data(), buffer.size(),
                                  UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x01010101U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x02020202U), SERIAL_DRIVER_OK);
  EXPECT_EQ(serial_driver_write_u32(descriptor, 0x03030303U),
            SERIAL_DRIVER_ERROR_TX_FULL);

  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x01010101U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x02020202U);
  EXPECT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_TX_EMPTY);
}

TEST_P(SerialDriverPortTest, PendingHandlesWrapAround) {
  std::array<uint8_t, 9> buffer{};
  serial_descriptor_t descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;

  descriptor = serial_port_init(PortDevice(), buffer.data(), buffer.size(),
                                  UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x11111111U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x22222222U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x33333333U), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_driver_pending_tx(descriptor), 8U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x22222222U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x33333333U);
}

INSTANTIATE_TEST_SUITE_P(
    AllPorts, SerialDriverPortTest,
    ::testing::Range(static_cast<size_t>(0U),
                     static_cast<size_t>(UART_FIFO_UART_COUNT)),
    [](const ::testing::TestParamInfo<size_t>& info) {
      return "Port" + std::to_string(info.param);
    });

TEST(SerialDriverTest, InitFailsWhenDescriptorMapIsFull) {
  std::array<uart_device_t, UART_DEVICE_COUNT + 1U> devices{};
  std::array<std::array<uint8_t, 8>, UART_DEVICE_COUNT + 1U> buffers{};
  std::array<serial_descriptor_t, UART_DEVICE_COUNT> descriptors{};
  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  for (size_t i = 0U; i < UART_DEVICE_COUNT; ++i) {
    descriptors[i] =
        serial_port_init(&devices[i], buffers[i].data(), buffers[i].size(),
                           UART_PORT_MODE_SERIAL);
    ASSERT_NE(descriptors[i], SERIAL_DESCRIPTOR_INVALID);
  }

  EXPECT_EQ(serial_port_init(&devices[UART_DEVICE_COUNT],
                               buffers[UART_DEVICE_COUNT].data(),
                               buffers[UART_DEVICE_COUNT].size(),
                               UART_PORT_MODE_SERIAL),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST_P(SerialDriverPortTest, DiscreteModeRejectsSerialOperations) {
  serial_descriptor_t descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;

  descriptor = serial_port_init(PortDevice(), nullptr, 0U,
                                  UART_PORT_MODE_DISCRETE);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  ASSERT_TRUE(PortDevice()->configured);
  ASSERT_EQ(PortDevice()->port_mode, UART_PORT_MODE_DISCRETE);

  EXPECT_EQ(serial_driver_write_u32(descriptor, 0x12345678U),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_pending_tx(descriptor), 0U);
}
