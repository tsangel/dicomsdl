/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * deflate.cc
 */

#include "deflate.h"

#include "dicom.h"
#include "zlib/zlib.h"

namespace dicom {  //-----------------------------------------------------------

#define CHUNK 0x10000

void inflate_dicomfile(uint8_t *data, size_t datasize, std::ostringstream &oss,
                       size_t skip_offset) {
  // write first skip_offset bytes without inflation
  oss.write((const char *)data, (long)skip_offset);
  data += skip_offset;
  datasize -= skip_offset;

  // codes from zpipe.c ------------------------------------------------------

  int ret;
  unsigned have;
  z_stream strm;
  unsigned char out[CHUNK];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit2(&strm, -15);
  if (ret != Z_OK) goto INFLATE_END;

  strm.avail_in = datasize;
  strm.next_in = (unsigned char *)data;

  do {
    strm.avail_out = CHUNK;
    strm.next_out = out;
    ret = inflate(&strm, Z_NO_FLUSH);
    // assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR; /* and fall through */
        (void)inflateEnd(&strm);
        goto INFLATE_END;
        break;
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
        goto INFLATE_END;
        break;
      default:
        break;
    }
    have = CHUNK - strm.avail_out;
    oss.write((const char *)out, (long)have);
  } while (strm.avail_out == 0);

  (void)inflateEnd(&strm);

INFLATE_END:
  if (ret != Z_STREAM_END)
    LOGERROR_AND_THROW("inflate_dicomfile - cannot inflate file.");

  oss.flush();
  return;
}

void deflate_dicomfile(uint8_t *data, size_t datasize, std::ostringstream &oss,
                       size_t skip_offset, int level) {
  // write first skip_offset bytes without inflation
  oss.write((const char *)data, (long)skip_offset);
  data += skip_offset;
  datasize -= skip_offset;

  // codes from zpipe.c ------------------------------------------------------

  int ret, flush;
  unsigned have;
  z_stream strm;
  // unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit2(&strm, level, Z_DEFLATED, -15, MAX_MEM_LEVEL,
                     Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) goto DEFLATE_END;

  strm.avail_in = datasize;
  flush = Z_FINISH;
  strm.next_in = (unsigned char *)data;

  /* run deflate() on input until output buffer not full, finish
   compression if all of source has been read in */
  do {
    strm.avail_out = CHUNK;
    strm.next_out = out;
    ret = deflate(&strm, flush); /* no bad return value */
    // assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    have = CHUNK - strm.avail_out;
    oss.write((const char *)out, (long)have);
  } while (strm.avail_out == 0);
  // assert(strm.avail_in == 0);     /* all input will be used */

  /* done when last data in file processed */
  // assert(ret == Z_STREAM_END);        /* stream will be complete */
  /* clean up and return */
  (void)deflateEnd(&strm);
  // return Z_OK;

  // build return value -----------------------

DEFLATE_END:
  if (ret != Z_STREAM_END)
    LOGERROR_AND_THROW("deflate_dicomfile - cannot deflate file.");

  oss.flush();
  return;
}

}  // namespace dicom
