# SPDX-License-Identifier: GPL-2.0+

# tests.c is used as it is; the libc names it needs come from shim/.
obj-y += tests.o syterkit/memtester_port.o

ccflags-y := -I$(srctree)/components/memtester -I$(srctree)/components/memtester/shim -O2 \
	-include $(srctree)/components/memtester/shim/memtester_limits.h -Wno-unused-parameter
