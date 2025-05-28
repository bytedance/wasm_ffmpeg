# Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
# SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#bin/bash
set -ex

#帮助 
# 
# bash build_wasm.sh 
#           --simd-mode=on/off/both 
#           --configure
# bash build_wasm.sh --simd-mode=on (如果simd-mode没有变动，可以不需要重新reconfigure，但是both必须reconfigure，会自动添加）


BASE_DIR="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"


increment_compile=0
for i in $@; do
    [[ ${i} == "-h" ]] && is_help=true
    [[ ${i} == "--configure" ]] && increment_compile=1
    [[ ${i} == debug ]] && build_type=debug
    [[ ${i} == release ]] && build_type=release
    [[ ${i} == fast_debug ]] && build_type=fast_debug
    [[ ${i} == "--simd-mode="* ]] && simd_mode="${i#*=}"
done



# find . -name *.o | xargs -L 1 -I {} rm {}

function build_ffmpeg() {
    enable_simd=$1
    reconfig=$2
    echo "enable_simd=$1, reconfig=$2"
    if [[ "${enable_simd}" == "1" ]];then
        SIMD_FLAG="-msimd128"
        ENABLE_WASMSIMD="--enable-wasmsimd"
        SIMD_OBJECTS=./libavcodec/wasm/*.o
    else
        SIMD_FLAG=""
        ENABLE_WASMSIMD=""
        SIMD_OBJECTS=""
    fi

    exportFunctions=$BASE_DIR/export.txt
    DYNAMIC_LINK_CONFIG="-fPIC \
                    -fvisibility=hidden
                    -Wl,--error-limit=0 \
                    -sSIDE_MODULE=2 \
                    -sAUTOLOAD_DYLIBS=1 \
                    -sUSE_PTHREADS=1 \
                    -sWASM_BIGINT \
                    -sERROR_ON_WASM_CHANGES_AFTER_LINK \
                    -s EXPORTED_FUNCTIONS="@$exportFunctions" \
    "

    if [ "$reconfig" == "1" ]; then
        emconfigure ./configure \
        --cc="emcc" \
        --cxx="em++" \
        --ar="emar" \
        --objcc=emcc \
        --dep-cc=emcc \
        --ranlib="emranlib" \
        --disable-inline-asm \
        --disable-gpl \
        --nm="$EMSDK/upstream/bin/llvm-nm -g" \
        --prefix=$BASE_DIR/dist \
        --enable-cross-compile \
        --target-os=none \
        --arch=x86_64 \
        --cpu=generic \
        --disable-programs \
        --disable-asm \
        --disable-doc \
        --disable-avdevice \
        --disable-w32threads \
        --enable-network \
        --disable-hwaccels \
        --disable-parsers \
        --disable-bsfs \
        --disable-debug \
        --disable-protocols \
        --disable-indevs \
        --disable-sdl2 \
        --disable-outdevs \
        --disable-postproc \
        --enable-protocol=file \
        --disable-stripping \
        --pkg-config-flags="--static" \
        --disable-shared \
        --enable-static \
        --extra-libs='-static' \
        --enable-nonfree \
        --enable-cross-compile \
        --disable-programs \
        --disable-asm \
        --disable-doc \
        --disable-avdevice \
        --disable-w32threads \
        --disable-network \
        --disable-hwaccels \
        --disable-parsers \
        --disable-bsfs \
        --disable-debug \
        --disable-protocols \
        --disable-indevs \
        --disable-sdl2 \
        --disable-outdevs \
        --disable-postproc \
        --enable-logging \
        --disable-indevs \
        --disable-outdevs \
        --disable-symver \
        --disable-programs \
        --disable-ffmpeg \
        --disable-ffplay \
        --disable-ffprobe \
        --disable-ffserver \
        --disable-doc \
        --disable-htmlpages \
        --disable-manpages \
        --disable-podpages \
        --disable-txtpages \
        --disable-avdevice \
        --enable-avcodec \
        --enable-avformat \
        --enable-avutil \
        --enable-swresample \
        --enable-swscale \
        --disable-postproc \
        --enable-avfilter \
        --enable-avresample \
        --enable-pthreads \
        --disable-decoders \
        --enable-decoder=opus \
        --enable-decoder=aac \
        --enable-decoder=ac3 \
        --enable-decoder=eac3 \
        --enable-decoder=dca \
        --enable-decoder=truehd \
        --enable-decoder=aac_latm \
        --enable-decoder=flv \
        --enable-decoder=h264 \
        --enable-decoder=mpeg4 \
        --enable-decoder='mp3*' \
        --enable-decoder=flac \
        --enable-decoder=rawvideo \
        --enable-decoder='pcm*' \
        --enable-decoder=qtrle \
        --enable-decoder=h263 \
        --enable-decoder=mpeg4 \
        --enable-decoder=amrnb \
        --enable-decoder=amrwb \
        --enable-decoder=vorbis \
        --disable-decoder=hevc \
        --disable-d3d11va \
        --disable-dxva2 \
        --disable-vaapi \
        --disable-vda \
        --disable-vdpau \
        --disable-videotoolbox \
        --disable-encoders \
        --disable-hwaccels \
        --disable-muxers \
        --disable-demuxers \
        --enable-demuxer=ogg \
        --enable-demuxer=aac \
        --enable-demuxer=concat \
        --enable-demuxer=data \
        --enable-demuxer=flv \
        --enable-demuxer=hls \
        --enable-demuxer=live_flv \
        --enable-demuxer=mov \
        --enable-demuxer=mpegps \
        --enable-demuxer=mpegts \
        --enable-demuxer=mpegvideo \
        --enable-demuxer=mpegtsraw \
        --enable-demuxer=rawvideo \
        --enable-demuxer=wav \
        --enable-demuxer=h264 \
        --enable-demuxer=flac \
        --enable-demuxer='pcm*' \
        --enable-demuxer=mp3 \
        --enable-demuxer=image2 \
        --enable-demuxer=matroska \
        --enable-demuxer=avi \
        --disable-parsers \
        --enable-parser=aac \
        --enable-parser=aac_latm \
        --enable-parser=h264 \
        --enable-parser='mpeg*' \
        --enable-parser=mjpeg \
        --enable-bsfs \
        --disable-bsf=text2movsub \
        --disable-bsf=mjpeg2jpeg \
        --disable-bsf=mjpega_dump_header \
        --disable-bsf=mov2textsub \
        --disable-bsf=imx_dump_header \
        --disable-bsf=chomp \
        --disable-bsf=noise \
        --disable-bsf=dump_extradata \
        --disable-bsf=remove_extradata \
        --disable-bsf=dca_core \
        --disable-protocols \
        --enable-protocol=tejshttp \
        --enable-protocol=file \
        --enable-protocol=http \
        --enable-protocol='tls*' \
        --enable-protocol='http*' \
        --enable-protocol=tcp \
        --enable-protocol=crypto \
        --disable-devices \
        --disable-filters \
        --enable-filter=amix \
        --enable-filter=aformat \
        --enable-filter=scale \
        --enable-filter=format \
        --enable-filter=aformat \
        --enable-filter=fps \
        --enable-filter=trim \
        --enable-filter=atrim \
        --enable-filter=vflip \
        --enable-filter=hflip \
        --enable-filter=transpose \
        --enable-filter=rotate \
        --enable-filter=yadif \
        --enable-filter=pan \
        --enable-filter=volume \
        --enable-filter=aresample \
        --enable-filter=atempo \
        --enable-filter=asetrate \
        --enable-filter=setpts \
        --enable-filter=overlay \
        --enable-filter=paletteuse \
        --enable-filter=areverse \
        --enable-filter=anull \
        --enable-filter=palettegen \
        --enable-filter=null \
        --disable-iconv \
        --disable-audiotoolbox \
        --disable-videotoolbox \
        --disable-armv5te \
        --disable-armv6 \
        --disable-armv6t2 \
        --enable-optimizations \
        --enable-small \
        ${ENABLE_WASMSIMD} \
        --extra-cflags="--static -flto -Wno-int-conversion -g2 -Oz ${SIMD_FLAG}"
        
        emmake make clean
        
    fi


    #--disable-pthreads \
    # emmake make clean
    emmake make -j12
    # make install

    VPX_PATH=libvpx-1.10.0
    if [[ "${enable_simd}" == "1" ]];then
        file_suffix=_simd
    else
        file_suffix=""
    fi
    rm -f libffmpeg${file_suffix}.a
    emar scr libffmpeg${file_suffix}.a ./libswresample/*.o ./libavfilter/*.o ./libavformat/*.o ./libswscale/*.o ./libavutil/*.o ./libavcodec/*.o ${SIMD_OBJECTS} 

    emcc $DYNAMIC_LINK_CONFIG libffmpeg${file_suffix}.a -o libffmpeg${file_suffix}.wasm

    make install
    cp libffmpeg${file_suffix}.a ./dist/lib/
    cp libffmpeg${file_suffix}.wasm ./dist/lib/
    cp config.h ./dist/include/config_ff${file_suffix}.h
    cp config_tmp.h ./dist/include/config.h
}

if [[ "${simd_mode}" == "on" ]];then
    build_ffmpeg 1 ${increment_compile}
elif [[ "${simd_mode}" == "off" ]];then
    build_ffmpeg 0 ${increment_compile}
elif [[ "${simd_mode}" == "both" ]];then
    build_ffmpeg 0 1
    build_ffmpeg 1 1
fi 

if [[ -f copy_dist.sh ]];then
    bash copy_dist.sh
else
    echo "copy_dist.sh not found. Ignoring copy"
fi
