# avif-decode-file-web

这个项目将Libavif库编译为Webassembly，并暴露了部分C语言编写的接口，以方便在Webassembly的环境中创建AVIF解码器，方便对图像的解码逻辑编写。有了这个库就能够在支持Webassembly的环境中软解AVIF文件，提供向后兼容的AVIF文件的能力。AV1 图像文件格式( AVIF ) 是一种开放的、免版税的图像文件格式规范，AVIF 表现出比JPEG更好的压缩效率，有更好的细节保留、更少的阻塞伪影和硬边缘周围的色彩渗色更少。相比传统的图像格式，AVIF有很大的优势，特别是对于动图而言，AVIF的图像文件大小和APNG对比能小十几倍的差距，这种开放的图像格式是未来的主流。但是目前来看这种格式的兼容性还是不太好，Android是[Android14](https://developer.android.com/media/platform/supported-formats#image-formats)中提供完全支持，IOS则在[Safari 16](https://developer.apple.com/documentation/safari-release-notes/safari-16_1-release-notes#New-Features)提供完全的支持，所以兼容性方面还是面临很大挑战。这个库因为是使用Webassembly而Webassembly在[17~18年](https://webassembly.org/features/)绝大部分移动端桌面端浏览器都已支持，所以能提供一种AVIF图像格式的兼容性解决方案

## 构建
本项目是在Window WSL2 Ubuntu 22.04.3 LTS版本上开发构建的。

### 环境

需要安装 Cmake、emscripten、meson

构建指令

```sh
bash build.sh
```

会在lib文件夹下生成js文件，Webassembly文件是内联到js文件中的

## 其它

### 安装 Cmake

```shell
sudo apt update
sudo apt install cmake
cmake --version
```

### 安装 Meson

```shell
sudo apt install python3 python3-pip python3-setuptools python3-wheel ninja-build
sudo apt install meson
meson --version
```

### 安装 Emscripten
```shell
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```