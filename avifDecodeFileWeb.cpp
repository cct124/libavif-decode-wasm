extern "C" {
#include "avif/avif.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}
#include <vector>

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

std::vector<Frame *> *avifDecoderFileRgba(uint8_t *fileBuffer,
                                          size_t bufferSize, int *framesSize) {
  avifDecoder *decoder = avifDecoderCreate();
  if (decoder == NULL) {
    fprintf(stderr, "Memory allocation failure\n");
  }

  avifResult result = avifDecoderSetIOMemory(decoder, fileBuffer, bufferSize);
  if (result != AVIF_RESULT_OK) {
    fprintf(stderr, "Cannot set IO on avifDecoder\n");
    cleanupResources(decoder);
  }

  result = avifDecoderParse(decoder);
  if (result != AVIF_RESULT_OK) {
    fprintf(stderr, "Failed to parse image\n");
    cleanupResources(decoder);
  }

  std::vector<Frame *> *frames = new std::vector<Frame *>();

  while (avifDecoderNextImage(decoder) == AVIF_RESULT_OK) {
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
    frames->push_back(frame);
  }
  *framesSize = frames->size();
  return frames;
}
}
