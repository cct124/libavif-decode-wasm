extern "C" {
#include "avif/avif.h"
#include <stdio.h>
#include <string.h>
}

extern "C" {
void cleanupResources(avifDecoder *decoder, avifRGBImage *rgb = nullptr) {
  if (decoder)
    avifDecoderDestroy(decoder);
  if (rgb)
    avifRGBImageFreePixels(rgb);
}

struct Frame {
  uint8_t *pixels;
  int width;
  int height;
  int stride;
  int depth;
};

struct Timing {
  double timescale; // 媒体的时间尺度（赫兹）
  double pts; // 呈现时间戳，以秒为单位（ptsInTimescales / timescale）
  double ptsInTimescales; // 呈现时间戳在"时间尺度"中的值
  double duration; // 持续时间，以秒为单位（durationInTimescales / timescale）
  double durationInTimescales; // 持续时间在"时间尺度"中的值
};

const char *getAvifVersion() { return avifVersion(); }

Frame *createFrame() { return new Frame(); }

Frame *avifDecoderFrame(avifResult result, avifDecoder *decoder) {
  if (avifDecoderNextImage(decoder) == AVIF_RESULT_OK) {
    avifRGBImage rgb;
    memset(&rgb, 0, sizeof(rgb));
    Frame *frame = new Frame();
    avifRGBImageSetDefaults(&rgb, decoder->image);

    result = avifRGBImageAllocatePixels(&rgb);
    if (result != AVIF_RESULT_OK) {
      fprintf(stderr, "Allocation of RGB samples failed: (%s)\n",
              avifResultToString(result));
      cleanupResources(decoder, &rgb);
    }

    result = avifImageYUVToRGB(decoder->image, &rgb);
    if (result != AVIF_RESULT_OK) {
      fprintf(stderr, "Conversion from YUV failed: (%s)\n",
              avifResultToString(result));
      cleanupResources(decoder, &rgb);
    }

    frame->pixels = rgb.pixels;
    frame->width = rgb.width;
    frame->height = rgb.height;
    frame->stride = rgb.rowBytes;
    frame->depth = rgb.depth;
    return frame;
  }
  return nullptr;
}

int avifImageCount(avifDecoder *decoder) { return decoder->imageCount; }

Timing *avifGetImageTiming(avifDecoder *decoder, int index) {
  Timing *timing = new Timing();
  avifImageTiming avifTiming;
  avifDecoderNthImageTiming(decoder, index, &avifTiming);
  timing->duration = avifTiming.duration;
  timing->durationInTimescales = avifTiming.durationInTimescales;
  timing->ptsInTimescales = avifTiming.ptsInTimescales;
  timing->pts = avifTiming.pts;
  timing->timescale = avifTiming.timescale;
  return timing;
}
}
