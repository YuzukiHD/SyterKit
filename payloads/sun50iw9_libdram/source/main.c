#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#include <rtc.h>

// init_DRAM in libdram.a
extern int init_DRAM(int type, void *buff);

void set_dram_size_rtc(uint32_t dram_size) {
    do {
        write32(SUNXI_RTC_DATA_BASE + RTC_FEL_INDEX * 4, dram_size);
        asm volatile("DSB");
        asm volatile("ISB");
    } while (read32(SUNXI_RTC_DATA_BASE + RTC_FEL_INDEX * 4) != dram_size);
}

// Initialize DRAM using the dram_para structure
void sys_init_dram(void) {
    set_timer_count();
    uint32_t dram_para[32] = {
            0x2a0,     // dram_para[0]
            0x8,       // dram_para[1]
            0xc0c0c0c, // dram_para[2]
            0xe0e0e0e, // dram_para[3]
            0xa0e,     // dram_para[4]
            0x7887ffff,// dram_para[5]
            0x30fa,    // dram_para[6]
            0x4000000, // dram_para[7]
            0x0,       // dram_para[8]
            0x34,      // dram_para[9]
            0x1b,      // dram_para[10]
            0x33,      // dram_para[11]
            0x3,       // dram_para[12]
            0x0,       // dram_para[13]
            0x0,       // dram_para[14]
            0x4,       // dram_para[15]
            0x72,      // dram_para[16]
            0x0,       // dram_para[17]
            0x9,       // dram_para[18]
            0x0,       // dram_para[19]
            0x0,       // dram_para[20]
            0x24,      // dram_para[21]
            0x0,       // dram_para[22]
            0x0,       // dram_para[23]
            0x0,       // dram_para[24]
            0x0,       // dram_para[25]
            0x39808080,// dram_para[26]
            0x402f6603,// dram_para[27]
            0x20262620,// dram_para[28]
            0xe0e0f0f, // dram_para[29]
            0x6061,    // dram_para[30]
            0x0        // dram_para[31]
    };

    uint32_t dram_size = 0;
    dram_size = init_DRAM(0, &dram_para);
    printf("Init DRAM Done, DRAM Size = %dM\n", dram_size);
    mdelay(10);
    set_dram_size_rtc(dram_size);
}

// fake function for link, we aleady set ddr voltage in SyterKit
int set_ddr_voltage(int set_vol) {
    printf("Set DRAM Voltage to %dmv\n", set_vol);
    return 0;
}