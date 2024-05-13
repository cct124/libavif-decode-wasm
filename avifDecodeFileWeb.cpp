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
  int stride;
  int depth;

  Frame(int w, int h, int s, int d, uint8_t *p)
      : width(w), height(h), stride(s), depth(d), pixels(p) {}

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

struct Timing {
  uint64_t timescale;
  double pts;
  uint64_t ptsInTimescales;
  double duration;
  uint64_t durationInTimescales;
};

const char *getAvifVersion() { return avifVersion(); }

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
    rgb.pixels = nullptr; // Transfer ownership
    return frame;
  }
  return nullptr;
}

int avifImageCount(avifDecoder *decoder) { return decoder->imageCount; }

std::unique_ptr<Timing> avifGetImageTiming(avifDecoder *decoder, int index) {
  std::unique_ptr<Timing> timing = std::make_unique<Timing>();
  avifImageTiming avifTiming;
  if (avifDecoderNthImageTiming(decoder, index, &avifTiming) ==
      AVIF_RESULT_OK) {
    timing->duration = avifTiming.duration;
    timing->durationInTimescales = avifTiming.durationInTimescales;
    timing->ptsInTimescales = avifTiming.ptsInTimescales;
    timing->pts = avifTiming.pts;
    timing->timescale = avifTiming.timescale;
  } else {
    throw std::runtime_error("Failed to get image timing");
  }
  return timing;
}
}