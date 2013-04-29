/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* EC for Link board configuration */

#include "adc.h"
#include "chip_temp_sensor.h"
#include "common.h"
#include "extpower.h"
#include "gpio.h"
#include "i2c.h"
#include "keyboard_scan.h"
#include "lid_switch.h"
#include "lm4_adc.h"
#include "peci.h"
#include "registers.h"
#include "switch.h"
#include "temp_sensor.h"
#include "timer.h"
#include "tmp006.h"
#include "util.h"
#include "x86_power.h"

/* GPIO signal list.  Must match order from enum gpio_signal. */
const struct gpio_info gpio_list[GPIO_COUNT] = {
	/* Inputs with interrupt handlers are first for efficiency */
	{"POWER_BUTTON_L",       LM4_GPIO_K, (1<<7), GPIO_INT_BOTH,
	 switch_interrupt},
	{"LID_OPEN",             LM4_GPIO_K, (1<<5), GPIO_INT_BOTH,
	 lid_interrupt},
	/* Other inputs */
	{"THERMAL_DATA_READY_L", LM4_GPIO_B, (1<<4), 0, NULL},
	{"AC_PRESENT",           LM4_GPIO_H, (1<<3), GPIO_INT_BOTH,
	 extpower_interrupt},
	{"BOARD_VERSION1",       LM4_GPIO_H, (1<<6), 0, NULL},
	{"BOARD_VERSION2",       LM4_GPIO_L, (1<<6), 0, NULL},
	{"BOARD_VERSION3",       LM4_GPIO_L, (1<<7), 0, NULL},
	{"PCH_BKLTEN",           LM4_GPIO_J, (1<<3), GPIO_INT_BOTH,
	 switch_interrupt},
	{"PCH_SLP_A_L",          LM4_GPIO_G, (1<<5), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_ME_CSW_DEV_L", LM4_GPIO_G, (1<<4), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_S3_L",         LM4_GPIO_J, (1<<0), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_S4_L",         LM4_GPIO_J, (1<<1), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_S5_L",         LM4_GPIO_J, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_SUS_L",        LM4_GPIO_G, (1<<3), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SUSWARN_L",        LM4_GPIO_G, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_1_5V_DDR",       LM4_GPIO_K, (1<<0), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_1_5V_PCH",       LM4_GPIO_K, (1<<1), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_1_8VS",          LM4_GPIO_K, (1<<3), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_5VALW",          LM4_GPIO_H, (1<<0), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_CPU_CORE",       LM4_GPIO_M, (1<<3), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_VCCP",           LM4_GPIO_K, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_VCCSA",          LM4_GPIO_H, (1<<1), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_VGFX_CORE",      LM4_GPIO_D, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"RECOVERY_L",           LM4_GPIO_H, (1<<7), GPIO_INT_BOTH,
	 switch_interrupt},
	{"USB1_STATUS_L",        LM4_GPIO_E, (1<<7), 0, NULL},
	{"USB2_STATUS_L",        LM4_GPIO_E, (1<<1), 0, NULL},
	{"WRITE_PROTECT",        LM4_GPIO_J, (1<<4), GPIO_INT_BOTH,
	 switch_interrupt},
	/* Outputs; all unasserted by default except for reset signals */
	{"CPU_PROCHOT",          LM4_GPIO_F, (1<<2), GPIO_OUT_LOW, NULL},
	{"ENABLE_1_5V_DDR",      LM4_GPIO_H, (1<<5), GPIO_OUT_LOW, NULL},
	{"ENABLE_5VALW",         LM4_GPIO_K, (1<<4), GPIO_OUT_HIGH, NULL},
	{"ENABLE_BACKLIGHT",     LM4_GPIO_H, (1<<4), GPIO_OUT_LOW, NULL},
	{"ENABLE_TOUCHPAD",      LM4_GPIO_C, (1<<6), GPIO_OUT_LOW, NULL},
	{"ENABLE_VCORE",         LM4_GPIO_F, (1<<7), GPIO_OUT_LOW, NULL},
	{"ENABLE_VS",            LM4_GPIO_G, (1<<6), GPIO_OUT_LOW, NULL},
	{"ENABLE_WLAN",          LM4_GPIO_Q, (1<<5), GPIO_OUT_LOW, NULL},
	{"ENTERING_RW",          LM4_GPIO_J, (1<<5), GPIO_OUT_LOW, NULL},
	{"LIGHTBAR_RESET_L",     LM4_GPIO_B, (1<<1), GPIO_OUT_LOW, NULL},
	{"PCH_A20GATE",          LM4_GPIO_Q, (1<<6), GPIO_OUT_LOW, NULL},
	{"PCH_DPWROK",           LM4_GPIO_G, (1<<0), GPIO_OUT_LOW, NULL},
	/*
	 * HDA_SDO is technically an output, but we need to leave it as an
	 * input until we drive it high.  So can't use open-drain (HI_Z).
	 */
	{"PCH_HDA_SDO",          LM4_GPIO_G, (1<<1), GPIO_INPUT, NULL},
	{"PCH_WAKE_L",           LM4_GPIO_F, (1<<0), GPIO_OUT_HIGH, NULL},
	{"PCH_NMI_L",            LM4_GPIO_M, (1<<2), GPIO_OUT_HIGH, NULL},
	{"PCH_PWRBTN_L",         LM4_GPIO_G, (1<<7), GPIO_OUT_HIGH, NULL},
	{"PCH_PWROK",            LM4_GPIO_F, (1<<5), GPIO_OUT_LOW, NULL},
	{"PCH_RCIN_L",           LM4_GPIO_Q, (1<<7), GPIO_HI_Z, NULL},
	{"PCH_RSMRST_L",         LM4_GPIO_F, (1<<1), GPIO_OUT_LOW, NULL},
	{"PCH_RTCRST_L",         LM4_GPIO_F, (1<<6), GPIO_HI_Z, NULL},
	{"PCH_SMI_L",            LM4_GPIO_F, (1<<4), GPIO_OUT_HIGH, NULL},
	{"PCH_SRTCRST_L",        LM4_GPIO_C, (1<<7), GPIO_HI_Z, NULL},
	{"PCH_SUSACK_L",         LM4_GPIO_F, (1<<3), GPIO_OUT_HIGH, NULL},
	{"RADIO_ENABLE_WLAN",    LM4_GPIO_D, (1<<0), GPIO_OUT_LOW, NULL},
	{"RADIO_ENABLE_BT",      LM4_GPIO_D, (1<<1), GPIO_OUT_LOW, NULL},
	{"SPI_CS_L",             LM4_GPIO_A, (1<<3), GPIO_HI_Z, NULL},
	{"TOUCHSCREEN_RESET_L",  LM4_GPIO_B, (1<<0), GPIO_OUT_LOW, NULL},
	{"USB1_CTL1",            LM4_GPIO_E, (1<<2), GPIO_OUT_LOW, NULL},
	{"USB1_CTL2",            LM4_GPIO_E, (1<<3), GPIO_OUT_LOW, NULL},
	{"USB1_CTL3",            LM4_GPIO_E, (1<<4), GPIO_OUT_LOW, NULL},
	{"USB1_ENABLE",          LM4_GPIO_E, (1<<5), GPIO_OUT_LOW, NULL},
	{"USB1_ILIM_SEL",        LM4_GPIO_E, (1<<6), GPIO_OUT_LOW, NULL},
	{"USB2_CTL1",            LM4_GPIO_D, (1<<4), GPIO_OUT_LOW, NULL},
	{"USB2_CTL2",            LM4_GPIO_D, (1<<5), GPIO_OUT_LOW, NULL},
	{"USB2_CTL3",            LM4_GPIO_D, (1<<6), GPIO_OUT_LOW, NULL},
	{"USB2_ENABLE",          LM4_GPIO_D, (1<<7), GPIO_OUT_LOW, NULL},
	{"USB2_ILIM_SEL",        LM4_GPIO_E, (1<<0), GPIO_OUT_LOW, NULL},
};

/* ADC channels. Must be in the exactly same order as in enum adc_channel. */
const struct adc_t adc_channels[ADC_CH_COUNT] = {
	/* EC internal temperature is calculated by
	 * 273 + (295 - 450 * ADC_VALUE / ADC_READ_MAX) / 2
	 * = -225 * ADC_VALUE / ADC_READ_MAX + 420.5
	 */
	{"ECTemp", LM4_ADC_SEQ0, -225, ADC_READ_MAX, 420,
	 LM4_AIN_NONE, 0x0e /* TS0 | IE0 | END0 */, 0, 0},

	/* Charger current is mapped from 0~4000mA to 0~1.6V.
	 * And ADC maps 0~3.3V to ADC_READ_MAX.
	 */
	{"ChargerCurrent", LM4_ADC_SEQ1, 33 * 4000, ADC_READ_MAX * 16, 0,
	 LM4_AIN(11), 0x06 /* IE0 | END0 */, LM4_GPIO_B, (1<<5)},
};

/* I2C ports */
const struct i2c_port_t i2c_ports[I2C_PORTS_USED] = {
	/* Note: battery and charger share a port.  Only include it once in
	 * this list so we don't double-initialize it. */
	{"batt_chg", I2C_PORT_BATTERY,  100},
	{"lightbar", I2C_PORT_LIGHTBAR, 400},
	{"thermal",  I2C_PORT_THERMAL,  100},
};

#define TEMP_PCH_REG_ADDR	((0x41 << 1) | I2C_FLAG_BIG_ENDIAN)
#define TEMP_CHARGER_REG_ADDR	((0x43 << 1) | I2C_FLAG_BIG_ENDIAN)
#define TEMP_USB_REG_ADDR	((0x46 << 1) | I2C_FLAG_BIG_ENDIAN)
#define TEMP_HINGE_REG_ADDR	((0x44 << 1) | I2C_FLAG_BIG_ENDIAN)

#define TEMP_PCH_ADDR TMP006_ADDR(I2C_PORT_THERMAL, TEMP_PCH_REG_ADDR)
#define TEMP_CHARGER_ADDR TMP006_ADDR(I2C_PORT_THERMAL, TEMP_CHARGER_REG_ADDR)
#define TEMP_USB_ADDR TMP006_ADDR(I2C_PORT_THERMAL, TEMP_USB_REG_ADDR)
#define TEMP_HINGE_ADDR TMP006_ADDR(I2C_PORT_THERMAL, TEMP_HINGE_REG_ADDR)

/* Temperature sensors data; must be in same order as enum temp_sensor_id. */
const struct temp_sensor_t temp_sensors[TEMP_SENSOR_COUNT] = {
#ifdef CONFIG_TMP006
	{"I2C-USB C-Die", TEMP_SENSOR_TYPE_IGNORED, tmp006_get_val, 0, 7},
	{"I2C-USB C-Object", TEMP_SENSOR_TYPE_IGNORED, tmp006_get_val, 1, 7},
	{"I2C-PCH D-Die", TEMP_SENSOR_TYPE_BOARD, tmp006_get_val, 2, 7},
	{"I2C-PCH D-Object", TEMP_SENSOR_TYPE_CASE, tmp006_get_val, 3, 7},
	{"I2C-Hinge C-Die", TEMP_SENSOR_TYPE_IGNORED, tmp006_get_val, 4, 7},
	{"I2C-Hinge C-Object", TEMP_SENSOR_TYPE_IGNORED, tmp006_get_val, 5, 7},
	{"I2C-Charger D-Die", TEMP_SENSOR_TYPE_BOARD, tmp006_get_val, 6, 7},
	{"I2C-Charger D-Object", TEMP_SENSOR_TYPE_CASE, tmp006_get_val, 7, 7},
#endif
#ifdef CONFIG_ADC
	{"ECInternal", TEMP_SENSOR_TYPE_BOARD, chip_temp_sensor_get_val, 0, 4},
#endif
#ifdef CONFIG_PECI
	{"PECI", TEMP_SENSOR_TYPE_CPU, peci_temp_sensor_get_val, 0, 2},
#endif
};

const struct tmp006_t tmp006_sensors[TMP006_COUNT] = {
	{"USB C", TEMP_USB_ADDR},
	{"PCH D", TEMP_PCH_ADDR},
	{"Hinge C", TEMP_HINGE_ADDR},
	{"Charger D", TEMP_CHARGER_ADDR},
};

struct keyboard_scan_config keyscan_config = {
	.output_settle_us = 40,
	.debounce_down_us = 6 * MSEC,
	.debounce_up_us = 30 * MSEC,
	.scan_period_us = 1500,
	.min_post_scan_delay_us = 1000,
	.poll_timeout_us = SECOND,
	.actual_key_mask = {
		0x14, 0xff, 0xff, 0xff, 0xff, 0xf5, 0xff,
		0xa4, 0xff, 0xf6, 0x55, 0xfa, 0xc8  /* full set */
	},
};