#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
extern "C" {
#include "avif/avif.h"
#include "string.h"
#include <stdlib.h>
}

class AvifImageCache {
public:
  ~AvifImageCache() { clearCache(); }

  // 函数用于打印 std::unordered_map<std::string, CacheEntry>
  void printCache() {
    for (const auto &pair : cache) {
      for (const auto &pair : cache) {
        std::cout << pair.first + " => " + std::to_string(pair.second.count)
                  << std::endl;
      }
    }
  }

  void cacheImage(const char *id, avifImage *image, int index) {
    try {
      auto it = cache.find(id);
      if (it == cache.end()) {
        char errorMessage[256]; // 确定足够大的缓冲区来存储消息
        snprintf(errorMessage, sizeof(errorMessage),
                 "Cache entry not initialized for ID: %s", id);
        throw std::runtime_error(errorMessage);
      }
      CacheEntry &entry = it->second;
      if (index < 0 || index >= entry.count) {
        throw std::out_of_range("Index out of bounds: " +
                                std::to_string(index));
      }
      entry.images[index] = image;
    } catch (const std::exception &e) {
      // 处理异常，例如记录错误信息或回退操作
      std::cerr << "Exception caught: " << e.what() << std::endl;
    }
  }

  void initializeCacheEntry(const char *id, int count) {
    if (count <= 0) {
      throw std::invalid_argument("Count must be greater than zero.");
    }
    clearCacheForId(id); // 清理现有的缓存条目（如果存在）
    avifImage **images = new avifImage *[count]();
    cache[id] = {images, count};
  }

  avifImage **getImages(const char *id, int count) {
    auto it = cache.find(id);
    if (it == cache.end()) {
      count = 0;
      return nullptr;
    }
    count = it->second.count;
    return it->second.images;
  }

  avifImage *getImage(const char *id, int index) {
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

  void clearCacheForId(const char *id) {
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

extern "C" {

typedef struct Timing {
  double timescale;
  double pts;
  double ptsInTimescales;
  double duration;
  double durationInTimescales;
} Timing;

avifImage *avifGetDecoderImage(avifDecoder *decoder) { return decoder->image; };

void avifSetDecoderMaxThreads(avifDecoder *decoder, int maxThreads) {
  decoder->maxThreads = maxThreads;
}

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
int avifGetImageWidth(avifDecoder *decoder) { return decoder->image->width; }
int avifGetImageHeight(avifDecoder *decoder) { return decoder->image->height; }

AvifImageCache *avifCreateAvifImageCache() { return new AvifImageCache(); };

void avifInitializeCacheEntry(AvifImageCache *avifImageCache, const char *id,
                              int count) {
  avifImageCache->initializeCacheEntry(id, count);
}

void avifCacheImage(AvifImageCache *avifImageCache, const char *id,
                    avifImage *image, int index) {
  avifImageCache->cacheImage(id, image, index);
}

void avifCacheImagePrintCache(AvifImageCache *avifImageCache) {
  avifImageCache->printCache();
}

avifImage *avifGetCacheImage(AvifImageCache *avifImageCache, const char *id,
                             int index) {
  return avifImageCache->getImage(id, index);
}
}
