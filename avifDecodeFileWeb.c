#include "avif/avif.h"
#include "string.h"
#include <stdlib.h>

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

uint8_t *avifGetRGBImagePixels(avifRGBImage *rgb) { return rgb->pixels; };
int avifGetRGBImageWidth(avifRGBImage *rgb) { return rgb->width; };
int avifGetRGBImageHeight(avifRGBImage *rgb) { return rgb->height; };
int avifGetRGBImageRowBytes(avifRGBImage *rgb) { return rgb->rowBytes; };
int avifGetRGBImageDepth(avifRGBImage *rgb) { return rgb->depth; };
int avifGetImageCount(avifDecoder *decoder) { return decoder->imageCount; }
