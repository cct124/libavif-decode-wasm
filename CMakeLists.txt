cmake_minimum_required(VERSION 3.0.0)
project(avifDecodeFileWeb VERSION 0.1.0 LANGUAGES C CXX)

# set(EMSCRIPTEN_HEADERS_DIR /root/emsdk/upstream/emscripten/cache/sysroot/include)
set(LIBAVIF_LIBS_DIR libavif-1.0.4/build)
set(LIBAVIF_HEADERS_DIR libavif-1.0.4/include)

add_library(libavif STATIC IMPORTED)
set_target_properties(libavif PROPERTIES IMPORTED_LOCATION ${libavif_libs_DIR}/libavif.a)

include(CTest)
enable_testing()

include_directories(${LIBAVIF_HEADERS_DIR})
# include_directories(${EMSCRIPTEN_HEADERS_DIR})
link_directories(${LIBAVIF_LIBS_DIR})
add_library(${PROJECT_NAME} avifDecodeFileWeb.cpp)
target_link_libraries(${PROJECT_NAME} libavif)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
