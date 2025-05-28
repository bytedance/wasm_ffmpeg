// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#include "config.h"
#include "libavutil/attributes.h"
#include "libavutil/cpu.h"
#include "libavutil/intreadwrite.h"
#include "libavcodec/h264qpel.h"
#include "libavcodec/copy_block.h"
#include "libavcodec/h264wasm.h"
#include "wasm_simd128.h"


#define load_with_perm_vec(a,b,c) wasm_v128_load((uint8_t *)b + a)
#define unaligned_load(a,b) wasm_v128_load((uint8_t *)b + a)

// a = av_clip_uint8((b + 16)>>5) must use wasm_i16x8_shr not wasm_u16x8_shr
#define PUT_OP_8X8_wasm(d, s, dst) \
        s = wasm_i16x8_shr(wasm_i16x8_add(s, wasm_i16x8_const_splat(16)), 5);   \
        *((int64_t*)d) = wasm_i64x2_extract_lane(wasm_u8x16_narrow_i16x8(s, s), 0)

// a = (a+av_clip_uint8((b + 16)>>5) + 1)>>1 must use wasm_i16x8_shr not wasm_u16x8_shr
#define AVG_OP_8X8_wasm(d, s, dst) \
        dst = wasm_u16x8_extend_low_u8x16(dst);                           \
        s = wasm_i16x8_shr(wasm_i16x8_add(s, wasm_i16x8_const_splat(16)), 5); \
        vagv = wasm_u16x8_avgr(s, dst);                                       \
        *((int64_t*)d) = wasm_i64x2_extract_lane(wasm_u8x16_narrow_i16x8(vagv, vagv), 0)

#define AVG_OP_wasm(a, b) a = (((a)+CLIP(((b) + 16)>>5)+1)>>1)
#define PUT_OP_wasm(a, b) a = CLIP(((b) + 16)>>5)

#define AVG_OP2_wasm(a, b)  a = (((a)+av_clip_uint8(((b) + 512)>>10)+1)>>1)
#define PUT_OP2_wasm(a, b)  a = av_clip_uint8(((b) + 512)>>10)

#define AVG_OP2_8X8_wasm(d, retLow, retHigh, dst)                                                                     \
    dst = wasm_u16x8_extend_low_u8x16(dst);                                                                           \
    v128_t dstLow = wasm_u32x4_extend_low_u16x8(dst);                                                                 \
    v128_t dstHigh = wasm_u32x4_extend_low_u16x8(dst);                                                                \
    retLow = wasm_u32x4_shr(wasm_i32x4_add(retLow, wasm_i32x4_const_splat(512)), 10);                                 \
    retHigh = wasm_u32x4_shr(wasm_i32x4_add(retHigh, wasm_i32x4_const_splat(512)), 10);                               \
    v128_t vagvLow = wasm_u32x4_shr(wasm_i32x4_add(wasm_i32x4_add(retLow, dstLow), wasm_i32x4_const_splat(1)), 1);    \
    v128_t vagvHigh = wasm_u32x4_shr(wasm_i32x4_add(wasm_i32x4_add(retHigh, dstHigh), wasm_i32x4_const_splat(1)), 1); \
    vagv = wasm_u16x8_narrow_i32x4(vagvLow, vagvHigh);                                                                \
    *((int64_t *)d) = wasm_i64x2_extract_lane(wasm_u8x16_narrow_i16x8(vagv, vagv), 0)

#define PUT_OP2_8X8_wasm(d, retLow, retHigh, dst)                                       \
    retLow = wasm_u32x4_shr(wasm_i32x4_add(retLow, wasm_i32x4_const_splat(512)), 10);   \
    retHigh = wasm_u32x4_shr(wasm_i32x4_add(retHigh, wasm_i32x4_const_splat(512)), 10); \
    v128_t s = wasm_u16x8_narrow_i32x4(retLow, retHigh);                                \
    *((int64_t *)d) = wasm_i64x2_extract_lane(wasm_u8x16_narrow_i16x8(s, s), 0)

#define OP_8X8_wasm                       PUT_OP_8X8_wasm
#define OP2_8X8_wasm                      PUT_OP2_8X8_wasm
#define OP2_wasm                          PUT_OP2_wasm
#define OP_wasm                           PUT_OP_wasm
#define PREFIX_h264_qpel8_h_lowpass_wasm        put_h264_qpel8_h_lowpass_wasm
#define PREFIX_h264_qpel8_v_lowpass_wasm        put_h264_qpel8_v_lowpass_wasm
#define PREFIX_h264_qpel8_hv_lowpass_wasm       put_h264_qpel8_hv_lowpass_wasm
#define OP_USE_C
#include "h264qpel8_wasm_template.c"
#undef OP_USE_C
#undef OP_8X8_wasm
#undef OP2_8X8_wasm
#undef OP2_wasm
#undef OP_wasm
#undef PREFIX_h264_qpel8_h_lowpass_wasm
#undef PREFIX_h264_qpel8_v_lowpass_wasm
#undef PREFIX_h264_qpel8_hv_lowpass_wasm



#define OP_8X8_wasm                       AVG_OP_8X8_wasm
#define OP2_8X8_wasm                      AVG_OP2_8X8_wasm
#define OP2_wasm                          AVG_OP2_wasm
#define OP_wasm                           AVG_OP_wasm
#define PREFIX_h264_qpel8_h_lowpass_wasm        avg_h264_qpel8_h_lowpass_wasm
#define PREFIX_h264_qpel8_v_lowpass_wasm        avg_h264_qpel8_v_lowpass_wasm
#define PREFIX_h264_qpel8_hv_lowpass_wasm       avg_h264_qpel8_hv_lowpass_wasm
#include "h264qpel8_wasm_template.c"
#undef OP_8X8_wasm
#undef OP2_8X8_wasm
#undef OP2_wasm
#undef OP2_wasm
#undef PREFIX_h264_qpel8_h_lowpass_wasm
#undef PREFIX_h264_qpel8_v_lowpass_wasm
#undef PREFIX_h264_qpel8_hv_lowpass_wasm

#define H264_LOWPASS(OPNAME, CODETYPE) \
static void OPNAME ## h264_qpel16_v_lowpass_ ## CODETYPE(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## h264_qpel8_v_lowpass_ ## CODETYPE(dst                , src                , dstStride, srcStride);\
    OPNAME ## h264_qpel8_v_lowpass_ ## CODETYPE(dst+8*sizeof(pixel), src+8*sizeof(pixel), dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## h264_qpel8_v_lowpass_ ## CODETYPE(dst                , src                , dstStride, srcStride);\
    OPNAME ## h264_qpel8_v_lowpass_ ## CODETYPE(dst+8*sizeof(pixel), src+8*sizeof(pixel), dstStride, srcStride);\
}\
\
static void OPNAME ## h264_qpel16_h_lowpass_ ## CODETYPE(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride){\
    OPNAME ## h264_qpel8_h_lowpass_ ## CODETYPE(dst                , src                , dstStride, srcStride);\
    OPNAME ## h264_qpel8_h_lowpass_ ## CODETYPE(dst+8*sizeof(pixel), src+8*sizeof(pixel), dstStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## h264_qpel8_h_lowpass_ ## CODETYPE(dst                , src                , dstStride, srcStride);\
    OPNAME ## h264_qpel8_h_lowpass_ ## CODETYPE(dst+8*sizeof(pixel), src+8*sizeof(pixel), dstStride, srcStride);\
}\
\
static void OPNAME ## h264_qpel16_hv_lowpass_ ## CODETYPE(uint8_t *dst, pixeltmp *tmp, const uint8_t *src, int dstStride, int tmpStride, int srcStride){\
    OPNAME ## h264_qpel8_hv_lowpass_ ## CODETYPE(dst                , tmp  , src                , dstStride, tmpStride, srcStride);\
    OPNAME ## h264_qpel8_hv_lowpass_ ## CODETYPE(dst+8*sizeof(pixel), tmp+8, src+8*sizeof(pixel), dstStride, tmpStride, srcStride);\
    src += 8*srcStride;\
    dst += 8*dstStride;\
    OPNAME ## h264_qpel8_hv_lowpass_ ## CODETYPE(dst                , tmp  , src                , dstStride, tmpStride, srcStride);\
    OPNAME ## h264_qpel8_hv_lowpass_ ## CODETYPE(dst+8*sizeof(pixel), tmp+8, src+8*sizeof(pixel), dstStride, tmpStride, srcStride);\
}\


#define pixeltmp int16_t
#define H264_MC2(OPNAME, SIZE, CODETYPE) \
static void OPNAME ## h264_qpel ## SIZE ## _mc00_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    OPNAME ## pixels ## SIZE ## _wasm(dst, src, stride, SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc10_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t half[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(half, src, SIZE*sizeof(pixel), stride);\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, src, half, stride, stride, SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc20_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    OPNAME ## h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(dst, src, stride, stride);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc30_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t half[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(half, src, SIZE*sizeof(pixel), stride);\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, src+sizeof(pixel), half, stride, stride, SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc01_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    uint8_t half[SIZE*SIZE*sizeof(pixel)];\
    copy_block ## SIZE (full, src - stride*2, SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(half, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, full_mid, half, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc02_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    copy_block ## SIZE (full, src - stride*2, SIZE*sizeof(pixel),  stride, SIZE + 5);\
    OPNAME ## h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(dst, full_mid, stride, SIZE*sizeof(pixel));\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc03_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    uint8_t half[SIZE*SIZE*sizeof(pixel)];\
    copy_block ## SIZE (full, src - stride*2, SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(half, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, full_mid+SIZE*sizeof(pixel), half, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc11_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    uint8_t halfH[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfV[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(halfH, src, SIZE*sizeof(pixel), stride);\
    copy_block ## SIZE (full, src - stride*2, SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(halfV, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfH, halfV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc31_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    uint8_t halfH[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfV[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(halfH, src, SIZE*sizeof(pixel), stride);\
    copy_block ## SIZE (full, src - stride*2 + sizeof(pixel), SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(halfV, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfH, halfV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc13_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    uint8_t halfH[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfV[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(halfH, src + stride, SIZE*sizeof(pixel), stride);\
    copy_block ## SIZE (full, src - stride*2, SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(halfV, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfH, halfV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc33_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    uint8_t halfH[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfV[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(halfH, src + stride, SIZE*sizeof(pixel), stride);\
    copy_block ## SIZE (full, src - stride*2 + sizeof(pixel), SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(halfV, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfH, halfV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc22_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    pixeltmp tmp[SIZE*(SIZE+5)*sizeof(pixel)];\
    OPNAME ## h264_qpel ## SIZE ## _hv_lowpass_ ## CODETYPE(dst, tmp, src, stride, SIZE*sizeof(pixel), stride);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc21_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    pixeltmp tmp[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t halfH[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfHV[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(halfH, src, SIZE*sizeof(pixel), stride);\
    put_h264_qpel ## SIZE ## _hv_lowpass_ ## CODETYPE(halfHV, tmp, src, SIZE*sizeof(pixel), SIZE*sizeof(pixel), stride);\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfH, halfHV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc23_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    pixeltmp tmp[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t halfH[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfHV[SIZE*SIZE*sizeof(pixel)];\
    put_h264_qpel ## SIZE ## _h_lowpass_ ## CODETYPE(halfH, src + stride, SIZE*sizeof(pixel), stride);\
    put_h264_qpel ## SIZE ## _hv_lowpass_ ## CODETYPE(halfHV, tmp, src, SIZE*sizeof(pixel), SIZE*sizeof(pixel), stride);\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfH, halfHV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc12_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    pixeltmp tmp[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t halfV[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfHV[SIZE*SIZE*sizeof(pixel)];\
    copy_block ## SIZE (full, src - stride*2, SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(halfV, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    put_h264_qpel ## SIZE ## _hv_lowpass_ ## CODETYPE(halfHV, tmp, src, SIZE*sizeof(pixel), SIZE*sizeof(pixel), stride);\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfV, halfHV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}\
\
static void OPNAME ## h264_qpel ## SIZE ## _mc32_ ## CODETYPE(uint8_t *dst, const uint8_t *src, ptrdiff_t stride)\
{\
    uint8_t full[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t * const full_mid= full + SIZE*2*sizeof(pixel);\
    pixeltmp tmp[SIZE*(SIZE+5)*sizeof(pixel)];\
    uint8_t halfV[SIZE*SIZE*sizeof(pixel)];\
    uint8_t halfHV[SIZE*SIZE*sizeof(pixel)];\
    copy_block ## SIZE (full, src - stride*2 + sizeof(pixel), SIZE*sizeof(pixel),  stride, SIZE + 5);\
    put_h264_qpel ## SIZE ## _v_lowpass_ ## CODETYPE(halfV, full_mid, SIZE*sizeof(pixel), SIZE*sizeof(pixel));\
    put_h264_qpel ## SIZE ## _hv_lowpass_ ## CODETYPE(halfHV, tmp, src, SIZE*sizeof(pixel), SIZE*sizeof(pixel), stride);\
    OPNAME ## pixels ## SIZE ## _l2_wasm(dst, halfV, halfHV, stride, SIZE*sizeof(pixel), SIZE*sizeof(pixel), SIZE);\
}                                       \
                                         

#define put_unligned_store(s, dest) wasm_v128_store(dest, s);

static inline void put_pixels8_wasm(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
{
#define OP(a, b) a = b
    int i;                                                              
    for (i = 0; i < h; i++) {                                           
        OP(*((pixel4 *) block), AV_RN4P(pixels));                       
        OP(*((pixel4 *) (block + 4 * sizeof(pixel))),                   
           AV_RN4P(pixels + 4 * sizeof(pixel)));                        
        pixels += line_size;                                            
        block  += line_size;                                            
    }
#undef OP
}

static inline void avg_pixels8_wasm(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)                     
{
#define OP(a, b) a = rnd_avg_pixel4(a, b)
    int i;                                                              
    for (i = 0; i < h; i++) {                                           
        OP(*((pixel4 *) block), AV_RN4P(pixels));                       
        OP(*((pixel4 *) (block + 4 * sizeof(pixel))),                   
           AV_RN4P(pixels + 4 * sizeof(pixel)));                        
        pixels += line_size;                                            
        block  += line_size;                                            
    }
#undef OP
}                                                                       

static inline void put_pixels16_l2_wasm( uint8_t * dst, const uint8_t * src1,
                                    const uint8_t * src2, int dst_stride,
                                    int src_stride1, int src_stride2, int h)
{
    int i;
    v128_t a, b, d, mask_;
    for (i = 0; i < h; i++) {
        a = unaligned_load(i * src_stride1, src1);
        b = load_with_perm_vec(i * src_stride2, src2, mask_);
        d = wasm_u8x16_avgr(a, b);
        put_unligned_store(d, dst);
        dst += dst_stride;
    }
}

static inline void put_pixels8_l2_wasm( uint8_t * dst, const uint8_t * src1,
                                    const uint8_t * src2, int dst_stride,
                                    int src_stride1, int src_stride2, int h)
{
// 有问题
//   int i;
//   v128_t a, b, d, mask_;
//   for (i = 0; i < h; i++) {
//       a = unaligned_load(i * src_stride1, src1);
//       b = load_with_perm_vec(i * src_stride2, src2, mask_);
//       d = wasm_u8x16_avgr(a, b);
//       *((int64_t*) dst) = wasm_i64x2_extract_lane(d, 0);
//       dst += dst_stride;
//   }

 #define OP(a, b) a = b 
 //#define OP(a, b) a = rnd_avg_pixel4(a, b)
     int i;                                                              
     for (i = 0; i < h; i++) {                                           
         pixel4 a, b;                                                    
         a = AV_RN4P(&src1[i * src_stride1]);                            
         b = AV_RN4P(&src2[i * src_stride2]);                            
         OP(*((pixel4 *) &dst[i * dst_stride]), rnd_avg_pixel4(a, b));   
         a = AV_RN4P(&src1[i * src_stride1 + 4 * sizeof(pixel)]);        
         b = AV_RN4P(&src2[i * src_stride2 + 4 * sizeof(pixel)]);        
         OP(*((pixel4 *) &dst[i * dst_stride + 4 * sizeof(pixel)]),      
            rnd_avg_pixel4(a, b));                                       
     }
 #undef OP
}



#define avg_unligned_store(s, dest){            \
    a = wasm_u8x16_avgr(wasm_v128_load(dst), s);         \
    wasm_v128_store(dst, a);                      \
 }

static void avg_pixels16_wasm(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
{
     int i;
//     for (i = 0; i < h; i++)
//     {
//         op_avg(*((uint32_t *)block), (((const union unaligned_32 *)(pixels))->l));
//         op_avg(*((uint32_t *)(block + 4 * sizeof(uint8_t))), (((const union unaligned_32 *)(pixels + 4 * sizeof(uint8_t)))->l));
//         pixels += line_size;
//         block += line_size;
//     }
     for (i = 0; i < h; i++)
     {
         v128_t a = wasm_v128_load(block);
         v128_t b = wasm_v128_load(pixels);
         a = wasm_u8x16_avgr(a, b);
         wasm_v128_store(block, a);
         pixels += line_size;
         block += line_size;
     }
}

static void put_pixels16_wasm(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
{
     int i;
//     for (i = 0; i < h; i++)
//     {
//         op_put(*((uint32_t *)block), (((const union unaligned_32 *)(pixels))->l));
//         op_put(*((uint32_t *)(block + 4 * sizeof(uint8_t))), (((const union unaligned_32 *)(pixels + 4 * sizeof(uint8_t)))->l));
//         pixels += line_size;
//         block += line_size;
//     }
     for (i = 0; i < h; i++)
     {
         v128_t b = wasm_v128_load(pixels);
         wasm_v128_store(block, b);
         pixels += line_size;
         block += line_size;
     }
}

static inline void avg_pixels16_l2_wasm( uint8_t * dst, const uint8_t * src1,
                                    const uint8_t * src2, int dst_stride,
                                    int src_stride1, int src_stride2, int h)
{
    int i;
    v128_t a, b, d, mask_;

    for (i = 0; i < h; i++) {
        a = unaligned_load(i * src_stride1, src1);
        b = load_with_perm_vec(i * src_stride2, src2, mask_);
        d = wasm_u8x16_avgr(a, b);
        avg_unligned_store(d, dst);
        dst += dst_stride;
    }
}

static inline void avg_pixels8_l2_wasm( uint8_t * dst, const uint8_t * src1,
                                    const uint8_t * src2, int dst_stride,
                                    int src_stride1, int src_stride2, int h)
{
//   int i;
//   v128_t a, b, d, mask_;
//   for (i = 0; i < h; i++) {
//       a = wasm_v128_load(src1 + i * src_stride1);
//       b = wasm_v128_load(src2 + i * src_stride2);
//       d = wasm_u8x16_avgr(a, b);
//       *((int64_t*) dst) = wasm_i64x2_extract_lane(d, 0);
//       dst += dst_stride;
//   }
#define OP(a, b) a = b
 #define OP(a, b) a = rnd_avg_pixel4(a, b)
     int i;
     for (i = 0; i < h; i++) {
         pixel4 a, b;
         a = AV_RN4P(&src1[i * src_stride1]);
         b = AV_RN4P(&src2[i * src_stride2]);
         OP(*((pixel4 *) &dst[i * dst_stride]), rnd_avg_pixel4(a, b));
         a = AV_RN4P(&src1[i * src_stride1 + 4 * sizeof(pixel)]);
         b = AV_RN4P(&src2[i * src_stride2 + 4 * sizeof(pixel)]);
         OP(*((pixel4 *) &dst[i * dst_stride + 4 * sizeof(pixel)]),
            rnd_avg_pixel4(a, b));
     }
 #undef OP
}

/* Implemented but could be faster
#define put_pixels16_l2_wasm(d,s1,s2,ds,s1s,h) put_pixels16_l2(d,s1,s2,ds,s1s,16,h)
#define avg_pixels16_l2_wasm(d,s1,s2,ds,s1s,h) avg_pixels16_l2(d,s1,s2,ds,s1s,16,h)
 */
H264_LOWPASS(put_, wasm)
H264_LOWPASS(avg_, wasm)

H264_MC2(put_, 16, wasm)
H264_MC2(avg_, 16, wasm)
H264_MC2(put_, 8, wasm)
H264_MC2(avg_, 8, wasm)

av_cold void ff_h264qpel_init_wasm(H264QpelContext *c, int bit_depth)
{
    const int high_bit_depth = bit_depth > 8;

    if (!high_bit_depth && vesdk_is_264_simd_enabled()) {
//        emscripten_log(EM_LOG_CONSOLE, "ff_h264qpel_init_wasm %d ", bit_depth);
        /** 
         * 
         * ZH: 函数: dspfunc 的wasm 128simd 指令集优化函数暂时不可用
         * 影响范围：测试用例 1280p 黑底白字case 下 会出现白字周边间接性（必现）的快效应、花屏问题
         * @TODO 修复计划内
         * 
         * EN: function: dspfunc has problem with decode video(1280p with white color text, background is black). The problem is block/mosaic around of white text
         * @TODO fixing
         * 
         */
#define dspfunc(PFX, IDX, NUM) \
        c->PFX ## _pixels_tab[IDX][ 0] = PFX ## NUM ## _mc00_wasm; \ 
        c->PFX ## _pixels_tab[IDX][ 1] = PFX ## NUM ## _mc10_wasm; \
        c->PFX ## _pixels_tab[IDX][ 2] = PFX ## NUM ## _mc20_wasm; \
        c->PFX ## _pixels_tab[IDX][ 3] = PFX ## NUM ## _mc30_wasm; \
        c->PFX ## _pixels_tab[IDX][ 4] = PFX ## NUM ## _mc01_wasm; \
        c->PFX ## _pixels_tab[IDX][ 5] = PFX ## NUM ## _mc11_wasm; \
        c->PFX ## _pixels_tab[IDX][ 6] = PFX ## NUM ## _mc21_wasm; \
        c->PFX ## _pixels_tab[IDX][ 7] = PFX ## NUM ## _mc31_wasm; \
        c->PFX ## _pixels_tab[IDX][ 8] = PFX ## NUM ## _mc02_wasm; \
        c->PFX ## _pixels_tab[IDX][ 9] = PFX ## NUM ## _mc12_wasm; \
        c->PFX ## _pixels_tab[IDX][10] = PFX ## NUM ## _mc22_wasm; \
        c->PFX ## _pixels_tab[IDX][11] = PFX ## NUM ## _mc32_wasm; \
        c->PFX ## _pixels_tab[IDX][12] = PFX ## NUM ## _mc03_wasm; \
        c->PFX ## _pixels_tab[IDX][13] = PFX ## NUM ## _mc13_wasm; \
        c->PFX ## _pixels_tab[IDX][14] = PFX ## NUM ## _mc23_wasm; \
        c->PFX ## _pixels_tab[IDX][15] = PFX ## NUM ## _mc33_wasm

        dspfunc(put_h264_qpel, 0, 16);
        dspfunc(avg_h264_qpel, 0, 16);

        dspfunc(put_h264_qpel, 1, 8);
        dspfunc(avg_h264_qpel, 1, 8);
#undef dspfunc
    }
}
