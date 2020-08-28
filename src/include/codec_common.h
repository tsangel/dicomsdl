/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * codec_common.h
 */

#ifndef DICOMSDL_CODEC_COMMON_H__
#define DICOMSDL_CODEC_COMMON_H__

#ifdef _MSC_VER
	#define CODEC_EXPORTS extern "C" __declspec(dllexport)
#else
	#define CODEC_EXPORTS extern "C"
#endif

#define ARGBUF_SIZE 256

typedef enum {
  DICOMSDL_CODEC_OK,
  DICOMSDL_CODEC_NOTSUPPORTED,
  DICOMSDL_CODEC_WARN,
  DICOMSDL_CODEC_INFO,
  DICOMSDL_CODEC_ERROR
} DICOMSDL_CODEC_RESULT;

typedef struct {
  char *data;   // pixel data to be encoded
                // or data buffer to store decoded pixel data
  long datasize; // size of data (in bytes)
                // should be larger than rowstep * rows
  int rowstep;  // number of bytes between each rows
                // rowstep may have negative value
                // absolute value of rowstep should be larger than
                // cols*ncomps*(prec>8?2:1)

  int rows;  // height
  int cols;  // width
  int prec;  // precision - number of bits of one component in a pixel
             // each pixel takes 2 bytes if prec > 8
  int sgnd;  // 1 = signed, 0 = unsigned
  int ncomps;  // 1 = gray, 3 = RGA, 4 = RGBA

  int lossy;   // 1 = lossy compression was done by encoder
               // 0 = lossless compression was done by encoder
               // 1 = lossy decompression was done by decoder
               // 0 = lossless decompression was done by decoder
               // -1 = decoder doesn't know about it

  char args[ARGBUF_SIZE];  // argument string for encoder/decoder
  char info[ARGBUF_SIZE];  // error message from encoder/decoder
  int result;  // DICOMSDL_CODEC_OK
               // DICOMSDL_CODEC_NOTSUPPORTED
               // DICOMSDL_CODEC_ERROR - error message is set in info[]
               // DICOMSDL_CODEC_INFO - some message in info[]
               // DICOMSDL_CODEC_WARN - some warning message in info[]
} imagecontainer;

/* decode pixel data
 *
 * return DICOMSDL_CODEC_OK - no error (DICOM_OK)
 *        DICOMSDL_CODEC_NOTSUPPORTED - I don't handle image with this tsuid
 *        DICOMSDL_CODEC_ERROR - error, set error message in info[]
 *        DICOMSDL_CODEC_INFO - some message in info[]
 *        DICOMSDL_CODEC_WARN - some warning message in info[]
 *
 * char* = transfer syntax UID value such as "1.2.840.10008.1.2.4.50"
 * char*, long = data and data size of encoded pixel
 * caller should set
 *  ic->data, ic->datasize , ic->rowstep, ic->rows, ic->cols,
 *  ic->prec, ic->sgnd, ic->ncomps
 *  and ic->args[] (if needed)
 *
 * caller should prepare
 *  ic->data with size ic->datasize
 *  ic->datasize should be equal to ic->rowstep*ic->rows*ic->ncomps*bpp
 *    where bpp = (prec > 8?2:1)
 *
 * decoder sets
 *  ic->data, ic->info[256];
 *  ic->lossy (1 = lossy, 0 = lossless, -1 = i don't know)
 */
typedef DICOMSDL_CODEC_RESULT (*decoder_fnptr)(const char *, char *, long,
                                               imagecontainer *);

/* free memory allocated by encoder
 */
typedef void (*free_memory_fnptr)(char *);

/* encode pixel data
 *
 * return DICOMSDL_CODEC_OK - no error (DICOM_OK)
 *        DICOMSDL_CODEC_NOTSUPPORTED - I don't handle image with this tsuid
 *        DICOMSDL_CODEC_ERROR - error, set error message in info[]
 *        DICOMSDL_CODEC_INFO - some message in info[]
 *        DICOMSDL_CODEC_WARN - some warning message in info[]
 *
 * char* = transfer syntax UID value such as "1.2.840.10008.1.2.4.50"
 * caller should set
 *  ic->data, ic->datasize, ic->rowstep, ic->rows, ic->cols,
 *  ic->prec, ic->sgnd, ic->ncomps,
 *  ic->args[256];
 *  char**, long* = data and its size of encoded pixel data
 *       data is allocated by encoder
 *
 *  free_memory_fnptr* = release memory which is allocated by encoder.
 *       caller should call (*free_memory_fnptr)(char*) after calling encoder.
 *
 * encode_function set
 *  ic->lossy; 1 if lossy compression was done, 0 for lossless compression
 *  ic->info[256];
 *  encoded data to char ** and long *.
 *  free memory function for allocated encoded data.
 */
typedef DICOMSDL_CODEC_RESULT (*encoder_fnptr)(const char *, imagecontainer *,
                                               char **, long *,
                                               free_memory_fnptr *);

#ifdef _MSC_VER
	#ifndef snprintf
		#define snprintf _snprintf
	#endif
	#ifndef strtok_r
		#define strtok_r(s,d,p) strtok_s(s,d,p)
	#endif
#endif

#include "codec_keys.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* argument string parser
 * parser take semicolon separated key=value pairs

	imagecontainer *ic = ...;
	argparser p(ic);

	int key;

	while ((key = p.get_next_argkey()) > 0) {
		switch (key) {
			case ARGKEY_RATE:
				rate = p.value_as_double(); // process double value
				break;
			case ARGKEY_LAYER:
				layer = p.value_as_int(); // process int value
				break;
			case ARGKEY_REVERSIBLE:
				rev = p.value_as_string(); // process string
                break;
			default:
				break;
        }
	}
	if (key < 0)
		{}; // error in parsing key
 */

class argparser {
  char buf[ARGBUF_SIZE];  // same to ARGBUF_SIZE
  int key;
  char *arg, *val;  // pointer to argument key and value string
  char *ctx;  // for strtok_r
  imagecontainer *ic;

 public:
  argparser(imagecontainer *ic) {
    this->ic = ic;
    memcpy(buf, ic->args, ARGBUF_SIZE);
    key = 0;
    arg = val = ctx = NULL;
  }

  /* get next argument's key
   * return -1 on error; error message is set at imagecontainer.info
   * return 0 when all argument are parsed
   */
  int get_next_argkey() {
    if (key == 0)
      arg = strtok_r(buf, ";", &ctx);  // get first key
    else
      arg = strtok_r(NULL, ";", &ctx);  // move to next key

    if (arg) {  // process key
      if (!(val = strchr(arg, '='))) {
        snprintf(ic->info, ARGBUF_SIZE, "arg key '%s' has no value", arg);
        return -1;  // error
      }

      *val++ = '\0';
      key = get_argkey(arg);
      if (key == 0) {
        snprintf(ic->info, ARGBUF_SIZE, "no such arg key '%s'", arg);
        return -1;  // error
      }
      return key;
    }

    return 0;  // no more key
  }

  int value_as_int() {
    return (key ? (int) strtol(val, (char **) NULL, 10) : 0);
  }

  double value_as_double() {
    return (key ? strtod(val, (char **) NULL) : 0.0);
  }

  const char* value_as_string() {
    return (const char *) val;
  }

};

#endif // DICOMSDL_CODEC_COMMON_H__
