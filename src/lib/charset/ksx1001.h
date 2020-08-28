/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * ksx1001.cc
 */

namespace dicom {
extern "C" {

#include "ksx1001_table.h"

#define ADVANCE_UC_MB(u,m)  do {\
  stat->wc += (u); stat->wc_left -= (u);\
  stat->mb += (m); stat->mb_left -= (m);\
  } while(0);

conv_result_t ksx1001_to_unicode(conv_stat_t *stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_null_to_unicode_g1;
  }

  while (stat->mb_left > 0) {
    if (stat->wc_left < 1) {
      if (expand_unicode_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uint8_t b1, b2;
    b1 = *stat->mb;
    if (b1 & 0x80) {
      if (stat->mb_left < 2)
        return CONV_TOOFEW;

      b2 = (stat->mb)[1];
      if (b1 < 0xa1 || b2 < 0xa1 || b1 > 0xfe || b2 > 0xfe)
        return CONV_ILLEGAL_SEQUENCE;
      b1 -= 0xa1;
      b2 -= 0xa1;

      uint16_t uc = map_ksx1001_multibyte_to_unicode  //
          [map_ksx1001_multibyte_to_unicode[b1] + b2];
      if (uc == 0)
        return CONV_ILLEGAL_SEQUENCE;

      *stat->wc = uc;
      ADVANCE_UC_MB(1, 2);
    } else {
      uc4_t c = stat->set_g0[b1];
      if (c == 0) {
        if (b1 == CONTROL_ESCAPE)
          return CONV_ESCAPE;
        else
          return CONV_DELIMITER;
      } else if (c == 0xfffd)
        return CONV_ILLEGAL_SEQUENCE;

      *stat->wc = b1;
      ADVANCE_UC_MB(1, 1);
    }
  }

  return CONV_OK;
}

/*
 dicom.convert_from_unicode('摩'.encode('UTF-32') , dicom.CODESET.KOREAN).decode('euc-kr')
 dicom.convert_to_unicode('摩'.encode('euc-kr') , dicom.CODESET.KOREAN).decode('UTF-32')
 dicom.convert_from_unicode('摩'.encode('UTF-32') , dicom.CODESET.GB18030).decode('gb18030')
 dicom.convert_from_unicode('\x81'.encode('GB18030'), dicom.CODESET.GB18030)
 */

typedef enum {
  HANGUL_CODESTAT_DEFAULT,
  HANGUL_CODESTAT_HANGUL
} hangul_code_stat_t;

conv_result_t unicode_to_ksx1001(conv_stat_t *stat) {
  hangul_code_stat_t codestat = HANGUL_CODESTAT_DEFAULT;

  while (stat->wc_left > 0) {
    if (stat->mb_left < 8) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t uc = *stat->wc;
    if (uc < 0x80) {
      if (IFDELIM(uc))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = uc;
      ADVANCE_UC_MB(1, 1);
    } else {
      if (uc > 0xffff)
        return CONV_ILLEGAL_UNICODE;

      int pageindex;
      int codeindex;
      uint16_t usebit;
      uint16_t mb = 0;

      pageindex = map_ksx1001_unicode_to_index[uc >> 8];
      if (pageindex == 0)
        return CONV_ILLEGAL_UNICODE;

      codeindex = pageindex + ((uc & 0xf0) >> 3);
      usebit = map_ksx1001_unicode_to_index[codeindex + 1];
      codeindex = map_ksx1001_unicode_to_index[codeindex];
      usebit >>= (15 - (uc & 0xf));
      if ((usebit & 1) == 0)
        return CONV_ILLEGAL_UNICODE;

      usebit >>= 1;
      codeindex += bitsum_table[usebit >> 8] + bitsum_table[usebit & 0xff];
      mb = map_ksx1001_index_to_multibyte[codeindex] | 0x8080;

      if ((codestat != HANGUL_CODESTAT_HANGUL)
          || (stat->last_charset_g1_used != CHARSET::KOREAN)) {
        codestat = HANGUL_CODESTAT_HANGUL;
        stat->last_charset_g1_used = CHARSET::KOREAN;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x24;
        stat->mb[2] = 0x29;
        stat->mb[3] = 0x43;
        ADVANCE_UC_MB(0, 4);
      }

      stat->mb[0] = mb >> 8;
      stat->mb[1] = mb & 0xff;
      ADVANCE_UC_MB(1, 2);
    }
  }
  return CONV_OK;
}

#undef ADVANCE_UC_MB

}
}
