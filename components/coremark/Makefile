# SPDX-License-Identifier: GPL-2.0+

# The EEMBC sources are used as they are; main() is renamed so that an
# application can bring the console up first.
obj-y += core_list_join.o core_main.o core_matrix.o core_state.o core_util.o
obj-y += syterkit/core_portme.o syterkit/ee_printf.o syterkit/cvt.o

CFLAGS_core_main.o := -Dmain=coremark_main

ccflags-y := -I$(srctree)/components/coremark -I$(srctree)/components/coremark/syterkit \
	-O2 -DPERFORMANCE_RUN=1 -DITERATIONS=$(CONFIG_COREMARK_ITERATIONS) \
	-DMULTITHREAD=$(CONFIG_COREMARK_THREADS) \
	-Wno-unused-parameter
