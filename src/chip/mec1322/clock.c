/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Clocks and power management settings */

#include "clock.h"
#include "common.h"
#include "console.h"
#include "registers.h"
#include "util.h"

/* Console output macros */
#define CPUTS(outstr) cputs(CC_CLOCK, outstr)
#define CPRINTS(format, args...) cprints(CC_CLOCK, format, ## args)

static int freq = 48000000;

void clock_wait_cycles(uint32_t cycles)
{
	asm("1: subs %0, #1\n"
	    "   bne 1b\n" :: "r"(cycles));
}

int clock_get_freq(void)
{
	return freq;
}

void clock_init(void)
{
	/* XOSEL = Single ended clock source */
	MEC1322_VBAT_CE |= 0x1;

	/* 32K clock enable */
	MEC1322_VBAT_CE |= 0x2;
}
