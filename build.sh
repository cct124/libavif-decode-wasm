export LIBAVIF_VERSION=1.0.4
export WORK_PWD=${PWD}

export REP_DAV1D="https://code.videolan.org/videolan/dav1d.git"
export REP_LIBYUV="https://chromium.googlesource.com/libyuv/libyuv"
export PROJECT_NAME="avifDecodeFileWeb"

export REP_LIBGAV1="https://chromium.googlesource.com/codecs/libgav1"
export REP_LIBGAV1_VERSION="0.19.0"
export DECODER="$1"
export DAV1D_FLAG="dav1d"
export GAV1_FLAG="gav1"

if [ ! -e "v${LIBAVIF_VERSION}.tar.gz" ]; then
    wget https://github.com/AOMediaCodec/libavif/archive/refs/tags/v${LIBAVIF_VERSION}.tar.gz
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}" ]; then
    tar xzf v${LIBAVIF_VERSION}.tar.gz
fi

if [ "$DECODER" = "$GAV1_FLAG" ]; then
    if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/dav1d" ]; then
        git clone -b 1.2.1 --depth 1 ${REP_DAV1D} ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/dav1d
    fi

    if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/dav1d/build" ]; then
        cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/dav1d
        mkdir build
        cd build
        meson setup --default-library=static --buildtype release .. --cross-file ${WORK_PWD}/cross_emscripten.ini
        ninja
    fi
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/libyuv" ]; then
    git clone --single-branch ${REP_LIBYUV} ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libyuv
    cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libyuv
    git checkout 464c51a0
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/libyuv/build" ]; then
    cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libyuv
    emcmake cmake -S . -B build
    make -C build
fi

if [ "$DECODER" = "$GAV1_FLAG" ]; then

    if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/libgav1" ]; then
        git clone -b v${REP_LIBGAV1_VERSION} --depth 1 ${REP_LIBGAV1} ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libgav1
        git clone -b 20220623.0 --depth 1 https://github.com/abseil/abseil-cpp.git ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libgav1/third_party/abseil-cpp
    fi

    rm -rf "libavif-${LIBAVIF_VERSION}/ext/libgav1/build"
    if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/libgav1/build" ]; then
        cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libgav1
        mkdir build
        cd build
        emcmake cmake -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DLIBGAV1_THREADPOOL_USE_STD_MUTEX=1 \
            -DLIBGAV1_ENABLE_EXAMPLES=0 \
            -DLIBGAV1_ENABLE_TESTS=0 \
            -DLIBGAV1_MAX_BITDEPTH=12 \
            -DLIBGAV1_ENABLE_AVX2=0 \
            -DLIBGAV1_ENABLE_SSE4_1=0 \
            ..
        ninja
    fi

fi

if [ "$DECODER" != "$DAV1D_FLAG" ] && [ "$DECODER" != "$GAV1_FLAG" ]; then
    echo "Unknown decoder. Please set DECODER to either 'dav1d' or 'gav1'."
    exit 1
fi

# 默认情况下，两个解码器均设置为OFF
DAV1D_STATUS="OFF"
GAV1_STATUS="OFF"
LIBAVIF_BUILD="dav1d_build"

# 根据DECODER的值设置对应的解码器状态为ON
if [ "$DECODER" = "$DAV1D_FLAG" ]; then
    DAV1D_STATUS="ON"
    LIBAVIF_BUILD="dav1d_build"
elif [ "$DECODER" = "$GAV1_FLAG" ]; then
    GAV1_STATUS="ON"
    LIBAVIF_BUILD="gav1_build"
else
    echo "Unknown decoder. Please set DECODER to either 'dav1d' or 'gav1'."
    exit 1
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/${LIBAVIF_BUILD}" ]; then
    cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}
    emcmake cmake -S . -B $LIBAVIF_BUILD \
        -DBUILD_SHARED_LIBS=OFF \
        -DAVIF_CODEC_DAV1D=$DAV1D_STATUS \
        -DAVIF_LOCAL_DAV1D=$DAV1D_STATUS \
        -DAVIF_CODEC_LIBGAV1=$GAV1_STATUS \
        -DAVIF_LOCAL_LIBGAV1=$GAV1_STATUS \
        -DAVIF_LOCAL_LIBYUV=ON
    make -C $LIBAVIF_BUILD
    cd ..
fi

cd ${WORK_PWD}
rm -rf build
emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S . -B build
make -C build

if [ ! -d "lib" ]; then
    mkdir lib
fi

export EXPORTED_RUNTIME_METHODS="[ \
    'ccall', \
    'getValue', \
    'cwrap' \
]"

export EXPORTED_FUNCTIONS="[ \
    '_memcpy', \
    '_memset', \
    '_malloc', \
    '_free', \
    '_getAvifVersion', \
    '_avifDecoderFrame', \
    '_avifDecoderCreate', \
    '_avifDecoderSetIOMemory', \
    '_avifDecoderNextImage', \
    '_avifRGBImageAllocatePixels', \
    '_avifImageYUVToRGB', \
    '_avifResultToString', \
    '_avifImageCount', \
    '_avifGetImageTiming', \
    '_avifDecoderDestroy', \
    '_avifDecoderParse' \
]"

DECODER_LIB="libavif-1.0.4/ext/dav1d/build/src/libdav1d.a"

# 根据DECODER的值设置对应的解码器状态为ON
if [ "$DECODER" = "$DAV1D_FLAG" ]; then
    DECODER_LIB="libavif-1.0.4/ext/dav1d/build/src/libdav1d.a"
elif [ "$DECODER" = "$GAV1_FLAG" ]; then
    DECODER_LIB="libavif-1.0.4/ext/libgav1/build/libgav1.a"
else
    echo "Unknown decoder. Please set DECODER to either 'dav1d' or 'gav1'."
    exit 1
fi

emcc build/lib${PROJECT_NAME}.a libavif-1.0.4/${LIBAVIF_BUILD}/libavif.a libavif-1.0.4/ext/libyuv/build/libyuv.a $DECODER_LIB \
    -s WASM=1 \
    -s WASM_ASYNC_COMPILATION=1 \
    -s EXIT_RUNTIME=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASSERTIONS=2 \
    -s INVOKE_RUN=0 \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s DISABLE_EXCEPTION_CATCHING=1 \
    -s EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}" \
    -s EXPORTED_RUNTIME_METHODS="${EXPORTED_RUNTIME_METHODS}" \
    -s STACK_SIZE=2097152 \
    -s WASM_BIGINT \
    -O0 \
    -flto \
    -o lib/${PROJECT_NAME}.js

emcc build/lib${PROJECT_NAME}.a libavif-1.0.4/${LIBAVIF_BUILD}/libavif.a libavif-1.0.4/ext/libyuv/build/libyuv.a $DECODER_LIB \
    -s WASM=1 \
    -s WASM_ASYNC_COMPILATION=1 \
    -s EXIT_RUNTIME=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASSERTIONS=1 \
    -s INVOKE_RUN=0 \
    -s ENVIRONMENT=web \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s DISABLE_EXCEPTION_CATCHING=1 \
    -s EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}" \
    -s EXPORTED_RUNTIME_METHODS="${EXPORTED_RUNTIME_METHODS}" \
    -s STACK_SIZE=2097152 \
    -s WASM_BIGINT \
    -O3 \
    -flto \
    -o lib/${PROJECT_NAME}.min.js
