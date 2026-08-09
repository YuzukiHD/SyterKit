#ifndef __G_REG_RPROC_H__
#define __G_REG_RPROC_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_SOC_SUN8IW21)
#include <drivers/soc/sun8iw21/reg-rproc.h>
#elif defined(CONFIG_SOC_SUN8IW20)
#include <drivers/soc/sun8iw20/reg-rproc.h>
#elif defined(CONFIG_SOC_SUN20IW1)
#include <drivers/soc/sun20iw1/reg-rproc.h>
#elif defined(CONFIG_SOC_SUN55IW3)
#include <drivers/soc/sun55iw3/reg-rproc.h>
#elif defined(CONFIG_SOC_SUN300IW1)
#include <drivers/soc/sun300iw1/reg-rproc.h>
#elif defined(CONFIG_SOC_SUN55IW6)
#include <drivers/soc/sun55iw6/reg-rproc.h>
#elif defined(CONFIG_SOC_SUN65IW1)
#include <drivers/soc/sun65iw1/reg-rproc.h>
#endif

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __G_REG_RPROC_H__