// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later


#include <stdint.h>

#include "libavutil/attributes.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/h264pred.h"
#include "libavcodec/h264wasm.h"
#include <wasm_simd128.h>
#include "libavcodec/bit_depth_template.c"
#include <emscripten.h>

static void ff_pred16x16_vert_wasm(uint8_t *src, ptrdiff_t stride);
#define LOAD8_SPLAT_STORE(i)  \
    a = wasm_v128_load8_splat(src -1 + i*stride);\
    wasm_v128_store(src+i*stride, a)


static void ff_pred16x16_hor_wasm(uint8_t *src, ptrdiff_t stride) {
    int i;
    v128_t a;
    LOAD8_SPLAT_STORE(0);
    LOAD8_SPLAT_STORE(1);
    LOAD8_SPLAT_STORE(2);
    LOAD8_SPLAT_STORE(3);
    LOAD8_SPLAT_STORE(4);
    LOAD8_SPLAT_STORE(5);
    LOAD8_SPLAT_STORE(6);
    LOAD8_SPLAT_STORE(7);
    LOAD8_SPLAT_STORE(8);
    LOAD8_SPLAT_STORE(9);
    LOAD8_SPLAT_STORE(10);
    LOAD8_SPLAT_STORE(11);
    LOAD8_SPLAT_STORE(12);
    LOAD8_SPLAT_STORE(13);
    LOAD8_SPLAT_STORE(14);
    LOAD8_SPLAT_STORE(15); 
}

static void ff_pred16x16_plane_wasm(uint8_t *src, ptrdiff_t stride)
{
    int i, j, k;
    int a;
    const pixel *const src0 = src + 7 - stride;
    const pixel *src1 = src + 8 * stride - 1;
    const pixel *src2 = src1 - 2 * stride; // == src+6*stride-1;
    int H = src0[1] - src0[-1];
    int V = src1[0] - src2[0];
    for (k = 2; k <= 8; ++k)
    {
        src1 += stride;
        src2 -= stride;
        H += k * (src0[k] - src0[-k]);
        V += k * (src1[0] - src2[0]);
    }
    {
        H = (5 * H + 32) >> 6;
        V = (5 * V + 32) >> 6;
    }

    a = 16 * (src1[0] + src2[16] + 1) - 7 * (V + H);
    v128_t vHFactorLow = wasm_i16x8_make(0, 1, 2, 3, 4, 5, 6, 7);
    v128_t vHFactorHigh = wasm_i16x8_make(8, 9, 10, 11, 12, 13, 14, 15);
    v128_t vH = wasm_i16x8_splat((int16_t)H);
    for (j = 16; j > 0; --j)
    {
        int b = a;
        a += V;
        // for (i = 0; i < 16; i += 4)
        // {
        //     src[i + 0] = CLIP((b + 0 * H) >> 5);
        //     src[i + 1] = CLIP((b + 1 * H) >> 5);
        //     src[i + 2] = CLIP((b + 2 * H) >> 5);
        //     src[i + 3] = CLIP((b + 3 * H) >> 5);
        //     b += 4 * H;
        // }
        // [ (a + 0 ~ 15 * H) >> 5 ] 
        // [ (a + V + 0 ~ 15 * H) >> 5 ]
        // [ (a + 2V + 0 ~ 15 * H) >> 5 ]
        // [ (a + 3V + 0 ~ 15 * H) >> 5 ]
        // ...
        // [ (a + 15V + 0 ~ 15 * H) >> 5 ]
        v128_t vb = wasm_i16x8_splat((int16_t)b);
        // low half
        v128_t vbLow = vb;
        vbLow = wasm_i16x8_add(vbLow, wasm_i16x8_mul(vH, vHFactorLow));
        vbLow = wasm_i16x8_shr(vbLow, 5);
        // high half
        v128_t vbHigh = vb;
        vbHigh = wasm_i16x8_add(vbHigh, wasm_i16x8_mul(vH, vHFactorHigh));
        vbHigh = wasm_i16x8_shr(vbHigh, 5);

        v128_t ret = wasm_u8x16_narrow_i16x8(vbLow, vbHigh);
        wasm_v128_store(src, ret);
        
        src += stride;
    }
}

#define DC_v(i, v) wasm_v128_store(src, v);  src += stride
#define DC_16x16(v) \
    DC_v(0, v); \
    DC_v(1, v); \
    DC_v(2, v); \
    DC_v(3, v); \
    DC_v(4, v); \
    DC_v(5, v); \
    DC_v(6, v); \
    DC_v(7, v); \
    DC_v(8, v); \
    DC_v(9, v); \
    DC_v(10, v); \
    DC_v(11, v); \
    DC_v(12, v); \
    DC_v(13, v); \
    DC_v(14, v); \
    DC_v(15, v); 

static void ff_pred16x16_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i, dc=0;
    pixel4 dcsplat;

    for(i=0;i<16; i++){
        dc+= src[-1+i*stride];
    }

    // for(i=0;i<16; i++){
    //     dc+= src[-stride + i];
    // }

    // 以下是将src - stride一行16个像素相加
    v128_t top = wasm_v128_load(src - stride);
    v128_t topLow = wasm_u16x8_extend_low_u8x16(top);
    v128_t topHigh = wasm_u16x8_extend_high_u8x16(top);
    v128_t topLowPlusHigh = wasm_i16x8_add(topLow, topHigh);
    topLow = wasm_i32x4_extend_low_i16x8(topLowPlusHigh);
    topHigh = wasm_i32x4_extend_high_i16x8(topLowPlusHigh);
    topLowPlusHigh = wasm_i32x4_add(topLow, topHigh);
    v128_t _1 = wasm_i32x4_shuffle(topLowPlusHigh, topLowPlusHigh, 1, 1, 1, 1);
    v128_t _2 = wasm_i32x4_shuffle(topLowPlusHigh, topLowPlusHigh, 2, 2, 2, 2);
    v128_t _3 = wasm_i32x4_shuffle(topLowPlusHigh, topLowPlusHigh, 3, 3, 3, 3);
    topLowPlusHigh = wasm_i32x4_add(topLowPlusHigh, _1);
    topLowPlusHigh = wasm_i32x4_add(topLowPlusHigh, _2);
    topLowPlusHigh = wasm_i32x4_add(topLowPlusHigh, _3);
    dc += wasm_i32x4_extract_lane(topLowPlusHigh, 0);

    uint8_t avg = (uint8_t)((dc+16)>>5);
    v128_t avgr = wasm_u8x16_splat(avg);
    DC_16x16(avgr);
}

static void ff_pred16x16_128_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i;
    v128_t _128 = wasm_i8x16_const_splat(128);
    DC_16x16(_128);
}

static void ff_pred16x16_left_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i, dc=0;
    pixel4 dcsplat;

    for(i=0;i<16; i++){
        dc+= src[-1+i*stride];
    }
    uint8_t avg = (uint8_t)((dc+8)>>4);
    v128_t avgr = wasm_u8x16_splat(avg);
    DC_16x16(avgr);
}

static void ff_pred16x16_top_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i, dc=0;
    pixel4 dcsplat;

    // for(i=0;i<16; i++){
    //     dc+= src[-stride + i];
    // }

    // 以下是将src - stride一行16个像素相加
    v128_t top = wasm_v128_load(src - stride);
    v128_t topLow = wasm_u16x8_extend_low_u8x16(top);
    v128_t topHigh = wasm_u16x8_extend_high_u8x16(top);
    v128_t topLowPlusHigh = wasm_i16x8_add(topLow, topHigh);
    topLow = wasm_i32x4_extend_low_i16x8(topLowPlusHigh);
    topHigh = wasm_i32x4_extend_high_i16x8(topLowPlusHigh);
    topLowPlusHigh = wasm_i32x4_add(topLow, topHigh);
    v128_t _1 = wasm_i32x4_shuffle(topLowPlusHigh, topLowPlusHigh, 1, 1, 1, 1);
    v128_t _2 = wasm_i32x4_shuffle(topLowPlusHigh, topLowPlusHigh, 2, 2, 2, 2);
    v128_t _3 = wasm_i32x4_shuffle(topLowPlusHigh, topLowPlusHigh, 3, 3, 3, 3);
    topLowPlusHigh = wasm_i32x4_add(topLowPlusHigh, _1);
    topLowPlusHigh = wasm_i32x4_add(topLowPlusHigh, _2);
    topLowPlusHigh = wasm_i32x4_add(topLowPlusHigh, _3);
    dc += wasm_i32x4_extract_lane(topLowPlusHigh, 0);

    uint8_t avg = (uint8_t)((dc+8)>>4);
    v128_t avgr = wasm_u8x16_splat(avg);
    DC_16x16(avgr);
}

static void ff_pred8x8_vert_wasm(uint8_t *_src, ptrdiff_t _stride) {
    int i;
    pixel *src = (pixel*)_src;
    int stride = _stride>>(sizeof(pixel)-1);

    int64_t ab = *(int64_t*)(src - stride);

    for(i=0; i<8; i++){
        *((int64_t*)(src+i*stride)) = ab;
    }
}

static void ff_pred8x8_hor_wasm(uint8_t *src, ptrdiff_t stride) {
    int i;

    for(i=0; i<8; i++){
        const uint64_t a = src[-1+i*stride] * 0x0101010101010101ULL;
        *((uint64_t*)(src+i*stride)) = a;
    }
}

static void ff_pred8x8_plane_wasm(uint8_t *src, ptrdiff_t stride)
{
    int j, k;
    int a;
    INIT_CLIP
    const pixel *const src0 = src + 3 - stride;
    const pixel *src1 = src + 4 * stride - 1;
    const pixel *src2 = src1 - 2 * stride; // == src+2*stride-1;
    int H = src0[1] - src0[-1];
    int V = src1[0] - src2[0];
    for (k = 2; k <= 4; ++k)
    {
        src1 += stride;
        src2 -= stride;
        H += k * (src0[k] - src0[-k]);
        V += k * (src1[0] - src2[0]);
    }
    H = (17 * H + 16) >> 5;
    V = (17 * V + 16) >> 5;

    a = 16 * (src1[0] + src2[8] + 1) - 3 * (V + H);
    v128_t _0123si = wasm_i32x4_const(0, 1, 2, 3);
    v128_t _4567si = wasm_i32x4_const(4, 5, 6, 7);
    for (j = 8; j > 0; --j)
    {
        int b = a;
        a += V;
        v128_t HArr = wasm_i32x4_splat(H);
        v128_t bArr = wasm_i32x4_splat(b);
        v128_t _src0123 = wasm_i32x4_shr(wasm_i32x4_add(bArr, wasm_i32x4_mul(_0123si, HArr)), 5);
        v128_t _src4567 = wasm_i32x4_shr(wasm_i32x4_add(bArr, wasm_i32x4_mul(_4567si, HArr)), 5);
        v128_t _src0_7 = wasm_i16x8_narrow_i32x4(_src0123, _src4567);
        _src0_7 = wasm_u8x16_narrow_i16x8(_src0_7, _src0_7);
        *((int64_t *)src) = wasm_i64x2_extract_lane(_src0_7, 0);
        src += stride;
    }
}

static void ff_pred8x8_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i;
    int dc0, dc1, dc2;
    pixel4 dc0splat, dc1splat, dc2splat, dc3splat;

    dc0=dc1=dc2=0;
    for(i=0;i<4; i++){
        dc0+= src[-1+i*stride] + src[i-stride];
        dc1+= src[4+i-stride];
        dc2+= src[-1+(i+4)*stride];
    }
    dc0splat = PIXEL_SPLAT_X4((dc0 + 4)>>3);
    dc1splat = PIXEL_SPLAT_X4((dc1 + 2)>>2);
    dc2splat = PIXEL_SPLAT_X4((dc2 + 2)>>2);
    dc3splat = PIXEL_SPLAT_X4((dc1 + dc2 + 4)>>3);

    int64_t dc01 = (((int64_t)dc1splat) << 32) + dc0splat;
    int64_t dc23 = (((int64_t)dc3splat) << 32) + dc2splat;

    for(i=0; i<4; i++){
        *((int64_t*)(src+i*stride)) = dc01;
    }
    for(i=4; i<8; i++){
        *((int64_t*)(src+i*stride)) = dc23;
    }
}

static void ff_pred8x8_128_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i;
    const uint64_t a = 128 * 0x0101010101010101ULL;
    for(i=0; i<8; i++){
        *((uint64_t*)(src+i*stride)) = a;
    }
}
static void ff_pred8x8_left_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i;
    int dc0, dc2;
    uint64_t dc0splat, dc2splat;

    dc0=dc2=0;
    for(i=0;i<4; i++){
        dc0+= src[-1+i*stride];
        dc2+= src[-1+(i+4)*stride];
    }
    dc0splat = ((dc0 + 2)>>2) * 0x0101010101010101ULL;
    dc2splat = ((dc2 + 2)>>2) * 0x0101010101010101ULL;

    for(i=0; i<4; i++){
        *(uint64_t*)(src+i*stride) = dc0splat;
    }
    for(i=4; i<8; i++){
        *(uint64_t*)(src+i*stride) = dc2splat;
    }
}
static void ff_pred8x8_top_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    int i;
    int dc0, dc1;
    uint64_t dc0splat, dc1splat;

    dc0=dc1=0;
    v128_t dc0vsrc = wasm_u16x8_extend_low_u8x16(wasm_v128_load(src - stride));
    v128_t dc0top32Low = wasm_i32x4_extend_low_i16x8(dc0vsrc);
    v128_t dc0top64Low = wasm_i64x2_extend_low_i32x4(dc0top32Low);
    v128_t dc0top64High = wasm_i64x2_extend_high_i32x4(dc0top32Low);
    v128_t dc0top64LowPlusHigh = wasm_i64x2_add(dc0top64Low, dc0top64High);

    v128_t dc1vsrc = wasm_u16x8_extend_low_u8x16(wasm_v128_load(src + 4 - stride));
    v128_t dc1top32Low = wasm_i32x4_extend_low_i16x8(dc1vsrc);
    v128_t dc1top64Low = wasm_i64x2_extend_low_i32x4(dc1top32Low);
    v128_t dc1top64High = wasm_i64x2_extend_high_i32x4(dc1top32Low);
    v128_t dc1top64LowPlusHigh = wasm_i64x2_add(dc1top64Low, dc1top64High);

    v128_t decRet = wasm_i64x2_add(
        wasm_i64x2_make(wasm_i64x2_extract_lane(dc0top64LowPlusHigh, 0), wasm_i64x2_extract_lane(dc1top64LowPlusHigh, 0)),
        wasm_i64x2_make(wasm_i64x2_extract_lane(dc0top64LowPlusHigh, 1), wasm_i64x2_extract_lane(dc1top64LowPlusHigh, 1))
    );

    v128_t decSplatRet = wasm_i64x2_mul(
        wasm_i64x2_shr(wasm_i64x2_add(decRet, wasm_i64x2_const_splat(2)), 2),
        wasm_i64x2_const_splat(0x0101010101010101ULL)
    );
    dc0splat = wasm_i64x2_extract_lane(decSplatRet, 0);
    dc1splat = wasm_i64x2_extract_lane(decSplatRet, 1);

    // dc0splat = ((dc0 + 2)>>2) * 0x0101010101010101ULL;
    // dc1splat = ((dc1 + 2)>>2) * 0x0101010101010101ULL;

    for(i=0; i<4; i++){
        *(uint64_t*)(src+i*stride) = dc0splat;
    }
    for(i=4; i<8; i++){
        *(uint64_t*)(src+i*stride) = dc1splat;
    }
}

static void pred4x4_dc_wasm(uint8_t *src, const uint8_t *topright,
                              ptrdiff_t stride)
{
    const int dc= (  src[-stride] + src[1-stride] + src[2-stride] + src[3-stride]
                   + src[-1+0*stride] + src[-1+1*stride] + src[-1+2*stride] + src[-1+3*stride] + 4) >>3;
    const pixel4 a = PIXEL_SPLAT_X4(dc);

    AV_WN4PA(src+0*stride, a);
    AV_WN4PA(src+1*stride, a);
    AV_WN4PA(src+2*stride, a);
    AV_WN4PA(src+3*stride, a);
}

static void pred4x4_top_dc_wasm(uint8_t *_src, const uint8_t *topright,
                                  ptrdiff_t _stride)
{
    pixel *src = (pixel*)_src;
    int stride = _stride>>(sizeof(pixel)-1);
    const int dc= (  src[-stride] + src[1-stride] + src[2-stride] + src[3-stride] + 2) >>2;
    const pixel4 a = PIXEL_SPLAT_X4(dc);

    AV_WN4PA(src+0*stride, a);
    AV_WN4PA(src+1*stride, a);
    AV_WN4PA(src+2*stride, a);
    AV_WN4PA(src+3*stride, a);
}

static void pred4x4_128_dc_wasm(uint8_t *_src, const uint8_t *topright,
                                  ptrdiff_t _stride)
{
    pixel *src = (pixel*)_src;
    int stride = _stride>>(sizeof(pixel)-1);
    const pixel4 a = PIXEL_SPLAT_X4(1<<(BIT_DEPTH-1));

    AV_WN4PA(src+0*stride, a);
    AV_WN4PA(src+1*stride, a);
    AV_WN4PA(src+2*stride, a);
    AV_WN4PA(src+3*stride, a);
}



static void ff_pred8x8_l0t_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    ff_pred8x8_top_dc_wasm(src, stride);
    pred4x4_dc_wasm(src, NULL, stride);
}

static void ff_pred8x8_0lt_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    ff_pred8x8_dc_wasm(src, stride);
    pred4x4_top_dc_wasm(src, NULL, stride);
}
static void ff_pred8x8_l00_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    ff_pred8x8_left_dc_wasm(src, stride);
    pred4x4_128_dc_wasm(src + 4*stride                  , NULL, stride);
    pred4x4_128_dc_wasm(src + 4*stride + 4*sizeof(pixel), NULL, stride);
}

static void ff_pred8x8_0l0_dc_wasm(uint8_t *src, ptrdiff_t stride) {
    ff_pred8x8_left_dc_wasm(src, stride);
    pred4x4_128_dc_wasm(src                  , NULL, stride);
    pred4x4_128_dc_wasm(src + 4*sizeof(pixel), NULL, stride);
}

av_cold void ff_h264_pred_init_wasm(H264PredContext *h, int codec_id,
                                   int bit_depth, const int chroma_format_idc)
{
    const int high_depth = bit_depth > 8;

    if (high_depth || !vesdk_is_264_simd_enabled())
        return;

//    emscripten_log(EM_LOG_CONSOLE, "ff_h264_pred_init_wasm %d", bit_depth);
    if (chroma_format_idc <= 1) { // 初步定位是这里的问题 修复8x8部分的帧内预测的解码部分
        h->pred8x8[VERT_PRED8x8     ] = ff_pred8x8_vert_wasm;
        h->pred8x8[HOR_PRED8x8      ] = ff_pred8x8_hor_wasm;
        if (codec_id != AV_CODEC_ID_VP7 && codec_id != AV_CODEC_ID_VP8)
            h->pred8x8[PLANE_PRED8x8] = ff_pred8x8_plane_wasm;
        h->pred8x8[DC_128_PRED8x8   ] = ff_pred8x8_128_dc_wasm;
        if (codec_id != AV_CODEC_ID_RV40 && codec_id != AV_CODEC_ID_VP7 &&
            codec_id != AV_CODEC_ID_VP8) {
            h->pred8x8[DC_PRED8x8     ] = ff_pred8x8_dc_wasm;
            h->pred8x8[LEFT_DC_PRED8x8] = ff_pred8x8_left_dc_wasm;
            h->pred8x8[TOP_DC_PRED8x8 ] = ff_pred8x8_top_dc_wasm; // Fixed: 8x8 Intra TOP部分有问题
            h->pred8x8[ALZHEIMER_DC_L0T_PRED8x8] = ff_pred8x8_l0t_dc_wasm;
            h->pred8x8[ALZHEIMER_DC_0LT_PRED8x8] = ff_pred8x8_0lt_dc_wasm;
            h->pred8x8[ALZHEIMER_DC_L00_PRED8x8] = ff_pred8x8_l00_dc_wasm;
            h->pred8x8[ALZHEIMER_DC_0L0_PRED8x8] = ff_pred8x8_0l0_dc_wasm;
        }
    }

    h->pred16x16[DC_PRED8x8     ] = ff_pred16x16_dc_wasm; 
    // h->pred16x16[VERT_PRED8x8   ] = ff_pred16x16_vert_wasm; // 自动向量化
    h->pred16x16[HOR_PRED8x8    ] = ff_pred16x16_hor_wasm;
    h->pred16x16[LEFT_DC_PRED8x8] = ff_pred16x16_left_dc_wasm;
    h->pred16x16[TOP_DC_PRED8x8 ] = ff_pred16x16_top_dc_wasm;
    h->pred16x16[DC_128_PRED8x8 ] = ff_pred16x16_128_dc_wasm;
    if (codec_id != AV_CODEC_ID_SVQ3 && codec_id != AV_CODEC_ID_RV40 &&
        codec_id != AV_CODEC_ID_VP7 && codec_id != AV_CODEC_ID_VP8)
        h->pred16x16[PLANE_PRED8x8  ] = ff_pred16x16_plane_wasm; // 关键帧切换处 亮度有问题
}
