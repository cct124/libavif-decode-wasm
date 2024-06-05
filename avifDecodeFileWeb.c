#include "avif/avif.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

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
