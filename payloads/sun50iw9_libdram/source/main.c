#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#include <dram.h>

// init_DRAM in libdram.a
extern int init_DRAM(int type, void *buff);

// Initialize DRAM using the dram_para structure
void sys_init_dram(void) {
    dram_para_t dram_para = {
            .dram_clk = 672,
            .dram_type = 7,
            .dram_dx_odt = 0x06060606,
            .dram_dx_dri = 0x0d0d0d0d,
            .dram_ca_dri = 0x1919,
            .dram_odt_en = 1,
            .dram_para1 = 0x30eA,
            .dram_para2 = 0x1000,
            .dram_mr0 = 0x0,
            .dram_mr1 = 0xc3,
            .dram_mr2 = 0x6,
            .dram_mr3 = 0x1,
            .dram_mr4 = 0x0,
            .dram_mr5 = 0x0,
            .dram_mr6 = 0x0,
            .dram_mr11 = 0x0,
            .dram_mr12 = 0x0,
            .dram_mr13 = 0x0,
            .dram_mr14 = 0x0,
            .dram_mr16 = 0x0,
            .dram_mr17 = 0x0,
            .dram_mr22 = 0x0,
            .dram_tpr0 = 0x0,
            .dram_tpr1 = 0x0,
            .dram_tpr2 = 0x0,
            .dram_tpr3 = 0x0,
            .dram_tpr6 = 0x2f958080,
            .dram_tpr10 = 0x00f97779,
            .dram_tpr11 = 0x0,
            .dram_tpr12 = 0x0,
            .dram_tpr13 = 0x60,
    };

    uint32_t dram_size = 0;
    dram_size = init_DRAM(0, &dram_para);
    printf("Init DRAM Done, DRAM Size = %dM\n", dram_size);
}

// fake function for link, we aleady set ddr voltage in SyterKit
int set_ddr_voltage(int set_vol) {
    printf("Set DRAM Voltage to %dmv\n", set_vol);
    return 0;
}