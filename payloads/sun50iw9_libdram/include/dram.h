#ifndef __DRAM_H__
#define __DRAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <types.h>

typedef struct {
    uint32_t dram_clk;
    uint32_t dram_type;
    uint32_t dram_dx_odt;
    uint32_t dram_dx_dri;
    uint32_t dram_ca_dri;
    uint32_t dram_para0;
    uint32_t dram_para1;
    uint32_t dram_para2;
    uint32_t dram_mr0;
    uint32_t dram_mr1;
    uint32_t dram_mr2;
    uint32_t dram_mr3;
    uint32_t dram_mr4;
    uint32_t dram_mr5;
    uint32_t dram_mr6;
    uint32_t dram_mr11;
    uint32_t dram_mr12;
    uint32_t dram_mr13;
    uint32_t dram_mr14;
    uint32_t dram_mr16;
    uint32_t dram_mr17;
    uint32_t dram_mr22;
    uint32_t dram_tpr0;
    uint32_t dram_tpr1;
    uint32_t dram_tpr2;
    uint32_t dram_tpr3;
    uint32_t dram_tpr6;
    uint32_t dram_tpr10;
    uint32_t dram_tpr11;
    uint32_t dram_tpr12;
    uint32_t dram_tpr13;
    uint32_t dram_tpr14;
} dram_para_t;

#ifdef __cplusplus
}
#endif

#endif /* __DRAM_H__ */
