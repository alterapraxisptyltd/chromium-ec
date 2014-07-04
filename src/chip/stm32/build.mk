# -*- makefile -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# STM32 chip specific files build
#

ifeq ($(CHIP_FAMILY),stm32f0)
# STM32F0xx sub-family has a Cortex-M0 ARM core
CORE:=cortex-m0
# Force ARMv6-M ISA used by the Cortex-M0
CFLAGS_CPU+=-march=armv6-m -mcpu=cortex-m0
else
# other STM32 SoCs have a Cortex-M3 ARM core
CORE:=cortex-m
# Force Cortex-M3 subset of instructions
CFLAGS_CPU+=-march=armv7-m -mcpu=cortex-m3
endif

# STM32F0xx and STM32F1xx are using the same flash controller
FLASH_FAMILY=$(subst stm32f0,stm32f,$(CHIP_FAMILY))
# Select between 16-bit and 32-bit timer for clock source
TIMER_TYPE=$(if $(CONFIG_STM_HWTIMER32),32,)

chip-y=dma.o system.o
chip-y+=jtag-$(CHIP_FAMILY).o clock-$(CHIP_FAMILY).o
chip-$(CONFIG_SPI)+=spi.o
chip-$(CONFIG_SW_CRC)+=crc.o
chip-$(CONFIG_COMMON_GPIO)+=gpio-$(CHIP_FAMILY).o
chip-$(CONFIG_COMMON_TIMER)+=hwtimer$(TIMER_TYPE).o
chip-$(CONFIG_I2C)+=i2c-$(CHIP_FAMILY).o
chip-$(CONFIG_WATCHDOG)+=watchdog.o
chip-$(HAS_TASK_CONSOLE)+=uart.o
chip-$(HAS_TASK_KEYSCAN)+=keyboard_raw.o
chip-$(HAS_TASK_POWERLED)+=power_led.o
chip-$(CONFIG_FLASH)+=flash-$(FLASH_FAMILY).o
chip-$(CONFIG_ADC)+=adc-$(CHIP_FAMILY).o
chip-$(CONFIG_PWM)+=pwm.o
chip-$(CONFIG_USB)+=usb.o usb_endpoints.o
chip-$(CONFIG_USB_HID)+=usb_hid.o
chip-$(CONFIG_USB_POWER_DELIVERY)+=usb_pd_phy.o
