#ifndef ENGINE_MATH_FIXED_H
#define ENGINE_MATH_FIXED_H

#include <stdint.h>

/* 16.16 fixed-point arithmetic */
typedef int32_t fixed_t;

#define FIXED_SHIFT 16
#define FIXED_ONE (1 << FIXED_SHIFT)

#define INT_TO_FIXED(x) ((fixed_t)((x) << FIXED_SHIFT))
#define FIXED_TO_INT(x) ((int)((x) >> FIXED_SHIFT))
#define FLOAT_TO_FIXED(x) ((fixed_t)((x) * FIXED_ONE))
#define FIXED_MUL(a, b) ((fixed_t)(((int64_t)(a) * (b)) >> FIXED_SHIFT))
#define FIXED_DIV(a, b) ((fixed_t)(((int64_t)(a) << FIXED_SHIFT) / (b)))

#endif /* ENGINE_MATH_FIXED_H */
