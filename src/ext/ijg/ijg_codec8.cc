/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * ijg_codec8.cc
 */

#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "imagecodec.h"
#include "ijg_codec.h"
// #include "logger.h"

//--------

extern "C" {
#include "8/jpeglib.h"
}

#include <setjmp.h>
#include <memory.h>

namespace dicom {  //------------------------------------------------------

typedef struct {
  int len;
  unsigned char *buf;
  unsigned char *ptr;
} MEMFILE;

// -----------------------------------------------------------------------
// codes for managing error

struct my_error_mgr {
  struct jpeg_error_mgr pub; /* "public" fields */
  jmp_buf setjmp_buffer; /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;
static void my_error_exit(j_common_ptr cinfo) {
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

//  (*cinfo->err->output_message) (cinfo);
//	char buffer[JMSG_LENGTH_MAX];
//	(*cinfo->err->format_message) (cinfo, buffer);
//	set_error_message(DICOM_CODEC_ERROR, "%s", buffer);

  longjmp(myerr->setjmp_buffer, 1);
}
static void my_output_message(j_common_ptr cinfo) {
  char buffer[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message)(cinfo, buffer);
  LOG_WARN("encode_ijg_jpeg8(...):%s", buffer);
}

// -----------------------------------------------------------------------
// encoder

DICOMSDL_CODEC_RESULT encode_ijg_jpeg8(imagecontainer *ic, char **data,
                                       long *datasize, JPEG_MODE jmode,
                                       int quality) {
  struct jpeg_compress_struct cinfo;

  // Step 1: allocate and initialize JPEG compression object
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  // Step 2: specify data destination

  MEMFILE mem;
  mem.buf = (unsigned char *)*data;  // memory has beed allocated by ijg_codec.cc
  mem.ptr = mem.buf;
  mem.len = *datasize;
  jpeg_stdio_dest(&cinfo, (FILE *) &mem);

  // Step 3: set parameters for compression

  cinfo.image_width = (JDIMENSION) (ic->cols);
  cinfo.image_height = (JDIMENSION) (ic->rows);
  cinfo.input_components = ic->ncomps;
  cinfo.in_color_space = (ic->ncomps == 3 ? JCS_RGB : JCS_GRAYSCALE);

  jpeg_set_defaults(&cinfo);
  switch (jmode) {
    case JPEG_BASELINE:
      jpeg_set_quality(&cinfo, quality, TRUE /* force baseline */);
      break;
    case JPEG_EXTENDED:
      jpeg_set_quality(&cinfo, quality, FALSE);
      break;
    case JPEG_LOSSLESS:
    case JPEG_LOSSLESS_SV1:
      jpeg_simple_lossless(&cinfo, 1 /* predictor */, 0 /* pt */);
      break;

    default:
      break;
  }

  //
  // Step 4: Start compressor

  jpeg_start_compress(&cinfo, TRUE);

  // Step 5: while (scan lines remain to be written)
  //           jpeg_write_scanlines(...);

  JSAMPROW row_pointer[1];
  char *src = ic->data;
  if (ic->rowstep < 0)
    src += -(ic->rowstep) * (ic->rows - 1);
  while (cinfo.next_scanline < cinfo.image_height) {
    //row_pointer[0] = (JSAMPROW)& src[cinfo.next_scanline * srcstep];
    row_pointer[0] = (JSAMPROW) src;
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    src += ic->rowstep;
  }

  // Step 6: Finish compression

  jpeg_finish_compress(&cinfo);

  // Step 7: release JPEG compression object

  jpeg_destroy_compress(&cinfo);

  *datasize = long(mem.ptr - mem.buf);

  return DICOMSDL_CODEC_OK;
}

// -----------------------------------------------------------------------
// decoder

DICOMSDL_CODEC_RESULT decode_ijg_jpeg8(char *src, int srclen,
                                       imagecontainer *ic) {
  struct jpeg_decompress_struct cinfo;

  // Step 1: allocate and initialize JPEG decompression object
  struct my_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  jerr.pub.output_message = my_output_message;
  if (setjmp(jerr.setjmp_buffer)) {
    char buffer[JMSG_LENGTH_MAX];  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    (cinfo.err->format_message)((j_common_ptr) (&cinfo), buffer);
    snprintf(ic->info, ARGBUF_SIZE, "%s", buffer);  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    jpeg_destroy_decompress(&cinfo);
    //strcpy(ic->info, get_error_message());
    return DICOMSDL_CODEC_ERROR;
  }
  jpeg_create_decompress(&cinfo);

  // Step 2: specify data source

  // init source stream
  MEMFILE mem;
  mem.len = srclen;
  mem.buf = (unsigned char*) src;
  mem.ptr = mem.buf;
  jpeg_stdio_src(&cinfo, (FILE *) &mem);

  // Step 3: read file parameters with jpeg_read_header()
  (void) jpeg_read_header(&cinfo, TRUE);

  ic->cols = cinfo.image_width;
  ic->rows = cinfo.image_height;
  ic->ncomps = cinfo.num_components;
  ic->prec = cinfo.data_precision;

  // Step 4: set parameters for decompression

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  // Step 5: Start decompressor
  (void) jpeg_start_decompress(&cinfo);
  int row_stride = cinfo.output_width * cinfo.output_components;
  JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo,
  JPOOL_IMAGE,
                                                 row_stride, 1);

  // Step 6: while (scan lines remain to be read)
  //             jpeg_read_scanlines(...);

  char *pixelbuf = ic->data;
  if (ic->rowstep < 0)
    pixelbuf += -ic->rowstep * (ic->rows - 1);

  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    memcpy(pixelbuf, buffer[0], row_stride);  // bpp = 1
    pixelbuf += ic->rowstep;
  }

  // Step 7: Finish decompression
  (void) jpeg_finish_decompress(&cinfo);

  // Step 8: Release JPEG decompression object
  jpeg_destroy_decompress(&cinfo);

  ic->info[0] = '\0';
  return DICOMSDL_CODEC_OK;
}

}  // namespace dicom ------------------------------------------------------
