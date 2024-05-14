#include <cstdlib>
extern "C" {
#include "avif/avif.h"
}
#include <cstring>
#include <iostream>
#include <memory>

extern "C" {
struct Frame {
  uint8_t *pixels; // Pointer to pixel data
  int width;
  int height;
  int rowBytes;
  int stride;
  int depth;

  Frame(int width, int height, int rowBytes, int depth,
        const uint8_t *sourcePixels)
      : width(width), height(height), rowBytes(rowBytes), depth(depth) {
    pixels = new uint8_t[height * rowBytes];
    memcpy(pixels, sourcePixels, height * rowBytes);
  }

  ~Frame() {
    if (pixels) {
      avifFree(pixels);
    }
  }

  // Disable copy and assignment
  Frame(const Frame &) = delete;
  Frame &operator=(const Frame &) = delete;

  // Allow move semantics
  Frame(Frame &&other) noexcept
      : width(other.width), height(other.height), stride(other.stride),
        depth(other.depth), pixels(other.pixels) {
    other.pixels = nullptr;
  }
  Frame &operator=(Frame &&other) noexcept {
    if (this != &other) {
      width = other.width;
      height = other.height;
      stride = other.stride;
      depth = other.depth;
      pixels = other.pixels;
      other.pixels = nullptr;
    }
    return *this;
  }
};

std::unique_ptr<Frame> avifDecoderFrame(avifDecoder *decoder) {
  if (avifDecoderNextImage(decoder) == AVIF_RESULT_OK) {
    avifRGBImage rgb;
    memset(&rgb, 0, sizeof(rgb));
    avifRGBImageSetDefaults(&rgb, decoder->image);
    avifResult result = avifRGBImageAllocatePixels(&rgb);
    if (result != AVIF_RESULT_OK) {
      throw std::runtime_error("Allocation of RGB samples failed: " +
                               std::string(avifResultToString(result)));
    }

    result = avifImageYUVToRGB(decoder->image, &rgb);
    if (result != AVIF_RESULT_OK) {
      avifRGBImageFreePixels(&rgb); // Ensure no memory leak
      throw std::runtime_error("Conversion from YUV failed: " +
                               std::string(avifResultToString(result)));
    }

    auto frame = std::make_unique<Frame>(rgb.width, rgb.height, rgb.rowBytes,
                                         rgb.depth, rgb.pixels);
    delete [] rgb.pixels;
    avifRGBImageFreePixels(&rgb);
    return frame;
  }
  return nullptr;
}

int avifImageCount(avifDecoder *decoder) { return decoder->imageCount; }

avifImageTiming *avifGetImageTiming(avifDecoder *decoder, int index) {
  avifImageTiming *avifTiming = new avifImageTiming();
  avifDecoderNthImageTiming(decoder, index, avifTiming);
  return avifTiming;
}
}