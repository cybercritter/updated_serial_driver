#include <array>
#include <cstdint>

extern "C" {
#include "serial_driver/serial_driver.h"
}

#include <gtest/gtest.h>

TEST(SerialDriverTest, InitRejectsInvalidArguments)
{
    serial_driver_t driver = {};
    std::array<uint8_t, 8> buffer{};

    EXPECT_EQ(serial_driver_init(nullptr, buffer.data(), buffer.size()), SERIAL_DRIVER_INVALID_ARG);
    EXPECT_EQ(serial_driver_init(&driver, nullptr, buffer.size()), SERIAL_DRIVER_INVALID_ARG);
    EXPECT_EQ(serial_driver_init(&driver, buffer.data(), 1U), SERIAL_DRIVER_INVALID_ARG);
}

TEST(SerialDriverTest, WriteAndReadMaintainsOrder)
{
    serial_driver_t driver = {};
    std::array<uint8_t, 8> buffer{};
    uint8_t value = 0U;

    ASSERT_EQ(serial_driver_init(&driver, buffer.data(), buffer.size()), SERIAL_DRIVER_OK);
    ASSERT_EQ(serial_driver_write_byte(&driver, 0x11U), SERIAL_DRIVER_OK);
    ASSERT_EQ(serial_driver_write_byte(&driver, 0x22U), SERIAL_DRIVER_OK);
    ASSERT_EQ(serial_driver_write_byte(&driver, 0x33U), SERIAL_DRIVER_OK);

    EXPECT_EQ(serial_driver_pending_tx(&driver), 3U);

    ASSERT_EQ(serial_driver_read_next_tx(&driver, &value), SERIAL_DRIVER_OK);
    EXPECT_EQ(value, 0x11U);
    ASSERT_EQ(serial_driver_read_next_tx(&driver, &value), SERIAL_DRIVER_OK);
    EXPECT_EQ(value, 0x22U);
    ASSERT_EQ(serial_driver_read_next_tx(&driver, &value), SERIAL_DRIVER_OK);
    EXPECT_EQ(value, 0x33U);
    EXPECT_EQ(serial_driver_pending_tx(&driver), 0U);
}

TEST(SerialDriverTest, FullAndEmptyStatesAreReported)
{
    serial_driver_t driver = {};
    std::array<uint8_t, 4> buffer{};
    uint8_t value = 0U;

    ASSERT_EQ(serial_driver_init(&driver, buffer.data(), buffer.size()), SERIAL_DRIVER_OK);

    ASSERT_EQ(serial_driver_write_byte(&driver, 0x01U), SERIAL_DRIVER_OK);
    ASSERT_EQ(serial_driver_write_byte(&driver, 0x02U), SERIAL_DRIVER_OK);
    ASSERT_EQ(serial_driver_write_byte(&driver, 0x03U), SERIAL_DRIVER_OK);
    EXPECT_EQ(serial_driver_write_byte(&driver, 0x04U), SERIAL_DRIVER_BUFFER_FULL);

    ASSERT_EQ(serial_driver_read_next_tx(&driver, &value), SERIAL_DRIVER_OK);
    EXPECT_EQ(value, 0x01U);
    ASSERT_EQ(serial_driver_read_next_tx(&driver, &value), SERIAL_DRIVER_OK);
    EXPECT_EQ(value, 0x02U);
    ASSERT_EQ(serial_driver_read_next_tx(&driver, &value), SERIAL_DRIVER_OK);
    EXPECT_EQ(value, 0x03U);
    EXPECT_EQ(serial_driver_read_next_tx(&driver, &value), SERIAL_DRIVER_BUFFER_EMPTY);
}
