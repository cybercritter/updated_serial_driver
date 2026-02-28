# Unused And Dead Code Inventory

Scope: `include/`, `src/`, and `tests/`.

Date: 2026-02-28

## Method

1. Built and ran tests/coverage from CMake targets.
2. Ran symbol-reference scans in `include/`, `src/`, and `tests/`.
3. Manually validated candidates against current implementation files.

This document intentionally reports only findings validated against the current
tree after the TX/RX refactor into `src/device_driver.c` and
`include/device_driver/device_driver_internal.h`.

## Unused Or Definition-Only Items

### Driver/API status values currently not emitted by runtime paths

- `SERIAL_DRIVER_ERROR_RX_FULL`
- `SERIAL_DRIVER_ERROR_INVALID_PORT`

These values remain in `serial_driver_error_t` for API compatibility/future use.

### UART error codes currently definition-only in this driver

- `UART_ERROR_FIFO_OVERFLOW`
- `UART_ERROR_FIFO_UNDERFLOW`
- `UART_ERROR_TIMEOUT`
- `UART_ERROR_PARITY`
- `UART_ERROR_FRAMING`
- `UART_ERROR_OVERRUN`
- `UART_ERROR_HARDWARE_FAULT`

These are declared in `include/device_driver/errors.h` but are not currently
returned by the queue/driver/hardware-mapper code paths in `src/`.

### Register map compatibility aliases currently definition-only

- `xr17v358_register_map_t`
- `xr17c358_device_config_registers_t`

These aliases exist for completeness and external integration points.

### Test-only queue helpers

- `serial_queue_is_empty(...)`
- `serial_queue_is_full(...)`

These helpers are used by queue tests and are not required by the main driver
data path.

## Effectively Dead Defensive Paths

No known dead defensive path remains in the current serial TX/RX poll flow.
Previously identified unreachable guards were removed and invariants are now
enforced by descriptor initialization and mode-entry validation.

## Notes

- Previous inventory references to `src/device_driver_tx.c` and
  `src/device_driver_rx.c` were removed because those files no longer exist.
- Register-offset macros in `register_map.h` are intentionally broad and many
  remain definition-only until hardware-specific integrations consume them.
