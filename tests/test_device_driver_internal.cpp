#include <cstdint>

extern "C"
{
#include "device_driver/device_driver_internal.h"
}

#include <gtest/gtest.h>

TEST(SerialDriverInternalHelpersTest, ExerciseGuardAndErrorBranches)
{
    ASSERT_EQ(serial_driver_common_init(), SERIAL_DRIVER_OK);

    EXPECT_EQ(serial_driver_get_entry(SERIAL_DESCRIPTOR_INVALID), nullptr);
    EXPECT_EQ(
        serial_driver_get_entry(static_cast<serial_descriptor_t>(UART_DEVICE_COUNT + 1U)),
        nullptr);
    EXPECT_EQ(serial_driver_get_entry(1U), nullptr);

    serial_driver_byte_fifo_reset(nullptr);
    serial_driver_byte_fifo_push(nullptr, 0x55U);
    EXPECT_EQ(serial_driver_byte_fifo_pop(nullptr), 0U);
    EXPECT_FALSE(serial_driver_byte_fifo_is_full(nullptr));
    uart_byte_fifo_t local_fifo = {};
    local_fifo.count = 1U;
    EXPECT_FALSE(serial_driver_byte_fifo_is_empty(&local_fifo));
    local_fifo.count = 0U;
    EXPECT_TRUE(serial_driver_byte_fifo_is_empty(&local_fifo));

    serial_descriptor_entry_t *entry = nullptr;
    EXPECT_EQ(serial_driver_get_mode_entry(1U, UART_PORT_MODE_SERIAL, &entry),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    uart_device_t test_device = {};
    serial_descriptor_map[0].initialized = true;
    serial_descriptor_map[0].mode = UART_PORT_MODE_SERIAL;
    serial_descriptor_map[0].uart_device = nullptr;
    EXPECT_EQ(serial_driver_get_mode_entry(1U, UART_PORT_MODE_SERIAL, &entry),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
    serial_descriptor_map[0].uart_device = &test_device;
    test_device.registers = nullptr;
    EXPECT_EQ(serial_driver_get_mode_entry(1U, UART_PORT_MODE_SERIAL, &entry),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    ASSERT_EQ(serial_queue_init(&test_device.tx_queue), UART_ERROR_NONE);
    serial_descriptor_entry_t tx_entry = {};
    tx_entry.uart_device = &test_device;
    tx_entry.port_index = 0U;

    size_t tx_bytes = 0U;
    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 1U, nullptr),
              SERIAL_DRIVER_ERROR_INVALID_ARG);

    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 1U, &tx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 0U);

    tx_entry.staged_word = 0x99U;
    tx_entry.staged_word_bytes = 1U;
    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 1U, &tx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 1U);
    EXPECT_FALSE(serial_driver_byte_fifo_is_empty(&uart_fifo_map.write_fifos[0]));

    serial_driver_byte_fifo_reset(&uart_fifo_map.write_fifos[0]);
    tx_entry.staged_word = 0U;
    tx_entry.staged_word_bytes = 0U;
    tx_entry.tx_input_staged_word = 0xABU;
    tx_entry.tx_input_staged_word_bytes = 1U;
    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 1U, &tx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 1U);

    serial_driver_byte_fifo_reset(&uart_fifo_map.write_fifos[0]);
    uart_fifo_map.write_fifos[0].count = UART_DEVICE_FIFO_SIZE_BYTES;
    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 1U, &tx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 0U);
    uart_fifo_map.write_fifos[0].count = 0U;

    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 0U, &tx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 0U);

    serial_driver_byte_fifo_reset(&uart_fifo_map.write_fifos[0]);
    ASSERT_EQ(serial_queue_init(&test_device.tx_queue), UART_ERROR_NONE);
    ASSERT_EQ(serial_queue_push(&test_device.tx_queue, 0x01020304U),
              UART_ERROR_NONE);
    tx_entry.staged_word = 0U;
    tx_entry.staged_word_bytes = 0U;
    tx_entry.tx_input_staged_word = 0xCDU;
    tx_entry.tx_input_staged_word_bytes = 1U;
    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 1U, &tx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(tx_bytes, 1U);

    test_device.tx_queue.initialized = false;
    tx_entry.staged_word = 0U;
    tx_entry.staged_word_bytes = 0U;
    tx_entry.tx_input_staged_word = 0U;
    tx_entry.tx_input_staged_word_bytes = 0U;
    EXPECT_EQ(serial_driver_transmit_to_device_fifo(&tx_entry, 1U, &tx_bytes),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    uart_device_t rx_device = {};
    ASSERT_EQ(serial_queue_init(&rx_device.rx_queue), UART_ERROR_NONE);
    serial_descriptor_entry_t rx_entry = {};
    rx_entry.uart_device = &rx_device;
    rx_entry.port_index = 0U;
    rx_entry.rx_staged_word = 0xAABBCCDDU;
    rx_entry.rx_staged_word_bytes = sizeof(uint32_t);

    for (size_t i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_WORDS; ++i)
    {
        ASSERT_EQ(serial_queue_push(&rx_device.rx_queue, static_cast<uint32_t>(i)),
                  UART_ERROR_NONE);
    }

    bool queue_full = false;
    EXPECT_EQ(serial_driver_flush_rx_staged_word(&rx_entry, &queue_full),
              SERIAL_DRIVER_OK);
    EXPECT_TRUE(queue_full);

    rx_device.rx_queue.initialized = false;
    EXPECT_EQ(serial_driver_flush_rx_staged_word(&rx_entry, &queue_full),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);

    size_t rx_bytes = 0U;
    EXPECT_EQ(serial_driver_receive_from_device_fifo(&rx_entry, 1U, nullptr),
              SERIAL_DRIVER_ERROR_INVALID_ARG);

    rx_entry.rx_staged_word = 0x11223344U;
    rx_entry.rx_staged_word_bytes = sizeof(uint32_t);
    EXPECT_EQ(serial_driver_receive_from_device_fifo(&rx_entry, 1U, &rx_bytes),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
    EXPECT_EQ(rx_bytes, 0U);

    ASSERT_EQ(serial_queue_init(&rx_device.rx_queue), UART_ERROR_NONE);
    for (size_t i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_WORDS; ++i)
    {
        ASSERT_EQ(serial_queue_push(&rx_device.rx_queue, static_cast<uint32_t>(i)),
                  UART_ERROR_NONE);
    }
    rx_entry.rx_staged_word = 0x01020304U;
    rx_entry.rx_staged_word_bytes = sizeof(uint32_t);
    EXPECT_EQ(serial_driver_receive_from_device_fifo(&rx_entry, 1U, &rx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(rx_bytes, 0U);

    ASSERT_EQ(serial_queue_init(&rx_device.rx_queue), UART_ERROR_NONE);
    serial_driver_byte_fifo_reset(&uart_fifo_map.read_fifos[0]);
    rx_entry.rx_staged_word = 0U;
    rx_entry.rx_staged_word_bytes = 0U;
    EXPECT_EQ(serial_driver_receive_from_device_fifo(&rx_entry, 1U, &rx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(rx_bytes, 0U);

    ASSERT_EQ(serial_queue_init(&rx_device.rx_queue), UART_ERROR_NONE);
    serial_driver_byte_fifo_reset(&uart_fifo_map.read_fifos[0]);
    serial_driver_byte_fifo_push(&uart_fifo_map.read_fifos[0], 0x10U);
    rx_entry.rx_staged_word = 0U;
    rx_entry.rx_staged_word_bytes = 0U;
    EXPECT_EQ(serial_driver_receive_from_device_fifo(&rx_entry, 1U, &rx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(rx_bytes, 1U);

    rx_device.rx_queue.initialized = false;
    serial_driver_byte_fifo_reset(&uart_fifo_map.read_fifos[0]);
    serial_driver_byte_fifo_push(&uart_fifo_map.read_fifos[0], 0x42U);
    rx_entry.rx_staged_word = 0U;
    rx_entry.rx_staged_word_bytes = sizeof(uint32_t) - 1U;
    EXPECT_EQ(serial_driver_receive_from_device_fifo(&rx_entry, 1U, &rx_bytes),
              SERIAL_DRIVER_ERROR_NOT_INITIALIZED);
    EXPECT_EQ(rx_bytes, 1U);

    ASSERT_EQ(serial_queue_init(&rx_device.rx_queue), UART_ERROR_NONE);
    serial_driver_byte_fifo_reset(&uart_fifo_map.read_fifos[0]);
    serial_driver_byte_fifo_push(&uart_fifo_map.read_fifos[0], 0x24U);
    rx_entry.rx_staged_word = 0U;
    rx_entry.rx_staged_word_bytes = sizeof(uint32_t) - 1U;
    EXPECT_EQ(serial_driver_receive_from_device_fifo(&rx_entry, 1U, &rx_bytes),
              SERIAL_DRIVER_OK);
    EXPECT_EQ(rx_bytes, 1U);
}
