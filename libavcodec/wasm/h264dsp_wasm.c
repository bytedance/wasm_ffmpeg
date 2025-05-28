// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later


#include "libavcodec/bit_depth_template.c"
#include "libavcodec/h264dec.h"
#include "libavcodec/h264dsp.h"
#include "libavcodec/h264wasm.h"
#include "wasm_simd128.h"
#include <emscripten.h>

static void h264_idct_add_wasm(uint8_t *_dst, int16_t *_block, int stride)
{
    pixel *dst = (pixel*)_dst;
    dctcoef *block = (dctcoef*)_block;
    block[0] += 1 << 5;

    v128_t vblock0 = wasm_i32x4_load_16x4(block);
    v128_t vblock2 = wasm_i32x4_load_16x4(block + 8);
    v128_t vz0 = wasm_i32x4_add(vblock0, vblock2);
    v128_t vz1 = wasm_i32x4_sub(vblock0, vblock2);

    v128_t vblock1 = wasm_i32x4_load_16x4(block + 4);
    v128_t vblock3 = wasm_i32x4_load_16x4(block + 12);
    v128_t vz2 = wasm_i32x4_sub(wasm_i32x4_shr(vblock1, 1), vblock3);
    v128_t vz3 = wasm_i32x4_add(vblock1, wasm_i32x4_shr(vblock3, 1));
    
    vblock0 = wasm_i32x4_add(vz0, vz3);
    vblock1 = wasm_i32x4_add(vz1, vz2);
    vblock2 = wasm_i32x4_sub(vz1, vz2);
    vblock3 = wasm_i32x4_sub(vz0, vz3);

    v128_t vtmp0 = wasm_v32x4_shuffle(vblock0, vblock1, 0, 2, 4, 6);
    v128_t vtmp1 = wasm_v32x4_shuffle(vblock0, vblock1, 1, 3, 5, 7);
    v128_t vtmp2 = wasm_v32x4_shuffle(vblock2, vblock3, 0, 2, 4, 6);
    v128_t vtmp3 = wasm_v32x4_shuffle(vblock2, vblock3, 1, 3, 5, 7);
    vblock0 = wasm_v32x4_shuffle(vtmp0, vtmp2, 0, 2, 4, 6);
    vblock1 = wasm_v32x4_shuffle(vtmp1, vtmp3, 0, 2, 4, 6);
    vblock2 = wasm_v32x4_shuffle(vtmp0, vtmp2, 1, 3, 5, 7);
    vblock3 = wasm_v32x4_shuffle(vtmp1, vtmp3, 1, 3, 5, 7);

    vz0 = wasm_i32x4_add(vblock0, vblock2);
    vz1 = wasm_i32x4_sub(vblock0, vblock2);
    vz2 = wasm_i32x4_sub(wasm_i32x4_shr(vblock1, 1), vblock3);
    vz3 = wasm_i32x4_add(vblock1, wasm_i32x4_shr(vblock3, 1));

    vblock0 = wasm_i32x4_shr(wasm_i32x4_add(vz0, vz3), 6);
    vblock1 = wasm_i32x4_shr(wasm_i32x4_add(vz1, vz2), 6);
    vblock2 = wasm_i32x4_shr(wasm_i32x4_sub(vz1, vz2), 6);
    vblock3 = wasm_i32x4_shr(wasm_i32x4_sub(vz0, vz3), 6);
    
    v128_t vdst = wasm_i32x4_make(
        *(int32_t*)(dst),
        *(int32_t*)(dst + stride),
        *(int32_t*)(dst + 2 * stride),
        *(int32_t*)(dst + 3 * stride));

    v128_t vdst0 = wasm_u16x8_extend_low_u8x16(vdst);
    v128_t vdst2 = wasm_i16x8_widen_high_u8x16(vdst);
    v128_t vdst1 = wasm_i32x4_widen_high_i16x8(vdst0);
    v128_t vdst3 = wasm_i32x4_widen_high_i16x8(vdst2);
    vdst0 = wasm_i32x4_extend_low_i16x8(vdst0);
    vdst2 = wasm_i32x4_extend_low_i16x8(vdst2);

    vdst0 = wasm_i32x4_add(vdst0, vblock0);
    vdst1 = wasm_i32x4_add(vdst1, vblock1);
    vdst2 = wasm_i32x4_add(vdst2, vblock2);
    vdst3 = wasm_i32x4_add(vdst3, vblock3);
    
    vdst0 = wasm_i16x8_narrow_i32x4(vdst0, vdst1);
    vdst2 = wasm_i16x8_narrow_i32x4(vdst2, vdst3);
    
    vdst0 = wasm_u8x16_narrow_i16x8(vdst0, vdst2);

    *(int32_t*)dst = wasm_i32x4_extract_lane(vdst0, 0);
    *(int32_t*)(dst + stride) = wasm_i32x4_extract_lane(vdst0, 1);
    *(int32_t*)(dst + 2 * stride) = wasm_i32x4_extract_lane(vdst0, 2);
    *(int32_t*)(dst + 3 * stride) = wasm_i32x4_extract_lane(vdst0, 3);

    // for(i=0; i<4; i++) {
    //     int c = snprintf(bufptr, 64, "b %d %d %d %d %d\n", stride, dst[i + 0*stride], dst[i + 1*stride], dst[i + 2*stride], dst[i + 3*stride]);
    //     bufptr += c;
    // }
    // emscripten_log(EM_LOG_CONSOLE | EM_LOG_WARN, printbuf);

    memset(block, 0, 16 * sizeof(dctcoef));

}

#define wasm_transpose_8x8(v0, v1, v2, v3, v4, v5, v6, v7) \
    {v128_t vt0, vt2, vt6;\
    vt0 = wasm_v16x8_shuffle(v0, v1, 0, 8, 2, 10, 4, 12, 6, 14);\
    v1 = wasm_v16x8_shuffle(v0, v1, 1, 9, 3, 11, 5, 13, 7, 15);\
    vt2 = wasm_v16x8_shuffle(v2, v3, 0, 8, 2, 10, 4, 12, 6, 14);\
    v3 = wasm_v16x8_shuffle(v2, v3, 1, 9, 3, 11, 5, 13, 7, 15);\
    v2 = wasm_v16x8_shuffle(v4, v5, 0, 8, 2, 10, 4, 12, 6, 14);\
    v5 = wasm_v16x8_shuffle(v4, v5, 1, 9, 3, 11, 5, 13, 7, 15);\
    vt6 = wasm_v16x8_shuffle(v6, v7, 0, 8, 2, 10, 4, 12, 6, 14);\
    v7 = wasm_v16x8_shuffle(v6, v7, 1, 9, 3, 11, 5, 13, 7, 15);\
    v4 = wasm_v32x4_shuffle(v2, vt6, 0, 4, 2, 6);\
    v6 = wasm_v32x4_shuffle(v2, vt6, 1, 5, 3, 7);\
    v2 = wasm_v32x4_shuffle(vt0, vt2, 0, 4, 2, 6);\
    vt6 = wasm_v32x4_shuffle(vt0, vt2, 1, 5, 3, 7);\
    vt2 = wasm_v32x4_shuffle(v1, v3, 0, 4, 2, 6);\
    vt0 = wasm_v32x4_shuffle(v1, v3, 1, 5, 3, 7);\
    v0 = wasm_v64x2_shuffle(v2, v4, 0, 2);\
    v4 = wasm_v64x2_shuffle(v2, v4, 1, 3);\
    v2 = wasm_v32x4_shuffle(v5, v7, 0, 4, 2, 6);\
    v7 = wasm_v32x4_shuffle(v5, v7, 1, 5, 3, 7);\
    v1 = wasm_v64x2_shuffle(vt2, v2, 0, 2);\
    v5 = wasm_v64x2_shuffle(vt2, v2, 1, 3);\
    v2 = wasm_v64x2_shuffle(vt6, v6, 0, 2);\
    v6 = wasm_v64x2_shuffle(vt6, v6, 1, 3);\
    v3 = wasm_v64x2_shuffle(vt0, v7, 0, 2);\
    v7 = wasm_v64x2_shuffle(vt0, v7, 1, 3);\
    }

static void h264_idct8_add_wasm(uint8_t *_dst, int16_t *_block, int stride){
    int i;
    pixel *dst = (pixel*)_dst;
    dctcoef *block = (dctcoef*)_block;
    stride >>= sizeof(pixel)-1;

    block[0] += 32;

    v128_t vblock0 = wasm_v128_load(block);
    v128_t vblock2 = wasm_v128_load(block + 2 * 8);
    v128_t vblock4 = wasm_v128_load(block + 4 * 8);
    v128_t vblock6 = wasm_v128_load(block + 6 * 8);

    v128_t va0 = wasm_i16x8_add(vblock0, vblock4);
    v128_t va2 = wasm_i16x8_sub(vblock0, vblock4);
    v128_t va4 = wasm_i16x8_sub(wasm_i16x8_shr(vblock2, 1), vblock6);
    v128_t va6 = wasm_i16x8_add(wasm_i16x8_shr(vblock6, 1), vblock2);

    v128_t vb0 = wasm_i16x8_add(va0, va6);
    v128_t vb2 = wasm_i16x8_add(va2, va4);
    v128_t vb4 = wasm_i16x8_sub(va2, va4);
    v128_t vb6 = wasm_i16x8_sub(va0, va6);

    v128_t vblock1 = wasm_v128_load(block + 1 * 8);
    v128_t vblock3 = wasm_v128_load(block + 3 * 8);
    v128_t vblock5 = wasm_v128_load(block + 5 * 8);
    v128_t vblock7 = wasm_v128_load(block + 7 * 8);
    
    v128_t va1 = wasm_i16x8_sub(wasm_i16x8_sub(vblock5, vblock3), wasm_i16x8_add(vblock7, wasm_i16x8_shr(vblock7, 1)));
    v128_t va3 = wasm_i16x8_sub(wasm_i16x8_add(vblock7, vblock1), wasm_i16x8_add(vblock3, wasm_i16x8_shr(vblock3, 1)));
    v128_t va5 = wasm_i16x8_add(wasm_i16x8_sub(vblock7, vblock1), wasm_i16x8_add(vblock5, wasm_i16x8_shr(vblock5, 1)));
    v128_t va7 = wasm_i16x8_add(wasm_i16x8_add(vblock5, vblock3), wasm_i16x8_add(vblock1, wasm_i16x8_shr(vblock1, 1)));

    v128_t vb1 = wasm_i16x8_add(wasm_i16x8_shr(va7, 2), va1);
    v128_t vb3 = wasm_i16x8_add(va3, wasm_i16x8_shr(va5, 2));
    v128_t vb5 = wasm_i16x8_sub(wasm_i16x8_shr(va3, 2), va5);
    v128_t vb7 = wasm_i16x8_sub(va7, wasm_i16x8_shr(va1, 2));
    
    vblock0 = wasm_i16x8_add(vb0, vb7);
    vblock7 = wasm_i16x8_sub(vb0, vb7);
    vblock1 = wasm_i16x8_add(vb2, vb5);
    vblock6 = wasm_i16x8_sub(vb2, vb5);
    vblock2 = wasm_i16x8_add(vb4, vb3);
    vblock5 = wasm_i16x8_sub(vb4, vb3);
    vblock3 = wasm_i16x8_add(vb6, vb1);
    vblock4 = wasm_i16x8_sub(vb6, vb1);

    wasm_transpose_8x8(vblock0, vblock1, vblock2, vblock3, vblock4, vblock5, vblock6, vblock7);

    va0 = wasm_i16x8_add(vblock0, vblock4);
    va2 = wasm_i16x8_sub(vblock0, vblock4);
    va4 = wasm_i16x8_sub(wasm_i16x8_shr(vblock2, 1), vblock6);
    va6 = wasm_i16x8_add(wasm_i16x8_shr(vblock6, 1), vblock2);

    vb0 = wasm_i16x8_add(va0, va6);
    vb2 = wasm_i16x8_add(va2, va4);
    vb4 = wasm_i16x8_sub(va2, va4);
    vb6 = wasm_i16x8_sub(va0, va6);
    
    va1 = wasm_i16x8_sub(wasm_i16x8_sub(vblock5, vblock3), wasm_i16x8_add(vblock7, wasm_i16x8_shr(vblock7, 1)));
    va3 = wasm_i16x8_sub(wasm_i16x8_add(vblock7, vblock1), wasm_i16x8_add(vblock3, wasm_i16x8_shr(vblock3, 1)));
    va5 = wasm_i16x8_add(wasm_i16x8_sub(vblock7, vblock1), wasm_i16x8_add(vblock5, wasm_i16x8_shr(vblock5, 1)));
    va7 = wasm_i16x8_add(wasm_i16x8_add(vblock5, vblock3), wasm_i16x8_add(vblock1, wasm_i16x8_shr(vblock1, 1)));

    vb1 = wasm_i16x8_add(wasm_i16x8_shr(va7, 2), va1);
    vb3 = wasm_i16x8_add(va3, wasm_i16x8_shr(va5, 2));
    vb5 = wasm_i16x8_sub(wasm_i16x8_shr(va3, 2), va5);
    vb7 = wasm_i16x8_sub(va7, wasm_i16x8_shr(va1, 2));

    v128_t vdst0 = wasm_u16x8_load8x8(dst + 0 * stride);
    v128_t vdst1 = wasm_u16x8_load8x8(dst + 1 * stride);
    v128_t vdst2 = wasm_u16x8_load8x8(dst + 2 * stride);
    v128_t vdst3 = wasm_u16x8_load8x8(dst + 3 * stride);
    v128_t vdst4 = wasm_u16x8_load8x8(dst + 4 * stride);
    v128_t vdst5 = wasm_u16x8_load8x8(dst + 5 * stride);
    v128_t vdst6 = wasm_u16x8_load8x8(dst + 6 * stride);
    v128_t vdst7 = wasm_u16x8_load8x8(dst + 7 * stride);

    vdst0 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_add(vb0, vb7), 6), vdst0);
    vdst1 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_add(vb2, vb5), 6), vdst1);
    vdst2 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_add(vb4, vb3), 6), vdst2);
    vdst3 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_add(vb6, vb1), 6), vdst3);
    vdst4 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_sub(vb6, vb1), 6), vdst4);
    vdst5 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_sub(vb4, vb3), 6), vdst5);
    vdst6 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_sub(vb2, vb5), 6), vdst6);
    vdst7 = wasm_i16x8_add(wasm_i16x8_shr(wasm_i16x8_sub(vb0, vb7), 6), vdst7);

    vdst0 = wasm_u8x16_narrow_i16x8(vdst0, vdst1);
    *(int64_t *)(dst + 0 * stride) = wasm_i64x2_extract_lane(vdst0, 0);
    *(int64_t *)(dst + 1 * stride) = wasm_i64x2_extract_lane(vdst0, 1);

    vdst2 = wasm_u8x16_narrow_i16x8(vdst2, vdst3);
    *(int64_t *)(dst + 2 * stride) = wasm_i64x2_extract_lane(vdst2, 0);
    *(int64_t *)(dst + 3 * stride) = wasm_i64x2_extract_lane(vdst2, 1);

    vdst4 = wasm_u8x16_narrow_i16x8(vdst4, vdst5);
    *(int64_t *)(dst + 4 * stride) = wasm_i64x2_extract_lane(vdst4, 0);
    *(int64_t *)(dst + 5 * stride) = wasm_i64x2_extract_lane(vdst4, 1);

    vdst6 = wasm_u8x16_narrow_i16x8(vdst6, vdst7);
    *(int64_t *)(dst + 6 * stride) = wasm_i64x2_extract_lane(vdst6, 0);
    *(int64_t *)(dst + 7 * stride) = wasm_i64x2_extract_lane(vdst6, 1);


    memset(block, 0, 64 * sizeof(dctcoef));
}

// assumes all AC coefs are 0
static void h264_idct_dc_add_wasm(uint8_t *_dst, int16_t *_block, int stride) {
    pixel *dst = (pixel*)_dst;
    dctcoef *block = (dctcoef*)_block;
    int dc = (block[0] + 32) >> 6;
    stride /= sizeof(pixel);
    block[0] = 0;

    v128_t vdc = wasm_i16x8_splat(*(int16_t *)&dc);
    v128_t vdst = wasm_i32x4_make(
        *(int32_t*)(dst),
        *(int32_t*)(dst + stride),
        *(int32_t*)(dst + 2 * stride),
        *(int32_t*)(dst + 3 * stride));

    v128_t vdst0 = wasm_u16x8_extend_low_u8x16(vdst);
    v128_t vdst1 = wasm_i16x8_widen_high_u8x16(vdst);

    vdst0 = wasm_i16x8_add(vdst0, vdc);
    vdst1 = wasm_i16x8_add(vdst1, vdc);
    
    vdst0 = wasm_u8x16_narrow_i16x8(vdst0, vdst1);

    *(int32_t*)dst = wasm_i32x4_extract_lane(vdst0, 0);
    *(int32_t*)(dst + stride) = wasm_i32x4_extract_lane(vdst0, 1);
    *(int32_t*)(dst + 2 * stride) = wasm_i32x4_extract_lane(vdst0, 2);
    *(int32_t*)(dst + 3 * stride) = wasm_i32x4_extract_lane(vdst0, 3);
}


static void h264_idct8_dc_add_wasm(uint8_t *_dst, int16_t *_block, int stride){
    int i, j;
    pixel *dst = (pixel*)_dst;
    dctcoef *block = (dctcoef*)_block;
    int dc = (block[0] + 32) >> 6;
    block[0] = 0;
    stride /= sizeof(pixel);
    v128_t vdc = wasm_i16x8_splat(dc);
    for( j = 0; j < 8; j++ )
    {
        v128_t vdst = wasm_u16x8_load8x8(dst);
        vdst = wasm_i16x8_add(vdst, vdc);
        vdst = wasm_u8x16_narrow_i16x8(vdst, vdc);
        *(int64_t *)dst = wasm_i64x2_extract_lane(vdst, 0);
        dst += stride;
    }
}

static void h264_idct_add8_wasm(uint8_t **dest, const int *block_offset,
                                   int16_t *block, int stride,
                                   const uint8_t nnzc[15 * 8])
{
    int i, j;
    for (j = 1; j < 3; j++) {
        for(i = j * 16; i < j * 16 + 4; i++){
            if(nnzc[ scan8[i] ])
                h264_idct_add_wasm(dest[j-1] + block_offset[i], block + i*16, stride);
            else if(block[i*16])
                h264_idct_dc_add_wasm(dest[j-1] + block_offset[i], block + i*16, stride);
        }
    }
}

static void h264_idct_add16_wasm(uint8_t *dst, const int *block_offset,
                                    int16_t *block, int stride,
                                    const uint8_t nnzc[15 * 8])
{
    int i;
    for(i=0; i<16; i++){
        int nnz = nnzc[ scan8[i] ];
        if(nnz){
            if(nnz==1 && block[i*16]) h264_idct_dc_add_wasm(dst + block_offset[i], block + i*16, stride);
            else                      h264_idct_add_wasm(dst + block_offset[i], block + i*16, stride);
        }
    }
}

static void h264_idct_add16intra_wasm(uint8_t *dst, const int *block_offset,
                                         int16_t *block, int stride,
                                         const uint8_t nnzc[15 * 8])
{
    int i;
    for(i=0; i<16; i++){
        if(nnzc[ scan8[i] ]) h264_idct_add_wasm(dst + block_offset[i], block + i*16, stride);
        else if(block[i*16]) h264_idct_dc_add_wasm(dst + block_offset[i], block + i*16, stride);
    }
}

static void h264_idct8_add4_wasm(uint8_t *dst, const int *block_offset,
                                    int16_t *block, int stride,
                                    const uint8_t nnzc[15 * 8])
{
    int i;
    for(i=0; i<16; i+=4){
        int nnz = nnzc[ scan8[i] ];
        if(nnz){
            if(nnz==1 && block[i*16]) h264_idct8_dc_add_wasm(dst + block_offset[i], block + i*16, stride);
            else                      h264_idct8_add_wasm(dst + block_offset[i], block + i*16, stride);
        }
    }
}

#define av_clip_simd(vtarget, vmin, vmax) wasm_i16x8_min(wasm_i16x8_max(vtarget, vmin), vmax) 
#define filter_vp1(vp0, vq0, vp1, vp2) wasm_i16x8_add(av_clip_simd( \
    wasm_i16x8_sub(wasm_i16x8_shr(wasm_i16x8_add(vp2, wasm_u16x8_avgr(vp0, vq0)),1), vp1)\
    , wasm_i16x8_neg(vtc_orig), vtc_orig), vp1)



static void h264_v_loop_filter_luma_wasm(uint8_t *p_pix, int stride, int alpha, int beta, int8_t *tc0)
{
    pixel *pix = (pixel*)p_pix;

    v128_t vp0all = wasm_v128_load(pix - 1*stride);
    v128_t vp1all = wasm_v128_load(pix - 2*stride);
    v128_t vp2all = wasm_v128_load(pix - 3*stride);
    v128_t vq0all = wasm_v128_load(pix + 0);
    v128_t vq1all = wasm_v128_load(pix + 1*stride);
    v128_t vq2all = wasm_v128_load(pix + 2*stride);

    #pragma clang loop unroll(full)
    for (int i=0; i<2; i++) {
        v128_t vtc_orig = wasm_i16x8_make(tc0[2*i], tc0[2*i], tc0[2*i], tc0[2*i], tc0[2*i+1], tc0[2*i+1], tc0[2*i+1], tc0[2*i+1]); 
        
        v128_t valpha = wasm_i16x8_splat(alpha);
        v128_t vbeta = wasm_i16x8_splat(beta);
        const v128_t vc0 = wasm_i64x2_const(0, 0);

        v128_t vp0, vp1, vp2, vq0, vq1, vq2;
        v128_t out_vp0, out_vp1, out_vq0, out_vq1;
        if (i == 0) {
            vp0 = wasm_u16x8_extend_low_u8x16(vp0all);
            vp1 = wasm_u16x8_extend_low_u8x16(vp1all);
            vp2 = wasm_u16x8_extend_low_u8x16(vp2all);
            vq0 = wasm_u16x8_extend_low_u8x16(vq0all);
            vq1 = wasm_u16x8_extend_low_u8x16(vq1all);
            vq2 = wasm_u16x8_extend_low_u8x16(vq2all);
        } else {
            vp0 = wasm_i16x8_widen_high_u8x16(vp0all);
            vp1 = wasm_i16x8_widen_high_u8x16(vp1all);
            vp2 = wasm_i16x8_widen_high_u8x16(vp2all);
            vq0 = wasm_i16x8_widen_high_u8x16(vq0all);
            vq1 = wasm_i16x8_widen_high_u8x16(vq1all);
            vq2 = wasm_i16x8_widen_high_u8x16(vq2all);
        }

        v128_t cond1 = wasm_i16x8_ge(vtc_orig, vc0);
        v128_t tempcond = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vp0, vq0)), valpha);
        cond1 = wasm_v128_and(cond1, tempcond);
        tempcond = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vp1, vp0)), vbeta);
        cond1 = wasm_v128_and(cond1, tempcond);
        tempcond = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vq1, vq0)), vbeta);
        cond1 = wasm_v128_and(cond1, tempcond);

        if (wasm_i64x2_extract_lane(cond1, 0) || wasm_i64x2_extract_lane(cond1, 1)) {
            v128_t vtc = vtc_orig;
            
            v128_t cond2 = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vp2, vp0)), vbeta);
            cond2 = wasm_v128_and(cond2, cond1);
            v128_t cond3 = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vq2, vq0)), vbeta);
            cond3 = wasm_v128_and(cond3, cond1);

            vtc = wasm_i16x8_add(vtc, wasm_u16x8_shr(cond2, 15));
            vtc = wasm_i16x8_add(vtc, wasm_u16x8_shr(cond3, 15));

            if (wasm_i64x2_extract_lane(cond2, 0) || wasm_i64x2_extract_lane(cond2, 1)) {
                v128_t filtered_vp1 = filter_vp1(vp0, vq0, vp1, vp2);
                out_vp1 = wasm_v128_bitselect(filtered_vp1, vp1, cond2);
                *(int64_t*)(pix - 2*stride) = wasm_i64x2_extract_lane(wasm_u8x16_narrow_i16x8(out_vp1, out_vp1), 0);
            }

            if (wasm_i64x2_extract_lane(cond3, 0) || wasm_i64x2_extract_lane(cond3, 1)) {
                v128_t filtered_vq1 = filter_vp1(vp0, vq0, vq1, vq2);
                out_vq1 = wasm_v128_bitselect(filtered_vq1, vq1, cond3);
                *(int64_t*)(pix + stride) = wasm_i64x2_extract_lane(wasm_u8x16_narrow_i16x8(out_vq1, out_vq1), 0);
            }

            const v128_t vc4 = wasm_i16x8_const(4, 4, 4, 4, 4, 4, 4, 4);
            v128_t vi_delta = av_clip_simd(wasm_i16x8_shr(wasm_i16x8_add(wasm_i16x8_add(wasm_i16x8_mul(wasm_i16x8_sub(vq0, vp0), vc4), wasm_i16x8_sub(vp1, vq1)), vc4) ,3), 
                wasm_i16x8_neg(vtc), vtc);
            
            v128_t filtered_vp0 = wasm_i16x8_add(vp0, vi_delta);
            out_vp0 = wasm_v128_bitselect(filtered_vp0, vp0, cond1);

            v128_t filtered_vq0 = wasm_i16x8_sub(vq0, vi_delta);
            out_vq0 = wasm_v128_bitselect(filtered_vq0, vq0, cond1);

            v128_t out_vtmp = wasm_u8x16_narrow_i16x8(out_vp0, out_vq0);
            *(int64_t *)(pix - stride) = wasm_i64x2_extract_lane(out_vtmp, 0);
            *(int64_t *)pix = wasm_i64x2_extract_lane(out_vtmp, 1);
        }

        pix += 8;
    }
}



static void h264_h_loop_filter_luma_wasm(uint8_t *p_pix, int stride, int alpha, int beta, int8_t *tc0)
{
    pixel *pix = (pixel*)p_pix;
    int i;

    #pragma clang loop unroll(full)
    for (i=0; i<2; i++) {
        v128_t vtc_orig = wasm_i16x8_make(tc0[2*i], tc0[2*i], tc0[2*i], tc0[2*i], tc0[2*i+1], tc0[2*i+1], tc0[2*i+1], tc0[2*i+1]); 
        
        v128_t valpha = wasm_i16x8_splat(alpha);
        v128_t vbeta = wasm_i16x8_splat(beta);
        const v128_t vc0 = wasm_i64x2_const(0, 0);

        v128_t vp0, vp1, vp2, vq0, vq1, vq2, v6, v7;
        v128_t out_vp0, out_vp1, out_vq0, out_vq1;

        vp2 = wasm_u16x8_load8x8(pix - 3);
        vp1 = wasm_u16x8_load8x8(pix - 3 + stride);
        vp0 = wasm_u16x8_load8x8(pix - 3 + 2 * stride);
        vq0 = wasm_u16x8_load8x8(pix - 3 + 3 * stride);
        vq1 = wasm_u16x8_load8x8(pix - 3 + 4 * stride);
        vq2 = wasm_u16x8_load8x8(pix - 3 + 5 * stride);
        v6 = wasm_u16x8_load8x8(pix - 3 + 6 * stride);
        v7 = wasm_u16x8_load8x8(pix - 3 + 7 * stride);

        wasm_transpose_8x8(vp2, vp1, vp0, vq0, vq1, vq2, v6, v7)

        v128_t cond1 = wasm_i16x8_ge(vtc_orig, vc0);
        v128_t tempcond = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vp0, vq0)), valpha);
        cond1 = wasm_v128_and(cond1, tempcond);
        tempcond = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vp1, vp0)), vbeta);
        cond1 = wasm_v128_and(cond1, tempcond);
        tempcond = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vq1, vq0)), vbeta);
        cond1 = wasm_v128_and(cond1, tempcond);

        if (wasm_i64x2_extract_lane(cond1, 0) || wasm_i64x2_extract_lane(cond1, 1)) {
            v128_t vtc = vtc_orig;
            
            v128_t cond2 = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vp2, vp0)), vbeta);
            cond2 = wasm_v128_and(cond2, cond1);
            v128_t cond3 = wasm_i16x8_lt(wasm_i16x8_abs(wasm_i16x8_sub(vq2, vq0)), vbeta);
            cond3 = wasm_v128_and(cond3, cond1);

            vtc = wasm_i16x8_add(vtc, wasm_u16x8_shr(cond2, 15));
            vtc = wasm_i16x8_add(vtc, wasm_u16x8_shr(cond3, 15));

            if (wasm_i64x2_extract_lane(cond2, 0) || wasm_i64x2_extract_lane(cond2, 1)) {
                v128_t filtered_vp1 = filter_vp1(vp0, vq0, vp1, vp2);
                out_vp1 = wasm_v128_bitselect(filtered_vp1, vp1, cond2);
            } else {
                out_vp1 = vp1;
            }

            if (wasm_i64x2_extract_lane(cond3, 0) || wasm_i64x2_extract_lane(cond3, 1)) {
                v128_t filtered_vq1 = filter_vp1(vp0, vq0, vq1, vq2);
                out_vq1 = wasm_v128_bitselect(filtered_vq1, vq1, cond3);
            } else {
                out_vq1 = vq1;
            }

            const v128_t vc4 = wasm_i16x8_const(4, 4, 4, 4, 4, 4, 4, 4);
            v128_t vi_delta = av_clip_simd(wasm_i16x8_shr(wasm_i16x8_add(wasm_i16x8_add(wasm_i16x8_mul(wasm_i16x8_sub(vq0, vp0), vc4), wasm_i16x8_sub(vp1, vq1)), vc4) ,3), 
                wasm_i16x8_neg(vtc), vtc);
            
            v128_t filtered_vp0 = wasm_i16x8_add(vp0, vi_delta);
            out_vp0 = wasm_v128_bitselect(filtered_vp0, vp0, cond1);

            v128_t filtered_vq0 = wasm_i16x8_sub(vq0, vi_delta);
            out_vq0 = wasm_v128_bitselect(filtered_vq0, vq0, cond1);

            wasm_transpose_8x8(vp2, out_vp1, out_vp0, out_vq0, out_vq1, vq2, v6, v7);
            v128_t vout_tmp = wasm_u8x16_narrow_i16x8(vp2, out_vp1);
            AV_WN64(pix - 3, wasm_i64x2_extract_lane(vout_tmp, 0));
            AV_WN64(pix - 3 + stride, wasm_i64x2_extract_lane(vout_tmp, 1));
            vout_tmp = wasm_u8x16_narrow_i16x8(out_vp0, out_vq0);
            AV_WN64(pix - 3 + 2 * stride, wasm_i64x2_extract_lane(vout_tmp, 0));
            AV_WN64(pix - 3 + 3 * stride, wasm_i64x2_extract_lane(vout_tmp, 1));
            vout_tmp = wasm_u8x16_narrow_i16x8(out_vq1, vq2);
            AV_WN64(pix - 3 + 4 * stride, wasm_i64x2_extract_lane(vout_tmp, 0));
            AV_WN64(pix - 3 + 5 * stride, wasm_i64x2_extract_lane(vout_tmp, 1));
            vout_tmp = wasm_u8x16_narrow_i16x8(v6, v7);
            AV_WN64(pix - 3 + 6 * stride, wasm_i64x2_extract_lane(vout_tmp, 0));
            AV_WN64(pix - 3 + 7 * stride, wasm_i64x2_extract_lane(vout_tmp, 1));
        }

        pix += 8 * stride;
    }
}


#define H264_WEIGHT_WASM(W) \
static void weight_h264_pixels ## W ## _wasm(uint8_t *_block, ptrdiff_t stride, int height, \
                                           int log2_denom, int weight, int offset) \
{ \
    int y; \
    pixel *block = (pixel*)_block; \
    stride >>= sizeof(pixel)-1; \
    offset = (unsigned)offset << (log2_denom + (BIT_DEPTH-8)); \
    if(log2_denom) offset += 1<<(log2_denom-1); \
    v128_t vweight = wasm_i16x8_splat(weight);\
    v128_t voffset = wasm_i16x8_splat(offset);\
\
    for (y = 0; y < height; y++, block += stride) { \
        v128_t vblock = wasm_u16x8_load8x8(block);\
        v128_t vdst = wasm_u8x16_narrow_i16x8(\
                wasm_i16x8_shr(\
                    wasm_i16x8_add(wasm_i16x8_mul(vblock, vweight), voffset),\
                    log2_denom \
                ), voffset\
            );\
        *(int64_t *)block = wasm_i64x2_extract_lane(vdst, 0);\
\
        if (W == 8) continue; \
        vblock = wasm_u16x8_load8x8(block + 8);\
        vdst = wasm_u8x16_narrow_i16x8(\
                wasm_i16x8_shr(\
                    wasm_i16x8_add(wasm_i16x8_mul(vblock, vweight), voffset),\
                    log2_denom \
                ), voffset\
            );\
        *(int64_t *)(block+8) = wasm_i64x2_extract_lane(vdst, 0);\
    } \
} \
\
static void biweight_h264_pixels ## W ## _wasm(uint8_t *_dst, uint8_t *_src, ptrdiff_t stride, int height, \
                                             int log2_denom, int weightd, int weights, int offset) \
{ \
    int y; \
    pixel *dst = (pixel*)_dst; \
    pixel *src = (pixel*)_src; \
    stride >>= sizeof(pixel)-1; \
    offset = (unsigned)offset << (BIT_DEPTH-8); \
    offset = (unsigned)((offset + 1) | 1) << log2_denom; \
    v128_t vweightd = wasm_i16x8_splat(weightd);\
    v128_t vweights = wasm_i16x8_splat(weights);\
    v128_t voffset = wasm_i16x8_splat(offset);\
    for (y = 0; y < height; y++, dst += stride, src += stride) { \
        v128_t vdst = wasm_u16x8_load8x8(dst);\
        v128_t vsrc = wasm_u16x8_load8x8(src);\
        vdst = wasm_u8x16_narrow_i16x8(\
                wasm_i16x8_shr(\
                    wasm_i16x8_add(\
                        wasm_i16x8_add(\
                            wasm_i16x8_mul(vsrc, vweights),\
                            wasm_i16x8_mul(vdst, vweightd)\
                        ),\
                        voffset\
                    ),\
                    log2_denom + 1\
                ), voffset\
            );\
        *(int64_t *)dst = wasm_i64x2_extract_lane(vdst, 0);\
        if(W==8) continue; \
        vdst = wasm_u16x8_load8x8(dst + 8);\
        vsrc = wasm_u16x8_load8x8(src + 8);\
        vdst = wasm_u8x16_narrow_i16x8(\
                wasm_i16x8_shr(\
                    wasm_i16x8_add(\
                        wasm_i16x8_add(\
                            wasm_i16x8_mul(vsrc, vweights),\
                            wasm_i16x8_mul(vdst, vweightd)\
                        ),\
                        voffset\
                    ),\
                    log2_denom + 1\
                ), voffset\
            );\
        *(int64_t *)(dst+8) = wasm_i64x2_extract_lane(vdst, 0);\
    } \
}

H264_WEIGHT_WASM(16);
H264_WEIGHT_WASM(8);

static void h264_v_loop_filter_chroma_wasm(uint8_t *pix, int xstride, int alpha, int beta, int8_t *tc0) {
    // 例子：stride=960，alpha=9，beta=3 
    // tc 4个字节 int8_t => i16x8: 00112233
 
    v128_t v0ss = wasm_i16x8_const_splat(0);
    v128_t v4ss = wasm_i16x8_const_splat(4);

    v128_t vp0 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix - xstride));
    v128_t vp1 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix - 2 * xstride));
    v128_t vq0 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix));
    v128_t vq1 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix + xstride));

    v128_t vp0_q0 = wasm_i16x8_abs(wasm_i16x8_sub(vp0, vq0));
    v128_t vp1_p0 = wasm_i16x8_abs(wasm_i16x8_sub(vp1, vp0));
    v128_t vq1_q0 = wasm_i16x8_abs(wasm_i16x8_sub(vq1, vq0));

    v128_t vbeta = wasm_i16x8_splat((int16_t)beta);
    v128_t valpha = wasm_i16x8_splat((int16_t)alpha);

    v128_t vtc0 = wasm_i16x8_load8x8(tc0); // 0123xxxx
    v128_t vtc = wasm_i16x8_shuffle(vtc0, vtc0, 0, 0, 1, 1, 2, 2, 3, 3);

    v128_t conditions1 = wasm_i16x8_lt(vp0_q0, valpha); // |p0 - q0| < alpha
    v128_t conditions2 = wasm_i16x8_lt(vp1_p0, vbeta); // |p1 - p0| < beta
    v128_t conditions3 = wasm_i16x8_lt(vq1_q0, vbeta); // |q1 - q0| < beta
    v128_t conditions4 = wasm_i16x8_gt(vtc, v0ss); // tc > 0
    v128_t conditions = wasm_v128_and(conditions1, 
            wasm_v128_and(conditions2, wasm_v128_and(conditions3, conditions4)));

    v128_t vdeltatmp = wasm_i16x8_sub(vq0, vp0); // (q0 - p0)
    vdeltatmp = wasm_i16x8_mul(vdeltatmp, v4ss); // *4
    vdeltatmp = wasm_i16x8_add(vdeltatmp, wasm_i16x8_sub(vp1, vq1));// + (p1-q1)
    vdeltatmp = wasm_i16x8_add(vdeltatmp, v4ss); // + 4
    vdeltatmp = wasm_i16x8_shr(vdeltatmp, 3); // >> 3
    vdeltatmp = wasm_i16x8_max(vdeltatmp, wasm_i16x8_sub(v0ss, vtc)); // max (a, -tc)
    v128_t vdelta = wasm_i16x8_min(vdeltatmp, vtc); // min (a, tc)

    v128_t vret1 = wasm_i16x8_add(vp0, vdelta); // p0 + delta
    v128_t vret2 = wasm_i16x8_sub(vq0, vdelta); // q0 - delta

#define REPLACE_LANE(i) \
    if (wasm_i16x8_extract_lane(conditions, i) == 0) { \
        vret1 = wasm_i16x8_replace_lane(vret1, i, wasm_i16x8_extract_lane(vp0, i)); \
        vret2 = wasm_i16x8_replace_lane(vret2, i, wasm_i16x8_extract_lane(vq0, i)); \
    }
    REPLACE_LANE(0);
    REPLACE_LANE(1);
    REPLACE_LANE(2);
    REPLACE_LANE(3);
    REPLACE_LANE(4);
    REPLACE_LANE(5);
    REPLACE_LANE(6);
    REPLACE_LANE(7);
#undef REPLACE_LANE
    
    vret1 = wasm_u8x16_narrow_i16x8(vret1, vret1);
    vret2 = wasm_u8x16_narrow_i16x8(vret2, vret2);

    int64_t ret1 = wasm_i64x2_extract_lane(vret1, 0);
    int64_t ret2 = wasm_i64x2_extract_lane(vret2, 0);
    *((int64_t *)(pix - xstride)) = ret1;
    *((int64_t *)(pix)) = ret2;
}

static void h264_v_loop_filter_chroma_intra_wasm(uint8_t *pix, int xstride, int alpha, int beta) {
    // int d;
    // int ystride = 1;
    // for( d = 0; d < 4 * 2; d++ ) {
    //     const int p0 = pix[-1*xstride];
    //     const int p1 = pix[-2*xstride];
    //     const int q0 = pix[0];
    //     const int q1 = pix[1*xstride];

    //     if( FFABS( p0 - q0 ) < alpha &&
    //         FFABS( p1 - p0 ) < beta &&
    //         FFABS( q1 - q0 ) < beta ) {

    //         pix[-xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
    //         pix[0]        = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
    //     }
    //     pix += ystride;
    // }    
    v128_t v0ss = wasm_i16x8_const_splat(0);
    v128_t v2ss = wasm_i16x8_const_splat(2);

    v128_t vp0 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix - xstride));
    v128_t vp1 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix - 2 * xstride));
    v128_t vq0 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix));
    v128_t vq1 = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pix + xstride));

    v128_t vp0_q0 = wasm_i16x8_abs(wasm_i16x8_sub(vp0, vq0));
    v128_t vp1_p0 = wasm_i16x8_abs(wasm_i16x8_sub(vp1, vp0));
    v128_t vq1_q0 = wasm_i16x8_abs(wasm_i16x8_sub(vq1, vq0));

    v128_t vbeta = wasm_i16x8_splat((int16_t)beta);
    v128_t valpha = wasm_i16x8_splat((int16_t)alpha);

    v128_t conditions1 = wasm_i16x8_lt(vp0_q0, valpha); // |p0 - q0| < alpha
    v128_t conditions2 = wasm_i16x8_lt(vp1_p0, vbeta); // |p1 - p0| < beta
    v128_t conditions3 = wasm_i16x8_lt(vq1_q0, vbeta); // |q1 - q0| < beta
    v128_t conditions = wasm_v128_and(conditions1, wasm_v128_and(conditions2, conditions3));
    
    v128_t vret1 = wasm_i16x8_mul(vp1, v2ss); // 2*p1
    vret1 = wasm_i16x8_add(vret1, vp0); // + p0
    vret1 = wasm_i16x8_add(vret1, vq1); // + q1
    vret1 = wasm_i16x8_add(vret1, v2ss); // + 2
    vret1 = wasm_i16x8_shr(vret1, 2); // >> 2

    v128_t vret2 = wasm_i16x8_mul(vq1, v2ss); // 2*q1
    vret2 = wasm_i16x8_add(vret2, vq0); // + q0
    vret2 = wasm_i16x8_add(vret2, vp1); // + p1
    vret2 = wasm_i16x8_add(vret2, v2ss); // + 2
    vret2 = wasm_i16x8_shr(vret2, 2); // >> 2

#define REPLACE_LANE(i) \
    if (wasm_i16x8_extract_lane(conditions, i) == 0) { \
        vret1 = wasm_i16x8_replace_lane(vret1, i, wasm_i16x8_extract_lane(vp0, i)); \
        vret2 = wasm_i16x8_replace_lane(vret2, i, wasm_i16x8_extract_lane(vq0, i)); \
    }
    REPLACE_LANE(0);
    REPLACE_LANE(1);
    REPLACE_LANE(2);
    REPLACE_LANE(3);
    REPLACE_LANE(4);
    REPLACE_LANE(5);
    REPLACE_LANE(6);
    REPLACE_LANE(7);
#undef REPLACE_LANE
    vret1 = wasm_u8x16_narrow_i16x8(vret1, vret1);
    vret2 = wasm_u8x16_narrow_i16x8(vret2, vret2);

    int64_t ret1 = wasm_i64x2_extract_lane(vret1, 0);
    int64_t ret2 = wasm_i64x2_extract_lane(vret2, 0);
    *((int64_t *)(pix - xstride)) = ret1;
    *((int64_t *)(pix)) = ret2;
}

static void h264_v_loop_filter_luma_intra_wasm_2(uint8_t *pix, int xstride, int alpha, int beta) {
    uint8_t* pix_cpy = pix;
    // int d;
    int ystride = 1;
    int inner_iters = 4;

    xstride >>= sizeof(pixel) - 1; // (uint8-1byte)1-1=0
    alpha <<= BIT_DEPTH - 8; // 8-8=0
    beta  <<= BIT_DEPTH - 8;

    v128_t vp3 = wasm_v128_load(pix_cpy - 4 * xstride);
    v128_t vp2 = wasm_v128_load(pix_cpy - 3 * xstride);
    v128_t vp1 = wasm_v128_load(pix_cpy - 2 * xstride);
    v128_t vp0 = wasm_v128_load(pix_cpy - 1 * xstride);

    v128_t vq0 = wasm_v128_load(pix_cpy + 0 * xstride);
    v128_t vq1 = wasm_v128_load(pix_cpy + 1 * xstride);
    v128_t vq2 = wasm_v128_load(pix_cpy + 2 * xstride);
    v128_t vq3 = wasm_v128_load(pix_cpy + 3 * xstride);

#define CALC_CORE(d) { \ 
        const int p2 = wasm_u8x16_extract_lane(vp2, d);\
        const int p1 = wasm_u8x16_extract_lane(vp1, d);\
        const int p0 = wasm_u8x16_extract_lane(vp0, d);\
        const int q0 = wasm_u8x16_extract_lane(vq0, d);\
        const int q1 = wasm_u8x16_extract_lane(vq1, d);\
        const int q2 = wasm_u8x16_extract_lane(vq3, d);\
\
        if( FFABS( p0 - q0 ) < alpha &&\
            FFABS( p1 - p0 ) < beta &&\
            FFABS( q1 - q0 ) < beta ) {\
\
            if(FFABS( p0 - q0 ) < (( alpha >> 2 ) + 2 )){\
                if( FFABS( p2 - p0 ) < beta)\
                {\
                    const int p3 = wasm_u8x16_extract_lane(vp3, d);\
                    pix_cpy[-1*xstride] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3; \
                    pix_cpy[-2*xstride] = ( p2 + p1 + p0 + q0 + 2 ) >> 2; \
                    pix_cpy[-3*xstride] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3; \
                } else {\
                    pix_cpy[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2; \
                }\
                if( FFABS( q2 - q0 ) < beta)\
                {\
                    const int q3 = wasm_u8x16_extract_lane(vq3, d);\
                    pix_cpy[0*xstride] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3; \
                    pix_cpy[1*xstride] = ( p0 + q0 + q1 + q2 + 2 ) >> 2; \
                    pix_cpy[2*xstride] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3; \
                } else {\
                    pix_cpy[0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2; \
                }\
            }else{\
                pix_cpy[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2; \
                pix_cpy[ 0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2; \
            }\
        }\
        pix_cpy += ystride; \
} // end define
//         pix += ystride;

    CALC_CORE(0);
    CALC_CORE(1);
    CALC_CORE(2);
    CALC_CORE(3);
    CALC_CORE(4);
    CALC_CORE(5);
    CALC_CORE(6);
    CALC_CORE(7);
    CALC_CORE(8);
    CALC_CORE(9);
    CALC_CORE(10);
    CALC_CORE(11);
    CALC_CORE(12);
    CALC_CORE(13);
    CALC_CORE(14);
    CALC_CORE(15);
#undef CALC_CORE
}

static void h264_v_loop_filter_luma_intra_wasm(uint8_t *pix, int xstride, int alpha, int beta) 
{
    uint8_t* pix_cpy = pix;
    int d;
    int ystride = 1;
    int inner_iters = 4;
    v128_t vp3 = wasm_v128_load(pix - 4 * xstride);
    v128_t vp2 = wasm_v128_load(pix - 3 * xstride);
    v128_t vp1 = wasm_v128_load(pix - 2 * xstride);
    v128_t vp0 = wasm_v128_load(pix - 1 * xstride);
    v128_t vq0 = wasm_v128_load(pix - 0 * xstride);
    v128_t vq1 = wasm_v128_load(pix - 1 * xstride);
    v128_t vq2 = wasm_v128_load(pix - 2 * xstride);
    v128_t vq3 = wasm_v128_load(pix - 3 * xstride);

    v128_t voutp0 = vp0;
    v128_t voutp1 = vp1;
    v128_t voutp2 = vp2;
    v128_t voutq0 = vq0;
    v128_t voutq1 = vq1;
    v128_t voutq2 = vq2;
#define CALC_CORE(d) { \ 
        const int p2 = wasm_u8x16_extract_lane(vp2, d); \
        const int p1 = wasm_u8x16_extract_lane(vp1, d);\
        const int p0 = wasm_u8x16_extract_lane(vp0, d);\
\
        const int q0 = wasm_u8x16_extract_lane(vq0, d);\
        const int q1 = wasm_u8x16_extract_lane(vq1, d);\
        const int q2 = wasm_u8x16_extract_lane(vq2, d);\
\
        if( FFABS( p0 - q0 ) < alpha &&\
            FFABS( p1 - p0 ) < beta &&\
            FFABS( q1 - q0 ) < beta ) {\
\
            if(FFABS( p0 - q0 ) < (( alpha >> 2 ) + 2 )){\
                if( FFABS( p2 - p0 ) < beta)\
                {\
                    const int p3 = wasm_u8x16_extract_lane(vp3, d);\
                    voutp0 = wasm_u8x16_replace_lane(voutp0, d, ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3);\
                    voutp1 = wasm_u8x16_replace_lane(voutp1, d, ( p2 + p1 + p0 + q0 + 2 ) >> 2);\
                    voutp2 = wasm_u8x16_replace_lane(voutp2, d, ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3);\
                } else {\
                    voutp0 = wasm_u8x16_replace_lane(voutp0, d, ( 2*p1 + p0 + q1 + 2 ) >> 2);\
                }\
                if( FFABS( q2 - q0 ) < beta)\
                {\
                    const int q3 = wasm_u8x16_extract_lane(vq3, d);\
                    voutq0 = wasm_u8x16_replace_lane(voutq0, d,  ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3);\
                    voutq1 = wasm_u8x16_replace_lane(voutq1, d,  ( p0 + q0 + q1 + q2 + 2 ) >> 2);\
                    voutq2 = wasm_u8x16_replace_lane(voutq1, d,  ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3);\
                } else {\
                    voutq0 = wasm_u8x16_replace_lane(voutq0, d,  ( 2*q1 + q0 + p1 + 2 ) >> 2);\
                }\
            }else{\
                voutp0 = wasm_u8x16_replace_lane(voutp0, d, ( 2*p1 + p0 + q1 + 2 ) >> 2);\
                voutq0 = wasm_u8x16_replace_lane(voutq0, d, ( 2*q1 + q0 + p1 + 2 ) >> 2);\
            }\
        }\
        pix += ystride;\
    }
    CALC_CORE(0);
    CALC_CORE(1);
    CALC_CORE(2);
    CALC_CORE(3);
    CALC_CORE(4);
    CALC_CORE(5);
    CALC_CORE(6);
    CALC_CORE(7);
    CALC_CORE(8);
    CALC_CORE(9);
    CALC_CORE(10);
    CALC_CORE(11);
    CALC_CORE(12);
    CALC_CORE(13);
    CALC_CORE(14);
    CALC_CORE(15);
#undef CALC_CORE

    wasm_v128_store(pix_cpy - 3 * xstride, voutp2);
    wasm_v128_store(pix_cpy - 2 * xstride, voutp1);
    wasm_v128_store(pix_cpy - 1 * xstride, voutp0);
    wasm_v128_store(pix_cpy - 0 * xstride, voutq0);
    wasm_v128_store(pix_cpy - 1 * xstride, voutq1);
    wasm_v128_store(pix_cpy - 2 * xstride, voutq2);
}

av_cold void ff_h264dsp_init_wasm(H264DSPContext *c, const int bit_depth,
                                 const int chroma_format_idc)
{
    
    if (bit_depth == 8 && vesdk_is_264_simd_enabled()) {
//        emscripten_log(EM_LOG_CONSOLE, "ff_h264dsp_init_wasm %d", bit_depth);
        c->h264_idct_add = h264_idct_add_wasm;
        if (chroma_format_idc <= 1)
            c->h264_idct_add8 = h264_idct_add8_wasm;
        c->h264_idct_add16      = h264_idct_add16_wasm;
        c->h264_idct_add16intra = h264_idct_add16intra_wasm;
        c->h264_idct_dc_add= h264_idct_dc_add_wasm;
        c->h264_idct8_dc_add = h264_idct8_dc_add_wasm;
        c->h264_idct8_add    = h264_idct8_add_wasm;
        c->h264_idct8_add4   = h264_idct8_add4_wasm;

        c->h264_v_loop_filter_luma= h264_v_loop_filter_luma_wasm; 
        c->h264_h_loop_filter_luma= h264_h_loop_filter_luma_wasm; 

        /**
         * 
         * ZH: 函数: h264_v_loop_filter_luma_intra_wasm 的wasm 128simd 指令集优化函数暂时不可用
         * 影响范围：测试用例 1280p 黑底白字case 下 会出现白字周边间接性（必现）的快效应、花屏问题
         * @TODO 修复计划内
         * 
         * EN: function: h264_v_loop_filter_luma_intra_wasm has problem with decode video(1280p with white color text, background is black). The problem is block/mosaic around of white text
         * @TODO fixing
         * 
         */
        c->h264_v_loop_filter_luma_intra = h264_v_loop_filter_luma_intra_wasm_2;

        c->weight_h264_pixels_tab[0]   = weight_h264_pixels16_wasm;
        c->weight_h264_pixels_tab[1]   = weight_h264_pixels8_wasm;
        c->biweight_h264_pixels_tab[0] = biweight_h264_pixels16_wasm;
        c->biweight_h264_pixels_tab[1] = biweight_h264_pixels8_wasm;

        c->h264_v_loop_filter_chroma = h264_v_loop_filter_chroma_wasm;
        c->h264_v_loop_filter_chroma_intra = h264_v_loop_filter_chroma_intra_wasm;
    }
}
