/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __G_SDHCI_H__
#define __G_SDHCI_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
    #include <sun8iw21/sys-sdhci.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
    #include <sun8iw20/sys-sdhci.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
    #include <sun50iw9/sys-sdhci.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
    #include <sun55iw3/sys-sdhci.h>
#elif defined(CONFIG_CHIP_SUN8IW8)
    #include <sun8iw8/sys-sdhci.h>
#else
#   error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* __G_SDHCI_H__ */
