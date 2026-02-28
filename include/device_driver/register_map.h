#ifndef SERIAL_DRIVER_REGISTER_MAP_H
#define SERIAL_DRIVER_REGISTER_MAP_H

/**
 * @file register_map.h
 * @brief Shared 16550 and XR17C358/XR17V358 UART register-map definitions.
 */

#include <stdint.h>

/** 16550 register offset 0x00 (RBR/THR/DLL alias). */
#define UART16550_REG_OFFSET_DATA 0x00U
/** 16550 register offset 0x01 (IER/DLM alias). */
#define UART16550_REG_OFFSET_INTERRUPT_ENABLE 0x01U
/** 16550 register offset 0x02 (IIR/FCR alias). */
#define UART16550_REG_OFFSET_FIFO_CONTROL 0x02U
/** 16550 register offset 0x03 (LCR). */
#define UART16550_REG_OFFSET_LCR 0x03U
/** 16550 register offset 0x04 (MCR). */
#define UART16550_REG_OFFSET_MCR 0x04U
/** 16550 register offset 0x05 (LSR). */
#define UART16550_REG_OFFSET_LSR 0x05U
/** 16550 register offset 0x06 (MSR). */
#define UART16550_REG_OFFSET_MSR 0x06U
/** 16550 register offset 0x07 (SCR). */
#define UART16550_REG_OFFSET_SCR 0x07U

/** MCR bit 0: DTR output control. */
#define UART_MCR_DTR_BIT (1U << 0U)
/** MCR bit 1: RTS output control (#RTS line). */
#define UART_MCR_RTS_BIT (1U << 1U)
/** MCR bit 2: OUT1 output control. */
#define UART_MCR_OUT1_BIT (1U << 2U)
/** MCR bit 3: OUT2 output control. */
#define UART_MCR_OUT2_BIT (1U << 3U)
/** MCR bit 4: local loopback enable. */
#define UART_MCR_LOOPBACK_BIT (1U << 4U)

/**
 * @brief Discrete line control bit for XR17C358/XR17V358 channels.
 *
 * In discrete mode the driver uses the per-channel #RTS output.
 */
#define UART_MCR_DISCRETE_LINE_BIT UART_MCR_RTS_BIT

/** Number of UART channels implemented by XR17V358. */
#define XR17V358_UART_CHANNEL_COUNT 8U
/** Per-channel register window size for XR17V358. */
#define XR17V358_CHANNEL_STRIDE_BYTES 0x0400U
/** Full XR17V358 channel-window bytes (8 channels x 0x400 bytes). */
#define XR17V358_REGISTER_MAP_BYTES                                            \
    (XR17V358_UART_CHANNEL_COUNT * XR17V358_CHANNEL_STRIDE_BYTES)

/** XR17V358 channel offset 0x00 (RBR/THR/DLL alias). */
#define XR17V358_UART_REG_OFFSET_DATA 0x00U
/** XR17V358 channel offset 0x01 (IER/DLM alias). */
#define XR17V358_UART_REG_OFFSET_INTERRUPT_ENABLE 0x01U
/** XR17V358 channel offset 0x02 (IIR/FCR alias). */
#define XR17V358_UART_REG_OFFSET_FIFO_CONTROL 0x02U
/** XR17V358 channel offset 0x03 (LCR). */
#define XR17V358_UART_REG_OFFSET_LCR 0x03U
/** XR17V358 channel offset 0x04 (MCR). */
#define XR17V358_UART_REG_OFFSET_MCR 0x04U
/** XR17V358 channel offset 0x05 (LSR). */
#define XR17V358_UART_REG_OFFSET_LSR 0x05U
/** XR17V358 channel offset 0x06 (MSR or RS485DLY depending on EFR[4]). */
#define XR17V358_UART_REG_OFFSET_MSR_OR_RS485DLY 0x06U
/** XR17V358 channel offset 0x07 (SPR). */
#define XR17V358_UART_REG_OFFSET_SPR 0x07U
/** XR17V358 channel offset 0x08 (FCTR). */
#define XR17V358_UART_REG_OFFSET_FCTR 0x08U
/** XR17V358 channel offset 0x09 (EFR). */
#define XR17V358_UART_REG_OFFSET_EFR 0x09U
/** XR17V358 channel offset 0x0A (TXCNT or TXTRG depending on EFR[4]). */
#define XR17V358_UART_REG_OFFSET_TXCNT_OR_TXTRG 0x0AU
/** XR17V358 channel offset 0x0B (RXCNT or RXTRG depending on EFR[4]). */
#define XR17V358_UART_REG_OFFSET_RXCNT_OR_RXTRG 0x0BU
/** XR17V358 channel offset 0x0C (XOFF1/XONRCVD1/XCHAR alias). */
#define XR17V358_UART_REG_OFFSET_XOFF1_OR_XONRCVD1_OR_XCHAR 0x0CU
/** XR17V358 channel offset 0x0D (XOFF2/XONRCVD2 alias). */
#define XR17V358_UART_REG_OFFSET_XOFF2_OR_XONRCVD2 0x0DU
/** XR17V358 channel offset 0x0E (XON1/XOFFRCVD1 alias). */
#define XR17V358_UART_REG_OFFSET_XON1_OR_XOFFRCVD1 0x0EU
/** XR17V358 channel offset 0x0F (XON2/XOFFRCVD2 alias). */
#define XR17V358_UART_REG_OFFSET_XON2_OR_XOFFRCVD2 0x0FU

/** XR17V358 channel offset 0x80 (INT0). */
#define XR17V358_REG_OFFSET_INT0 0x0080U
/** XR17V358 channel offset 0x81 (INT1). */
#define XR17V358_REG_OFFSET_INT1 0x0081U
/** XR17V358 channel offset 0x82 (INT2). */
#define XR17V358_REG_OFFSET_INT2 0x0082U
/** XR17V358 channel offset 0x83 (INT3). */
#define XR17V358_REG_OFFSET_INT3 0x0083U
/** XR17V358 channel offset 0x84 (TIMERCNTL). */
#define XR17V358_REG_OFFSET_TIMERCNTL 0x0084U
/** XR17V358 channel offset 0x85 (REGA). */
#define XR17V358_REG_OFFSET_REGA 0x0085U
/** XR17V358 channel offset 0x86 (TIMERLSB). */
#define XR17V358_REG_OFFSET_TIMERLSB 0x0086U
/** XR17V358 channel offset 0x87 (TIMERMSB). */
#define XR17V358_REG_OFFSET_TIMERMSB 0x0087U
/** XR17V358 channel offset 0x88 (8XMODE). */
#define XR17V358_REG_OFFSET_8XMODE 0x0088U
/** XR17V358 channel offset 0x89 (4XMODE). */
#define XR17V358_REG_OFFSET_4XMODE 0x0089U
/** XR17V358 channel offset 0x8A (RESET). */
#define XR17V358_REG_OFFSET_RESET 0x008AU
/** XR17V358 channel offset 0x8B (SLEEP). */
#define XR17V358_REG_OFFSET_SLEEP 0x008BU
/** XR17V358 channel offset 0x8C (DREV). */
#define XR17V358_REG_OFFSET_DREV 0x008CU
/** XR17V358 channel offset 0x8D (DVID). */
#define XR17V358_REG_OFFSET_DVID 0x008DU
/** XR17V358 channel offset 0x8E (REGB). */
#define XR17V358_REG_OFFSET_REGB 0x008EU
/** XR17V358 channel offset 0x8F (MPIOINT[7:0]). */
#define XR17V358_REG_OFFSET_MPIOINT_7_0 0x008FU
/** XR17V358 channel offset 0x90 (MPIOLVL[7:0]). */
#define XR17V358_REG_OFFSET_MPIOLVL_7_0 0x0090U
/** XR17V358 channel offset 0x91 (MPIO3T[7:0]). */
#define XR17V358_REG_OFFSET_MPIO3T_7_0 0x0091U
/** XR17V358 channel offset 0x92 (MPIOINV[7:0]). */
#define XR17V358_REG_OFFSET_MPIOINV_7_0 0x0092U
/** XR17V358 channel offset 0x93 (MPIOSEL[7:0]). */
#define XR17V358_REG_OFFSET_MPIOSEL_7_0 0x0093U
/** XR17V358 channel offset 0x94 (MPIOOD[7:0]). */
#define XR17V358_REG_OFFSET_MPIOOD_7_0 0x0094U
/** XR17V358 channel offset 0x95 (MPIOINT[15:8]). */
#define XR17V358_REG_OFFSET_MPIOINT_15_8 0x0095U
/** XR17V358 channel offset 0x96 (MPIOLVL[15:8]). */
#define XR17V358_REG_OFFSET_MPIOLVL_15_8 0x0096U
/** XR17V358 channel offset 0x97 (MPIO3T[15:8]). */
#define XR17V358_REG_OFFSET_MPIO3T_15_8 0x0097U
/** XR17V358 channel offset 0x98 (MPIOINV[15:8]). */
#define XR17V358_REG_OFFSET_MPIOINV_15_8 0x0098U
/** XR17V358 channel offset 0x99 (MPIOSEL[15:8]). */
#define XR17V358_REG_OFFSET_MPIOSEL_15_8 0x0099U
/** XR17V358 channel offset 0x9A (MPIOOD[15:8]). */
#define XR17V358_REG_OFFSET_MPIOOD_15_8 0x009AU

/** XR17V358 global offset 0x0100 (channel 0 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_0_FIFO_DATA 0x0100U
/** XR17V358 global offset 0x0200 (channel 0 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_0_FIFO_DATA_WITH_STATUS 0x0200U
/** XR17V358 global offset 0x0300 (channel 0 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_0_FIFO_LSR_STATUS 0x0300U

/** XR17V358 global offset 0x0500 (channel 1 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_1_FIFO_DATA 0x0500U
/** XR17V358 global offset 0x0600 (channel 1 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_1_FIFO_DATA_WITH_STATUS 0x0600U
/** XR17V358 global offset 0x0700 (channel 1 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_1_FIFO_LSR_STATUS 0x0700U

/** XR17V358 global offset 0x0900 (channel 2 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_2_FIFO_DATA 0x0900U
/** XR17V358 global offset 0x0A00 (channel 2 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_2_FIFO_DATA_WITH_STATUS 0x0A00U
/** XR17V358 global offset 0x0B00 (channel 2 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_2_FIFO_LSR_STATUS 0x0B00U

/** XR17V358 global offset 0x0D00 (channel 3 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_3_FIFO_DATA 0x0D00U
/** XR17V358 global offset 0x0E00 (channel 3 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_3_FIFO_DATA_WITH_STATUS 0x0E00U
/** XR17V358 global offset 0x0F00 (channel 3 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_3_FIFO_LSR_STATUS 0x0F00U

/** XR17V358 global offset 0x1100 (channel 4 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_4_FIFO_DATA 0x1100U
/** XR17V358 global offset 0x1200 (channel 4 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_4_FIFO_DATA_WITH_STATUS 0x1200U
/** XR17V358 global offset 0x1300 (channel 4 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_4_FIFO_LSR_STATUS 0x1300U

/** XR17V358 global offset 0x1500 (channel 5 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_5_FIFO_DATA 0x1500U
/** XR17V358 global offset 0x1600 (channel 5 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_5_FIFO_DATA_WITH_STATUS 0x1600U
/** XR17V358 global offset 0x1700 (channel 5 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_5_FIFO_LSR_STATUS 0x1700U

/** XR17V358 global offset 0x1900 (channel 6 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_6_FIFO_DATA 0x1900U
/** XR17V358 global offset 0x1A00 (channel 6 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_6_FIFO_DATA_WITH_STATUS 0x1A00U
/** XR17V358 global offset 0x1B00 (channel 6 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_6_FIFO_LSR_STATUS 0x1B00U

/** XR17V358 global offset 0x1D00 (channel 7 FIFO read/write data). */
#define XR17V358_REG_OFFSET_CHANNEL_7_FIFO_DATA 0x1D00U
/** XR17V358 global offset 0x1E00 (channel 7 FIFO data with status). */
#define XR17V358_REG_OFFSET_CHANNEL_7_FIFO_DATA_WITH_STATUS 0x1E00U
/** XR17V358 global offset 0x1F00 (channel 7 FIFO LSR status bytes). */
#define XR17V358_REG_OFFSET_CHANNEL_7_FIFO_LSR_STATUS 0x1F00U

/** XR17V358 FIFO depth in bytes. */
#define XR17V358_FIFO_DEPTH 256U

/**
 * @brief UART data register view at offset 0.
 *
 * Register alias depends on DLAB bit in LCR:
 * - DLAB=0: RBR (read) / THR (write)
 * - DLAB=1: DLL
 */
typedef union DataRegisters
{
    volatile uint8_t rbr;
    volatile uint8_t thr;
    volatile uint8_t dll;
} uart16550_data_reg_t;

/**
 * @brief UART interrupt/divisor-high register view at offset 1.
 *
 * Register alias depends on DLAB bit in LCR:
 * - DLAB=0: IER
 * - DLAB=1: DLM
 */
typedef union InterruptRegisters
{
    volatile uint8_t ier;
    volatile uint8_t dlm;
} uart16550_interrupt_reg_t;

/**
 * @brief UART FIFO/interrupt-ID register view at offset 2.
 *
 * - Read: IIR
 * - Write: FCR
 */
typedef union FifoRegisters
{
    volatile uint8_t iir;
    volatile uint8_t fcr;
} uart16550_fifo_reg_t;

/**
 * @brief Memory-mapped layout for a 16550-compatible UART.
 */
typedef struct UART16550Registers
{
    /** Offset 0x00: RBR/THR/DLL depending on access and DLAB. */
    uart16550_data_reg_t data;
    /** Offset 0x01: IER/DLM depending on DLAB. */
    uart16550_interrupt_reg_t interrupt_enable;
    /** Offset 0x02: IIR (read) / FCR (write). */
    uart16550_fifo_reg_t fifo_control;
    /** Offset 0x03: Line Control Register. */
    volatile uint8_t lcr;
    /** Offset 0x04: Modem Control Register. */
    volatile uint8_t mcr;
    /** Offset 0x05: Line Status Register. */
    volatile uint8_t lsr;
    /** Offset 0x06: Modem Status Register. */
    volatile uint8_t msr;
    /** Offset 0x07: Scratch Register. */
    volatile uint8_t scr;
} uart16550_registers_t;

/**
 * @brief XR17V358 offset 0x06 register view.
 *
 * Register alias depends on enhanced mode:
 * - Enhanced register select disabled: MSR
 * - Enhanced register select enabled (EFR[4]=1): RS485DLY
 */
typedef union XR17V358MSRRegisters
{
    volatile uint8_t msr;
    volatile uint8_t rs485dly;
} xr17v358_msr_reg_t;

/**
 * @brief XR17V358 offset 0x0A register view.
 *
 * Register alias depends on enhanced mode:
 * - Enhanced register select disabled: TXCNT
 * - Enhanced register select enabled (EFR[4]=1): TXTRG
 */
typedef union XR17V358TXRegisters
{
    volatile uint8_t txcnt;
    volatile uint8_t txtrg;
} xr17v358_tx_reg_t;

/**
 * @brief XR17V358 offset 0x0B register view.
 *
 * Register alias depends on enhanced mode:
 * - Enhanced register select disabled: RXCNT
 * - Enhanced register select enabled (EFR[4]=1): RXTRG
 */
typedef union XR17V358RXRegisters
{
    volatile uint8_t rxcnt;
    volatile uint8_t rxtrg;
} xr17v358_rx_reg_t;

/**
 * @brief XR17V358 offset 0x0C register view.
 *
 * Register alias depends on enhanced mode:
 * - XOFF1
 * - XONRCVD1
 * - XCHAR
 */
typedef union XR17V358FlowControl1Registers
{
    volatile uint8_t xoff1;
    volatile uint8_t xonrcvd1;
    volatile uint8_t xchar;
} xr17v358_flow_control_1_reg_t;

/**
 * @brief XR17V358 offset 0x0D register view.
 *
 * Register alias depends on enhanced mode:
 * - XOFF2
 * - XONRCVD2
 */
typedef union XR17V358FlowControl2Registers
{
    volatile uint8_t xoff2;
    volatile uint8_t xonrcvd2;
} xr17v358_flow_control_2_reg_t;

/**
 * @brief XR17V358 offset 0x0E register view.
 *
 * Register alias depends on enhanced mode:
 * - XON1
 * - XOFFRCVD1
 */
typedef union XR17V358FlowControl3Registers
{
    volatile uint8_t xon1;
    volatile uint8_t xoffrcvd1;
} xr17v358_flow_control_3_reg_t;

/**
 * @brief XR17V358 offset 0x0F register view.
 *
 * Register alias depends on enhanced mode:
 * - XON2
 * - XOFFRCVD2
 */
typedef union XR17V358FlowControl4Registers
{
    volatile uint8_t xon2;
    volatile uint8_t xoffrcvd2;
} xr17v358_flow_control_4_reg_t;

/**
 * @brief XR17V358 channel UART register block (offsets 0x00-0x0F).
 */
typedef struct XR17V358UartChannelRegisters
{
    /** Offset 0x00: RBR/THR/DLL depending on access and DLAB. */
    uart16550_data_reg_t data;
    /** Offset 0x01: IER/DLM depending on DLAB. */
    uart16550_interrupt_reg_t interrupt_enable;
    /** Offset 0x02: IIR (read) / FCR (write). */
    uart16550_fifo_reg_t fifo_control;
    /** Offset 0x03: Line Control Register. */
    volatile uint8_t lcr;
    /** Offset 0x04: Modem Control Register. */
    volatile uint8_t mcr;
    /** Offset 0x05: Line Status Register. */
    volatile uint8_t lsr;
    /** Offset 0x06: MSR / RS485DLY alias. */
    xr17v358_msr_reg_t msr_or_rs485dly;
    /** Offset 0x07: Scratchpad Register (SPR). */
    volatile uint8_t spr;
    /** Offset 0x08: Feature Control Register (FCTR). */
    volatile uint8_t fctr;
    /** Offset 0x09: Enhanced Feature Register (EFR). */
    volatile uint8_t efr;
    /** Offset 0x0A: TXCNT / TXTRG alias. */
    xr17v358_tx_reg_t txcnt_or_txtrg;
    /** Offset 0x0B: RXCNT / RXTRG alias. */
    xr17v358_rx_reg_t rxcnt_or_rxtrg;
    /** Offset 0x0C: XOFF1 / XONRCVD1 / XCHAR alias. */
    xr17v358_flow_control_1_reg_t flow_control_1;
    /** Offset 0x0D: XOFF2 / XONRCVD2 alias. */
    xr17v358_flow_control_2_reg_t flow_control_2;
    /** Offset 0x0E: XON1 / XOFFRCVD1 alias. */
    xr17v358_flow_control_3_reg_t flow_control_3;
    /** Offset 0x0F: XON2 / XOFFRCVD2 alias. */
    xr17v358_flow_control_4_reg_t flow_control_4;
} xr17v358_uart_channel_registers_t;

/**
 * @brief XR17V358 device-configuration register block (offsets 0x80-0x9A).
 */
typedef struct XR17V358DeviceConfigurationRegisters
{
    volatile uint8_t int0;
    volatile uint8_t int1;
    volatile uint8_t int2;
    volatile uint8_t int3;
    volatile uint8_t timercntl;
    volatile uint8_t rega;
    volatile uint8_t timerlsb;
    volatile uint8_t timermsb;
    volatile uint8_t mode_8x;
    volatile uint8_t mode_4x;
    volatile uint8_t reset;
    volatile uint8_t sleep;
    volatile uint8_t drev;
    volatile uint8_t dvid;
    volatile uint8_t regb;
    volatile uint8_t mpioint_7_0;
    volatile uint8_t mpiolvl_7_0;
    volatile uint8_t mpio3t_7_0;
    volatile uint8_t mpioinv_7_0;
    volatile uint8_t mpiosel_7_0;
    volatile uint8_t mpiood_7_0;
    volatile uint8_t mpioint_15_8;
    volatile uint8_t mpiolvl_15_8;
    volatile uint8_t mpio3t_15_8;
    volatile uint8_t mpioinv_15_8;
    volatile uint8_t mpiosel_15_8;
    volatile uint8_t mpiood_15_8;
} xr17v358_device_config_registers_t;

/**
 * @brief XR17V358 direct FIFO window (offset 0x100-0x1FF).
 *
 * Reading accesses FIFO data bytes.
 * Writing pushes FIFO data bytes.
 */
typedef union XR17V358DataFIFORegisters
{
    volatile uint8_t rx_data[XR17V358_FIFO_DEPTH];
    volatile uint8_t tx_data[XR17V358_FIFO_DEPTH];
} xr17v358_data_fifo_registers_t;

/**
 * @brief XR17V358 FIFO data-with-status window (offset 0x200-0x3FF).
 */
typedef struct XR17V358FIFODataWithStatusRegisters
{
    /** Offset 0x200-0x2FF: FIFO data bytes. */
    volatile uint8_t data[XR17V358_FIFO_DEPTH];
    /** Offset 0x300-0x3FF: line-status bytes corresponding to data bytes. */
    volatile uint8_t lsr_status[XR17V358_FIFO_DEPTH];
} xr17v358_fifo_data_with_status_registers_t;

/**
 * @brief Full XR17V358 per-channel register window (0x000-0x3FF).
 */
typedef struct XR17V358ChannelRegisterMap
{
    /** Offset 0x000-0x00F: UART core + enhanced register aliases. */
    xr17v358_uart_channel_registers_t uart;
    /** Offset 0x010-0x07F: reserved. */
    volatile uint8_t reserved_0010_007f[0x70U];
    /** Offset 0x080-0x09A: device-configuration registers. */
    xr17v358_device_config_registers_t device_config;
    /** Offset 0x09B-0x0FF: reserved. */
    volatile uint8_t reserved_009b_00ff[0x65U];
    /** Offset 0x100-0x1FF: direct FIFO data window. */
    xr17v358_data_fifo_registers_t fifo_data;
    /** Offset 0x200-0x3FF: FIFO data and status window. */
    xr17v358_fifo_data_with_status_registers_t fifo_data_with_status;
} xr17v358_channel_register_map_t;

/**
 * @brief Full XR17V358 register map (8 channels).
 */
typedef struct XR17V358RegisterMap
{
    xr17v358_channel_register_map_t channels[XR17V358_UART_CHANNEL_COUNT];
} xr17v358_register_map_t;

/**
 * @brief XR17C358 compatibility aliases.
 *
 * XR17C358 and XR17V358 share the same logical register map in this driver.
 */
#define XR17C358_UART_CHANNEL_COUNT XR17V358_UART_CHANNEL_COUNT
#define XR17C358_CHANNEL_STRIDE_BYTES XR17V358_CHANNEL_STRIDE_BYTES
#define XR17C358_REGISTER_MAP_BYTES XR17V358_REGISTER_MAP_BYTES
#define XR17C358_FIFO_DEPTH XR17V358_FIFO_DEPTH

typedef xr17v358_device_config_registers_t xr17c358_device_config_registers_t;
typedef xr17v358_channel_register_map_t xr17c358_channel_register_map_t;

#endif
