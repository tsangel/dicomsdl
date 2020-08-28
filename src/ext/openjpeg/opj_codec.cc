/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * opj_codec.cc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include "openjp2/openjpeg.h"
#include "imagecodec.h"
#include "opj_codec.h"

namespace dicom {  //-------------------------------------------------------

DICOMSDL_CODEC_RESULT __decode_opj_jpeg2k(char *src, int srclen,
                                          imagecontainer *ic);
DICOMSDL_CODEC_RESULT opj_image_to_image(opj_image_t *image,
                                         imagecontainer *ic);

// Error callback functions ----------------------------------------------

static void error_callback(const char *msg, void *client_data) {
  imagecontainer *ic = (imagecontainer *) client_data;
  ic->result = DICOMSDL_CODEC_ERROR;
  snprintf(ic->info, ARGBUF_SIZE, "%s", msg);
}

static void warning_callback(const char *msg, void *client_data) {
  imagecontainer *ic = (imagecontainer *) client_data;
  ic->result = DICOMSDL_CODEC_WARN;
  snprintf(ic->info, ARGBUF_SIZE, "%s", msg);
}

static void info_callback(const char *msg, void *client_data) {
  imagecontainer *ic = (imagecontainer *) client_data;
  ic->result = DICOMSDL_CODEC_INFO;
  snprintf(ic->info, ARGBUF_SIZE, "[INFO] %s", msg);
}

// ----------------------------------------------------------------------------

struct bytestream {
  char *data;
  size_t datasize;
  size_t offset;
};

static OPJ_SIZE_T __bs_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes,
                            bytestream* bs) {
  size_t remaining = bs->datasize - bs->offset;
  size_t nb_read = (remaining > p_nb_bytes ? p_nb_bytes : remaining);
  if (nb_read == 0)
    return (OPJ_SIZE_T) -1;
  memcpy(p_buffer, bs->data + bs->offset, nb_read);
  bs->offset += nb_read;
  return nb_read;
}

static OPJ_SIZE_T __bs_write(void * p_buffer, OPJ_SIZE_T p_nb_bytes,
                             bytestream* bs) {
  size_t remaining = bs->datasize - bs->offset;
  size_t nb_write = (remaining > p_nb_bytes ? p_nb_bytes : remaining);
  memcpy(bs->data + bs->offset, p_buffer, nb_write);
  bs->offset += nb_write;
  return nb_write;
}

static OPJ_OFF_T __bs_skip(OPJ_OFF_T p_nb_bytes, bytestream* bs) {
  OPJ_OFF_T newoffset = bs->offset;
  newoffset += p_nb_bytes;
  if (newoffset < 0 || newoffset > OPJ_OFF_T(bs->datasize))
    return -1;
  bs->offset = (size_t) newoffset;
  return p_nb_bytes;
}

static OPJ_BOOL __bs_seek(OPJ_OFF_T p_nb_bytes, bytestream* bs) {
  if (p_nb_bytes < 0 || p_nb_bytes > OPJ_OFF_T(bs->datasize))
    return OPJ_FALSE;
  bs->offset = (size_t) p_nb_bytes;
  return OPJ_TRUE;
}

opj_stream_t* dicomsdl_create_memory_stream(bytestream *bs,
                                            OPJ_BOOL p_is_read_stream) {
  opj_stream_t* l_stream = 00;

  if (bs->data == NULL || bs->datasize == 0)
    return NULL;

  l_stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, p_is_read_stream);
  if (!l_stream)
    return NULL;

  bs->offset = 0;

  opj_stream_set_user_data(l_stream, bs, NULL);
  opj_stream_set_user_data_length(l_stream, bs->datasize);
  opj_stream_set_read_function(l_stream, (opj_stream_read_fn) __bs_read);
  opj_stream_set_write_function(l_stream, (opj_stream_write_fn) __bs_write);
  opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) __bs_skip);
  opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) __bs_seek);

  return l_stream;
}

extern "C" void opj_codec_free_memory(char *data)
{
  if (data)
    free(data);
}

// Encoder ---------------------------------------------------------------------

opj_image_t* image_to_opj_image(imagecontainer *ic);

extern "C" DICOMSDL_CODEC_RESULT opj_encoder(
    const char *tsuid, imagecontainer *ic, char **data, long *datasize,
    free_memory_fnptr *free_memory_fn) {
  if (
      // PS3.5 A.4.4 JPEG 2000 Image Compression
      strcmp("1.2.840.10008.1.2.4.90", tsuid) != 0 &&  // JPEG 2000 Image Compression (Lossless Only)
      strcmp("1.2.840.10008.1.2.4.91", tsuid) != 0  // JPEG 2000 Image Compression
  )
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (data == NULL|| datasize == NULL|| free_memory_fn == NULL) {
    snprintf(ic->info, ARGBUF_SIZE,  "jpg_encoder(...): "
             "data or datasize or free_memory_fn is NULL.");
    return DICOMSDL_CODEC_ERROR;
  }

  *free_memory_fn = opj_codec_free_memory;

  bool bSuccess;
  DICOMSDL_CODEC_RESULT result;
  opj_cparameters_t parameters;  // compression parameters
  opj_stream_t *l_stream = NULL;
  opj_codec_t* l_codec = NULL;
  opj_image_t *image = NULL;
  char *encoded_pixel = NULL;
  bytestream bs;

  opj_set_default_encoder_parameters(&parameters);
  parameters.cp_comment = (char *) "openjp2/dicomsdl";
  parameters.tcp_mct = ic->ncomps == 3 ? 1 : 0;

  // process argument string for encoder -------------------------------
  double rate = 1.0;
  int layers = 0;
  int level = 5;
  bool reversible = true;

  argparser p(ic);  // argument is set in ic->args
  int key = 0;
  while ((key = p.get_next_argkey()) > 0) {
    switch (key) {
      case ARGKEY_RATE:
        rate = p.value_as_double();
        if (rate > 1)
          reversible = false;
        break;
      case ARGKEY_LAYER:
        layers = p.value_as_int();
        if (layers < 1)
          layers = 1;
        break;
      case ARGKEY_LEVEL:
        level = p.value_as_int();
        break;
      case ARGKEY_REVERSIBLE: {
        const char *s = p.value_as_string();
        reversible = (s && tolower(*s) == 'y');
      }
        break;
      case ARGKEY_QUALITY: {
        double quality = p.value_as_double();
        if (quality == 0.0 || quality > 100)
          quality = 100.;
        rate = 100. / quality;
        if (rate > 1)
          reversible = false;
      }
        break;
      default:
        break;
    }
  }
  if (key != 0) {
    // argument key error, error message in ic->info
    result = DICOMSDL_CODEC_ERROR;
    goto fin_encoder;
  }

  // if JPEG 2000 Image Compression (Lossless Only)
  if (strcmp("1.2.840.10008.1.2.4.90", tsuid) == 0)
    reversible = true;

  if (reversible) {
    parameters.irreversible = 0;  // use reversible DWT 5-3
    parameters.tcp_numlayers = 1;
    parameters.tcp_rates[0] = 0;
    parameters.cp_disto_alloc = 1;
  } else {
    parameters.irreversible = 1;
    if (layers < 1)
      layers = 3;
    parameters.tcp_numlayers = layers;
    for (int i = layers - 1; i >= 0; i--) {
      parameters.tcp_rates[i] = rate;
      rate *= 1.4142;
    }
    parameters.cp_disto_alloc = 1;
  }

  if (level)
    parameters.numresolution = level;

  // prepare opj image -------------------------------------------------

  image = image_to_opj_image(ic);
  if (image) {
    // PS3.5-2009, A.4.4 JPEG 2000 image compression
    // The optional JP2 file format header shall NOT be included.
    // The role of the JP2 file format header is fulfilled by the
    // non-pixel data attributes in the DICOM data set.

    l_codec = opj_create_compress(OPJ_CODEC_J2K);

    // catch events using our callbacks and give a local context
    opj_set_info_handler(l_codec, info_callback, ic);
    opj_set_warning_handler(l_codec, warning_callback, ic);
    opj_set_error_handler(l_codec, error_callback, ic);

    if (!opj_setup_encoder(l_codec, &parameters, image)) {
      snprintf(ic->info, ARGBUF_SIZE,
               "failed to encode image: opj_setup_encoder");
      result = DICOMSDL_CODEC_ERROR;
      goto fin_encoder;
    }

    encoded_pixel = (char *) malloc(ic->datasize);
    bs.data = encoded_pixel;
    bs.datasize = ic->datasize;

    l_stream = dicomsdl_create_memory_stream(&bs, 0);
    if (!l_stream) {
      snprintf(ic->info, ARGBUF_SIZE,
               "failed to create l_stream: opj_setup_encoder");
      goto fin_encoder;
    }

    /* encode the image */
    bSuccess = opj_start_compress(l_codec, image, l_stream);

    snprintf(ic->info, ARGBUF_SIZE, "opj_encoder(...): ");

    if (!bSuccess) {
      int n = strlen(ic->info);
      snprintf(ic->info + n, ARGBUF_SIZE - n,
               "failed to encode image: opj_start_compress.");
    }

    bSuccess = bSuccess && opj_encode(l_codec, l_stream);
    if (!bSuccess) {
      int n = strlen(ic->info);
      snprintf(ic->info + n, ARGBUF_SIZE - n,
               "failed to encode image: opj_encode.");
    }
    bSuccess = bSuccess && opj_end_compress(l_codec, l_stream);
    if (!bSuccess) {
      int n = strlen(ic->info);
      snprintf(ic->info + n, ARGBUF_SIZE - n,
               "failed to encode image: opj_end_compress.");
    }

    if (!bSuccess) {
      result = DICOMSDL_CODEC_ERROR;
      goto fin_encoder;
    }

    if (bSuccess) {
      *data = encoded_pixel;
      *datasize = bs.datasize;
      ic->lossy = (reversible ? 0 : 1);
      result = DICOMSDL_CODEC_OK;
    }
  }

  fin_encoder:

  if (encoded_pixel)
    free(encoded_pixel);
  if (image)
    opj_image_destroy(image);
  if (l_stream)
    opj_stream_destroy(l_stream);  // close and free the byte stream
  if (l_codec)
    opj_destroy_codec(l_codec);  // free remaining compression structures

  *data = NULL;
  *datasize = 0;

  return result;
}

template<class T> void __copyto(T *src, int srcstep, int *dst, int w, int h) {
  int wc;
  char *ptr = (char *) src;

  while (h--) {
    src = (T *) ptr;
    wc = w;
    while (wc--)
      *dst++ = *src++;
    ptr += srcstep;
  }
}

opj_image_t* image_to_opj_image(imagecontainer *ic) {

  int i, numcomps, w, h, precision, signedness;
  numcomps = ic->ncomps;
  w = ic->cols;
  h = ic->rows;
  precision = ic->prec;
  signedness = ic->sgnd;
  OPJ_COLOR_SPACE color_space = (
      numcomps == 1 ? OPJ_CLRSPC_GRAY : OPJ_CLRSPC_SRGB);

  opj_image_cmptparm_t cmptparm[3]; /* maximum of 3 components */
  memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));

  for (i = 0; i < numcomps; i++) {
    cmptparm[i].prec = precision;
    cmptparm[i].bpp = precision;
    cmptparm[i].sgnd = signedness;
    cmptparm[i].dx = 1;  // set subsampling_dx as  1
    cmptparm[i].dy = 1;  // set subsampling_dy as  1
    cmptparm[i].w = w;
    cmptparm[i].h = h;
  }

  opj_image_t * image = opj_image_create(numcomps, &cmptparm[0], color_space);
  if (!image)
    return NULL;

  image->x0 = 0;
  image->y0 = 0;
  image->x1 = w;
  image->y1 = h;

  char *src = ic->data;
  if (ic->rowstep < 0)
    src += -(ic->rowstep) * (h - 1);

  if (numcomps == 1) {
    if (precision > 8) {
      signedness ?
          __copyto((short *) src, ic->rowstep, image->comps[0].data, w, h) :
          __copyto((unsigned short *) src, ic->rowstep, image->comps[0].data, w,
                   h);
    } else {  // bpp == 1
      signedness ?
          __copyto((char *) src, ic->rowstep, image->comps[0].data, w, h) :
          __copyto((unsigned char *) src, ic->rowstep, image->comps[0].data, w,
                   h);
    }
  } else {
    int *r = image->comps[0].data, *g = image->comps[1].data, *b =
        image->comps[2].data;
    unsigned char *p;
    int wc, hc = h;

    while (hc--) {
      p = (unsigned char *) src, wc = w;
      while (wc--) {
        *r++ = *p++;
        *g++ = *p++;
        *b++ = *p++;
      }
      src += ic->rowstep;
    }
  }

  return image;
}

// Decoder ---------------------------------------------------------------

extern "C" DICOMSDL_CODEC_RESULT opj_decoder(const char *tsuid, char *data, long datasize,
                           imagecontainer *ic)
{
  if (
      // PS3.5 A.4.4 JPEG 2000 Image Compression
      strcmp("1.2.840.10008.1.2.4.90", tsuid) != 0 &&  // JPEG 2000 Image Compression (Lossless Only)
      strcmp("1.2.840.10008.1.2.4.91", tsuid) != 0  // JPEG 2000 Image Compression
  )
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (!data) {
    snprintf(ic->info, ARGBUF_SIZE, "opj_decoder(...): data == NULL");
    return DICOMSDL_CODEC_ERROR;
  }

  if (ic->datasize < ic->rowstep * ic->rows
      ||  ic->rowstep < ic->cols * (ic->prec > 8 ? 2 : 1) * ic->ncomps) {
    snprintf(ic->info, ARGBUF_SIZE, "opj_decoder(...): "
             "pixelbuf for decoded image is too small; "
             "buflen %d < rowstep %d * rows %d or "
             "rowstep < cols %d * (prec %d > 8 ? 2 : 1) * ncomps %d",
             int(ic->datasize), ic->rowstep, ic->rows,
             ic->cols, ic->prec, ic->ncomps
    );
    return DICOMSDL_CODEC_ERROR;
  }

  return __decode_opj_jpeg2k(data, datasize, ic);
}

// Decoder : subroutines -------------------------------------------------

//void dump_components(opj_image_t *image) {
//  fprintf(::stderr, "> image has %d component(s).\n", image->numcomps);
//  fprintf(::stderr, "> (%d,%d - %d,%d), colorspace=%d\n", image->x0, image->y0,
//          image->x1, image->y1, image->color_space);
//
//  for (int i = 0; i < image->numcomps; i++) {
//    fprintf(::stderr, ">> component %d: ", i);
//    fprintf(::stderr, ">> dx=%d, dy=%d, w=%d, h=%d, x0=%d, y0=%d, "
//            "prec=%d, bpp=%d, sgnd=%d, resno_decoded=%d, factor=%d\n",
//            image->comps[i].dx, image->comps[i].dy, image->comps[i].w,
//            image->comps[i].h, image->comps[i].x0, image->comps[i].y0,
//            image->comps[i].prec, image->comps[i].bpp, image->comps[i].sgnd,
//            image->comps[i].resno_decoded, image->comps[i].factor);
//  }
//}

bool is_supported_gray_format(opj_image_t *image) {
  return (image->numcomps == 1 && image->comps[0].factor == 0);
}

bool is_supported_rgb_format(opj_image_t *image) {
  if (image->numcomps != 3)
    return false;
  return (image->comps[0].w == image->comps[1].w
      && image->comps[1].w == image->comps[2].w
      && image->comps[0].h == image->comps[1].h
      && image->comps[1].h == image->comps[2].h
      && image->comps[0].dx == image->comps[1].dx
      && image->comps[1].dx == image->comps[2].dx
      && image->comps[0].dy == image->comps[1].dy
      && image->comps[1].dy == image->comps[2].dy && image->comps[0].prec == 8
      && image->comps[1].prec == 8 && image->comps[2].prec == 8
      && image->comps[0].factor == 0 && image->comps[1].factor == 0
      && image->comps[2].factor == 0);
}

template<class T> void __copyfrom(T *dst, int dststep, int *src, int w, int h) {
  int wc;
  char *ptr = (char *) dst;

  while (h--) {
    dst = (T *) ptr;
    wc = w;
    while (wc--)
      *dst++ = *src++;
    ptr += dststep;
  }
}

int is_jp2(char *src, int srclen) {
  if (srclen < 8)
    return -1;
  if (!memcmp(src + 4, "jP  ", 4))
    return 1;  // jp2
  return 0;  // try codestream
}

DICOMSDL_CODEC_RESULT __decode_opj_jpeg2k(char *src, int srclen,
                                          imagecontainer *ic) {
  opj_dparameters_t parameters;
  opj_image_t* image = NULL;
  opj_codec_t* l_codec = NULL;
  opj_stream_t *l_stream = NULL; /* Stream */
  opj_codestream_index_t* cstr_index = NULL;

  DICOMSDL_CODEC_RESULT result = DICOMSDL_CODEC_OK;

  opj_set_default_decoder_parameters(&parameters);

  bytestream bs;
  bs.data = src;
  bs.datasize = srclen;
  bs.offset = 0;

  l_stream = dicomsdl_create_memory_stream(&bs, 1);
  if (!l_stream) {
    snprintf(ic->info, ARGBUF_SIZE, "__decode_opj_jpeg2k(...): "
             "ERROR -> failed to create the stream");
    result = DICOMSDL_CODEC_ERROR;
    goto fin;
  }

  if (is_jp2(src, srclen) > 0) {  // -- JP2
    l_codec = opj_create_decompress(OPJ_CODEC_JP2);
  } else {  // try codestream
    l_codec = opj_create_decompress(OPJ_CODEC_J2K);
  }

  // catch events using our callbacks and give a local context
  opj_set_info_handler(l_codec, info_callback, ic);
  opj_set_warning_handler(l_codec, warning_callback, ic);
  opj_set_error_handler(l_codec, error_callback, ic);

  // Setup the decoder decoding parameters using user parameters
  if (!opj_setup_decoder(l_codec, &parameters)) {
    snprintf(ic->info, ARGBUF_SIZE, "__decode_opj_jpeg2k(...): "
             "ERROR -> opj_decompress: failed to setup the decoder");
    result = DICOMSDL_CODEC_ERROR;
    goto fin;
  }

  // Read the main header of the codestream and if necessary the JP2 boxes
  if (!opj_read_header(l_stream, l_codec, &image)) {
    snprintf(ic->info, ARGBUF_SIZE, "__decode_opj_jpeg2k(...): "
             "ERROR -> opj_decompress: failed to read the header");
    result = DICOMSDL_CODEC_ERROR;
    goto fin;
  }

  // Optional if you want decode the entire image
  if (!opj_set_decode_area(l_codec, image, 0, 0, 0, 0)){
    snprintf(ic->info, ARGBUF_SIZE, "__decode_opj_jpeg2k(...): "
             "ERROR -> opj_decompress: failed to set the decoded area");
    result = DICOMSDL_CODEC_ERROR;
    goto fin;
  }

  // Get the decoded image
  if (!(opj_decode(l_codec, l_stream, image)
      && opj_end_decompress(l_codec, l_stream))) {
    snprintf(ic->info, ARGBUF_SIZE, "__decode_opj_jpeg2k(...): "
             "ERROR -> opj_decompress: failed to decode image!");
    result = DICOMSDL_CODEC_ERROR;
    goto fin;
  }

  // TODO - color space and icc profile management
  // TODO - /* Force output precision */
  // TODO - /* Upsample components */
  // TODO - /* Force RGB output */

  result = opj_image_to_image(image, ic);

  fin: if (l_stream)
    opj_stream_destroy(l_stream);
  if (l_codec)
    opj_destroy_codec(l_codec);
  if (image)
    opj_image_destroy(image);
  opj_destroy_cstr_index(&cstr_index);

  return result;
}

DICOMSDL_CODEC_RESULT opj_image_to_image(opj_image_t *image,
                                         imagecontainer *ic) {
  int width = 0, height = 0, precision = 0;
  int ncomponent = image->numcomps;
  int signedness = 0;
  unsigned char *dst = (unsigned char *) (ic->data);

  // Gray scale image
  if (is_supported_gray_format(image)) {
    width = image->comps[0].w;
    height = image->comps[0].h;
    precision = image->comps[0].prec;
    signedness = image->comps[0].sgnd;

    if (ic->rowstep < 0)
      dst += -(ic->rowstep) * (height - 1);

    if (precision > 8) {
      signedness ?
          __copyfrom((short *) dst, ic->rowstep, image->comps[0].data, width,
                     height) :
          __copyfrom((unsigned short *) dst, ic->rowstep, image->comps[0].data,
                     width, height);
    } else {  // bpp == 1
      signedness ?
          __copyfrom((char *) dst, ic->rowstep, image->comps[0].data, width,
                     height) :
          __copyfrom((unsigned char *) dst, ic->rowstep, image->comps[0].data,
                     width, height);
    }
  }

  // 24bit RGB image
  else if (is_supported_rgb_format(image)) {
    width = image->comps[0].w;
    height = image->comps[0].h;
    precision = image->comps[0].prec;  // 8
    signedness = image->comps[0].sgnd;

    if (ic->rowstep < 0)
      dst += -ic->rowstep * (height - 1);
    int *r = image->comps[0].data, *g = image->comps[1].data, *b =
        image->comps[2].data;
    unsigned char *q;
    int wc, h = height;

    while (h--) {
      q = dst, wc = width;
      while (wc--) {
        *q++ = *r++;
        *q++ = *g++;
        *q++ = *b++;
      }
      dst += ic->rowstep;
    }
  }

  // Can't decode image
  else {
    snprintf(ic->info, ARGBUF_SIZE, "opj_image_to_image(...): "
            "cannot decode image");
//    dump_components(image);
    return DICOMSDL_CODEC_ERROR;  // DICOM_ERROR
  }

  ic->rows = height;
  ic->cols = width;
  ic->prec = precision;
  ic->sgnd = signedness;
  ic->ncomps = ncomponent;

  return DICOMSDL_CODEC_OK;  // DICOM_OK
}

}  // namespace dicom ------------------------------------------------------
