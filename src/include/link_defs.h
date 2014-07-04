/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Symbols from linker definitions
 */

#ifndef __CROS_EC_LINK_DEFS_H
#define __CROS_EC_LINK_DEFS_H

#include "console.h"
#include "hooks.h"
#include "host_command.h"
#include "task.h"
#include "test_util.h"

/* Console commands */
extern const struct console_command __cmds[];
extern const struct console_command __cmds_end[];

/* Hooks */
extern const struct hook_data __hooks_init[];
extern const struct hook_data __hooks_init_end[];
extern const struct hook_data __hooks_pre_freq_change[];
extern const struct hook_data __hooks_pre_freq_change_end[];
extern const struct hook_data __hooks_freq_change[];
extern const struct hook_data __hooks_freq_change_end[];
extern const struct hook_data __hooks_sysjump[];
extern const struct hook_data __hooks_sysjump_end[];
extern const struct hook_data __hooks_chipset_pre_init[];
extern const struct hook_data __hooks_chipset_pre_init_end[];
extern const struct hook_data __hooks_chipset_startup[];
extern const struct hook_data __hooks_chipset_startup_end[];
extern const struct hook_data __hooks_chipset_resume[];
extern const struct hook_data __hooks_chipset_resume_end[];
extern const struct hook_data __hooks_chipset_suspend[];
extern const struct hook_data __hooks_chipset_suspend_end[];
extern const struct hook_data __hooks_chipset_shutdown[];
extern const struct hook_data __hooks_chipset_shutdown_end[];
extern const struct hook_data __hooks_ac_change[];
extern const struct hook_data __hooks_ac_change_end[];
extern const struct hook_data __hooks_lid_change[];
extern const struct hook_data __hooks_lid_change_end[];
extern const struct hook_data __hooks_pwrbtn_change[];
extern const struct hook_data __hooks_pwrbtn_change_end[];
extern const struct hook_data __hooks_charge_state_change[];
extern const struct hook_data __hooks_charge_state_change_end[];
extern const struct hook_data __hooks_tick[];
extern const struct hook_data __hooks_tick_end[];
extern const struct hook_data __hooks_second[];
extern const struct hook_data __hooks_second_end[];

/* Deferrable functions */
extern const struct deferred_data __deferred_funcs[];
extern const struct deferred_data __deferred_funcs_end[];

/* USB data */
extern const uint8_t __usb_desc[];
extern const uint8_t __usb_desc_end[];
extern const uint16_t __usb_ram_start[];

/* I2C fake devices for unit testing */
extern const struct test_i2c_read_dev __test_i2c_read8[];
extern const struct test_i2c_read_dev __test_i2c_read8_end[];
extern const struct test_i2c_write_dev __test_i2c_write8[];
extern const struct test_i2c_write_dev __test_i2c_write8_end[];
extern const struct test_i2c_read_dev __test_i2c_read16[];
extern const struct test_i2c_read_dev __test_i2c_read16_end[];
extern const struct test_i2c_write_dev __test_i2c_write16[];
extern const struct test_i2c_write_dev __test_i2c_write16_end[];
extern const struct test_i2c_read_string_dev __test_i2c_read_string[];
extern const struct test_i2c_read_string_dev __test_i2c_read_string_end[];

/* Host commands */
extern const struct host_command __hcmds[];
extern const struct host_command __hcmds_end[];

/* IRQs (interrupt handlers) */
extern const struct irq_priority __irqprio[];
extern const struct irq_priority __irqprio_end[];
extern const void *__irqhandler[];

/* Shared memory buffer.  Use via shared_mem.h interface. */
extern uint8_t __shared_mem_buf[];

#endif /* __CROS_EC_LINK_DEFS_H */
