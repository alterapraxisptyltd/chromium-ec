/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "adc.h"
#include "adc_chip.h"
#include "common.h"
#include "console.h"
#include "clock.h"
#include "dma.h"
#include "hooks.h"
#include "registers.h"
#include "task.h"
#include "timer.h"
#include "util.h"

#define ADC_SINGLE_READ_TIMEOUT 3000 /* 3 ms */

struct mutex adc_lock;

static int restore_clock;

static const struct dma_option dma_adc_option = {
	STM32_DMAC_ADC, (void *)&STM32_ADC_DR,
	STM32_DMA_CCR_MSIZE_16_BIT | STM32_DMA_CCR_PSIZE_16_BIT,
};

static inline void adc_set_channel(int sample_id, int channel)
{
	uint32_t mask, val;
	volatile uint32_t *sqr_reg;
	int reg_id;

	reg_id = 5 - sample_id / 6;

	mask = 0x1f << ((sample_id % 6) * 5);
	val = channel << ((sample_id % 6) * 5);
	sqr_reg = &STM32_ADC_SQR(reg_id);

	*sqr_reg = (*sqr_reg & ~mask) | val;
}

static void adc_configure(int ain_id)
{
	/* Set ADC channel */
	adc_set_channel(0, ain_id);

	/* Disable DMA */
	STM32_ADC_CR2 &= ~(1 << 8);

	/* Disable scan mode */
	STM32_ADC_CR1 &= ~(1 << 8);
}

static void adc_configure_all(void)
{
	int i;

	/* Set ADC channels */
	STM32_ADC_SQR1 = (ADC_CH_COUNT - 1) << 20;
	for (i = 0; i < ADC_CH_COUNT; ++i)
		adc_set_channel(i, adc_channels[i].channel);

	/* Enable DMA */
	STM32_ADC_CR2 |= (1 << 8);

	/* Enable scan mode */
	STM32_ADC_CR1 |= (1 << 8);
}

static inline int adc_powered(void)
{
	return STM32_ADC_SR & (1 << 6); /* ADONS */
}

static void adc_enable_clock(void)
{
	STM32_RCC_APB2ENR |= (1 << 9);
	/* ADCCLK = HSI / 2 = 8MHz*/
	STM32_ADC_CCR |= (1 << 16);
}

static void adc_init(void)
{
	/*
	 * For STM32L, ADC clock source is HSI/2 = 8 MHz. HSI must be enabled
	 * for ADC.
	 *
	 * Note that we are not powering on ADC on EC initialization because
	 * STM32L ADC module requires HSI clock. Instead, ADC module is powered
	 * on/off in adc_prepare()/adc_release().
	 */

	/* Enable ADC clock. */
	adc_enable_clock();

	if (!adc_powered())
		/* Power on ADC module */
		STM32_ADC_CR2 |= (1 << 0);  /* ADON */

	/* Set right alignment */
	STM32_ADC_CR2 &= ~(1 << 11);

	/*
	 * Set sample time of all channels to 16 cycles.
	 * Conversion takes (12+16)/8M = 3.34 us.
	 */
	STM32_ADC_SMPR1 = 0x24924892;
	STM32_ADC_SMPR2 = 0x24924892;
	STM32_ADC_SMPR3 = 0x24924892;
}

static void adc_prepare(void)
{
	if (!adc_powered()) {
		clock_enable_module(MODULE_ADC, 1);
		adc_init();
		restore_clock = 1;
	}
}

static void adc_release(void)
{
	if (restore_clock) {
		clock_enable_module(MODULE_ADC, 0);
		restore_clock = 0;
	}

	/*
	 * Power down the ADC.  The ADC consumes a non-trivial amount of power,
	 * so it's wasteful to leave it on.
	 */
	if (adc_powered())
		STM32_ADC_CR2 = 0;
}

static inline int adc_conversion_ended(void)
{
	return STM32_ADC_SR & (1 << 1);
}

int adc_read_channel(enum adc_channel ch)
{
	const struct adc_t *adc = adc_channels + ch;
	int value;
	timestamp_t deadline;

	mutex_lock(&adc_lock);

	adc_prepare();

	adc_configure(adc->channel);

	/* Clear EOC bit */
	STM32_ADC_SR &= ~(1 << 1);

	/* Start conversion */
	STM32_ADC_CR2 |= (1 << 30); /* SWSTART */

	/* Wait for EOC bit set */
	deadline.val = get_time().val + ADC_SINGLE_READ_TIMEOUT;
	value = ADC_READ_ERROR;
	do {
		if (adc_conversion_ended()) {
			value = STM32_ADC_DR & ADC_READ_MAX;
			break;
		}
	} while (!timestamp_expired(deadline, NULL));

	adc_release();

	mutex_unlock(&adc_lock);
	return (value == ADC_READ_ERROR) ? ADC_READ_ERROR :
	       value * adc->factor_mul / adc->factor_div + adc->shift;
}

int adc_read_all_channels(int *data)
{
	int i;
	int16_t raw_data[ADC_CH_COUNT];
	const struct adc_t *adc;
	int ret = EC_SUCCESS;

	mutex_lock(&adc_lock);

	adc_prepare();

	adc_configure_all();

	dma_start_rx(&dma_adc_option, ADC_CH_COUNT, raw_data);

	/* Start conversion */
	STM32_ADC_CR2 |= (1 << 30); /* SWSTART */

	if (dma_wait(STM32_DMAC_ADC)) {
		ret = EC_ERROR_UNKNOWN;
		goto exit_all_channels;
	}
	dma_clear_isr(STM32_DMAC_ADC);

	for (i = 0; i < ADC_CH_COUNT; ++i) {
		adc = adc_channels + i;
		data[i] = raw_data[i] * adc->factor_mul / adc->factor_div +
			  adc->shift;
	}

exit_all_channels:
	adc_release();
	mutex_unlock(&adc_lock);

	return ret;
}
