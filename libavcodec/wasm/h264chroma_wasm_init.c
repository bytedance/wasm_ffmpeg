// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#include "config.h"
#include "libavutil/attributes.h"
#include "libavutil/cpu.h"
#include "libavutil/intreadwrite.h"
#include "libavcodec/h264chroma.h"
#include "libavcodec/h264wasm.h"


#define PUT_OP_WASM(vdst, vsrc) wasm_u8x16_narrow_i16x8(wasm_i16x8_shr(wasm_i16x8_add(vsrc, vc32), 6), vc32)
#define AVG_OP_WASM(vdst, vsrc) wasm_u8x16_narrow_i16x8(wasm_u16x8_avgr(vdst,wasm_i16x8_shr(wasm_i16x8_add(vsrc, vc32), 6)), vc32)


#define OP_U8_WASM                          PUT_OP_WASM
#define PREFIX_h264_chroma_mc8_wasm         put_h264_chroma_mc8_wasm
#define PREFIX_h264_chroma_mc8_num             wasm_put_h264_chroma_mc8_num
#define PREFIX_h264_chroma_mc4_wasm         put_h264_chroma_mc4_wasm
#define PREFIX_h264_chroma_mc4_num             wasm_put_h264_chroma_mc4_num
#include "h264chroma_wasm_template.c"
#undef OP_U8_WASM
#undef PREFIX_h264_chroma_mc8_wasm
#undef PREFIX_h264_chroma_mc8_num
#undef PREFIX_h264_chroma_mc4_wasm
#undef PREFIX_h264_chroma_mc4_num

#define OP_U8_WASM                          AVG_OP_WASM
#define PREFIX_h264_chroma_mc8_wasm         avg_h264_chroma_mc8_wasm
#define PREFIX_h264_chroma_mc8_num             wasm_avg_h264_chroma_mc8_num
#define PREFIX_h264_chroma_mc4_wasm         avg_h264_chroma_mc4_wasm
#define PREFIX_h264_chroma_mc4_num             wasm_avg_h264_chroma_mc4_num
#include "h264chroma_wasm_template.c"
#undef OP_U8_WASM
#undef PREFIX_h264_chroma_mc8_wasm
#undef PREFIX_h264_chroma_mc8_num
#undef PREFIX_h264_chroma_mc4_wasm
#undef PREFIX_h264_chroma_mc4_num

av_cold void ff_h264chroma_init_wasm(H264ChromaContext *c, int bit_depth)
{

    const int high_bit_depth = bit_depth > 8;

    if (!high_bit_depth && vesdk_is_264_simd_enabled()) {
        c->put_h264_chroma_pixels_tab[0] = put_h264_chroma_mc8_wasm;
        c->put_h264_chroma_pixels_tab[1] = put_h264_chroma_mc4_wasm;
        c->avg_h264_chroma_pixels_tab[0] = avg_h264_chroma_mc8_wasm; 
        c->avg_h264_chroma_pixels_tab[1] = avg_h264_chroma_mc4_wasm; 
    }
}
