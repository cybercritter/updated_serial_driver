#include <cstdint>

extern "C" {
#include "serial_driver/queue.h"
}

#include <gtest/gtest.h>

TEST(SerialQueueTest, InitRejectsNull) {
  EXPECT_EQ(serial_queue_init(nullptr), UART_ERROR_INVALID_ARG);
}

TEST(SerialQueueTest, InvalidAndUninitializedPathsAreReported) {
  serial_queue_t queue = {};
  uint8_t value = 0U;

  EXPECT_EQ(serial_queue_push(nullptr, 0x11U), UART_ERROR_NOT_INITIALIZED);
  EXPECT_EQ(serial_queue_push(&queue, 0x11U), UART_ERROR_NOT_INITIALIZED);

  EXPECT_EQ(serial_queue_pop(nullptr, &value), UART_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_queue_pop(&queue, nullptr), UART_ERROR_INVALID_ARG);
  EXPECT_EQ(serial_queue_pop(&queue, &value), UART_ERROR_NOT_INITIALIZED);

  EXPECT_EQ(serial_queue_size(nullptr), 0U);
  EXPECT_EQ(serial_queue_size(&queue), 0U);
  EXPECT_FALSE(serial_queue_is_full(nullptr));
  EXPECT_FALSE(serial_queue_is_full(&queue));
}

TEST(SerialQueueTest, PushPopMaintainsOrder) {
  serial_queue_t queue = {};
  uint8_t value = 0U;

  ASSERT_EQ(serial_queue_init(&queue), UART_ERROR_NONE);
  ASSERT_TRUE(serial_queue_is_empty(&queue));

  ASSERT_EQ(serial_queue_push(&queue, 0x11U), UART_ERROR_NONE);
  ASSERT_EQ(serial_queue_push(&queue, 0x22U), UART_ERROR_NONE);
  ASSERT_EQ(serial_queue_push(&queue, 0x33U), UART_ERROR_NONE);

  EXPECT_EQ(serial_queue_size(&queue), 3U);
  EXPECT_FALSE(serial_queue_is_empty(&queue));

  ASSERT_EQ(serial_queue_pop(&queue, &value), UART_ERROR_NONE);
  EXPECT_EQ(value, 0x11U);
  ASSERT_EQ(serial_queue_pop(&queue, &value), UART_ERROR_NONE);
  EXPECT_EQ(value, 0x22U);
  ASSERT_EQ(serial_queue_pop(&queue, &value), UART_ERROR_NONE);
  EXPECT_EQ(value, 0x33U);

  EXPECT_EQ(serial_queue_size(&queue), 0U);
  EXPECT_TRUE(serial_queue_is_empty(&queue));
}

TEST(SerialQueueTest, FullAndEmptyStatesAreReported) {
  serial_queue_t queue = {};
  uint8_t value = 0U;

  ASSERT_EQ(serial_queue_init(&queue), UART_ERROR_NONE);

  for (size_t i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_BYTES; ++i) {
    ASSERT_EQ(serial_queue_push(&queue, static_cast<uint8_t>(i & 0xFFU)),
              UART_ERROR_NONE);
  }

  EXPECT_TRUE(serial_queue_is_full(&queue));
  EXPECT_EQ(serial_queue_push(&queue, 0xEEU), UART_ERROR_FIFO_FULL);

  for (size_t i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_BYTES; ++i) {
    ASSERT_EQ(serial_queue_pop(&queue, &value), UART_ERROR_NONE);
  }

  EXPECT_TRUE(serial_queue_is_empty(&queue));
  EXPECT_EQ(serial_queue_pop(&queue, &value), UART_ERROR_FIFO_EMPTY);
}

TEST(SerialQueueTest, WrapAroundAfterPop) {
  serial_queue_t queue = {};
  uint8_t value = 0U;

  ASSERT_EQ(serial_queue_init(&queue), UART_ERROR_NONE);

  for (size_t i = 0U; i < SERIAL_QUEUE_FIXED_SIZE_BYTES; ++i) {
    ASSERT_EQ(serial_queue_push(&queue, static_cast<uint8_t>(i & 0xFFU)),
              UART_ERROR_NONE);
  }

  for (size_t i = 0U; i < 100U; ++i) {
    ASSERT_EQ(serial_queue_pop(&queue, &value), UART_ERROR_NONE);
  }

  for (size_t i = 0U; i < 100U; ++i) {
    ASSERT_EQ(serial_queue_push(&queue, 0xA5U), UART_ERROR_NONE);
  }

  EXPECT_TRUE(serial_queue_is_full(&queue));
}
