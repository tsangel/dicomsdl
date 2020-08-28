/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * utf8.cc
 */

#include "csconv.h"

namespace dicom {

#define ADVANCE_UC_MB(u,m)  do {\
  stat->wc += (u); stat->wc_left -= (u);\
  stat->mb += (m); stat->mb_left -= (m);\
  } while(0);

conv_result_t utf8_to_unicode(conv_stat_t *stat) {
  uint8_t b1, b2, b3, b4;
  uc4_t u;
  while (stat->mb_left > 0) {
    if (stat->wc_left < 2) {
      if (expand_unicode_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }
    b1 = stat->mb[0];
    // 0xxx xxxx
    if (b1 < 0x80) {
      *stat->wc = b1;
      // printf("%02x-> %04u\n", b1, *stat->uc);
      ADVANCE_UC_MB(1, 1);
    } else
    // 110x xxxx  10xx xxxx
    if ((b1 & 0xe0) == 0xc0) {
      if (stat->mb_left < 2)
        return CONV_TOOFEW;
      b2 = stat->mb[1];
      if ((b2 & 0xc0) != 0x80)
        return CONV_ILLEGAL_SEQUENCE;
      *stat->wc = (((uc4_t) b1 & 0x1f) << 6) + ((uc4_t) b2 & 0x3f);
      // printf("%02x %02x -> %04u\n", b1, b2, *stat->uc);
      ADVANCE_UC_MB(1, 2);
    } else
    // 1110 xxxx  10xx xxxx  10xx xxxx
    if ((b1 & 0xf0) == 0xe0) {
      if (stat->mb_left < 3)
        return CONV_TOOFEW;
      b2 = stat->mb[1];
      b3 = stat->mb[2];
      if (((b2 & 0xc0) != 0x80) || ((b3 & 0xc0) != 0x80))
        return CONV_ILLEGAL_SEQUENCE;
      *stat->wc = (((uc4_t) b1 & 0x0f) << 12) + (((uc4_t) b2 & 0x3f) << 6)
          + ((uc4_t) b3 & 0x3f);
      // printf("%02x %02x %02x -> %04u\n", b1, b2, b3, *stat->uc);
      ADVANCE_UC_MB(1, 3);
    } else
    // 1111 0xxx  10xx xxxx  10xx xxxx  10xx xxxx
    if ((b1 & 0xf8) == 0xf0) {
      if (stat->mb_left < 4)
        return CONV_TOOFEW;
      b2 = stat->mb[1];
      b3 = stat->mb[2];
      b4 = stat->mb[3];
      if (((b2 & 0xc0) != 0x80) || ((b3 & 0xc0) != 0x80) || ((b3 & 0xc0) != 0x80))
        return CONV_ILLEGAL_SEQUENCE;

      // printf("%02x %02x %02x %02x -> %04u\n", b1, b2, b3, b4, *stat->uc);
# if WCHAR_WIDTH == 32
      *stat->wc = (((uc4_t) b1 & 0x07) << 18) + (((uc4_t) b2 & 0x3f) << 12)
          + (((uc4_t) b3 & 0x3f) << 6) + ((uc4_t) b4 & 0x3f);
      ADVANCE_UC_MB(1, 4);
# elif WCHAR_WIDTH == 16
      stat->wc[0] = (((wchar_t) b1 & 0x07) << 8) + (((wchar_t) b2 & 0x3f) << 2)
          + (((wchar_t) b3 & 0x30) >> 4) + 0xD800 - 0x40;
      stat->wc[1] = (((wchar_t) b3 & 0x0f) << 6) + ((wchar_t) b4 & 0x3f) + 0xDC00;
      ADVANCE_UC_MB(2, 4);
# else
#  error "I don't know sizeof(wchar_t)"
# endif

    }
  }
  return CONV_OK;
}

conv_result_t unicode_to_utf8(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t u = *stat->wc;
    int ucsize = 1;
    if ((u & 0xFC00) == 0xD800) {
      if (stat->wc_left < 2)
        return CONV_ILLEGAL_UNICODE;
      if ((stat->wc[1] & 0xFC00) != 0xDC00)
        return CONV_ILLEGAL_UNICODE;
      u = ((stat->wc[0] & 0x3FF) << 10) + (stat->wc[1] & 0x3FF) + 0x10000;
      ucsize = 2;
    }

    // 0xxx xxxx
    //  111 1111
    if (u < 0x80) {
      stat->mb[0] = u;
      ADVANCE_UC_MB(1, 1);
    } else
    // 110x xxxx  10xx xxxx
    //       111  1122 2222
    if (u < 0x800) {
      stat->mb[0] = (u >> 6) + 0xc0;
      stat->mb[1] = (u & 0x3f) + 0x80;
      ADVANCE_UC_MB(1, 2);
    } else
    // 1110 xxxx  10xx xxxx  10xx xxxx
    // 1111 2222  2233 3333
    if (u < 0x10000) {
      stat->mb[0] = (u >> 12) + 0xe0;
      stat->mb[1] = ((u & 0xfc0) >> 6) + 0x80;
      stat->mb[2] = (u & 0x3f) + 0x80;
      ADVANCE_UC_MB(1, 3);
    } else
    // 1111 0xxx  10xx xxxx  10xx xxxx  10xx xxxx
    //    1 1122  2222 3333  3344 4444
    if (u < 0x110000) {
      stat->mb[0] = (u >> 18) + 0xf0;
      stat->mb[1] = ((u & 0x3f000) >> 12) + 0x80;
      stat->mb[2] = ((u & 0xfc0) >> 6) + 0x80;
      stat->mb[3] = (u & 0x3f) + 0x80;
      ADVANCE_UC_MB(ucsize, 4);
    } else
      return CONV_ILLEGAL_UNICODE;
  }
  return CONV_OK;
}

}
