#include "avif/avif.h"
#include "string.h"
#include <stdlib.h>

int avifImageCount(avifDecoder *decoder) { return decoder->imageCount; }

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

uint8_t *avifGetRGBImagePixels(avifRGBImage *rgb) { return rgb->pixels; };
int avifGetRGBImageWidth(avifRGBImage *rgb) { return rgb->width; };
int avifGetRGBImageHeight(avifRGBImage *rgb) { return rgb->height; };
int avifGetRGBImageRowBytes(avifRGBImage *rgb) { return rgb->rowBytes; };
int avifGetRGBImageDepth(avifRGBImage *rgb) { return rgb->depth; };