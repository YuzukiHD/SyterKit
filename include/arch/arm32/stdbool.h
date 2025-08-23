#ifndef __STDBOOL_H__
#define __STDBOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum {
	false = 0,
	true = 1,
};

typedef int8_t bool;

#ifdef __cplusplus
}
#endif

#endif// __STDBOOL_H__