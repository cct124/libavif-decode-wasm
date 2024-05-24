#include <emscripten/bind.h>
#include <iostream>
#include <string>
#include <unordered_map>
extern "C" {
#include "avif/avif.h"
#include "string.h"
#include <stdlib.h>

using namespace emscripten;

class AvifImageCache {
public:
  ~AvifImageCache() { clearCache(); }

  void cacheImage(const std::string &id, avifImage *image, int index) {
    auto it = cache.find(id);
    if (it == cache.end()) {
      throw std::runtime_error("Cache entry not initialized for ID: " + id);
    }
    CacheEntry &entry = it->second;
    if (index < 0 || index >= entry.count) {
      throw std::out_of_range("Index out of bounds: " + std::to_string(index));
    }
    entry.images[index] = image;
  }

  void initializeCacheEntry(const std::string &id, int count) {
    clearCacheForId(id); // 清理现有的缓存条目（如果存在）
    avifImage **images = new avifImage *[count]();
    cache[id] = {images, count};
  }

  avifImage **getImages(const std::string &id, int count) {
    auto it = cache.find(id);
    if (it == cache.end()) {
      count = 0;
      return nullptr;
    }
    count = it->second.count;
    return it->second.images;
  }

  avifImage *getImage(const std::string &id, int index) {
    auto it = cache.find(id);
    if (it == cache.end()) {
      // 如果找不到指定ID的缓存条目，返回nullptr
      return nullptr;
    }

    // 获取缓存条目
    CacheEntry &entry = it->second;

    if (index < 0 || index >= entry.count) {
      // 如果索引超出了数组的有效范围，返回nullptr
      return nullptr;
    }

    // 返回指定索引处的图像
    return entry.images[index];
  }

  void clearCacheForId(const std::string &id) {
    auto it = cache.find(id);
    if (it != cache.end()) {
      for (size_t i = 0; i < it->second.count; ++i) {
        avifImageDestroy(it->second.images[i]);
      }
      delete[] it->second.images;
      cache.erase(it);
    }
  }

  void clearCache() {
    for (auto &pair : cache) {
      for (size_t i = 0; i < pair.second.count; ++i) {
        avifImageDestroy(pair.second.images[i]);
      }
      delete[] pair.second.images;
    }
    cache.clear();
  }

private:
  struct CacheEntry {
    avifImage **images;
    int count;
  };

  std::unordered_map<std::string, CacheEntry> cache;
};

typedef struct Timing {
  double timescale;
  double pts;
  double ptsInTimescales;
  double duration;
  double durationInTimescales;
} Timing;

avifImage *avifGetDecoderImage(avifDecoder *decoder) { return decoder->image; };

avifRGBImage *avifGetRGBImage() {
  // 使用 malloc 分配内存
  avifRGBImage *rgb = (avifRGBImage *)malloc(sizeof(avifRGBImage));
  if (rgb == NULL) {
    // 如果内存分配失败，返回 NULL
    return NULL;
  }
  // 使用 memset 初始化内存区域
  memset(rgb, 0, sizeof(avifRGBImage));
  return rgb;
}

struct Timing *avifGetImageTiming(avifDecoder *decoder, int index) {
  // 使用 malloc 分配内存
  Timing *timing = (Timing *)malloc(sizeof(Timing));
  if (timing == NULL) {
    // 如果内存分配失败，返回 NULL
    return NULL;
  }
  // 使用 memset 初始化内存区域
  memset(timing, 0, sizeof(Timing));

  avifImageTiming avifTiming;
  if (avifDecoderNthImageTiming(decoder, index, &avifTiming) ==
      AVIF_RESULT_OK) {
    timing->duration = avifTiming.duration;
    timing->durationInTimescales = avifTiming.durationInTimescales;
    timing->ptsInTimescales = avifTiming.ptsInTimescales;
    timing->pts = avifTiming.pts;
    timing->timescale = avifTiming.timescale;
  } else {
    return NULL;
  }
  return timing;
}

avifImage *avifGetCreateImage() { return new avifImage(); }

void avifGetCopyImage(const avifImage *srcImage, avifImage *dstImage) {
  // 首先清理目标图像结构（如果已经被使用过）
  avifImageDestroy(dstImage);

  // 创建一个新的空的目标图像
  dstImage = avifImageCreateEmpty();

  // 复制图像数据。AVIF_PLANES_ALL 表示复制所有的 YUV 和 Alpha 平面
  avifResult copyResult = avifImageCopy(dstImage, srcImage, AVIF_PLANES_ALL);
  if (copyResult != AVIF_RESULT_OK) {
    fprintf(stderr, "Failed to copy avif image: %s\n",
            avifResultToString(copyResult));
  }
}

avifRGBImage *avifGetImageToRGBImage(const avifImage *image) {
  avifRGBImage *rgb = avifGetRGBImage();
  avifRGBImageSetDefaults(rgb, image);
  avifResult result = avifRGBImageAllocatePixels(rgb);
  if (result != AVIF_RESULT_OK) {
    fprintf(stderr, "Allocation of RGB samples failed: (%s)\n",
            avifResultToString(result));
    avifRGBImageFreePixels(rgb);
  }

  result = avifImageYUVToRGB(image, rgb);
  if (result != AVIF_RESULT_OK) {
    fprintf(stderr, "Conversion from YUV failed: (%s)\n",
            avifResultToString(result));
    avifRGBImageFreePixels(rgb);
  }
  return rgb;
}

uint8_t *avifGetRGBImagePixels(avifRGBImage *rgb) { return rgb->pixels; };
int avifGetRGBImageWidth(avifRGBImage *rgb) { return rgb->width; };
int avifGetRGBImageHeight(avifRGBImage *rgb) { return rgb->height; };
int avifGetRGBImageRowBytes(avifRGBImage *rgb) { return rgb->rowBytes; };
int avifGetRGBImageDepth(avifRGBImage *rgb) { return rgb->depth; };
int avifGetImageCount(avifDecoder *decoder) { return decoder->imageCount; }

EMSCRIPTEN_BINDINGS(AvifImageCache_bindings) {
  class_<AvifImageCache>("AvifImageCache")
      .constructor<>()
      .function("cacheImage", &AvifImageCache::cacheImage,
                allow_raw_pointers()) // 允许原始指针
      .function("getImage", &AvifImageCache::getImage,
                allow_raw_pointers()) // 允许原始指针
      .function("initializeCacheEntry", &AvifImageCache::initializeCacheEntry)
      .function("clearCache", &AvifImageCache::clearCache);
}
}
