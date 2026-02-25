# updated_serial_driver

16550 UART driver starter project in C with CMake-based builds and GoogleTest unit tests.

## Overview

This repository provides:

- A testable serial-driver pipeline for staged TX/RX word and byte movement.
- 16550 UART register and device data structures for memory-mapped UART instances.
- A global UART device table (`uart_devices`) for multi-UART systems.
- A unit test target using GoogleTest to validate core queue behavior.

## Project layout

- `CMakeLists.txt`: build configuration, library target, and test integration.
- `include/device_driver/device_driver.h`: public serial driver API.
- `src/device_driver.c`: descriptor/state lifecycle and shared internals.
- `src/device_driver_tx.c`: TX path implementation.
- `src/device_driver_rx.c`: RX path and poll implementation.
- `include/device_driver/registers.h`: 16550 register map, UART/FIFO errors, and device descriptors.
- `src/registers.c`: global UART device table definition.
- `tests/test_device_driver.cpp`: unit tests.

## Build requirements

- CMake 3.21+
- C11 compiler
- C++11 compiler (tests only)
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

### Core serial driver API

Defined in `include/device_driver/device_driver.h`:

- `serial_driver_common_init(...)`
- `serial_port_init(...)`
- `serial_driver_write(...)`
- `serial_driver_read(...)`
- `serial_driver_poll(...)`
- `serial_driver_pending_tx(...)`

Status values:

- `SERIAL_DRIVER_OK`
- `SERIAL_DRIVER_ERROR_INVALID_ARG`
- `SERIAL_DRIVER_ERROR_TX_FULL`
- `SERIAL_DRIVER_ERROR_RX_EMPTY`

### UART register/device API

Defined in `include/device_driver/registers.h`:

- `uart16550_registers_t`: memory-mapped 16550-compatible register block
- `uart_circular_buffer_t`: generic TX/RX circular buffer structure
- `uart_device_t`: one UART instance (register pointer, name, base address, TX/RX buffers)
- `uart_devices[UART_DEVICE_COUNT]`: global device table
- `uart_error_t`: general UART and FIFO error enum

## Design notes

- The internal word queue uses count-based full/empty tracking.
- Register offset aliases are modeled with unions:
  - Offset `0x00`: `RBR/THR/DLL`
  - Offset `0x01`: `IER/DLM`
  - Offset `0x02`: `IIR/FCR`
- Hardware access fields are `volatile` to preserve register semantics.

## Example usage

```c
#include <stdint.h>
#include "device_driver/device_driver.h"

if (serial_driver_common_init() == SERIAL_DRIVER_OK) {
    serial_descriptor_t descriptor =
        serial_port_init(SERIAL_PORT_0, UART_PORT_MODE_SERIAL);
    uint8_t data[1] = {0x55U};
    size_t bytes_written = 0U;
    (void)serial_driver_write(descriptor, data, sizeof(data), &bytes_written);
}
```

## Current scope and next steps

Current implementation covers queue management and register/device modeling. It does not yet include:

- UART initialization/configuration routines (baud, parity, stop bits)
- Interrupt-driven TX/RX handlers
- Hardware abstraction for platform-specific register mapping
