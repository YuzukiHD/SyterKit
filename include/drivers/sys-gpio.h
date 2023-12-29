/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_GPIO_H__
#define __SYS_GPIO_H__

#if defined(CONFIG_CHIP_GPIO_V1)
    #include <gpio/sys-gpio-v1.h>
#else
    #include <gpio/sys-gpio-v2.h>
#endif

#endif