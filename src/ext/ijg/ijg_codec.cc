/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * ijg_codec.cc
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "imagecodec.h"
#include "ijg_codec.h"

namespace dicom { //------------------------------------------------------

#define GETWORD(c, n)	if (p+1<q) \
					{ c = p; n = ((unsigned char)p[0]) * 256 + \
								  (unsigned char)p[1]; p+=2; } \
					else goto EXITLOOP;
#define GETBYTE(c, n)	if (p<q) \
					{ c = p; p++;  n = (unsigned char )*c; } \
					else goto EXITLOOP;
#define SKIP(n)		if (p+n<=q) { p += n; } \
					else goto EXITLOOP;

DICOMSDL_CODEC_RESULT encode_ijg_jpeg8(imagecontainer *ic, char **data,
                                       long *datasize, JPEG_MODE jmode,
                                       int quality);
DICOMSDL_CODEC_RESULT encode_ijg_jpeg12(imagecontainer *ic, char **data,
                                        long *datasize, JPEG_MODE jmode,
                                        int quality);
DICOMSDL_CODEC_RESULT encode_ijg_jpeg16(imagecontainer *ic, char **data,
                                        long *datasize, JPEG_MODE jmode,
                                        int quality);

DICOMSDL_CODEC_RESULT ijg_encoder(const char *tsuid, imagecontainer *ic,
                                  char **data, long *datasize,
                                  free_memory_fnptr *free_memory_fn) {
  JPEG_MODE jmode;

  // PS3.5 A.4.1 JPEG Image Compression
  // JPEG Baseline (Process 1): Default Transfer Syntax for Lossy JPEG 8 Bit Image Compression
  if (strcmp("1.2.840.10008.1.2.4.50", tsuid) == 0)
    jmode = JPEG_BASELINE;
  else
  // JPEG Extended (Process 2 & 4): Default Transfer Syntax for Lossy JPEG 12 Bit Image Compression (Process 4 only)
  if (strcmp("1.2.840.10008.1.2.4.51", tsuid) == 0)
    jmode = JPEG_EXTENDED;
  else
  // JPEG Lossless, Non-Hierarchical (Process 14)
  if (strcmp("1.2.840.10008.1.2.4.57", tsuid) == 0)
    jmode = JPEG_LOSSLESS;
  else
  // JPEG Lossless, Non-Hierarchical, First-Order Prediction (Process 14 [Selection Value 1]): Default Transfer Syntax for Lossless JPEG Image Compression
  if (strcmp("1.2.840.10008.1.2.4.70", tsuid) == 0)
    jmode = JPEG_LOSSLESS_SV1;
  else
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (!*data || datasize || free_memory_fn) {
    snprintf(ic->info, ARGBUF_SIZE,  "ijg_encoder(...): "
             "*data or datasize or *free_memory_fn is NULL.");
    return DICOMSDL_CODEC_ERROR;
  }
  *free_memory_fn = ijg_codec_free_memory;

  // check if jpeg encoder support image's precision.
  if (jmode == JPEG_BASELINE && ic->prec > 8) {
    sprintf(ic->info,
      "JPEG BASE PROCESS 1 encoding "
      "doesn't allow %d bits precision", ic->prec);
    return DICOMSDL_CODEC_ERROR;
  }
  if (jmode == JPEG_EXTENDED && ic->prec > 12) {
    sprintf(ic->info,
      "JPEG EXTENDED PROCESS 2 & 4 encoding "
      "doesn't allow %d bits precision", ic->prec);
    return DICOMSDL_CODEC_ERROR;
  }

  // parse argument

	int quality = 100;

	argparser p(ic); // argument is set in ic->args
	int key = 0;
	while ((key = p.get_next_argkey()) > 0) {
		switch (key) {
			case ARGKEY_QUALITY:
				{
					quality = p.value_as_int();
					if (quality <= 0 || quality > 100)
						quality = 100;
				}
				break;
			default:
				break;
        }
	}
	if (key != 0) {
		// argument key error, error message in ic->info
		return DICOMSDL_CODEC_ERROR;
	}

	// encode according to image's precision
	DICOMSDL_CODEC_RESULT ret;
	*data = (char *)malloc(ic->rows * ic->rowstep * 2);  // reserve just a large buffer
	*datasize = ic->rows * ic->rowstep * 2;

	if (ic->prec > 12)
		ret = encode_ijg_jpeg16(ic, data, datasize, jmode, quality);
	else if (ic->prec > 8)
		ret = encode_ijg_jpeg12(ic, data, datasize, jmode, quality);
	else
		ret = encode_ijg_jpeg8(ic, data, datasize, jmode, quality);

	ic->lossy = ((jmode==JPEG_BASELINE||jmode==JPEG_EXTENDED)?1:0);
	return ret;
}

// -----------------------------------------------------------------------

JPEG_MODE scan_jpeg_header(char *src, int srclen, imagecontainer *ic)
{
	int n;
	unsigned char *c;
	unsigned char *p = (unsigned char *)src,
			      *q = (unsigned char *)src+srclen;
	JPEG_MODE jmode = JPEG_UNKNOWN;

	ic->prec = 0;

	GETWORD(c, n);
	if (n != 0xffd8) goto EXITLOOP; // check SOI
	for (;;) {
		GETBYTE(c, n);
		if (n != 0xff) break; // Not a JPEG file
		GETBYTE(c, n);
		switch (n) {
		case 0xc0: // SOF0
		case 0xc1: // SOF1
		case 0xc3: // SOF3
			// itu-t81.pdf, Figure B.3 - Frame header syntax
			if (*c == 0xc0)		jmode = JPEG_BASELINE;
			else if (*c == 0xc1)	jmode = JPEG_EXTENDED;
			else					jmode = JPEG_LOSSLESS; // 0xc3

			GETWORD(c, n);
			GETBYTE(c, ic->prec);
			GETWORD(c, ic->rows);
			GETWORD(c, ic->cols);
			GETBYTE(c, ic->ncomps);
			break;

		// Other SOFs
		case 0xc2:	case 0xc5:	case 0xc6:	case 0xc7:
		case 0xc8:	case 0xc9:	case 0xca:	case 0xcb:
		case 0xcd:	case 0xce:	case 0xcf: // SOF15
			goto EXITLOOP; // Can't handle other than SOF0, SOF1 and SOF3
			break;

		case 0xc4: /* DHT */ case 0xcc: /* DAC */ case 0xda: /* SOS */
		case 0xdb: /* DQT */ case 0xdc: /* DNL */ case 0xdd: /* DRI */
		case 0xde: /* DHP */ case 0xdf: /* EXP */ case 0xfe: /* COM */
		case 0xe0:	case 0xe1:	case 0xe2:	case 0xe3:
		case 0xe4:	case 0xe5:	case 0xe6:	case 0xe7:
		case 0xe8:	case 0xe9:	case 0xea:	case 0xeb:
		case 0xec:	case 0xed:	case 0xee:	case 0xef:	// APPn
			GETWORD(c, n);	// length
			SKIP(n-2);
			break;

		case 0xd0:	case 0xd1:	case 0xd2:	case 0xd3:
		case 0xd4:	case 0xd5:	case 0xd6:	case 0xd7:	// RSTn
		case 0x01: /* TEM */ case 0xd9: /* EOI */
			break;	// no parameter
		}

		if (ic->prec)
			break;
	}
	return jmode;

	EXITLOOP:
	return jmode;
};


DICOMSDL_CODEC_RESULT decode_ijg_jpeg8
	(char *src, int srclen, imagecontainer *ic);
DICOMSDL_CODEC_RESULT decode_ijg_jpeg12
	(char *src, int srclen, imagecontainer *ic);
DICOMSDL_CODEC_RESULT decode_ijg_jpeg16
	(char *src, int srclen, imagecontainer *ic);


DICOMSDL_CODEC_RESULT ijg_decoder(const char *tsuid, char *data, long datasize,
                                             imagecontainer *ic)
 {
  if (
      // PS3.5 A.4.1 JPEG Image Compression
      strcmp("1.2.840.10008.1.2.4.50", tsuid) != 0 &&  // JPEG Baseline (Process 1): Default Transfer Syntax for Lossy JPEG 8 Bit Image Compression
      strcmp("1.2.840.10008.1.2.4.51", tsuid) != 0 &&  // JPEG Extended (Process 2 & 4): Default Transfer Syntax for Lossy JPEG 12 Bit Image Compression (Process 4 only)
      strcmp("1.2.840.10008.1.2.4.57", tsuid) != 0 &&  // JPEG Lossless, Non-Hierarchical (Process 14)
      strcmp("1.2.840.10008.1.2.4.70", tsuid) != 0  // "JPEG Lossless, Non-Hierarchical, First-Order Prediction (Process 14 [Selection Value 1]): Default Transfer Syntax for Lossless JPEG Image Compression
  )
    return DICOMSDL_CODEC_NOTSUPPORTED;

  if (!data) {
    snprintf(ic->info, ARGBUF_SIZE, "ijg_decoder(...): data == NULL");
    return DICOMSDL_CODEC_ERROR;
  }

  if (ic->datasize < ic->rowstep * ic->rows
      ||  ic->rowstep < ic->cols * (ic->prec > 8 ? 2 : 1) * ic->ncomps) {
    snprintf(ic->info, ARGBUF_SIZE, "ijg_decoder(...): "
             "pixelbuf for decoded image is too small; "
             "buflen %d < rowstep %d * rows %d or "
             "rowstep < cols %d * (prec %d > 8 ? 2 : 1) * ncomps %d",
             int(ic->datasize), ic->rowstep, ic->rows,
             ic->cols, ic->prec, ic->ncomps
    );
    return DICOMSDL_CODEC_ERROR;
  }

  DICOMSDL_CODEC_RESULT ret;
	JPEG_MODE jmode;

	// scan jpeg header
	jmode = scan_jpeg_header(data, datasize, ic);

	if (jmode != JPEG_UNKNOWN) {
		if (ic->prec > 12)
			ret = decode_ijg_jpeg16(data, datasize, ic);
		else if (ic->prec > 8)
			ret = decode_ijg_jpeg12(data, datasize, ic);
		else
			ret = decode_ijg_jpeg8(data, datasize, ic);
	} else {
	  // error message is in ic->info
		strcpy(ic->info, "cannot read jpeg header.");
		ret = DICOMSDL_CODEC_ERROR;  // ERROR
	}
	return ret;
}

extern "C" void ijg_codec_free_memory(char *data) {
  if (data)
    free(data);
}

} // namespace dicom ------------------------------------------------------
