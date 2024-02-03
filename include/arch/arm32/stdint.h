#ifndef __STDINT_H__
#define __STDINT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <types.h>

typedef signed char s8_t;
typedef unsigned char u8_t;

typedef signed short s16_t;
typedef unsigned short u16_t;

typedef signed int s32_t;
typedef unsigned int u32_t;

typedef signed long long s64_t;
typedef unsigned long long u64_t;

typedef signed long long intmax_t;
typedef unsigned long long uintmax_t;

typedef signed long long ptrdiff_t;
typedef signed long long intptr_t;
typedef unsigned long int uintptr_t;

typedef s8_t int8_t;
typedef u8_t uint8_t;

typedef s16_t int16_t;
typedef u16_t uint16_t;

typedef s32_t int32_t;
typedef u32_t uint32_t;

typedef s64_t int64_t;
typedef u64_t uint64_t;

#ifdef __cplusplus
}
#endif

#endif /* __STDINT_H__ */