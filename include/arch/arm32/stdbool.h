#ifndef __STDBOOL_H__
#define __STDBOOL_H__

#ifdef __cplusplus
/* In C++, bool, true, and false are keywords. */
#else

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
/* In C23, bool, true, and false are keywords. */
#else
#define bool _Bool
#define true 1
#define false 0
#endif

#endif// __cplusplus

#define __bool_true_false_are_defined 1

#endif// __STDBOOL_H__