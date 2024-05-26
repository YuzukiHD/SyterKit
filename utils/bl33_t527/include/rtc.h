#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SUNXI_RTC_BASE (0x07090000)
#define SUNXI_RTC_DATA_BASE (SUNXI_RTC_BASE + 0x100)

#define RTC_FEL_INDEX 2
#define DRAM_PARA_CLK 3
#define DRAM_PARA_TPR13 4

void set_timer_count();

#ifdef __cplusplus
}
#endif

#endif /* __RTC_H__ */
