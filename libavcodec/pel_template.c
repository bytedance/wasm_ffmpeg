/*
 * Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
 * This file is part of FFmpeg.
 * This file has been modified by [ByteDance Ltd. and/or its affiliates.]
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stddef.h>
#include <stdint.h>

#include "libavutil/intreadwrite.h"
#include "pixels.h"
#include "rnd_avg.h"

#include "bit_depth_template.c"

#define DEF_PEL(OPNAME, OP)                                             \
static inline void FUNCC(OPNAME ## _pixels2)(uint8_t *block,            \
                                             const uint8_t *pixels,     \
                                             ptrdiff_t line_size,       \
                                             int h)                     \
{                                                                       \
    int i;                                                              \
    for (i = 0; i < h; i++) {                                           \
        OP(*((pixel2 *) block), AV_RN2P(pixels));                       \
        pixels += line_size;                                            \
        block  += line_size;                                            \
    }                                                                   \
}                                                                       \
                                                                        \
static inline void FUNCC(OPNAME ## _pixels4)(uint8_t *block,            \
                                             const uint8_t *pixels,     \
                                             ptrdiff_t line_size,       \
                                             int h)                     \
{                                                                       \
    int i;                                                              \
    for (i = 0; i < h; i++) {                                           \
        OP(*((pixel4 *) block), AV_RN4P(pixels));                       \
        pixels += line_size;                                            \
        block  += line_size;                                            \
    }                                                                   \
}                                                                       \
                                                                        \
static inline void FUNCC(OPNAME ## _pixels8)(uint8_t *block,            \
                                             const uint8_t *pixels,     \
                                             ptrdiff_t line_size,       \
                                             int h)                     \
{                                                                       \
    int i;                                                              \
    for (i = 0; i < h; i++) {                                           \
        OP(*((pixel4 *) block), AV_RN4P(pixels));                       \
        OP(*((pixel4 *) (block + 4 * sizeof(pixel))),                   \
           AV_RN4P(pixels + 4 * sizeof(pixel)));                        \
        pixels += line_size;                                            \
        block  += line_size;                                            \
    }                                                                   \
}                                                                       \
                                                                        \
CALL_2X_PIXELS(FUNCC(OPNAME ## _pixels16),                              \
               FUNCC(OPNAME ## _pixels8),                               \
               8 * sizeof(pixel))

#define op_avg(a, b) a = rnd_avg_pixel4(a, b)
#define op_put(a, b) a = b

DEF_PEL(avg, op_avg)
DEF_PEL(put, op_put)
#undef op_avg
#undef op_put

// static inline void avg_pixels2_8_c(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
// {
//     int i;
//     for (i = 0; i < h; i++)
//     {
//         op_avg(*((uint16_t *)block), (((const union unaligned_16 *)(pixels))->l));
//         pixels += line_size;
//         block += line_size;
//     }
// }
// static inline void avg_pixels4_8_c(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
// {
//     int i;
//     for (i = 0; i < h; i++)
//     {
//         op_avg(*((uint32_t *)block), (((const union unaligned_32 *)(pixels))->l));
//         pixels += line_size;
//         block += line_size;
//     }
// }
// static inline void avg_pixels8_8_c(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
// {
//     int i;
//     for (i = 0; i < h; i++)
//     {
//         op_avg(*((uint32_t *)block), (((const union unaligned_32 *)(pixels))->l));
//         op_avg(*((uint32_t *)(block + 4 * sizeof(uint8_t))), (((const union unaligned_32 *)(pixels + 4 * sizeof(uint8_t)))->l));
//         pixels += line_size;
//         block += line_size;
//     }
// }
// static void avg_pixels16_8_c(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
// {
//     avg_pixels8_8_c(block, pixels, line_size, h);
//     avg_pixels8_8_c(block + 8 * sizeof(uint8_t), pixels + 8 * sizeof(uint8_t), line_size, h);
// }