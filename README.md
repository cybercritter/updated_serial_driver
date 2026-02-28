# updated_device_driver

16550 / XR17C358-style UART driver in C with CMake-based builds and GoogleTest
unit tests.

## Overview

This repository provides:

- A descriptor-based serial driver API for up to 8 UART ports.
- Poll-driven TX/RX data movement between software queues and per-port byte
  FIFOs.
- Serial-mode controls (local loopback) and discrete-mode control (#RTS line).
- A pluggable hardware mapper callback for binding UART device slots to
  platform register blocks.

## Project layout

- `CMakeLists.txt`: build options, library target, tests, docs, and coverage.
- `src/device_driver.c`: public serial driver implementation.
- `include/device_driver/device_driver_internal.h`: internal helpers used by
  `src/device_driver.c` (not a public API).
- `src/hw_abstraction.c`: default hardware mapper and mapper registration.
- `src/queue.c`: fixed-size 32-bit software queue implementation.
- `src/registers.c`: global `uart_devices` and `uart_fifo_map` definitions.
- `include/device_driver/device_driver.h`: public serial driver API.
- `include/device_driver/hw_abstraction.h`: hardware mapping callback API.
- `include/device_driver/registers.h`: UART device slot, FIFO map, and modes.
- `include/device_driver/register_map.h`: 16550/XR17V358 register map types and
  offset macros.
- `include/device_driver/errors.h`: shared UART-level error codes.
- `tests/test_device_driver.cpp`: GoogleTest coverage for serial/discrete APIs.
- `tests/test_queue.cpp`: GoogleTest coverage for queue utilities.
- `tests/device_driver_test_main.c`: C-only executable smoke test.

## Build requirements

- CMake 3.21+
- C11 compiler
- C++11 compiler (tests only)
- GoogleTest source:
  - automatically downloaded when needed, or
  - provided locally via `-DDEVICE_DRIVER_LOCAL_GTEST_SOURCE=/path/to/googletest`

## CMake options

- `DEVICE_DRIVER_BUILD_TESTS` (default: `ON`)
- `DEVICE_DRIVER_BUILD_DOCS` (default: `ON`)
- `DEVICE_DRIVER_ENABLE_COVERAGE` (default: `OFF`, requires GCC/Clang + gcovr)
- `DEVICE_DRIVER_LOCAL_GTEST_SOURCE` (default: empty)

Legacy option aliases are still accepted for compatibility:
`SERIAL_DRIVER_BUILD_TESTS`, `SERIAL_DRIVER_BUILD_DOCS`,
`SERIAL_DRIVER_ENABLE_COVERAGE`.

## Build

```bash
cmake -S . -B build -DDEVICE_DRIVER_BUILD_TESTS=ON -DDEVICE_DRIVER_BUILD_DOCS=OFF
cmake --build build
```

## Run tests

```bash
ctest --test-dir build --output-on-failure
```

## Generate coverage

```bash
cmake -S . -B build -DDEVICE_DRIVER_ENABLE_COVERAGE=ON -DDEVICE_DRIVER_BUILD_DOCS=OFF
cmake --build build --target coverage
```

Coverage outputs:

- `build/coverage.xml`
- `build/coverage.html`

## Generate docs

If Doxygen is installed:

```bash
cmake -S . -B build -DDEVICE_DRIVER_BUILD_DOCS=ON -DDEVICE_DRIVER_BUILD_TESTS=OFF
cmake --build build --target docs
```

Generated HTML docs are written to `build/docs/html/index.html`.

## Public APIs

Core serial driver API (`include/device_driver/device_driver.h`):

- `serial_port_init(...)`
- `serial_driver_write(...)`
- `serial_driver_read(...)`
- `serial_driver_poll(...)`
- `serial_driver_enable_loopback(...)`
- `serial_driver_disable_loopback(...)`
- `serial_driver_enable_discrete(...)`
- `serial_driver_disable_discrete(...)`

Driver status values:

- `SERIAL_DRIVER_OK`
- `SERIAL_DRIVER_ERROR_INVALID_ARG`
- `SERIAL_DRIVER_ERROR_NOT_INITIALIZED`
- `SERIAL_DRIVER_ERROR_NOT_CONFIGURED`
- `SERIAL_DRIVER_ERROR_TX_FULL`
- `SERIAL_DRIVER_ERROR_TX_EMPTY`
- `SERIAL_DRIVER_ERROR_RX_FULL`
- `SERIAL_DRIVER_ERROR_RX_EMPTY`
- `SERIAL_DRIVER_ERROR_INVALID_PORT`

Hardware mapping hooks (`include/device_driver/hw_abstraction.h`):

- `serial_driver_hw_set_mapper(...)`
- `serial_driver_hw_reset_mapper(...)`

Register and device model headers:

- `include/device_driver/register_map.h`
- `include/device_driver/registers.h`
- `include/device_driver/queue.h`
- `include/device_driver/errors.h`

## Implementation notes

- `serial_driver_poll()` always services TX first. RX is serviced only when
  there is no pending TX staged data or queued TX data.
- TX/RX queues are word-based (`uint32_t`) with byte staging to preserve byte
  ordering across API boundaries.
- Serial/discrete mode gating is enforced per descriptor.

## Example usage

```c
#include <stdint.h>

#include "device_driver/device_driver.h"

serial_descriptor_t descriptor =
    serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);

uint8_t data[1] = {0x55U};
size_t bytes_written = 0U;
size_t tx_bytes = 0U;
size_t rx_bytes = 0U;

(void)serial_driver_enable_loopback(descriptor);
(void)serial_driver_write(descriptor, data, sizeof(data), &bytes_written);
(void)serial_driver_poll(descriptor, bytes_written, 0U, &tx_bytes, &rx_bytes);
```

## Current scope

Implemented:

- Descriptor allocation and per-port mode selection.
- Software queueing and FIFO transfer helpers.
- Poll-driven TX and RX data movement.
- Loopback and discrete control-bit management.

Not yet implemented:

- UART line configuration (baud, parity, stop bits).
- Interrupt-driven TX/RX handlers.
