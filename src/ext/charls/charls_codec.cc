/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * charls_codec.cc
 */

#include <stdio.h>

#include "dicom.h"
#include "codec_common.h"
#include "charls_codec.h"
#include "src/util.h"
#include "interface.h"

namespace dicom {  //------------------------------------------------------

DICOMSDL_CODEC_RESULT charls_decoder(const char *tsuid, char *data,
                                     long datasize, imagecontainer *ic) {
  if (
  // PS3.5 A.4.3 JPEG-LS Image Compression
  strcmp("1.2.840.10008.1.2.4.80", tsuid) != 0 &&  // JPEG-LS Lossless Image Compression
      strcmp("1.2.840.10008.1.2.4.81", tsuid) != 0  // JPEG-LS Lossy (Near-Lossless) Image Compression
          )
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (!data) {
    snprintf(ic->info, ARGBUF_SIZE, "charls_decoder(...): data == NULL");
    return DICOMSDL_CODEC_ERROR;
  }

  if (ic->datasize < ic->rowstep * ic->rows
      || ic->rowstep < ic->cols * (ic->prec > 8 ? 2 : 1) * ic->ncomps) {
    snprintf(ic->info, ARGBUF_SIZE, "charls_decoder(...): "
             "pixelbuf for decoded image is too small; "
             "buflen %d < rowstep %d * rows %d or "
             "rowstep < cols %d * (prec %d > 8 ? 2 : 1) * ncomps %d",
             int(ic->datasize), ic->rowstep, ic->rows, ic->cols, ic->prec,
             ic->ncomps);
    return DICOMSDL_CODEC_ERROR;
  }

  JlsParameters info;
  JLS_ERROR error = JpegLsReadHeader(data, datasize, &info);

  if (error != OK) {
    snprintf(ic->info, ARGBUF_SIZE, "charls_decoder(...): "
             "error in JpegLsReadHeader");
    return DICOMSDL_CODEC_ERROR;
  }

  std::vector<BYTE> dataUnc;
  dataUnc.resize(info.bytesperline * info.height);

  error = JpegLsDecode(&dataUnc[0], dataUnc.size(), data, datasize, NULL);

  if (error != OK) {
    snprintf(ic->info, ARGBUF_SIZE, "charls_decoder(...): "
             "error in JpegLsDecode");
    return DICOMSDL_CODEC_ERROR;
  }

  if (ic->rows != info.height || ic->cols != info.width) {
    snprintf(ic->info, ARGBUF_SIZE, "error: info mismatch "
             "DICOM info (%d x %d) != JPEGLS info (%d x %d)",
             ic->cols, ic->rows, info.width, info.height);
    return DICOMSDL_CODEC_ERROR;
  }

  int rowstep = (ic->rowstep > 0 ? ic->rowstep : -ic->rowstep);
  int bytesperline = (rowstep > info.bytesperline ? info.bytesperline : rowstep);

  uint8_t *q;
  if (ic->rowstep > 0)
    q = (uint8_t *) (ic->data);
  else
    q = (uint8_t*) (ic->data + (ic->rows - 1) * (-ic->rowstep));
  for (int j = 0; j < ic->rows; j++) {
    memcpy(q, &dataUnc[info.bytesperline * j], bytesperline);
    q += ic->rowstep;
  }

  return DICOMSDL_CODEC_OK;
}

DICOMSDL_CODEC_RESULT charls_encoder(const char *tsuid, imagecontainer *ic,
                                     char **data, long *datasize,
                                     free_memory_fnptr *free_memory_fn) {
  if (
  // PS3.5 A.4.3 JPEG-LS Image Compression
  strcmp("1.2.840.10008.1.2.4.80", tsuid) != 0 &&  // JPEG-LS Lossless Image Compression
      strcmp("1.2.840.10008.1.2.4.81", tsuid) != 0  // JPEG-LS Lossy (Near-Lossless) Image Compression
          )
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (data == NULL|| datasize == 0|| free_memory_fn == NULL) {
    snprintf(ic->info, ARGBUF_SIZE, "charls_decoder(...): "
             "data or datasize or free_memory_fn is NULL.");
    return DICOMSDL_CODEC_ERROR;
  }

  *free_memory_fn = charls_codec_free_memory;

  /*
   * { do encode pixel data }
   * *datasize = { encoded data size }
   * *data = (char *)malloc(*datasize);
   * { store encoded data to *data }
   * free_memory_fn = dummy_codec_free_memory;
   * return
   */

  *data = NULL;
  *datasize = 0;

  snprintf(ic->info, ARGBUF_SIZE, "DUMMY ENCODER");
  return DICOMSDL_CODEC_ERROR;
}

extern "C" void charls_codec_free_memory(char *data) {
  if (data)
    free(data);
}

}  // namespace dicom -----------------------------------------------------
