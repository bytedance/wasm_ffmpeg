// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#include "config.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <emscripten.h>
#include "libavutil/avassert.h"
#include "libavutil/mem.h"
#include "libavcodec/bit_depth_template.c"

#ifdef PREFIX_h264_qpel8_h_lowpass_wasm
static void PREFIX_h264_qpel8_h_lowpass_wasm(uint8_t *dst,
                                                 const uint8_t *src,
                                                 int dstStride, int srcStride)
{
    const int h=8;
    INIT_CLIP
    int i;
    // pixel *dst = (pixel*)_dst;
    // const pixel *src = (const pixel*)_src;
    dstStride >>= sizeof(pixel)-1;
    srcStride >>= sizeof(pixel)-1;

    v128_t v20mul = wasm_i16x8_const_splat((int16_t)20); 
    v128_t v5mul = wasm_i16x8_const_splat((int16_t)5); 
    v128_t data_step1_left_low, data_step1_right_low, data_step1_mul_ret;
    v128_t data_step2_left_low, data_step2_right_low, data_step2_mul_ret; 
    v128_t data_step3_left_low, data_step3_right_low, data_step3_add_ret; 
    int16_t b_val1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for(i=0; i<h; i++)
    {
        data_step1_left_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src)); 
        data_step1_right_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src + 1)); 
        data_step1_mul_ret = wasm_i16x8_mul( 
            wasm_i16x8_add(data_step1_left_low, data_step1_right_low), v20mul); 

        data_step2_left_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src - 1)); 
        data_step2_right_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src + 2)); 
        data_step2_mul_ret = wasm_i16x8_mul( 
            wasm_i16x8_add(data_step2_left_low, data_step2_right_low), v5mul); 

        data_step3_left_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src - 2)); 
        data_step3_right_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src + 3)); 
        data_step3_add_ret = wasm_i16x8_add(data_step3_left_low, data_step3_right_low); 

        v128_t sub_ret = wasm_i16x8_add(
            wasm_i16x8_sub(data_step1_mul_ret, data_step2_mul_ret), 
            data_step3_add_ret); 

        /* don't remove this code part, it's bak
        wasm_v128_store(b_val1, sub_ret);
        OP_wasm(dst[0], b_val1[0]);
        OP_wasm(dst[1], b_val1[1]);
        OP_wasm(dst[2], b_val1[2]);
        OP_wasm(dst[3], b_val1[3]);
        OP_wasm(dst[4], b_val1[4]);
        OP_wasm(dst[5], b_val1[5]);
        OP_wasm(dst[6], b_val1[6]);
        OP_wasm(dst[7], b_val1[7]);
        */

        v128_t vdst = wasm_v128_load(dst);
        v128_t vagv;
        OP_8X8_wasm(dst, sub_ret, vdst);

        dst+=dstStride;
        src+=srcStride;
    }
}
#endif // PREFIX_h264_qpel8_h_lowpass_wasm
#ifdef PREFIX_h264_qpel8_v_lowpass_wasm
static void PREFIX_h264_qpel8_v_lowpass_wasm(uint8_t *dst, const uint8_t *src,
                                            int dstStride, int srcStride)
{
#ifdef OP_USE_C
    const int w=8;
    INIT_CLIP
    int i;
    for(i=0; i<w; i++)
    {
        const int srcB= src[-2*srcStride];
        const int srcA= src[-1*srcStride];
        const int src0= src[0 *srcStride];
        const int src1= src[1 *srcStride];
        const int src2= src[2 *srcStride];
        const int src3= src[3 *srcStride];
        const int src4= src[4 *srcStride];
        const int src5= src[5 *srcStride];
        const int src6= src[6 *srcStride];
        const int src7= src[7 *srcStride];
        const int src8= src[8 *srcStride];
        const int src9= src[9 *srcStride];
        const int src10=src[10*srcStride];
        OP_wasm(dst[0*dstStride], (src0+src1)*20 - (srcA+src2)*5 + (srcB+src3));
        OP_wasm(dst[1*dstStride], (src1+src2)*20 - (src0+src3)*5 + (srcA+src4));
        OP_wasm(dst[2*dstStride], (src2+src3)*20 - (src1+src4)*5 + (src0+src5));
        OP_wasm(dst[3*dstStride], (src3+src4)*20 - (src2+src5)*5 + (src1+src6));
        OP_wasm(dst[4*dstStride], (src4+src5)*20 - (src3+src6)*5 + (src2+src7));
        OP_wasm(dst[5*dstStride], (src5+src6)*20 - (src4+src7)*5 + (src3+src8));
        OP_wasm(dst[6*dstStride], (src6+src7)*20 - (src5+src8)*5 + (src4+src9));
        OP_wasm(dst[7*dstStride], (src7+src8)*20 - (src6+src9)*5 + (src5+src10));
        dst++;
        src++;
    }
#else
    const int w=8;
    INIT_CLIP
    int i;

    v128_t v20mul = wasm_i16x8_const_splat((int16_t)20); 
    v128_t v5mul = wasm_i16x8_const_splat((int16_t)5); 
    uint32_t *data_mem = NULL; 
    data_mem = (uint32_t *)malloc(sizeof(uint32_t) * 13); 
    uint32_t *data_mem_ptr = NULL; 
    int32_t b_ret_val[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for(i=0; i<w; i++)
    {
        int update_idx = 0; 
        data_mem[update_idx++]= src[-2*srcStride];
        data_mem[update_idx++]= src[-1*srcStride];
        data_mem[update_idx++]= src[0 *srcStride];
        data_mem[update_idx++]= src[1 *srcStride];
        data_mem[update_idx++]= src[2 *srcStride];
        data_mem[update_idx++]= src[3 *srcStride];
        data_mem[update_idx++]= src[4 *srcStride];
        data_mem[update_idx++]= src[5 *srcStride];
        data_mem[update_idx++]= src[6 *srcStride];
        data_mem[update_idx++]= src[7 *srcStride];
        data_mem[update_idx++]= src[8 *srcStride];
        data_mem[update_idx++]= src[9 *srcStride];
        data_mem[update_idx]= src[10*srcStride];
        data_mem_ptr = data_mem + 2; 

        // step1
        v128_t i32_step1_left_low = wasm_v128_load(data_mem_ptr + 0); 
        v128_t i32_step1_right_low =  wasm_v128_load(data_mem_ptr + 1);
        v128_t i32_step1_left_high = wasm_v128_load(data_mem_ptr + 4 + 0); 
        v128_t i32_step1_right_high =  wasm_v128_load(data_mem_ptr + 4 + 1);

        v128_t i32_step1_ret_low = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step1_left_low, i32_step1_right_low), 
            wasm_i32x4_extend_low_i16x8(v20mul)
        );
        v128_t i32_step1_ret_high = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step1_left_high, i32_step1_right_high), 
            wasm_i32x4_extend_low_i16x8(v20mul)
        );;

        // step2
        v128_t i32_step2_left_low = wasm_v128_load(data_mem_ptr - 1); 
        v128_t i32_step2_right_low = wasm_v128_load(data_mem_ptr + 2); 
        v128_t i32_step2_left_high = wasm_v128_load(data_mem_ptr + 4 - 1); 
        v128_t i32_step2_right_high = wasm_v128_load(data_mem_ptr + 4 + 2); 

        v128_t i32_step2_ret_low = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step2_left_low, i32_step2_right_low), 
            wasm_i32x4_extend_low_i16x8(v5mul)
        );
        v128_t i32_step2_ret_high = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step2_left_high, i32_step2_right_high), 
            wasm_i32x4_extend_low_i16x8(v5mul)
        );;

        // step3
        v128_t i32_step3_left_low = wasm_v128_load(data_mem_ptr - 2); 
        v128_t i32_step3_right_low = wasm_v128_load(data_mem_ptr + 3); 
        v128_t i32_step3_left_high = wasm_v128_load(data_mem_ptr + 4 - 2); 
        v128_t i32_step3_right_high = wasm_v128_load(data_mem_ptr + 4 + 3); 

        v128_t i32_step3_ret_low = wasm_i32x4_add(i32_step3_left_low, i32_step3_right_low);
        v128_t i32_step3_ret_high = wasm_i32x4_add(i32_step3_left_high, i32_step3_right_high);

        // step4
        v128_t i32_ret_low = wasm_i32x4_add(wasm_i32x4_sub(i32_step1_ret_low, i32_step2_ret_low) , i32_step3_ret_low);
        v128_t i32_ret_high = wasm_i32x4_add(wasm_i32x4_sub(i32_step1_ret_high, i32_step2_ret_high) , i32_step3_ret_high);

        wasm_v128_store(b_ret_val, i32_ret_low);
        wasm_v128_store(b_ret_val + 4, i32_ret_high);
        
        OP_wasm(dst[0*dstStride], b_ret_val[0]); 
        OP_wasm(dst[1*dstStride], b_ret_val[1]); 
        OP_wasm(dst[2*dstStride], b_ret_val[2]); 
        OP_wasm(dst[3*dstStride], b_ret_val[3]); 
        OP_wasm(dst[4*dstStride], b_ret_val[4]); 
        OP_wasm(dst[5*dstStride], b_ret_val[5]); 
        OP_wasm(dst[6*dstStride], b_ret_val[6]); 
        OP_wasm(dst[7*dstStride], b_ret_val[7]); 
        dst++;
        src++;
    }
    if (data_mem != NULL) { 
        free(data_mem); 
        data_mem = NULL; 
        data_mem_ptr = NULL; 
    }
#endif
}
#endif // PREFIX_h264_qpel8_v_lowpass_wasm

#ifdef PREFIX_h264_qpel8_hv_lowpass_wasm
static void PREFIX_h264_qpel8_hv_lowpass_wasm(uint8_t *dst, int16_t *tmp, const uint8_t *src, 
                                              int dstStride, int tmpStride, int srcStride)
{
    const int h=8;
    const int w=8;
    const int pad = (BIT_DEPTH == 10) ? (-10 * ((1<<BIT_DEPTH)-1)) : 0;
    INIT_CLIP
    int i;
    dstStride >>= sizeof(pixel)-1;
    srcStride >>= sizeof(pixel)-1;
    src -= 2*srcStride;

    v128_t v20mul = wasm_i16x8_const_splat((int16_t)20); 
    v128_t v5mul = wasm_i16x8_const_splat((int16_t)5); 
    v128_t pad_add = wasm_i16x8_const_splat((int16_t)pad); 
    v128_t data_step1_left_low, data_step1_right_low, data_step1_ret;
    v128_t data_step2_left_low, data_step2_right_low, data_step2_ret; 
    v128_t data_step3_left_low, data_step3_right_low, data_step3_ret; 
    for(i=0; i<h+5; i++)
    {
        data_step1_left_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src)); 
        data_step1_right_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src + 1)); 
        data_step1_ret = wasm_i16x8_add( 
            wasm_i16x8_mul( 
                wasm_i16x8_add(data_step1_left_low, data_step1_right_low), v20mul), pad_add); 

        data_step2_left_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src - 1)); 
        data_step2_right_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src + 2)); 
        data_step2_ret = wasm_i16x8_mul( 
            wasm_i16x8_add(data_step2_left_low, data_step2_right_low), v5mul); 

        data_step3_left_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src - 2)); 
        data_step3_right_low = wasm_u16x8_extend_low_u8x16( 
            wasm_v128_load(src + 3)); 
        data_step3_ret = wasm_i16x8_add(data_step3_left_low, data_step3_right_low); 

        wasm_v128_store(tmp, wasm_i16x8_add( 
            wasm_i16x8_sub(data_step1_ret, data_step2_ret), data_step3_ret)); 
        tmp+=tmpStride;
        src+=srcStride;
    }
    tmp -= tmpStride*(h+5-2);


    int32_t *data_mem = NULL; 
    data_mem = (int32_t *)malloc(sizeof(int32_t) * 13); 
    int32_t *data_mem_ptr = NULL; 
    int32_t b_ret_val[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for(i=0; i<w; i++)
    {
        int update_idx = 0; 
        data_mem[update_idx++]= tmp[-2*tmpStride] - pad;
        data_mem[update_idx++]= tmp[-1*tmpStride] - pad;
        data_mem[update_idx++]= tmp[0 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[1 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[2 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[3 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[4 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[5 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[6 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[7 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[8 *tmpStride] - pad;
        data_mem[update_idx++]= tmp[9 *tmpStride] - pad;
        data_mem[update_idx]  = tmp[10*tmpStride] - pad;
        data_mem_ptr = data_mem + 2; 

        // step1
        v128_t i32_step1_left_low = wasm_v128_load(data_mem_ptr + 0); 
        v128_t i32_step1_right_low =  wasm_v128_load(data_mem_ptr + 1);
        v128_t i32_step1_left_high = wasm_v128_load(data_mem_ptr + 4 + 0); 
        v128_t i32_step1_right_high =  wasm_v128_load(data_mem_ptr + 4 + 1);

        v128_t i32_step1_ret_low = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step1_left_low, i32_step1_right_low), 
            wasm_i32x4_extend_low_i16x8(v20mul)
        );
        v128_t i32_step1_ret_high = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step1_left_high, i32_step1_right_high), 
            wasm_i32x4_extend_low_i16x8(v20mul)
        );;

        // step2
        v128_t i32_step2_left_low = wasm_v128_load(data_mem_ptr - 1); 
        v128_t i32_step2_right_low = wasm_v128_load(data_mem_ptr + 2); 
        v128_t i32_step2_left_high = wasm_v128_load(data_mem_ptr + 4 - 1); 
        v128_t i32_step2_right_high = wasm_v128_load(data_mem_ptr + 4 + 2); 

        v128_t i32_step2_ret_low = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step2_left_low, i32_step2_right_low), 
            wasm_i32x4_extend_low_i16x8(v5mul)
        );
        v128_t i32_step2_ret_high = wasm_i32x4_mul(
            wasm_i32x4_add(i32_step2_left_high, i32_step2_right_high), 
            wasm_i32x4_extend_low_i16x8(v5mul)
        );;

        // step3
        v128_t i32_step3_left_low = wasm_v128_load(data_mem_ptr - 2); 
        v128_t i32_step3_right_low = wasm_v128_load(data_mem_ptr + 3); 
        v128_t i32_step3_left_high = wasm_v128_load(data_mem_ptr + 4 - 2); 
        v128_t i32_step3_right_high = wasm_v128_load(data_mem_ptr + 4 + 3); 

        v128_t i32_step3_ret_low = wasm_i32x4_add(i32_step3_left_low, i32_step3_right_low);
        v128_t i32_step3_ret_high = wasm_i32x4_add(i32_step3_left_high, i32_step3_right_high);

        // step4
        v128_t i32_ret_low = wasm_i32x4_add(wasm_i32x4_sub(i32_step1_ret_low, i32_step2_ret_low) , i32_step3_ret_low);
        v128_t i32_ret_high = wasm_i32x4_add(wasm_i32x4_sub(i32_step1_ret_high, i32_step2_ret_high) , i32_step3_ret_high);

        wasm_v128_store(b_ret_val, i32_ret_low);
        wasm_v128_store(b_ret_val + 4, i32_ret_high);
        
        OP2_wasm(dst[0*dstStride], b_ret_val[0]); 
        OP2_wasm(dst[1*dstStride], b_ret_val[1]); 
        OP2_wasm(dst[2*dstStride], b_ret_val[2]); 
        OP2_wasm(dst[3*dstStride], b_ret_val[3]); 
        OP2_wasm(dst[4*dstStride], b_ret_val[4]); 
        OP2_wasm(dst[5*dstStride], b_ret_val[5]); 
        OP2_wasm(dst[6*dstStride], b_ret_val[6]); 
        OP2_wasm(dst[7*dstStride], b_ret_val[7]); 
        dst++;
        tmp++;
    }
    if (data_mem != NULL) { 
        free(data_mem); 
        data_mem = NULL; 
        data_mem_ptr = NULL; 
    }
}
#endif// PREFIX_h264_qpel8_hv_lowpass_wasm
