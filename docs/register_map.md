# 16550 UART Register Map

Source of truth: `include/device_driver/register_map.h`

| Offset | Struct Member | Alias/Register | Access Notes |
| --- | --- | --- | --- |
| `0x00` | `data` | `RBR` / `THR` / `DLL` | `RBR` read (`DLAB=0`), `THR` write (`DLAB=0`), `DLL` (`DLAB=1`) |
| `0x01` | `interrupt_enable` | `IER` / `DLM` | `IER` (`DLAB=0`), `DLM` (`DLAB=1`) |
| `0x02` | `fifo_control` | `IIR` / `FCR` | `IIR` read, `FCR` write |
| `0x03` | `lcr` | `LCR` | Line Control Register |
| `0x04` | `mcr` | `MCR` | Modem Control Register |
| `0x05` | `lsr` | `LSR` | Line Status Register |
| `0x06` | `msr` | `MSR` | Modem Status Register |
| `0x07` | `scr` | `SCR` | Scratch Register |

## Offset Macros

The shared header defines these macros for direct offset-based access:

- `UART16550_REG_OFFSET_DATA`
- `UART16550_REG_OFFSET_INTERRUPT_ENABLE`
- `UART16550_REG_OFFSET_FIFO_CONTROL`
- `UART16550_REG_OFFSET_LCR`
- `UART16550_REG_OFFSET_MCR`
- `UART16550_REG_OFFSET_LSR`
- `UART16550_REG_OFFSET_MSR`
- `UART16550_REG_OFFSET_SCR`
