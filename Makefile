# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Embedded Controller firmware build system
#

BOARD ?= bds
VENDOR ?= unknown

PROJECT?=ec

# Output directory for build objects
out?=build/$(VENDOR)/$(BOARD)
obj?=build

export top := $(CURDIR)
export src := src
export srck := $(top)/util/kconfig
export objutil ?= $(obj)/util
export objk := $(objutil)/kconfig

# Kconfig things #
export KCONFIG_AUTOHEADER := $(obj)/config.h
export KCONFIG_AUTOCONFIG := $(obj)/auto.conf
export KCONFIG_DEPENDENCIES := $(obj)/auto.conf.cmd
export KCONFIG_SPLITCONFIG := $(obj)/config
export KCONFIG_TRISTATE := $(obj)/tristate.conf
export KCONFIG_NEGATIVES := 1

CONFIG_SHELL := sh
KBUILD_DEFCONFIG := configs/defconfig
UNAME_RELEASE := $(shell uname -r)
DOTCONFIG ?= .config
export KCONFIG_CONFIG = $(DOTCONFIG)
HAVE_DOTCONFIG := $(wildcard $(DOTCONFIG))
MAKEFLAGS += -rR --no-print-directory

# Make is silent per default, but 'make V=1' will show all compiler calls.
Q:=@
ifneq ($(V),1)
ifneq ($(Q),)
.SILENT:
endif
endif

HOSTCC = gcc
HOSTCXX = g++
HOSTCFLAGS := -g
HOSTCXXFLAGS := -g

all: real-all

# This include must come _before_ the pattern rules below!
# Order _does_ matter for pattern rules.
include util/kconfig/Makefile

# Three cases where we don't need fully populated $(obj) lists:
# 1. when no .config exists
# 2. when make config (in any flavour) is run
# 3. when make distclean is run
# Don't waste time on reading all build.mk's in these cases
ifeq ($(strip $(HAVE_DOTCONFIG)),)
NOCOMPILE:=1
endif
ifneq ($(MAKECMDGOALS),)
ifneq ($(filter %config %clean cross%,$(MAKECMDGOALS)),)
NOCOMPILE:=1
endif
ifeq ($(MAKECMDGOALS), %clean)
NOMKDIR:=1
endif
endif

ifeq ($(NOCOMPILE),1)
real-all: menuconfig

else # NOCOMPILE == 0

include $(HAVE_DOTCONFIG)

# Include toolchain specifics
ifeq ($(CONFIG_COMPILER_LLVM_CLANG),y)
# TODO - write one for Clang also
include Makefile.toolchain.clang
endif
ifeq ($(CONFIG_COMPILER_GCC),y)
include Makefile.toolchain.gcc
endif
#
ifeq ($(CONFIG_COMPILER_LLVM_CLANG),y)
HOSTCC = clang
HOSTCXX = clang++
endif

# The board makefile sets $CHIP and the chip makefile sets $CORE.
# Include those now, since they must be defined for _flag_cfg below.
include src/board/$(VENDOR)/$(BOARD)/build.mk
include src/chip/$(CHIP)/build.mk

# Create uppercase config variants, to avoid mixed case constants.
# Also translate '-' to '_', so 'cortex-m' turns into 'CORTEX_M'.  This must
# be done before evaluating config.h.
uppercase = $(shell echo $(1) | tr '[:lower:]-' '[:upper:]_')
UC_BOARD:=$(call uppercase,$(BOARD))
UC_CHIP:=$(call uppercase,$(CHIP))
UC_CHIP_FAMILY:=$(call uppercase,$(CHIP_FAMILY))
UC_CHIP_VARIANT:=$(call uppercase,$(CHIP_VARIANT))
UC_CORE:=$(call uppercase,$(CORE))
UC_PROJECT:=$(call uppercase,$(PROJECT))

# Transform the configuration into make variables.  This must be done after
# the board/project/chip/core variables are defined, since some of the configs
# are dependent on particular configurations.
includes=src/include src/core/$(CORE)/include src $(dirs) $(out) test
ifeq ($(TEST_BUILD),y)
	_tsk_lst:=$(shell echo "CONFIG_TASK_LIST CONFIG_TEST_TASK_LIST" | \
		    $(CPP) -P -Isrc/board/$(VENDOR)/$(BOARD) -Itest \
		    -D"TASK_NOTEST(n, r, d, s)=" -D"TASK_ALWAYS(n, r, d, s)=n" \
		    -D"TASK_TEST(n, r, d, s)=n" -imacros ec.tasklist \
		    -imacros $(PROJECT).tasklist)
else
	_tsk_lst:=$(shell echo "CONFIG_TASK_LIST" | $(CPP) -P \
		    -Isrc/board/$(VENDOR)/$(BOARD) -D"TASK_NOTEST(n, r, d, s)=n" \
		    -D"TASK_ALWAYS(n, r, d, s)=n" -imacros ec.tasklist)
endif
_tsk_cfg:=$(foreach t,$(_tsk_lst) ,HAS_TASK_$(t))
CPPFLAGS+=$(foreach t,$(_tsk_cfg),-D$(t))
_flag_cfg:=$(shell $(CPP) $(CPPFLAGS) -P -dM -Isrc/chip/$(CHIP) -Isrc/board/$(VENDOR)/$(BOARD) \
	src/include/config.h | grep -o "\#define CONFIG_[A-Z0-9_]*" | \
	cut -c9- | sort)
#_flag_cfg:=$(shell cat build/config.h | grep -o "\#define CONFIG_[A-Z0-9_]*" | \
#	cut -c9- | sort)

$(foreach c,$(_tsk_cfg) $(_flag_cfg),$(eval $(c)=y))

ifneq ($(CONFIG_COMMON_RUNTIME),y)
	_irq_list:=$(shell $(CPP) $(CPPFLAGS) -P -Isrc/chip/$(CHIP) -Isrc/board/$(VENDOR)/$(BOARD) \
		-D"ENABLE_IRQ(x)=EN_IRQ x" -imacros src/chip/$(CHIP)/registers.h \
		src/board/$(VENDOR)/$(BOARD)/ec.irqlist | grep "EN_IRQ .*" | cut -c8-)
	CPPFLAGS+=$(foreach irq,$(_irq_list),\
		    -D"irq_$(irq)_handler_optional=irq_$(irq)_handler")
endif

# Get build configuration from sub-directories
# Note that this re-includes the board and chip makefiles
include src/board/$(VENDOR)/$(BOARD)/build.mk
include src/chip/$(CHIP)/build.mk
include src/core/$(CORE)/build.mk

$(eval BOARD_$(UC_BOARD)=y)

include src/common/build.mk
include src/driver/build.mk
include src/power/build.mk
-include private/build.mk
include test/build.mk
include util/build.mk
include util/lock/build.mk

includes+=$(includes-y)

objs_from_dir=$(sort $(foreach obj, $($(2)-y), \
	        $(out)/$(1)/$(firstword $($(2)-mock-$(PROJECT)-$(obj)) $(obj))))

# Get all sources to build
all-y=$(call objs_from_dir,src/core/$(CORE),core)
all-y+=$(call objs_from_dir,src/chip/$(CHIP),chip)
all-y+=$(call objs_from_dir,src/board/$(VENDOR)/$(BOARD),board)
all-y+=$(call objs_from_dir,private,private)
all-y+=$(call objs_from_dir,src/common,common)
all-y+=$(call objs_from_dir,src/driver,driver)
all-y+=$(call objs_from_dir,src/power,power)
all-y+=$(call objs_from_dir,test,$(PROJECT))
dirs=src/core/$(CORE) src/chip/$(CHIP) src/board/$(VENDOR)/$(BOARD) private src/common src/power test util
dirs+=$(shell find src/driver -type d)

# The primary target needs to be here before we include the
# other files

real-all: real-target
.PHONY : real-target

endif # ifeq NOCOMPILE,1

include Makefile.rules
