#ifndef SERIAL_DRIVER_REGISTER_MAP_H
#define SERIAL_DRIVER_REGISTER_MAP_H

/**
 * @file register_map.h
 * @brief Shared 16550 UART register map and register-offset aliases.
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

/**
 * @brief UART data register view at offset 0.
 *
 * Register alias depends on DLAB bit in LCR:
 * - DLAB=0: RBR (read) / THR (write)
 * - DLAB=1: DLL
 */
typedef union DataRegisters {
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
typedef union InterruptRegisters {
    volatile uint8_t ier;
    volatile uint8_t dlm;
} uart16550_interrupt_reg_t;

/**
 * @brief UART FIFO/interrupt-ID register view at offset 2.
 *
 * - Read: IIR
 * - Write: FCR
 */
typedef union FifoRegisters {
    volatile uint8_t iir;
    volatile uint8_t fcr;
} uart16550_fifo_reg_t;

/**
 * @brief Memory-mapped layout for a 16550-compatible UART.
 */
typedef struct UART16550Registers {
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

#endif
