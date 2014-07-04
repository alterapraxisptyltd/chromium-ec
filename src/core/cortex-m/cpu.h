/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Registers map and defintions for Cortex-MLM4x processor
 */

#ifndef __CPU_H
#define __CPU_H

#include <stdint.h>

/* Macro to access 32-bit registers */
#define CPUREG(addr) (*(volatile uint32_t*)(addr))

/* Nested Vectored Interrupt Controller */
#define CPU_NVIC_EN(x)         CPUREG(0xe000e100 + 4 * (x))
#define CPU_NVIC_DIS(x)        CPUREG(0xe000e180 + 4 * (x))
#define CPU_NVIC_UNPEND(x)     CPUREG(0xe000e280 + 4 * (x))
#define CPU_NVIC_PRI(x)        CPUREG(0xe000e400 + 4 * (x))
#define CPU_NVIC_APINT         CPUREG(0xe000ed0c)
#define CPU_NVIC_SWTRIG        CPUREG(0xe000ef00)

#define CPU_SCB_SYSCTRL        CPUREG(0xe000ed10)

#define CPU_NVIC_CCR           CPUREG(0xe000ed14)
#define CPU_NVIC_SHCSR         CPUREG(0xe000ed24)
#define CPU_NVIC_MMFS          CPUREG(0xe000ed28)
#define CPU_NVIC_HFSR          CPUREG(0xe000ed2c)
#define CPU_NVIC_DFSR          CPUREG(0xe000ed30)
#define CPU_NVIC_MFAR          CPUREG(0xe000ed34)
#define CPU_NVIC_BFAR          CPUREG(0xe000ed38)

enum {
	CPU_NVIC_MMFS_BFARVALID		= 1 << 15,
	CPU_NVIC_MMFS_MFARVALID		= 1 << 7,

	CPU_NVIC_CCR_DIV_0_TRAP		= 1 << 4,
	CPU_NVIC_CCR_UNALIGN_TRAP	= 1 << 3,

	CPU_NVIC_HFSR_DEBUGEVT		= 1UL << 31,
	CPU_NVIC_HFSR_FORCED		= 1 << 30,
	CPU_NVIC_HFSR_VECTTBL		= 1 << 1,

	CPU_NVIC_SHCSR_MEMFAULTENA	= 1 << 16,
	CPU_NVIC_SHCSR_BUSFAULTENA	= 1 << 17,
	CPU_NVIC_SHCSR_USGFAULTENA	= 1 << 18,
};

/* Set up the cpu to detect faults */
void cpu_init(void);

#endif /* __CPU_H */
