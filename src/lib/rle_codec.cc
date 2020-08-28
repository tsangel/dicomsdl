/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * rle_codec.cc
 */

#include "rle_codec.h"

#include <stdio.h>

#include "codec_common.h"
#include "dicom.h"

namespace dicom {  //------------------------------------------------------


static long decode_rle_segment(Buffer<uint8_t>& encdata, Buffer<uint8_t>& decdata) {
  uint8_t *p = encdata.data;
  uint8_t *p_end = p + encdata.size;
  uint8_t *q = decdata.data;
  uint8_t *q_end = q + decdata.size;
  size_t decoded_bytes = 0;

  uint8_t c;
  int i;
  while (p < p_end - 1) {
    c = *p++;
    if (c > 0x80) {
      for (i = 0; i < (0x101 - c); i++)
        *q++ = *p;
      p++;
    } else if (c < 0x80) {
      for (i = 0; i < (c + 1); i++)
        *q++ = *p++;
    } else {
      return -1; // error
    }
  }
  return q - decdata.data;
}

static DICOMSDL_CODEC_RESULT decode_rle(char *src, long srclen,
                                        imagecontainer *ic) {
  uint32_t *header;
  header = (uint32_t *) src;
  int nsegments = int(header[0]);
  DICOMSDL_CODEC_RESULT result;

  // prepare buffer for decoded pixels
  int decdata_size = ic->rows * ic->cols * (ic->prec > 8 ? 2 : 1) * ic->ncomps;
  Buffer<uint8_t> decdata(decdata_size);

  // process each segments
  uint8_t *q =  decdata.data;
  size_t remaining = decdata.size;
  for (int i = 1; i <= nsegments; i++) {
    size_t startoffset, endoffset;
    startoffset = header[i];
    if (i == nsegments)
      endoffset = srclen;
    else
      endoffset = header[i + 1];

    Buffer<uint8_t> enc_segment((uint8_t *)src + startoffset,
                                endoffset - startoffset);
    Buffer<uint8_t> dec_segment(q, remaining);

    long ndecoded = decode_rle_segment(enc_segment, dec_segment);
    if (ndecoded < 0)
      break; // error
    remaining -= (size_t)ndecoded;
    q += ndecoded;
  }

  if (remaining > 0) {
    snprintf(ic->info, ARGBUF_SIZE, "decode_rle(...): "
             "out buffer is not filled by %d bytes.",
             int(remaining));
    result = DICOMSDL_CODEC_ERROR;
    goto DECODE_END;
  }

  // change planar configuration color-by-plane to color-by-pixel
  {
    int nplanes;
    if (ic->prec > 8 && ic->ncomps == 1)
      nplanes = 2;
    else if (ic->prec == 8)
      nplanes = ic->ncomps;
    else {
      snprintf(ic->info, ARGBUF_SIZE, "decode_rle(...): "
               "unsupported image format - %d bits and %d planes.",
               ic->prec, ic->ncomps);
      return DICOMSDL_CODEC_ERROR;
      goto DECODE_END;
    }
    { // copy
      int msb_first = (ic->prec > 8 ? 0 : 1);
      uint8_t *p, *q;
      int plane_strides = ic->rows * ic->cols;
      for (int k = 0; k < nplanes; k++) {
        p = decdata.data + plane_strides * k;
        q = (uint8_t *) (ic->data);
        if (ic->rowstep < 0)
          q += -(ic->rowstep) * (ic->rows - 1);
        if (msb_first)
          q += k;
        else
          q += nplanes - k - 1;

        for (int j = 0; j < ic->rows; j++) {
          for (int i = 0; i < ic->cols; i++) {
            q[i * nplanes] = p[i];
          }
          p += ic->cols;
          q += ic->rowstep;
        }
      }
    }
  }
  result = DICOMSDL_CODEC_OK;
 DECODE_END:
  //  decdata.free(); // `~Buffer` will free allocated data.
   return result;
 }

extern "C" DICOMSDL_CODEC_RESULT rle_decoder(const char *tsuid, char *data,
                                             long datasize,
                                             imagecontainer *ic) {
  if (
  // PS3.5 A.4.2 RLE Image Compression
  strcmp("1.2.840.10008.1.2.5", tsuid) != 0  // RLE Lossless
      )
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (!data) {
    snprintf(ic->info, ARGBUF_SIZE, "data == NULL.");
    return DICOMSDL_CODEC_ERROR;
  }

  if (ic->datasize < ic->rowstep * ic->rows
      || ic->rowstep < ic->cols * (ic->prec > 8 ? 2 : 1) * ic->ncomps) {
    snprintf(ic->info, ARGBUF_SIZE, "pixelbuf for decoded image is too small; "
             "buflen %d < rowstep %d * rows %d or "
             "rowstep < cols %d * (prec %d > 8 ? 2 : 1) * ncomps %d",
             int(ic->datasize), ic->rowstep, ic->rows, ic->cols, ic->prec,
             ic->ncomps);
    return DICOMSDL_CODEC_ERROR;
  }

  if (datasize < 64 + 2 + 2) {
    snprintf(ic->info, ARGBUF_SIZE, "datasize (%d) is too small.",
             int(datasize));
    return DICOMSDL_CODEC_ERROR;
  }

  return decode_rle(data, datasize, ic);
}

extern "C" DICOMSDL_CODEC_RESULT rle_encoder(
    const char *tsuid, imagecontainer *ic, char **data, long *datasize,
    free_memory_fnptr *free_memory_fn) {
  if (
  // PS3.5 A.4.2 RLE Image Compression
  strcmp("1.2.840.10008.1.2.5", tsuid) != 0  // RLE Lossless
      )
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (!*data || datasize || *free_memory_fn) {
    snprintf(ic->info, ARGBUF_SIZE, "rle_encoder(...): "
             "*data or datasize or *free_memory_fn is NULL.");
    return DICOMSDL_CODEC_ERROR;
  }

  *data = NULL;
  *datasize = 0;
  *free_memory_fn = rle_codec_free_memory;

  snprintf(ic->info, ARGBUF_SIZE, "rle_encoder(...): "
           "not implemented yet.");
  return DICOMSDL_CODEC_ERROR;
}

extern "C" void rle_codec_free_memory(char *data) {
  ::free(data);
}

}  // namespace dicom -----------------------------------------------------
