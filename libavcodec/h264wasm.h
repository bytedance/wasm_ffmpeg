// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#ifndef AVCODEC_WASM_H264WASM_H
#define AVCODEC_WASM_H264WASM_H

#include <emscripten.h>
#include "libavcodec/cabac.h"


EMSCRIPTEN_KEEPALIVE void vesdk_set_264_simd(int enable);
EMSCRIPTEN_KEEPALIVE int vesdk_is_264_simd_enabled(void);
EMSCRIPTEN_KEEPALIVE int test_get_cabac_inline_wasm(CABACContext *c, uint8_t * const state);

#endif // AVCODEC_WASM_H264WASM_H
