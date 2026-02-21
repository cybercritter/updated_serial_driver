# updated_serial_driver

16550 UART driver starter project in C with CMake-based builds and GoogleTest unit tests.

## Overview

This repository provides:

- A small, testable software TX queue (`serial_driver_t`) implemented as a circular buffer.
- 16550 UART register and device data structures for memory-mapped UART instances.
- A global UART device table (`uart_devices`) for multi-UART systems.
- A unit test target using GoogleTest to validate core queue behavior.

## Project layout

- `CMakeLists.txt`: build configuration, library target, and test integration.
- `include/serial_driver/serial_driver.h`: core TX queue API.
- `src/serial_driver.c`: core TX queue implementation.
- `include/serial_driver/registers.h`: 16550 register map, UART/FIFO errors, and device descriptors.
- `src/registers.c`: global UART device table definition.
- `tests/test_serial_driver.cpp`: unit tests.

## Build requirements

- CMake 3.21+
- C11 compiler
- C++17 compiler (tests only)
- Network access during first configure to fetch GoogleTest

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run tests

```bash
ctest --test-dir build --output-on-failure
```

## Generate docs

If Doxygen is installed, CMake provides a `docs` target:

```bash
cmake -S . -B build
cmake --build build --target docs
```

Generated HTML docs are written to `build/docs/html/index.html`.

## Public APIs

### Core serial queue API

Defined in `include/serial_driver/serial_driver.h`:

- `serial_driver_init(...)`
- `serial_driver_write_byte(...)`
- `serial_driver_read_next_tx(...)`
- `serial_driver_pending_tx(...)`

Status values:

- `SERIAL_DRIVER_OK`
- `SERIAL_DRIVER_INVALID_ARG`
- `SERIAL_DRIVER_BUFFER_FULL`
- `SERIAL_DRIVER_BUFFER_EMPTY`

### UART register/device API

Defined in `include/serial_driver/registers.h`:

- `uart16550_registers_t`: memory-mapped 16550-compatible register block
- `uart_circular_buffer_t`: generic TX/RX circular buffer structure
- `uart_device_t`: one UART instance (register pointer, name, base address, TX/RX buffers)
- `uart_devices[UART_DEVICE_COUNT]`: global device table
- `uart_error_t`: general UART and FIFO error enum

## Design notes

- The core queue keeps one slot empty to distinguish full from empty states.
- Register offset aliases are modeled with unions:
  - Offset `0x00`: `RBR/THR/DLL`
  - Offset `0x01`: `IER/DLM`
  - Offset `0x02`: `IIR/FCR`
- Hardware access fields are `volatile` to preserve register semantics.

## Example usage

```c
#include <stdint.h>
#include "serial_driver/serial_driver.h"

serial_driver_t driver;
uint8_t tx_buffer[64];

if (serial_driver_init(&driver, tx_buffer, sizeof(tx_buffer)) == SERIAL_DRIVER_OK) {
    (void)serial_driver_write_byte(&driver, 0x55U);
}
```

## Current scope and next steps

Current implementation covers queue management and register/device modeling. It does not yet include:

- UART initialization/configuration routines (baud, parity, stop bits)
- Interrupt-driven TX/RX handlers
- Hardware abstraction for platform-specific register mapping
