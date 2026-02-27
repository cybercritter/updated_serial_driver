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
- `include/device_driver/register_map.h`: shared 16550 register map and offsets.
- `include/device_driver/registers.h`: UART device-slot, FIFO map, and mode definitions.
- `include/device_driver/hw_abstraction.h`: platform mapper hooks for UART register binding.
- `include/device_driver/queue.h`: software queue types and queue helpers.
- `include/device_driver/errors.h`: shared UART/driver error codes.
- `src/hw_abstraction.c`: default mapper and mapper registration state.
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
- `serial_driver_write_u32(...)`
- `serial_driver_read_next_tx_u32(...)`
- `serial_driver_transmit_to_device_fifo(...)`
- `serial_driver_receive_from_device_fifo(...)`
- `serial_driver_read(...)`
- `serial_driver_read_u32(...)`
- `serial_driver_poll(...)`
- `serial_driver_pending_rx(...)`
- `serial_driver_pending_tx(...)`
- `serial_driver_get_uart_device(...)`

Status values:

- `SERIAL_DRIVER_OK`
- `SERIAL_DRIVER_ERROR_INVALID_ARG`
- `SERIAL_DRIVER_ERROR_TX_FULL`
- `SERIAL_DRIVER_ERROR_RX_EMPTY`

Hardware mapping hooks:

- `serial_driver_hw_set_mapper(...)`
- `serial_driver_hw_reset_mapper(...)`

### UART register/device API

Defined in `include/device_driver/register_map.h`, `include/device_driver/registers.h`,
`include/device_driver/hw_abstraction.h`, `include/device_driver/queue.h`, and
`include/device_driver/errors.h`:

- `uart16550_registers_t`: memory-mapped 16550-compatible register block
- `uart16550_data_reg_t`, `uart16550_interrupt_reg_t`, `uart16550_fifo_reg_t`: union aliases for overlapping 16550 register offsets
- `uart_device_t`: one UART instance (register pointer, name, base address, mode, TX/RX queues)
- `uart_byte_fifo_t` / `uart_fifo_map_t`: per-UART device FIFO models used by poll/transmit paths
- `serial_queue_t`: software 32-bit circular queue used by TX/RX paths
- `uart_port_mode_t`: serial/discrete port mode enum
- `uart_error_t`: common UART and FIFO error enum
- `serial_driver_hw_map_fn`: callback type for platform-specific register mapping
- `uart_devices[UART_DEVICE_COUNT]`: global device table
- `uart_fifo_map`: global read/write FIFO map for 8 UART channels
- Register map table: `docs/register_map.md`

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
