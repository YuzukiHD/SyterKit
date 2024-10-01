#ifndef __G_REG_RPROC_H__
#define __G_REG_RPROC_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/reg-rproc.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
#include <sun8iw20/reg-rproc.h>
#elif defined(CONFIG_CHIP_SUN20IW1)
#include <sun20iw1/reg-rproc.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
#include <sun55iw3/reg-rproc.h>
#elif defined(CONFIG_CHIP_SUN20IW5)
#include <sun20iw5/reg-rproc.h>
#endif

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __G_REG_RPROC_H__