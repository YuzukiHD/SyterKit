/* SPDX-License-Identifier: GPL-2.0+ */
/* The only libm function the CoreMark number formatter needs. */
#ifndef CM_MATH_H
#define CM_MATH_H
double modf(double x, double *iptr);
#endif
