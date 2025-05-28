// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later


#ifndef AVCODEC_PPC_HPELDSP_WASM_H
#define AVCODEC_PPC_HPELDSP_WASM_H

#include <stddef.h>
#include <stdint.h>

void ff_avg_pixels16_wasm(uint8_t *block, const uint8_t *pixels,
                             ptrdiff_t line_size, int h);
void ff_put_pixels16_wasm(uint8_t *block, const uint8_t *pixels,
                             ptrdiff_t line_size, int h);
void ff_avg_pixels8_wasm(uint8_t * block, const uint8_t * pixels, ptrdiff_t line_size, int h);
void ff_put_pixels8_wasm(uint8_t * block, const uint8_t * pixels, ptrdiff_t line_size, int h);

#endif /* AVCODEC_PPC_HPELDSP_WASM_H */
