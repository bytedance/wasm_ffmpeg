
// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#include "config.h"

#include "libavutil/attributes.h"
#include "libavutil/cpu.h"
#include "libavcodec/hpeldsp.h"
#include "hpeldsp_wasm.h"
#include <wasm_simd128.h>

#define unaligned_load(a,b) wasm_v128_load((uint8_t *)b + a)

/* next one assumes that ((line_size % 16) == 0) */
void ff_put_pixels16_wasm(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
{
    register v128_t pixelsv1;
    register v128_t pixelsv1B;
    register v128_t pixelsv1C;
    register v128_t pixelsv1D;

    int i;
    register ptrdiff_t line_size_2 = line_size << 1;
    register ptrdiff_t line_size_3 = line_size + line_size_2;
    register ptrdiff_t line_size_4 = line_size << 2;

// hand-unrolling the loop by 4 gains about 15%
// mininum execution time goes from 74 to 60 cycles
// it's faster than -funroll-loops, but using
// -funroll-loops w/ this is bad - 74 cycles again.
// all this is on a 7450, tuning for the 7450
    for (i = 0; i < h; i += 4) {
        pixelsv1  = unaligned_load( 0, pixels);
        pixelsv1B = unaligned_load(line_size, pixels);
        pixelsv1C = unaligned_load(line_size_2, pixels);
        pixelsv1D = unaligned_load(line_size_3, pixels);
        wasm_v128_store((unsigned char*)block, pixelsv1);
        wasm_v128_store((unsigned char*)block + line_size, pixelsv1B);
        wasm_v128_store((unsigned char*)block + line_size_2, pixelsv1C);
        wasm_v128_store((unsigned char*)block + line_size_3, pixelsv1D);
        pixels+=line_size_4;
        block +=line_size_4;
    }
}

/* next one assumes that ((line_size % 16) == 0) */
#define op_avg(a,b)  a = ( ((a)|(b)) - ((((a)^(b))&0xFEFEFEFEUL)>>1) )
void ff_avg_pixels16_wasm(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
{
    register v128_t pixelsv, blockv;

    int i;
    for (i = 0; i < h; i++) {
        blockv = wasm_v128_load(block);
        pixelsv = wasm_v128_load(pixels);
        blockv = wasm_u8x16_avgr(blockv, pixelsv);
        wasm_v128_store((unsigned char*)block, blockv);
        pixels+=line_size;
        block +=line_size;
    }
}


/* next one assumes that ((line_size % 8) == 0) */
void ff_avg_pixels8_wasm(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
{
    int i;
    for (i = 0; i < h; i++)
    {
        v128_t src = wasm_u16x8_extend_low_u8x16(wasm_v128_load(pixels));
        v128_t dst = wasm_u16x8_extend_low_u8x16(wasm_v128_load(block));
        dst = wasm_u16x8_avgr(src, dst);
        dst = wasm_u8x16_narrow_i16x8(dst, dst);
        *(int64_t*)(block) = wasm_i64x2_extract_lane(dst, 0);

        pixels += line_size;
        block += line_size;
    }
}

void ff_put_pixels8_wasm(uint8_t * block, const uint8_t * pixels, ptrdiff_t line_size, int h)
{
    int i;
    for (i = 0; i < h; i++)
    {
        *(int64_t*)(block) = *(int64_t*)(pixels);

        pixels += line_size;
        block += line_size;
    }
}

// TODO 以下

// /* next one assumes that ((line_size % 8) == 0) */
// static void put_pixels8_xy2_altivec(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
// {
//     register int i;
//     register vector unsigned char pixelsv1, pixelsv2, pixelsavg;
//     register vector unsigned char blockv;
//     register vector unsigned short pixelssum1, pixelssum2, temp3;
//     register const vector unsigned char vczero = (const vector unsigned char)vec_splat_u8(0);
//     register const vector unsigned short vctwo = (const vector unsigned short)vec_splat_u16(2);

//     pixelsv1 = VEC_LD(0, pixels);
//     pixelsv2 = VEC_LD(1, pixels);
//     pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//     pixelsv2 = VEC_MERGEH(vczero, pixelsv2);

//     pixelssum1 = vec_add((vector unsigned short)pixelsv1,
//                          (vector unsigned short)pixelsv2);
//     pixelssum1 = vec_add(pixelssum1, vctwo);

//     for (i = 0; i < h ; i++) {
//         int rightside = ((unsigned long)block & 0x0000000F);
//         blockv = vec_ld(0, block);

//         pixelsv1 = unaligned_load(line_size, pixels);
//         pixelsv2 = unaligned_load(line_size+1, pixels);
//         pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//         pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//         pixelssum2 = vec_add((vector unsigned short)pixelsv1,
//                              (vector unsigned short)pixelsv2);
//         temp3 = vec_add(pixelssum1, pixelssum2);
//         temp3 = vec_sra(temp3, vctwo);
//         pixelssum1 = vec_add(pixelssum2, vctwo);
//         pixelsavg = vec_packsu(temp3, (vector unsigned short) vczero);

//         if (rightside) {
//             blockv = vec_perm(blockv, pixelsavg, vcprm(0, 1, s0, s1));
//         } else {
//             blockv = vec_perm(blockv, pixelsavg, vcprm(s0, s1, 2, 3));
//         }

//         vec_st(blockv, 0, block);

//         block += line_size;
//         pixels += line_size;
//     }
// }

// /* next one assumes that ((line_size % 8) == 0) */
// static void put_no_rnd_pixels8_xy2_altivec(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
// {
//     register int i;
//     register vector unsigned char pixelsv1, pixelsv2, pixelsavg;
//     register vector unsigned char blockv;
//     register vector unsigned short pixelssum1, pixelssum2, temp3;
//     register const vector unsigned char vczero = (const vector unsigned char)vec_splat_u8(0);
//     register const vector unsigned short vcone = (const vector unsigned short)vec_splat_u16(1);
//     register const vector unsigned short vctwo = (const vector unsigned short)vec_splat_u16(2);

//     pixelsv1 = VEC_LD(0, pixels);
//     pixelsv2 = VEC_LD(1, pixels);
//     pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//     pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//     pixelssum1 = vec_add((vector unsigned short)pixelsv1,
//                          (vector unsigned short)pixelsv2);
//     pixelssum1 = vec_add(pixelssum1, vcone);

//     for (i = 0; i < h ; i++) {
//         int rightside = ((unsigned long)block & 0x0000000F);
//         blockv = vec_ld(0, block);

//         pixelsv1 = unaligned_load(line_size, pixels);
//         pixelsv2 = unaligned_load(line_size+1, pixels);
//         pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//         pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//         pixelssum2 = vec_add((vector unsigned short)pixelsv1,
//                              (vector unsigned short)pixelsv2);
//         temp3 = vec_add(pixelssum1, pixelssum2);
//         temp3 = vec_sra(temp3, vctwo);
//         pixelssum1 = vec_add(pixelssum2, vcone);
//         pixelsavg = vec_packsu(temp3, (vector unsigned short) vczero);

//         if (rightside) {
//             blockv = vec_perm(blockv, pixelsavg, vcprm(0, 1, s0, s1));
//         } else {
//             blockv = vec_perm(blockv, pixelsavg, vcprm(s0, s1, 2, 3));
//         }

//         vec_st(blockv, 0, block);

//         block += line_size;
//         pixels += line_size;
//     }
// }

// /* next one assumes that ((line_size % 16) == 0) */
// static void put_pixels16_xy2_altivec(uint8_t * block, const uint8_t * pixels, ptrdiff_t line_size, int h)
// {
//     register int i;
//     register vector unsigned char pixelsv1, pixelsv2, pixelsv3, pixelsv4;
//     register vector unsigned char blockv;
//     register vector unsigned short temp3, temp4,
//         pixelssum1, pixelssum2, pixelssum3, pixelssum4;
//     register const vector unsigned char vczero = (const vector unsigned char)vec_splat_u8(0);
//     register const vector unsigned short vctwo = (const vector unsigned short)vec_splat_u16(2);

//     pixelsv1 = VEC_LD(0, pixels);
//     pixelsv2 = VEC_LD(1, pixels);
//     pixelsv3 = VEC_MERGEL(vczero, pixelsv1);
//     pixelsv4 = VEC_MERGEL(vczero, pixelsv2);
//     pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//     pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//     pixelssum3 = vec_add((vector unsigned short)pixelsv3,
//                          (vector unsigned short)pixelsv4);
//     pixelssum3 = vec_add(pixelssum3, vctwo);
//     pixelssum1 = vec_add((vector unsigned short)pixelsv1,
//                          (vector unsigned short)pixelsv2);
//     pixelssum1 = vec_add(pixelssum1, vctwo);

//     for (i = 0; i < h ; i++) {
//         blockv = vec_ld(0, block);

//         pixelsv1 = unaligned_load(line_size, pixels);
//         pixelsv2 = unaligned_load(line_size+1, pixels);

//         pixelsv3 = VEC_MERGEL(vczero, pixelsv1);
//         pixelsv4 = VEC_MERGEL(vczero, pixelsv2);
//         pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//         pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//         pixelssum4 = vec_add((vector unsigned short)pixelsv3,
//                              (vector unsigned short)pixelsv4);
//         pixelssum2 = vec_add((vector unsigned short)pixelsv1,
//                              (vector unsigned short)pixelsv2);
//         temp4 = vec_add(pixelssum3, pixelssum4);
//         temp4 = vec_sra(temp4, vctwo);
//         temp3 = vec_add(pixelssum1, pixelssum2);
//         temp3 = vec_sra(temp3, vctwo);

//         pixelssum3 = vec_add(pixelssum4, vctwo);
//         pixelssum1 = vec_add(pixelssum2, vctwo);

//         blockv = vec_packsu(temp3, temp4);

//         vec_st(blockv, 0, block);

//         block += line_size;
//         pixels += line_size;
//     }
// }

// /* next one assumes that ((line_size % 16) == 0) */
// static void put_no_rnd_pixels16_xy2_altivec(uint8_t * block, const uint8_t * pixels, ptrdiff_t line_size, int h)
// {
//     register int i;
//     register vector unsigned char pixelsv1, pixelsv2, pixelsv3, pixelsv4;
//     register vector unsigned char blockv;
//     register vector unsigned short temp3, temp4,
//         pixelssum1, pixelssum2, pixelssum3, pixelssum4;
//     register const vector unsigned char vczero = (const vector unsigned char)vec_splat_u8(0);
//     register const vector unsigned short vcone = (const vector unsigned short)vec_splat_u16(1);
//     register const vector unsigned short vctwo = (const vector unsigned short)vec_splat_u16(2);

//     pixelsv1 = VEC_LD(0, pixels);
//     pixelsv2 = VEC_LD(1, pixels);
//     pixelsv3 = VEC_MERGEL(vczero, pixelsv1);
//     pixelsv4 = VEC_MERGEL(vczero, pixelsv2);
//     pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//     pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//     pixelssum3 = vec_add((vector unsigned short)pixelsv3,
//                          (vector unsigned short)pixelsv4);
//     pixelssum3 = vec_add(pixelssum3, vcone);
//     pixelssum1 = vec_add((vector unsigned short)pixelsv1,
//                          (vector unsigned short)pixelsv2);
//     pixelssum1 = vec_add(pixelssum1, vcone);

//     for (i = 0; i < h ; i++) {
//         pixelsv1 = unaligned_load(line_size, pixels);
//         pixelsv2 = unaligned_load(line_size+1, pixels);

//         pixelsv3 = VEC_MERGEL(vczero, pixelsv1);
//         pixelsv4 = VEC_MERGEL(vczero, pixelsv2);
//         pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//         pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//         pixelssum4 = vec_add((vector unsigned short)pixelsv3,
//                              (vector unsigned short)pixelsv4);
//         pixelssum2 = vec_add((vector unsigned short)pixelsv1,
//                              (vector unsigned short)pixelsv2);
//         temp4 = vec_add(pixelssum3, pixelssum4);
//         temp4 = vec_sra(temp4, vctwo);
//         temp3 = vec_add(pixelssum1, pixelssum2);
//         temp3 = vec_sra(temp3, vctwo);

//         pixelssum3 = vec_add(pixelssum4, vcone);
//         pixelssum1 = vec_add(pixelssum2, vcone);

//         blockv = vec_packsu(temp3, temp4);

//         VEC_ST(blockv, 0, block);

//         block += line_size;
//         pixels += line_size;
//     }
// }

// /* next one assumes that ((line_size % 8) == 0) */
// static void avg_pixels8_xy2_altivec(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
// {
//     register int i;
//     register vector unsigned char pixelsv1, pixelsv2, pixelsavg;
//     register vector unsigned char blockv, blocktemp;
//     register vector unsigned short pixelssum1, pixelssum2, temp3;

//     register const vector unsigned char vczero = (const vector unsigned char)
//                                         vec_splat_u8(0);
//     register const vector unsigned short vctwo = (const vector unsigned short)
//                                         vec_splat_u16(2);

//     pixelsv1 = VEC_LD(0, pixels);
//     pixelsv2 = VEC_LD(1, pixels);
//     pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//     pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//     pixelssum1 = vec_add((vector unsigned short)pixelsv1,
//                          (vector unsigned short)pixelsv2);
//     pixelssum1 = vec_add(pixelssum1, vctwo);

//     for (i = 0; i < h ; i++) {
//         int rightside = ((unsigned long)block & 0x0000000F);
//         blockv = vec_ld(0, block);

//         pixelsv1 = unaligned_load(line_size, pixels);
//         pixelsv2 = unaligned_load(line_size+1, pixels);

//         pixelsv1 = VEC_MERGEH(vczero, pixelsv1);
//         pixelsv2 = VEC_MERGEH(vczero, pixelsv2);
//         pixelssum2 = vec_add((vector unsigned short)pixelsv1,
//                              (vector unsigned short)pixelsv2);
//         temp3 = vec_add(pixelssum1, pixelssum2);
//         temp3 = vec_sra(temp3, vctwo);
//         pixelssum1 = vec_add(pixelssum2, vctwo);
//         pixelsavg = vec_packsu(temp3, (vector unsigned short) vczero);

//         if (rightside) {
//             blocktemp = vec_perm(blockv, pixelsavg, vcprm(0, 1, s0, s1));
//         } else {
//             blocktemp = vec_perm(blockv, pixelsavg, vcprm(s0, s1, 2, 3));
//         }

//         blockv = vec_avg(blocktemp, blockv);
//         vec_st(blockv, 0, block);

//         block += line_size;
//         pixels += line_size;
//     }
// }


av_cold void ff_hpeldsp_init_ppc(HpelDSPContext *c, int flags)
{

    c->avg_pixels_tab[0][0]        = ff_avg_pixels16_wasm;
    c->avg_pixels_tab[1][0]        = ff_avg_pixels8_wasm;
    // c->avg_pixels_tab[1][3]        = avg_pixels8_xy2_altivec;

    c->put_pixels_tab[0][0]        = ff_put_pixels16_wasm;
    // c->put_pixels_tab[1][3]        = put_pixels8_xy2_altivec;
    // c->put_pixels_tab[0][3]        = put_pixels16_xy2_altivec;

    c->put_no_rnd_pixels_tab[0][0] = ff_put_pixels16_wasm;
    // c->put_no_rnd_pixels_tab[1][3] = put_no_rnd_pixels8_xy2_altivec;
    // c->put_no_rnd_pixels_tab[0][3] = put_no_rnd_pixels16_xy2_altivec;
}
