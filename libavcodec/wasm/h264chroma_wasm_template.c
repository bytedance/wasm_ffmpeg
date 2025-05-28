// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later


#include "libavutil/mem.h"
#include <wasm_simd128.h>
#include "libavcodec/bit_depth_template.c"


#ifdef PREFIX_h264_chroma_mc4_wasm
static void PREFIX_h264_chroma_mc4_wasm(uint8_t *_dst /*align 8*/, uint8_t *_src /*align 1*/, ptrdiff_t stride, int h, int x, int y)\
{
    pixel *dst = (pixel*)_dst;
    pixel *src = (pixel*)_src;
    const int A=(8-x)*(8-y);
    const int B=(  x)*(8-y);
    const int C=(8-x)*(  y);
    const int D=(  x)*(  y);
    int i;

    v128_t vA = wasm_i16x8_splat(A);
    v128_t vB = wasm_i16x8_splat(B);
    v128_t vC = wasm_i16x8_splat(C);
    v128_t vD = wasm_i16x8_splat(D);
    const v128_t vc32 = wasm_i16x8_const(32, 32, 32, 32, 32, 32, 32, 32);
    if(D){
        for(i=0; i<h; i++){
            v128_t vsrc0 = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat(src));
            v128_t vsrc1 = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat((1 + src)));
            v128_t vsrcs0 = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat((stride + src)));
            v128_t vsrcs1 = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat((stride + 1 + src)));
            v128_t vtmp = wasm_i16x8_add(
                wasm_i16x8_add(
                    wasm_i16x8_mul(vsrc0, vA),
                    wasm_i16x8_mul(vsrc1, vB)
                ),
                wasm_i16x8_add(
                    wasm_i16x8_mul(vsrcs0, vC),
                    wasm_i16x8_mul(vsrcs1, vD)
                )
            );
            
            v128_t vdst = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat(dst));
            vdst = OP_U8_WASM(vdst, vtmp);
            *(int32_t *) dst = wasm_i32x4_extract_lane(vdst, 0);

            dst+= stride;
            src+= stride;
        }
    } else if (B + C) {
        v128_t vE = wasm_i16x8_add(vB, vC);
        const ptrdiff_t step = C ? stride : 1;
        for(i=0; i<h; i++){
            v128_t vsrc0 = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat(src));
            v128_t vsrcs0 = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat((step + src)));
            v128_t vtmp = wasm_i16x8_add(wasm_i16x8_mul(vsrc0, vA), wasm_i16x8_mul(vsrcs0, vE));
            
            v128_t vdst = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat(dst));
            vdst = OP_U8_WASM(vdst, vtmp);
            *(int32_t *) dst = wasm_i32x4_extract_lane(vdst, 0);

            dst+= stride;
            src+= stride;
        }
    } else {
        for ( i = 0; i < h; i++){
            v128_t vsrc0 = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat(src));
            v128_t vtmp = wasm_i16x8_mul(vsrc0, vA);

            v128_t vdst = wasm_i16x8_widen_low_u8x16(wasm_v32x4_load_splat(dst));
            vdst = OP_U8_WASM(vdst, vtmp);
            *(int32_t *) dst = wasm_i32x4_extract_lane(vdst, 0);
            dst += stride;
            src += stride;
        }
    }
}
#endif


#ifdef PREFIX_h264_chroma_mc8_wasm
static void PREFIX_h264_chroma_mc8_wasm(uint8_t *_dst /*align 8*/, uint8_t *_src /*align 1*/, ptrdiff_t stride, int h, int x, int y) 
{
    pixel *dst = (pixel*)_dst;
    pixel *src = (pixel*)_src;
    const int A=(8-x)*(8-y);
    const int B=(  x)*(8-y);
    const int C=(8-x)*(  y);
    const int D=(  x)*(  y);
    int i;

    v128_t vA = wasm_i16x8_splat(A);
    v128_t vB = wasm_i16x8_splat(B);
    v128_t vC = wasm_i16x8_splat(C);
    v128_t vD = wasm_i16x8_splat(D);
    const v128_t vc32 = wasm_i16x8_const(32, 32, 32, 32, 32, 32, 32, 32);
    if(D){\
        for(i=0; i<h; i++){\
            v128_t vsrc0 = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(src));
            v128_t vsrc1 = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(1 + src));
            v128_t vsrcs0 = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(stride + src));
            v128_t vsrcs1 = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(stride + 1 + src));
            v128_t vtmp = wasm_i16x8_add(
                wasm_i16x8_add(
                    wasm_i16x8_mul(vsrc0, vA),
                    wasm_i16x8_mul(vsrc1, vB)
                ),
                wasm_i16x8_add(
                    wasm_i16x8_mul(vsrcs0, vC),
                    wasm_i16x8_mul(vsrcs1, vD)
                )
            );

            v128_t vdst = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(dst));
            vdst = OP_U8_WASM(vdst, vtmp);
            *(int64_t *) dst = wasm_i64x2_extract_lane(vdst, 0);
            dst+= stride;
            src+= stride;
        }
    } else if (B + C) {\
        v128_t vE = wasm_i16x8_add(vB, vC);
        const ptrdiff_t step = C ? stride : 1;
        for(i=0; i<h; i++){
            v128_t vsrc0 = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(src));
            v128_t vsrcs0 = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(step + src));
            v128_t vtmp = wasm_i16x8_add(wasm_i16x8_mul(vsrc0, vA), wasm_i16x8_mul(vsrcs0, vE));
            
            v128_t vdst = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(dst));
            vdst = OP_U8_WASM(vdst, vtmp);
            *(int64_t *) dst = wasm_i64x2_extract_lane(vdst, 0);

            dst+= stride;
            src+= stride;
        }
    } else {
        for ( i = 0; i < h; i++){
            v128_t vsrc0 = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(src));
            v128_t vtmp = wasm_i16x8_mul(vsrc0, vA);

            v128_t vdst = wasm_i16x8_widen_low_u8x16(wasm_v64x2_load_splat(dst));
            vdst = OP_U8_WASM(vdst, vtmp);
            *(int64_t *) dst = wasm_i64x2_extract_lane(vdst, 0);
            dst += stride;
            src += stride;
        }
    }
}
#endif

/* this code assume that stride % 16 == 0 */
#ifdef PREFIX_no_rnd_vc1_chroma_mc8_wasm
static void PREFIX_no_rnd_vc1_chroma_mc8_wasm(uint8_t *dst, uint8_t *src,
                                                 ptrdiff_t stride, int h,
                                                 int x, int y)
{
   DECLARE_ALIGNED(16, signed int, ABCD)[4] =
                        {((8 - x) * (8 - y)),
                         ((    x) * (8 - y)),
                         ((8 - x) * (    y)),
                         ((    x) * (    y))};
    register int i;
    v128_t fperm;
    LOAD_ZERO;
    const v128_t vABCD = wasm_v128_load(ABCD);
    const v128_t vA = wasm_i16x8_splat(wasm_i16x8_extract_lane(vABCD, 0));
    const v128_t vB = wasm_i16x8_splat(wasm_i16x8_extract_lane(vABCD, 2));
    const v128_t vC = wasm_i16x8_splat(wasm_i16x8_extract_lane(vABCD, 4));
    const v128_t vD = wasm_i16x8_splat(wasm_i16x8_extract_lane(vABCD, 6));
    const v128_t v28ss = wasm_i16x8_const_splat(28);
    const v128_t v6us  = wasm_u16x8_const_splat(6);

    v128_t vsrcperm0, vsrcperm1;
    v128_t vsrc0uc, vsrc1uc;
    v128_t vsrc0ssH, vsrc1ssH;
    v128_t vsrc2uc, vsrc3uc;
    v128_t vsrc2ssH, vsrc3ssH, psum;
    v128_t vdst, ppsum, vfdst, fsum;
    GET_VSRC(vsrc0uc, vsrc1uc, 0, 16, vsrcperm0, vsrcperm1, src);

    vsrc0ssH = (v128_t)WASM_MERGEH(zerov, (v128_t)vsrc0uc);
    vsrc1ssH = (v128_t)WASM_MERGEH(zerov, (v128_t)vsrc1uc);

    for (i = 0 ; i < h ; i++) {
        GET_VSRC(vsrc2uc, vsrc3uc, stride, 16, vsrcperm0, vsrcperm1, src);
        CHROMA_MC8_WASM_CORE(wasm_i16x8_const_splat(0), add28);
    }
}
#endif

#undef noop
#undef add28
#undef CHROMA_MC8_WASM_CORE
