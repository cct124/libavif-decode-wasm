export LIBAVIF_VERSION=1.0.4
export WORK_PWD=${PWD}
export REP_DAV1D="https://code.videolan.org/videolan/dav1d.git"
export REP_LIBYUV="https://chromium.googlesource.com/libyuv/libyuv"
export PROJECT_NAME="avifDecodeFileWeb"

if [ ! -e "v${LIBAVIF_VERSION}.tar.gz" ]; then
    wget https://github.com/AOMediaCodec/libavif/archive/refs/tags/v${LIBAVIF_VERSION}.tar.gz
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}" ]; then
    tar xzf v${LIBAVIF_VERSION}.tar.gz
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/dav1d" ]; then
    git clone -b 1.2.1 --depth 1 ${REP_DAV1D} ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/dav1d
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/libyuv" ]; then
    git clone --single-branch ${REP_LIBYUV} ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libyuv
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/dav1d/build" ]; then
    cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/dav1d
    mkdir build
    cd build
    meson setup --default-library=static --buildtype release .. --cross-file ${WORK_PWD}/cross_emscripten.ini
    ninja
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/ext/libyuv/build" ]; then
    cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}/ext/libyuv
    emcmake cmake -S . -B build
    emmake make -C build
fi

if [ ! -d "libavif-${LIBAVIF_VERSION}/build" ]; then
    cd ${WORK_PWD}/libavif-${LIBAVIF_VERSION}
    emcmake cmake -S . -B build \
        -DBUILD_SHARED_LIBS=OFF \
        -DAVIF_CODEC_DAV1D=ON \
        -DAVIF_LOCAL_LIBYUV=ON \
        -DAVIF_LOCAL_DAV1D=ON
    emmake make -C build
    cd ..
fi

cd ${WORK_PWD}
rm -rf build
emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S . -B build
emmake make -C build

if [ ! -d "lib" ]; then
    mkdir lib
fi

export EXPORTED_FUNCTIONS="[ \
    '_memcpy', \
    '_memset', \
    '_malloc', \
    '_free', \
    '_getAvifVersion', \
    '_cleanupResources', \
    '_avifDecoderFileRgba' \
]"

emcc build/lib${PROJECT_NAME}.a libavif-1.0.4/build/libavif.a libavif-1.0.4/ext/libyuv/build/libyuv.a libavif-1.0.4/ext/dav1d/build/src/libdav1d.a \
    -s WASM=0 \
    -s WASM_ASYNC_COMPILATION=0 \
    -s EXIT_RUNTIME=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASSERTIONS=0 \
    -s INVOKE_RUN=0 \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s DISABLE_EXCEPTION_CATCHING=1 \
    -s EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}" \
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
    -O0 \
    -o lib/${PROJECT_NAME}.js

emcc build/lib${PROJECT_NAME}.a libavif-1.0.4/build/libavif.a libavif-1.0.4/ext/libyuv/build/libyuv.a libavif-1.0.4/ext/dav1d/build/src/libdav1d.a \
    -s WASM=0 \
    -s WASM_ASYNC_COMPILATION=0 \
    -s EXIT_RUNTIME=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASSERTIONS=0 \
    -s INVOKE_RUN=0 \
    -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
    -s DISABLE_EXCEPTION_CATCHING=1 \
    -s EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}" \
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
    -Os \
    -o lib/${PROJECT_NAME}.min.js
