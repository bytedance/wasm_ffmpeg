# Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
# SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

BASE_DIR=$(cd `dirname $0`; pwd)
echo ${BASE_DIR}

LIBVPX=libvpx-1.10.0

if [ -d ./${LIBVPX} ];then 
    rm -rf ${LIBVPX}
fi

git clone git@code.byted.org:miaoyuqiao/libvpx.git ${LIBVPX}

cd ${BASE_DIR}/${LIBVPX}/

AR=emar
CC=emcc
ranlib=emranlib

emconfigure ./configure \
--prefix=${BASE_DIR}/${LIBVPX}/libvpx_install \
--enable-static \
--disable-shared \
--disable-examples \
--disable-tools \
--disable-unit-tests \
--target=generic-gnu

emmake make clean
emmake make -j8
emmake make install

if [ -f libvpx_emcc.a ];then
    rm -f libvpx_emcc.a
fi

VPX_PATH=./

emar scr libvpx_emcc.a \
./${VPX_PATH}/*.o \
./${VPX_PATH}/vp8/*.o \
./${VPX_PATH}/vp8/common/*.o \
./${VPX_PATH}/vp8/common/generic/*.o \
./${VPX_PATH}/vp8/decoder/*.o \
./${VPX_PATH}/vp8/encoder/*.o \
./${VPX_PATH}/vp9/*.o \
./${VPX_PATH}/vp9/common/*.o \
./${VPX_PATH}/vp9/decoder/*.o \
./${VPX_PATH}/vp9/encoder/*.o \
./${VPX_PATH}/vpx/src/*.o \
./${VPX_PATH}/vpx_dsp/*.o \
./${VPX_PATH}/vpx_mem/*.o \
./${VPX_PATH}/vpx_scale/*.o \
./${VPX_PATH}/vpx_scale/generic/*.o \
./${VPX_PATH}/vpx_util/*.o

cd ${BASE_DIR}/${LIBVPX}/libvpx_install/lib/

if [ -f libvpx.a ];then
    rm -f libvpx.a
fi

cd ${BASE_DIR}

cp ${LIBVPX}/libvpx_emcc.a ${LIBVPX}/libvpx_install/lib/libvpx.a

cd ${BASE_DIR}

if [ -d ./libvpx_install ];then
    rm -r libvpx_install
fi

cp -r ${LIBVPX}/libvpx_install libvpx_install