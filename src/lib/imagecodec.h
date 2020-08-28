/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * imagecodec.h
 */


#ifndef __IMAGECODEC_H__
#define __IMAGECODEC_H__

#include "dicom.h"

namespace dicom {  // ----------------------------------------------------------
extern "C" {

#include "codec_common.h"

// if return value != DICOMSDL_CODEC_OK, check ic->info
DICOMSDL_CODEC_RESULT decode_pixeldata(const char *tsuid, char *data,
                                       long datasize, imagecontainer *ic);

// caller should free_memory(*data);
DICOMSDL_CODEC_RESULT encode_pixeldata(const char *tsuid, imagecontainer *ic,
                                       char **data, long *datasize,
                                       free_memory_fnptr *free_memory_fn);

typedef enum {
  JPEG_UNKNOWN = 0,
  JPEG_BASELINE = 1,
  JPEG_EXTENDED = 2,
  JPEG_LOSSLESS = 14,
  JPEG_LOSSLESS_SV1 = 70
} JPEG_MODE;

typedef enum {
  JPEG2K_UNKNOWN = 0,
  JPEG2K_LOSSY = 1,
  JPEG2K_LOSSLESS = 2
} JPEG2K_MODE;

}
}  // namespace dicom ----------------------------------------------------------

#endif // __IMAGECODEC_H__
