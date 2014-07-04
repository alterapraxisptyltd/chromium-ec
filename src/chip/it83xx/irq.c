/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * IT83xx chip-specific part of the IRQ handling.
 */

#include "common.h"
#include "irq_chip.h"
#include "registers.h"

#define IRQ_GROUP(n, cpu_ints...) \
	{(uint32_t)&CONCAT2(IT83XX_INTC_ISR, n) - IT83XX_INTC_BASE, \
	 (uint32_t)&CONCAT2(IT83XX_INTC_IER, n) - IT83XX_INTC_BASE, \
	 ##cpu_ints}

static const struct {
	uint8_t isr_off;
	uint8_t ier_off;
	uint8_t cpu_int[8];
} irq_groups[20] = {
	IRQ_GROUP(0,  {-1,  2,  5,  4,  6,  2,  2,  4}),
	IRQ_GROUP(1,  { 7,  6,  6,  5,  2,  2,  2,  8}),
	IRQ_GROUP(2,  { 6,  2,  8,  8,  8,  2, 12, -1}),
	IRQ_GROUP(3,  { 5,  4,  4,  4, 11, 11,  3,  2}),
	IRQ_GROUP(4,  {11, 11, 11, 11,  8,  9,  9,  9}),
	IRQ_GROUP(5,  {-1, -1, -1, -1, -1, -1, -1, -1}),
	IRQ_GROUP(6,  { 2,  2,  2,  2,  2,  2,  2,  2}),
	IRQ_GROUP(7,  {10, 10,  3, -1,  3,  3,  3,  3}),
	IRQ_GROUP(8,  { 4,  4,  4,  4,  4,  4, 12, 12}),
	IRQ_GROUP(9,  { 2,  2,  2,  2,  2,  2,  2,  2}),
	IRQ_GROUP(10, { 3,  6, 12, 12,  5,  2,  2,  2}),
	IRQ_GROUP(11, { 2,  2,  2,  2,  2,  2,  2,  2}),
	IRQ_GROUP(12, { 2,  2,  2,  2,  2,  2,  2,  2}),
	IRQ_GROUP(13, { 2,  2,  2,  2,  2,  2,  2,  2}),
	IRQ_GROUP(14, { 2,  2,  2,  2,  2,  2,  2,  2}),
	IRQ_GROUP(15, { 2,  2,  2,  2,  2,  2,  2,  2}),
	IRQ_GROUP(16, { 2,  2,  2,  2,  2,  2,  2, -1}),
	IRQ_GROUP(17, {-1, -1, -1, -1, -1, -1, -1, -1}),
	IRQ_GROUP(18, { 2,  2,  2,  2,  2,  4,  4,  7}),
	IRQ_GROUP(19, { 6,  6, 12,  3,  3,  3,  3,  3}),
};

int chip_enable_irq(int irq)
{
	int group = irq / 8;
	int bit = irq % 8;

	IT83XX_INTC_REG(irq_groups[group].ier_off) |= 1 << bit;
	IT83XX_INTC_REG(IT83XX_INTC_EXT_IER_OFF(group)) |= 1 << bit;

	return irq_groups[group].cpu_int[bit];
}

int chip_disable_irq(int irq)
{
	int group = irq / 8;
	int bit = irq % 8;

	IT83XX_INTC_REG(irq_groups[group].ier_off) &= ~(1 << bit);
	IT83XX_INTC_REG(IT83XX_INTC_EXT_IER_OFF(group)) &= ~(1 << bit);

	return -1; /* we don't want to mask other IRQs */
}

int chip_clear_pending_irq(int irq)
{
	int group = irq / 8;
	int bit = irq % 8;

	IT83XX_INTC_REG(irq_groups[group].isr_off) |= 1 << bit;

	return -1; /* everything has been done */
}

int chip_trigger_irq(int irq)
{
	int group = irq / 8;
	int bit = irq % 8;

	return irq_groups[group].cpu_int[bit];
}

void chip_init_irqs(void)
{
	/* TODO(crosbug.com/p/23575): IMPLEMENT ME ! */
}
