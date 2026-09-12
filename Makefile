# SPDX-License-Identifier: GPL-2.0+

VERSION := 0
PATCHLEVEL := 5
SUBLEVEL := 0

.DEFAULT_GOAL := all
MAKEFLAGS += --no-print-directory --silent

srctree := $(CURDIR)
ifneq ($(O),)
objtree := $(abspath $(O))
else
objtree := $(srctree)
endif

# Keep this order explicit: configuration establishes CONFIG_*, target derives
# the platform context, and the remaining modules add build rules for it.
include $(srctree)/scripts/Makefile.config
include $(srctree)/scripts/Makefile.target
include $(srctree)/scripts/Makefile.devicetree
include $(srctree)/scripts/Makefile.rust
include $(srctree)/scripts/Makefile.prepare
include $(srctree)/scripts/Makefile.images
include $(srctree)/scripts/Makefile.host
include $(srctree)/scripts/Makefile.clean
include $(srctree)/scripts/Makefile.check
