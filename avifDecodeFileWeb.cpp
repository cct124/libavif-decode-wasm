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
}
