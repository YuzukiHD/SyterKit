/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __G_SDHCI_H__
#define __G_SDHCI_H__

#if defined(CONFIG_CHIP_SUN8IW21)
    #include <sun8iw21/sys-sdhci.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
    #include <sun8iw20/sys-sdhci.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
    #include <sun50iw9/sys-sdhci.h>
#else
#   error "Unsupported chip"
#endif

#endif /* __G_SDHCI_H__ */
