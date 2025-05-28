// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#include "h264wasm.h"
#include <emscripten.h>

static int vesdk_simd_enabled = 0;

EMSCRIPTEN_KEEPALIVE void vesdk_set_264_simd(int enable) {
    vesdk_simd_enabled = enable;
    emscripten_log(EM_LOG_CONSOLE, "vesdk_set_264_simd %d ", enable);
}

EMSCRIPTEN_KEEPALIVE int vesdk_is_264_simd_enabled() {
    return vesdk_simd_enabled;
}

// extern int get_cabac_inline_wasm(CABACContext *c, uint8_t * const state);

EMSCRIPTEN_KEEPALIVE int test_get_cabac_inline_wasm(CABACContext *c, uint8_t * const state) {
    c = (CABACContext*) malloc(sizeof(CABACContext));
    c->low = 66621952;
    c->range=510;
    // return get_cabac_inline_wasm(c, 100);
    return 0;
}

// static av_always_inline int get_cabac_inline(CABACContext *c, uint8_t * const state){
//     int s = *state;
//     int RangeLPS= ff_h264_lps_range[2*(c->range&0xC0) + s];
//     int bit, lps_mask;
//     int rangeTmp = c->range - RangeLPS;
//     lps_mask= ((rangeTmp<<(CABAC_BITS+1)) - c->low)>>31;

//     int lowTmp = c->low - ((rangeTmp<<(CABAC_BITS+1)) & lps_mask);
//     rangeTmp += (RangeLPS - rangeTmp) & lps_mask;

//     s^=lps_mask;
//     *state= (ff_h264_mlps_state+128)[s];
//     bit= s&1;

//     lps_mask= ff_h264_norm_shift[rangeTmp];
//     c->range = rangeTmp << lps_mask;
//     c->low  = lowTmp << lps_mask;
//     if(!(c->low & CABAC_MASK))
//         refill2(c);
//     return bit;
// }
