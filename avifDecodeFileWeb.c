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

// avifDecoderParse() 或 avifDecoderNextImage() 需要下载多少AVIF内容才能返回
// OK，这一点因文件的打包方式而异。 理想情况下，AVIF的末端仅是一个包含大量 AV1
// 负载的大型 mdat 或 moov 盒子，所有元数据（元盒子，Exif/XMP 负载等）
// 尽可能靠前放置。任何尾随的 MP4 盒子（如 free 等）都会导致 avifDecoderParse()
// 需要等待下载这些盒子， 因为在不知道剩余盒子是什么的情况下无法确保解析成功。

typedef struct avifIOStreamingReader {
  avifIO io; // 如果您打算将此结构体转换为 avifIO *，此字段必须位于首位。
  avifROData rodata; // 实际数据。
  size_t downloadedBytes; // 到目前为止“下载”的字节数。这将决定我们何时返回
                          // AVIF_RESULT_WAIT_ION_IO
                          // 在此示例中。示例将逐步增加此值（从 0 开始）， 直到
                          // avifDecoderParse() 返回非 AVIF_RESULT_WAIT_ION_IO
                          // 的结果，然后继续增加 直到 avifDecoderNextImage()
                          // 返回非 AVIF_RESULT_WAIT_ION_IO 的结果。
} avifIOStreamingReader;

// 此示例已将文档与 avif/avif.h 中的 avifIOReadFunc
// 文档交错，以帮助解释为什么会有这些检查。
static avifResult avifIOStreamingReaderRead(struct avifIO *io,
                                            uint32_t readFlags, uint64_t offset,
                                            size_t size, avifROData *out) {
  avifIOStreamingReader *reader = (avifIOStreamingReader *)io;

  if (readFlags != 0) {
    // 不支持的 readFlags
    return AVIF_RESULT_IO_ERROR;
  }

  // * 如果 offset 超过内容大小（即超过 EOF），返回 AVIF_RESULT_IO_ERROR。
  if (offset > reader->rodata.size) {
    return AVIF_RESULT_IO_ERROR;
  }

  // * 如果 offset *恰好* 位于 EOF，提供任何 0 字节缓冲区并返回 AVIF_RESULT_OK。
  if (offset == reader->rodata.size) {
    out->data = reader->rodata.data;
    out->size = 0;
    return AVIF_RESULT_OK;
  }

  // * 如果 (offset+size) 超过内容的大小，必须提供一个截断的缓冲区，
  //   该缓冲区提供从偏移量到 EOF 的所有字节，并返回 AVIF_RESULT_OK。
  uint64_t availableSize = reader->rodata.size - offset;
  if (size > availableSize) {
    size = (size_t)availableSize;
  }

  // * 如果 (offset+size) 超过内容的大小，但整个范围尚未可用
  //   （由于网络状况或其他原因），返回 AVIF_RESULT_WAITING_ON_IO。
  if (offset > reader->downloadedBytes) {
    return AVIF_RESULT_WAITING_ON_IO;
  }
  if (size > (reader->downloadedBytes - offset)) {
    return AVIF_RESULT_WAITING_ON_IO;
  }

  // * 如果 (offset+size) 未超过内容的大小，必顀提供整个范围并
  //   返回 AVIF_RESULT_OK。
  out->data = reader->rodata.data + offset;
  out->size = size;
  return AVIF_RESULT_OK;
}

static void avifIOStreamingReaderDestroy(struct avifIO *io) { free(io); }

// 如内存分配失败，返回 null。
avifIOStreamingReader *avifIOCreateStreamingReader(const uint8_t *data,
                                                   size_t size) {
  avifIOStreamingReader *reader =
      (avifIOStreamingReader *)calloc(1, sizeof(avifIOStreamingReader));
  if (!reader)
    return NULL;

  // io.destroy 可以是 NULL，这种情况下您需要自行清理您的
  // reader。这允许使用现有的、栈上的或成员变量作为 avifIO*。
  reader->io.destroy = avifIOStreamingReaderDestroy;

  // 读取器的核心是这个函数。参见上面的实现和评论。
  reader->io.read = avifIOStreamingReaderRead;

  // 参见 avif/avif.h 中关于 sizeHint 的文档。虽然设置 sizeHint
  // 不是必需的，但推荐这样做。
  reader->io.sizeHint = size;

  // 参见 avif/avif.h 中关于 persistent 的文档。启用此项会对您从 io.read
  // 返回的缓冲区的生命周期施加重大限制， 但会减少内存开销和内存复制操作。
  reader->io.persistent = AVIF_TRUE;

  reader->rodata.data = data;
  reader->rodata.size = size;
  return reader;
}

void avifSetDownloadedBytes(avifIOStreamingReader *io, int downloadedBytes) {
  io->downloadedBytes = downloadedBytes;
}

int avifGetDownloadedBytes(avifIOStreamingReader *io) {
  return io->downloadedBytes;
}

int avifGetSizeHint(avifIOStreamingReader *io) { return io->io.sizeHint; }

void avifSetDecoderExifXMP(avifDecoder *decoder, int avifBool) {
  decoder->ignoreExif = decoder->ignoreXMP = avifBool;
}