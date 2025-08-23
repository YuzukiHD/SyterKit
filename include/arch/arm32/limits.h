#ifndef __LIMITS_H__
#define __LIMITS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define INT8_MIN (-1 - 0x7f)
#define INT16_MIN (-1 - 0x7fff)
#define INT32_MIN (-1 - 0x7fffffff)
#define INT64_MIN (-1 - 0x7fffffffffffffff)

#define INT8_MAX (0x7f)
#define INT16_MAX (0x7fff)
#define INT32_MAX (0x7fffffff)
#define INT64_MAX (0x7fffffffffffffff)

#define INT_MAX (0x7fffffff)

#define FLT_MIN 1.175494351e-38F
#define FLT_MAX 3.402823466e+38F

#define UINT8_MAX (0xff)
#define UINT16_MAX (0xffff)
#define UINT32_MAX (0xffffffffU)
#define UINT64_MAX (0xffffffffffffffffU)

#ifdef __cplusplus
}
#endif

#endif// __LIMITS_H__