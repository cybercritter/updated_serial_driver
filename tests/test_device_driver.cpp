#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

extern "C" {
#include "device_driver/device_driver.h"
#include "device_driver/device_driver_internal.h"
}

#include <gtest/gtest.h>

class SerialDriverPortTest : public ::testing::TestWithParam<size_t> {
 protected:
  void SetUp() override {
    ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  }

  serial_ports_t Port() const {
    return static_cast<serial_ports_t>(GetParam());
  }

  uart_device_t* PortDevice() { return &uart_devices[GetParam()]; }
};

static uint8_t PopWriteFifoByte(size_t fifo_index) {
  if (fifo_index >= UART_FIFO_UART_COUNT) {
    ADD_FAILURE() << "Invalid write FIFO index: " << fifo_index;
    return 0U;
  }
  uart_byte_fifo_t* fifo = &uart_fifo_map.write_fifos[fifo_index];
  uint8_t value = fifo->data[fifo->tail];
  fifo->tail = (fifo->tail + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
  fifo->count -= 1U;
  return value;
}

static void PushReadFifoByte(size_t fifo_index, uint8_t value) {
  if (fifo_index >= UART_FIFO_UART_COUNT) {
    ADD_FAILURE() << "Invalid read FIFO index: " << fifo_index;
    return;
  }
  uart_byte_fifo_t* fifo = &uart_fifo_map.read_fifos[fifo_index];
  fifo->data[fifo->head] = value;
  fifo->head = (fifo->head + 1U) % UART_DEVICE_FIFO_SIZE_BYTES;
  fifo->count += 1U;
}

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

TEST(SerialDriverTest, InvalidDescriptorPathsAfterCommonInitAreReported) {
  uint32_t value = 0U;
  size_t bytes = 0U;

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_driver_write_u32(1U, 0x11111111U),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(1U, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_pending_tx(1U), 0U);
  EXPECT_EQ(serial_driver_get_uart_device(1U), nullptr);

  EXPECT_EQ(serial_driver_write_u32(SERIAL_DESCRIPTOR_INVALID, 0x12345678U),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(SERIAL_DESCRIPTOR_INVALID, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_pending_tx(SERIAL_DESCRIPTOR_INVALID), 0U);
  EXPECT_EQ(serial_driver_get_uart_device(SERIAL_DESCRIPTOR_INVALID), nullptr);

  EXPECT_EQ(serial_driver_write_u32(UART_DEVICE_COUNT + 1U, 0xCAFEBABEU),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(UART_DEVICE_COUNT + 1U, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(
      serial_driver_transmit_to_device_fifo(UART_DEVICE_COUNT + 1U, 4U, &bytes),
      SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_receive_from_device_fifo(UART_DEVICE_COUNT + 1U, 4U,
                                                   &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_u32(UART_DEVICE_COUNT + 1U, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_pending_rx(UART_DEVICE_COUNT + 1U), 0U);
  EXPECT_EQ(serial_driver_pending_tx(UART_DEVICE_COUNT + 1U), 0U);
  EXPECT_EQ(serial_driver_get_uart_device(UART_DEVICE_COUNT + 1U), nullptr);

  EXPECT_EQ(serial_driver_transmit_to_device_fifo(SERIAL_DESCRIPTOR_INVALID, 4U,
                                                  &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_receive_from_device_fifo(SERIAL_DESCRIPTOR_INVALID,
                                                   4U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(
      serial_driver_poll(SERIAL_DESCRIPTOR_INVALID, 4U, 4U, &bytes, &bytes),
      SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_transmit_to_device_fifo(1U, 4U, nullptr),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_driver_receive_from_device_fifo(1U, 4U, nullptr),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_driver_write(1U, nullptr, 1U, &bytes),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  const std::array<uint8_t, 1> empty_payload{{0x00U}};
  EXPECT_EQ(serial_driver_write(1U, empty_payload.data(), 1U, nullptr),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_driver_read(1U, nullptr, 1U, &bytes),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(
      serial_driver_read(1U, reinterpret_cast<uint8_t*>(&value), 1U, nullptr),
      SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_driver_poll(1U, 4U, 4U, nullptr, &bytes),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_driver_poll(1U, 4U, 4U, &bytes, nullptr),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_driver_read_u32(1U, nullptr),
            SERIAL_DRIVER_ERROR_INVALID_ARG);
}

TEST(SerialDriverTest, NotInitializedRxAndTxApiPathsAreReported) {
  size_t bytes = 99U;
  uint8_t one_byte = 0U;
  uint32_t value = 0U;

  serial_driver_common_initialized = false;

  EXPECT_EQ(serial_driver_transmit_to_device_fifo(1U, 1U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(bytes, 0U);

  bytes = 99U;
  EXPECT_EQ(serial_driver_receive_from_device_fifo(1U, 1U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(bytes, 0U);

  bytes = 99U;
  EXPECT_EQ(serial_driver_read(1U, &one_byte, 1U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(bytes, 0U);

  EXPECT_EQ(serial_driver_read_u32(1U, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_pending_rx(1U), 0U);
}

TEST(SerialDriverTest, InitRejectsInvalidArguments) {
  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_port_init(static_cast<serial_ports_t>(UART_DEVICE_COUNT),
                             UART_PORT_MODE_SERIAL),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST(SerialDriverTest, PortInitRequiresCommonInit) {
  serial_driver_common_initialized = false;
  EXPECT_EQ(serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST(SerialDriverTest, GenericWriteCoversEdgeErrorPaths) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  auto discrete_descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes_written = 0U;
  std::array<uint8_t, 4> data4{{0x01U, 0x02U, 0x03U, 0x04U}};
  std::array<uint8_t, 1> data1{{0x05U}};

  EXPECT_EQ(serial_driver_write(descriptor, data1.data(), data1.size(),
                                &bytes_written),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  discrete_descriptor =
      serial_port_init(SERIAL_PORT_1, UART_PORT_MODE_DISCRETE);
  ASSERT_NE(discrete_descriptor, SERIAL_DESCRIPTOR_INVALID);

  EXPECT_EQ(serial_driver_write(discrete_descriptor,
                                reinterpret_cast<const uint8_t*>(data1.data()),
                                data1.size(), &bytes_written),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);

  for (size_t i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_WORDS; ++i) {
    ASSERT_EQ(serial_driver_write_u32(descriptor, static_cast<uint32_t>(i)),
              SERIAL_DRIVER_OK);
  }

  ASSERT_EQ(serial_driver_write(descriptor, data4.data(), data4.size(),
                                &bytes_written),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_written, data4.size());
  EXPECT_EQ(serial_driver_write(descriptor, data1.data(), data1.size(),
                                &bytes_written),
            SERIAL_DRIVER_ERROR_TX_FULL);
  EXPECT_EQ(bytes_written, 0U);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  uart_devices[SERIAL_PORT_0].tx_queue.initialized = false;

  EXPECT_EQ(serial_driver_write(descriptor, data4.data(), data4.size(),
                                &bytes_written),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_write(descriptor, data1.data(), data1.size(),
                                &bytes_written),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
}

TEST(SerialDriverTest, TransmitReceiveAndPollCoverErrorPaths) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes = 0U;

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  uart_devices[SERIAL_PORT_0].tx_queue.initialized = false;

  EXPECT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 1U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  PushReadFifoByte(0U, 0x11U);
  PushReadFifoByte(0U, 0x22U);
  PushReadFifoByte(0U, 0x33U);
  PushReadFifoByte(0U, 0x44U);
  uart_devices[SERIAL_PORT_0].rx_queue.initialized = false;

  EXPECT_EQ(serial_driver_receive_from_device_fifo(descriptor, 4U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_receive_from_device_fifo(descriptor, 1U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  PushReadFifoByte(0U, 0xAAU);
  PushReadFifoByte(0U, 0xBBU);
  PushReadFifoByte(0U, 0xCCU);
  PushReadFifoByte(0U, 0xDDU);
  uart_devices[SERIAL_PORT_0].rx_queue.initialized = false;

  EXPECT_EQ(serial_driver_poll(descriptor, 0U, 4U, &bytes, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
}

TEST(SerialDriverTest, GenericReadAndPendingCoverStagedAndErrorPaths) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  auto discrete_descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;
  const std::array<uint8_t, 4> data4{{0x01U, 0x02U, 0x03U, 0x04U}};
  std::string read_buf(2U, '\0');
  size_t bytes_read = 0U;
  size_t bytes_received = 0U;
  size_t bytes_written = 0U;

  EXPECT_EQ(serial_driver_read_u32(1U, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_pending_rx(1U), 0U);
  EXPECT_EQ(serial_driver_read(1U, reinterpret_cast<uint8_t*>(read_buf.data()),
                               1U, &bytes_read),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  discrete_descriptor =
      serial_port_init(SERIAL_PORT_1, UART_PORT_MODE_DISCRETE);
  ASSERT_NE(discrete_descriptor, SERIAL_DESCRIPTOR_INVALID);
  EXPECT_EQ(serial_driver_read(discrete_descriptor,
                               reinterpret_cast<uint8_t*>(read_buf.data()), 1U,
                               &bytes_read),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_read(SERIAL_DESCRIPTOR_INVALID,
                               reinterpret_cast<uint8_t*>(read_buf.data()), 1U,
                               &bytes_read),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  EXPECT_EQ(serial_driver_read(descriptor,
                               reinterpret_cast<uint8_t*>(read_buf.data()), 1U,
                               &bytes_read),
            SERIAL_DRIVER_ERROR_RX_EMPTY);
  EXPECT_EQ(bytes_read, 0U);

  uart_devices[SERIAL_PORT_0].rx_queue.initialized = false;
  EXPECT_EQ(serial_driver_read(descriptor,
                               reinterpret_cast<uint8_t*>(read_buf.data()), 1U,
                               &bytes_read),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  PushReadFifoByte(0U, 0xA1U);
  PushReadFifoByte(0U, 0xB2U);
  ASSERT_EQ(
      serial_driver_receive_from_device_fifo(descriptor, 2U, &bytes_received),
      SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_read(descriptor,
                               reinterpret_cast<uint8_t*>(read_buf.data()), 2U,
                               &bytes_read),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_read, 2U);
  EXPECT_EQ(static_cast<uint8_t>(read_buf[0]), 0xA1U);
  EXPECT_EQ(static_cast<uint8_t>(read_buf[1]), 0xB2U);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  uart_devices[SERIAL_PORT_0].tx_queue.initialized = false;
  EXPECT_EQ(serial_driver_write(descriptor, data4.data(), data4.size(),
                                &bytes_written),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_pending_tx(descriptor), 1U);
  EXPECT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x04030201U);
}

TEST(SerialDriverTest, CoverageReachableDefensivePaths) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes = 0U;
  size_t bytes_written = 0U;
  uint32_t value = 0U;
  size_t fifo_index = 0U;
  const std::array<uint8_t, 4> data4{{0x10U, 0x20U, 0x30U, 0x40U}};

  EXPECT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 1U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_receive_from_device_fifo(descriptor, 1U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_driver_write(1U, data4.data(), data4.size(), &bytes_written),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  ASSERT_EQ(serial_driver_write(descriptor, data4.data(), data4.size(),
                                &bytes_written),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_written, data4.size());
  EXPECT_EQ(serial_driver_pending_tx(descriptor), 1U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x40302010U);

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  uart_devices[SERIAL_PORT_0].rx_queue.count = SERIAL_QUEUE_FIXED_SIZE_WORDS;
  PushReadFifoByte(fifo_index, 0x11U);
  PushReadFifoByte(fifo_index, 0x22U);
  PushReadFifoByte(fifo_index, 0x33U);
  PushReadFifoByte(fifo_index, 0x44U);

  ASSERT_EQ(serial_driver_receive_from_device_fifo(descriptor, 4U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 4U);
  ASSERT_EQ(serial_driver_receive_from_device_fifo(descriptor, 1U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 0U);
}

TEST(SerialDriverTest, PollSkipsRxWhenTxQueueHasPendingWord) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t tx_bytes = 0U;
  size_t rx_bytes = 0U;
  size_t fifo_index = 0U;

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x01020304U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0xA1A2A3A4U), SERIAL_DRIVER_OK);

  PushReadFifoByte(fifo_index, 0x11U);
  PushReadFifoByte(fifo_index, 0x22U);
  PushReadFifoByte(fifo_index, 0x33U);
  PushReadFifoByte(fifo_index, 0x44U);

  ASSERT_EQ(serial_driver_poll(descriptor, 0U, 8U, &tx_bytes, &rx_bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(tx_bytes, 0U);
  EXPECT_EQ(rx_bytes, 0U);
  EXPECT_EQ(uart_fifo_map.read_fifos[fifo_index].count, 4U);
}

TEST(SerialDriverTest, ReadAllowsZeroLengthWithNullBuffer) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes_read = 123U;

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);
  descriptor = serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  EXPECT_EQ(serial_driver_read(descriptor, nullptr, 0U, &bytes_read),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_read, 0U);
}

TEST_P(SerialDriverPortTest, InitRejectsInvalidArguments) {
  EXPECT_EQ(serial_port_init(Port(), static_cast<uart_port_mode_t>(99)),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST_P(SerialDriverPortTest, WriteAndReadMaintainsOrder) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  ASSERT_EQ(serial_driver_get_uart_device(descriptor), PortDevice());

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x11223344U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0xAABBCCDDU), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x01020304U), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_driver_pending_tx(descriptor), 3U);

  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x11223344U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0xAABBCCDDU);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x01020304U);
  EXPECT_EQ(serial_driver_pending_tx(descriptor), 0U);
}

TEST_P(SerialDriverPortTest, FullAndEmptyStatesAreReported) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;
  size_t i = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  for (i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_WORDS; ++i) {
    ASSERT_EQ(serial_driver_write_u32(descriptor, static_cast<uint32_t>(i)),
              SERIAL_DRIVER_OK);
  }

  EXPECT_EQ(serial_driver_write_u32(descriptor, 0x03030303U),
            SERIAL_DRIVER_ERROR_TX_FULL);

  for (i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_WORDS; ++i) {
    ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(value, static_cast<uint32_t>(i));
  }

  EXPECT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_TX_EMPTY);
}

TEST_P(SerialDriverPortTest, PendingHandlesWrapAround) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x11111111U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x22222222U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x33333333U), SERIAL_DRIVER_OK);

  EXPECT_EQ(serial_driver_pending_tx(descriptor), 2U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x22222222U);
  ASSERT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x33333333U);
}

TEST(SerialDriverTest, InitFailsWhenDescriptorMapIsFull) {
  std::array<serial_descriptor_t, UART_DEVICE_COUNT> descriptors{};
  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  std::for_each(descriptors.begin(), descriptors.end(),
                [&, idx = 0U](serial_descriptor_t& descriptor) mutable {
                  descriptor = serial_port_init(
                      static_cast<serial_ports_t>(idx), UART_PORT_MODE_SERIAL);
                  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
                  ++idx;
                });

  EXPECT_EQ(serial_port_init(static_cast<serial_ports_t>(UART_DEVICE_COUNT),
                             UART_PORT_MODE_SERIAL),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST(SerialDriverTest, ReinitializingSamePortReturnsExistingDescriptor) {
  auto first = SERIAL_DESCRIPTOR_INVALID;
  auto second = SERIAL_DESCRIPTOR_INVALID;

  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  first = serial_port_init(SERIAL_PORT_3, UART_PORT_MODE_SERIAL);
  ASSERT_NE(first, SERIAL_DESCRIPTOR_INVALID);

  second = serial_port_init(SERIAL_PORT_3, UART_PORT_MODE_DISCRETE);
  ASSERT_NE(second, SERIAL_DESCRIPTOR_INVALID);
  EXPECT_EQ(second, first);
  EXPECT_EQ(serial_driver_get_uart_device(second),
            &uart_devices[SERIAL_PORT_3]);
}

TEST(SerialDriverTest, PortInitFailsWhenNoDescriptorSlotsAreAvailable) {
  ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

  for (auto& descriptor : serial_descriptor_map) {
    descriptor.initialized = true;
    descriptor.uart_device = &uart_devices[SERIAL_PORT_1];
    descriptor.mode = UART_PORT_MODE_SERIAL;
  }

  EXPECT_EQ(serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL),
            SERIAL_DESCRIPTOR_INVALID);
}

TEST_P(SerialDriverPortTest, DiscreteModeRejectsSerialOperations) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;
  size_t bytes = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_DISCRETE);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  ASSERT_TRUE(PortDevice()->configured);
  ASSERT_EQ(PortDevice()->port_mode, UART_PORT_MODE_DISCRETE);

  EXPECT_EQ(serial_driver_write_u32(descriptor, 0x12345678U),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 4U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_receive_from_device_fifo(descriptor, 4U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_poll(descriptor, 4U, 4U, &bytes, &bytes),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_read_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_NOT_CONFIGURED);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 0U);
  EXPECT_EQ(serial_driver_pending_tx(descriptor), 0U);
}

TEST_P(SerialDriverPortTest, QueueNotInitializedMapsToDriverNotInitialized) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;
  size_t bytes = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x12345678U), SERIAL_DRIVER_OK);
  PortDevice()->tx_queue.initialized = false;

  EXPECT_EQ(serial_driver_write_u32(descriptor, 0x12345678U),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_read_next_tx_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 4U, &bytes),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

  PortDevice()->rx_queue.initialized = false;
  EXPECT_EQ(serial_driver_read_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
}

TEST_P(SerialDriverPortTest, PollReceiveFromDeviceRxFifoAndReadWord) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;
  size_t bytes = 0U;
  size_t fifo_index = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  PushReadFifoByte(fifo_index, 0x44U);
  PushReadFifoByte(fifo_index, 0x33U);
  PushReadFifoByte(fifo_index, 0x22U);
  PushReadFifoByte(fifo_index, 0x11U);

  ASSERT_EQ(serial_driver_receive_from_device_fifo(descriptor, 4U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 4U);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 1U);

  ASSERT_EQ(serial_driver_read_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x11223344U);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 0U);
}

TEST_P(SerialDriverPortTest, PollReceiveRespectsMaxBytesAndStaging) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;
  size_t bytes = 0U;
  size_t fifo_index = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  PushReadFifoByte(fifo_index, 0x04U);
  PushReadFifoByte(fifo_index, 0x03U);
  PushReadFifoByte(fifo_index, 0x02U);
  PushReadFifoByte(fifo_index, 0x01U);

  ASSERT_EQ(serial_driver_receive_from_device_fifo(descriptor, 2U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 2U);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 0U);

  ASSERT_EQ(serial_driver_receive_from_device_fifo(descriptor, 2U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 2U);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 1U);

  ASSERT_EQ(serial_driver_receive_from_device_fifo(descriptor, 1U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 0U);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 1U);

  ASSERT_EQ(serial_driver_read_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x01020304U);
}

TEST_P(SerialDriverPortTest, PollReceiveReturnsEmptyWhenNoRxData) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  uint32_t value = 0U;
  size_t bytes = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);

  ASSERT_EQ(serial_driver_receive_from_device_fifo(descriptor, 8U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 0U);
  EXPECT_EQ(serial_driver_read_u32(descriptor, &value),
            SERIAL_DRIVER_ERROR_RX_EMPTY);
}

TEST_P(SerialDriverPortTest, PollDefersRxUntilTxIsFullyDrained) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t tx_bytes = 0U;
  size_t rx_bytes = 0U;
  uint32_t value = 0U;
  size_t fifo_index = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x11223344U), SERIAL_DRIVER_OK);
  PushReadFifoByte(fifo_index, 0x04U);
  PushReadFifoByte(fifo_index, 0x03U);
  PushReadFifoByte(fifo_index, 0x02U);
  PushReadFifoByte(fifo_index, 0x01U);

  ASSERT_EQ(serial_driver_poll(descriptor, 2U, 4U, &tx_bytes, &rx_bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(tx_bytes, 2U);
  EXPECT_EQ(rx_bytes, 0U);
  EXPECT_EQ(uart_fifo_map.read_fifos[fifo_index].count, 4U);

  ASSERT_EQ(serial_driver_poll(descriptor, 2U, 4U, &tx_bytes, &rx_bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(tx_bytes, 2U);
  EXPECT_EQ(rx_bytes, 4U);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 1U);

  ASSERT_EQ(serial_driver_read_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x01020304U);
}

TEST_P(SerialDriverPortTest, PollReadsRxWhenNoTxPending) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t tx_bytes = 0U;
  size_t rx_bytes = 0U;
  uint32_t value = 0U;
  size_t fifo_index = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  PushReadFifoByte(fifo_index, 0xEFU);
  PushReadFifoByte(fifo_index, 0xBEU);
  PushReadFifoByte(fifo_index, 0xADU);
  PushReadFifoByte(fifo_index, 0xDEU);

  ASSERT_EQ(serial_driver_poll(descriptor, 8U, 8U, &tx_bytes, &rx_bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(tx_bytes, 0U);
  EXPECT_EQ(rx_bytes, 4U);
  EXPECT_EQ(serial_driver_pending_rx(descriptor), 1U);

  ASSERT_EQ(serial_driver_read_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0xDEADBEEFU);
}

TEST_P(SerialDriverPortTest, TransmitToDeviceFifoMaintainsByteOrder) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes = 0U;
  size_t fifo_index = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x11223344U), SERIAL_DRIVER_OK);
  ASSERT_EQ(serial_driver_write_u32(descriptor, 0xAABBCCDDU), SERIAL_DRIVER_OK);

  ASSERT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 8U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 8U);
  EXPECT_EQ(uart_fifo_map.write_fifos[fifo_index].count, 8U);

  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x44U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x33U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x22U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x11U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0xDDU);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0xCCU);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0xBBU);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0xAAU);
}

TEST_P(SerialDriverPortTest, GenericWriteAcceptsByteLengthAndTransmitsInOrder) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes_written = 0U;
  size_t bytes_transmitted = 0U;
  size_t fifo_index = 0U;
  const uint8_t payload[] = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U};

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  ASSERT_EQ(
      serial_driver_write(descriptor, payload, sizeof(payload), &bytes_written),
      SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_written, sizeof(payload));

  ASSERT_EQ(serial_driver_transmit_to_device_fifo(descriptor, sizeof(payload),
                                                  &bytes_transmitted),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_transmitted, sizeof(payload));
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x11U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x22U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x33U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x44U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x55U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x66U);
}

TEST_P(SerialDriverPortTest, GenericReadSupportsPartialAndU32Alignment) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes_received = 0U;
  size_t bytes_read = 0U;
  size_t fifo_index = 0U;
  std::string read_buf(3U, '\0');
  uint32_t value = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  PushReadFifoByte(fifo_index, 0xAAU);
  PushReadFifoByte(fifo_index, 0xBBU);
  PushReadFifoByte(fifo_index, 0xCCU);
  PushReadFifoByte(fifo_index, 0xDDU);
  ASSERT_EQ(
      serial_driver_receive_from_device_fifo(descriptor, 4U, &bytes_received),
      SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_received, 4U);

  ASSERT_EQ(serial_driver_read(descriptor,
                               reinterpret_cast<uint8_t*>(read_buf.data()),
                               read_buf.size(), &bytes_read),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_read, read_buf.size());
  EXPECT_EQ(static_cast<uint8_t>(read_buf[0]), 0xAAU);
  EXPECT_EQ(static_cast<uint8_t>(read_buf[1]), 0xBBU);
  EXPECT_EQ(static_cast<uint8_t>(read_buf[2]), 0xCCU);

  PushReadFifoByte(fifo_index, 0x11U);
  PushReadFifoByte(fifo_index, 0x22U);
  PushReadFifoByte(fifo_index, 0x33U);
  ASSERT_EQ(
      serial_driver_receive_from_device_fifo(descriptor, 3U, &bytes_received),
      SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes_received, 3U);

  ASSERT_EQ(serial_driver_read_u32(descriptor, &value), SERIAL_DRIVER_OK);
  EXPECT_EQ(value, 0x332211DDU);
}

TEST_P(SerialDriverPortTest, TransmitRespectsMaxBytesAndKeepsStagedWord) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes = 0U;
  size_t fifo_index = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  ASSERT_EQ(serial_driver_write_u32(descriptor, 0x01020304U), SERIAL_DRIVER_OK);

  ASSERT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 2U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 2U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x04U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x03U);

  ASSERT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 2U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 2U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x02U);
  EXPECT_EQ(PopWriteFifoByte(fifo_index), 0x01U);
}

TEST_P(SerialDriverPortTest, TransmitStopsWhenDeviceFifoIsFullAt255Bytes) {
  auto descriptor = SERIAL_DESCRIPTOR_INVALID;
  size_t bytes = 0U;
  size_t fifo_index = 0U;

  descriptor = serial_port_init(Port(), UART_PORT_MODE_SERIAL);
  ASSERT_NE(descriptor, SERIAL_DESCRIPTOR_INVALID);
  fifo_index = static_cast<size_t>(descriptor - 1U);

  for (int i = 0U; i < 64U; ++i) {
    ASSERT_EQ(serial_driver_write_u32(descriptor, static_cast<uint32_t>(i)),
              SERIAL_DRIVER_OK);
  }

  ASSERT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 300U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, UART_DEVICE_FIFO_SIZE_BYTES);
  EXPECT_EQ(uart_fifo_map.write_fifos[fifo_index].count,
            UART_DEVICE_FIFO_SIZE_BYTES);

  ASSERT_EQ(serial_driver_transmit_to_device_fifo(descriptor, 32U, &bytes),
            SERIAL_DRIVER_OK);
  EXPECT_EQ(bytes, 0U);
}

INSTANTIATE_TEST_SUITE_P(
    AllPorts, SerialDriverPortTest,
    ::testing::Range(static_cast<size_t>(0U),
                     static_cast<size_t>(UART_FIFO_UART_COUNT)),
    [](const ::testing::TestParamInfo<size_t>& info) {
      return "Port" + std::to_string(info.param);
    });
