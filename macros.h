/**
 * @file
 * Useful macros you shouldn't have to rewrite evey time you need them.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_MACROS_H_
#define NUVC_MACROS_H_

#include <math.h>
#include <stdint.h>

/** Number of elements present in array. */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof(array[0])))
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FOURCC(c0, c1, c2, c3)                                                                                         \
    (((uint32_t)(c0)) | ((uint32_t)(c1) << 8) | ((uint32_t)(c2) << 16) | ((uint32_t)(c3) << 24))
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FOURCC(c0, c1, c2, c3)                                                                                         \
    (((uint32_t)(c0) << 24) | ((uint32_t)(c1) << 16) | ((uint32_t)(c2) << 8) | ((uint32_t)(c3)))
#else
#error Unsupported endianness.
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define CLAMP(x, min, max)                                                                                             \
    ({                                                                                                                 \
        const typeof(x) _x_temp = (x);                                                                                 \
        const typeof(min) _min_temp = (min);                                                                           \
        const typeof(max) _max_temp = (max);                                                                           \
        (_x_temp > _max_temp ? _max_temp : (_x_temp < _min_temp ? _min_temp : _x_temp));                               \
    })

typedef enum valueClampStatus {
    VALUE_CLAMP_NONE = 0,
    VALUE_CLAMP_LOW,
    VALUE_CLAMP_HIGH,
} valueClampStatus_t;
#define CLAMP_W_STATUS(x, min, max, status)                                                                            \
    ({                                                                                                                 \
        const typeof(x) _x_temp = (x);                                                                                 \
        const typeof(min) _min_temp = (min);                                                                           \
        const typeof(max) _max_temp = (max);                                                                           \
        status = _x_temp > _max_temp ? VALUE_CLAMP_HIGH : (_x_temp < _min_temp ? VALUE_CLAMP_LOW : VALUE_CLAMP_NONE);  \
        (_x_temp > _max_temp ? _max_temp : (_x_temp < _min_temp ? _min_temp : _x_temp));                               \
    })

#define DB_POW_TO_LIN(_x_) (pow(10.0, (_x_) / 10.0))
#define LIN_TO_DB_POW(_x_) (10.0 * log10((_x_)))

#define EVALUATE_TO_STRING(mac) _TO_STRING(mac)
#define _TO_STRING(str) #str

/* If a function's stack still fails with this macro, it should likely be refactored */
#define BEGIN_NEEDS_LARGE_STACK _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic error \"-Wstack-usage=4096\"")
#define END_NEEDS_LARGE_STACK _Pragma("GCC diagnostic pop")

#endif /* NUVC_MACROS_H_ */
