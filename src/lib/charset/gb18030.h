/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * chinese.cc
 */

namespace dicom {
extern "C" {

#include "gb18030_table.h"

#define ADVANCE_UC_MB(u,m)  do {\
  stat->wc += (u); stat->wc_left -= (u);\
  stat->mb += (m); stat->mb_left -= (m);\
  } while(0);

conv_result_t gb18030_to_unicode(conv_stat_t *stat) {
  uint8_t b1, b2, b3, b4;

  while (stat->mb_left > 0) {
    if (stat->wc_left < 2) {
      if (expand_unicode_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    // GBK area ----------------------------------------------------------------
    // unicode < U+0080, single byte

    b1 = stat->mb[0];

    if (b1 < 0x80) {
      *stat->wc = b1;
      ADVANCE_UC_MB(1, 1);
    }  // end of codes for 1 byte sequence -----
    else {

      // GBK including GB2312 --------------------------------------------------

      if (stat->mb_left < 2)
        return CONV_TOOFEW;

      b2 = stat->mb[1];

      if (b1 >= 0x81 && b1 <= 0xfe && b2 >= 0x40 && b2 <= 0xfe) {
        if (b2 == 0x7f)
          return CONV_ILLEGAL_SEQUENCE;

        int index = (b1 - 0x81) * 190 + b2 - 0x40;  // each row has 190 characters
        if (b2 > 0x7f)
          index--;

        *stat->wc = map_gb18030_to_unicode[index];
        ADVANCE_UC_MB(1, 2);
      }  // end of codes for 2-byte sequence
      else {

        // GB18030 -------------------------------------------------------------

        if (stat->mb_left < 4)
          return CONV_TOOFEW;

        b3 = stat->mb[2];
        b4 = stat->mb[3];

        // unicode < U+FFFF ----------------------------------------------------

        if ((b1 >= 0x81) && (b1 <= 0x84) && (b2 >= 0x30) && (b2 <= 0x39)) {
          if ((b3 < 0x81) || (b3 > 0xfe) || (b4 < 0x30) || (b4 > 0x39))
            return CONV_ILLEGAL_SEQUENCE;

          // Calculate using gb18030_codepoint_range and gb18030_translate_offset.

          unsigned int cp = (b4 - 0x30) + (b3 - 0x81) * 10 + (b2 - 0x30) * 1260
              + (b1 - 0x81) * 12600;

          const int gb18030_codepoint_size = sizeof(gb18030_codepoint_range)
              / sizeof(uint16_t) / 2;

          if (cp > gb18030_codepoint_range[gb18030_codepoint_size * 2 - 1])
            return CONV_ILLEGAL_SEQUENCE;

          // Binary search gb18030_codepoint_range
          int l = 0, r = gb18030_codepoint_size, m;
          while (true) {
            m = (l + r) / 2;

            if (cp < gb18030_codepoint_range[m * 2])
              r = m;
            else if (cp > gb18030_codepoint_range[m * 2 + 1])
              l = m;
            else
              break;
          }

          *stat->wc = gb18030_unicode_range[m * 2] + cp
              - gb18030_codepoint_range[m * 2];
          ADVANCE_UC_MB(1, 4);
        } else

        // unicode U+10000-U+10FFFF --------------------------------------------

        if ((b1 >= 0x90) && (b1 <= 0xe3) && (b2 >= 0x30) && (b2 <= 0x39)) {
          if ((b3 < 0x81) || (b3 > 0xfe) || (b4 < 0x30) || (b4 > 0x39))
            return CONV_ILLEGAL_SEQUENCE;
          uc4_t u = (b4 - 0x30) + (b3 - 0x81) * 10 + (b2 - 0x30) * 1260
              + (b1 - 0x90) * 12600 + 0x10000;
          if (u > 0x10ffff) {
            return CONV_ILLEGAL_SEQUENCE;
          }

# if WCHAR_WIDTH == 32
          *stat->wc = u;
          ADVANCE_UC_MB(1, 4);
# elif WCHAR_WIDTH == 16
          u -= 0x10000;
          stat->wc[0] = 0xD800 | (u >> 10);
          stat->wc[1] = 0xDC00 | (u & 0x3FF);
          ADVANCE_UC_MB(2, 4);
# else
#  error "I don't know sizeof(wchar_t)"
# endif
        } else
          return CONV_ILLEGAL_SEQUENCE;
      }  // end of codes for 4-byte sequence
    }  // end of codes for 2 or 4-byte sequence
  }  // end of while
  return CONV_OK;
}

conv_result_t gb2312_to_unicode(conv_stat_t *stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_null_to_unicode_g1;
  }

  uint8_t b1, b2, b3, b4;

  while (stat->mb_left > 0) {
    if (stat->wc_left < 1) {
      if (expand_unicode_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    // unicode < U+0080, single byte -------------------------------------------
    b1 = stat->mb[0];

    if (b1 < 0x80) {
      uc4_t c = stat->set_g0[b1];
      if (c == 0) {
        if (b1 == CONTROL_ESCAPE)
          return CONV_ESCAPE;
        else
          return CONV_DELIMITER;
      } else if (c == 0xfffd)
        return CONV_ILLEGAL_SEQUENCE;

      *stat->wc = c;
      ADVANCE_UC_MB(1, 1);
    }  // end of codes for 1 byte sequence -----
    else {
      // GBK including GB2312 --------------------------------------------------

      if (stat->mb_left < 2)
        return CONV_TOOFEW;

      b2 = stat->mb[1];

      if (b1 >= 0xa1 && b1 <= 0xfe && b2 >= 0xa1 && b2 <= 0xfe) {
        int index = (b1 - 0x81) * 190 + b2 - 0x40;  // each row has 190 characters
        index--;  // The table has a hole at 0x7f.
        *stat->wc = map_gb18030_to_unicode[index];
        ADVANCE_UC_MB(1, 2);
      } else
        return CONV_ILLEGAL_SEQUENCE;
    }  // end of codes for 2-byte sequence -----
  }  // end of while
  return CONV_OK;
}

conv_result_t unicode_to_gbk(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 8) {
      if (expand_unicode_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t uc = *stat->wc;
    uint8_t b1, b2, b3, b4;
    uint32_t cp;

    // unicode < U+80 ----------------------------------------------------------
    if (uc < 0x80) {
      *stat->mb = uc;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // unicode < U+10000 -------------------------------------------------------
    if ((uc >= 0x80) && (uc <= 0xffff)) {
      uint16_t mb = 0;

      // Try GBK
      int pageindex;
      int codeindex;
      uint16_t usebit;

      pageindex = map_gb18030_unicode_to_index[uc >> 8];
      if (pageindex > 0) {
        codeindex = pageindex + ((uc & 0xf0) >> 3);
        usebit = map_gb18030_unicode_to_index[codeindex + 1];
        codeindex = map_gb18030_unicode_to_index[codeindex];
        usebit >>= (15 - (uc & 0xf));
        if (usebit & 1) {
          usebit >>= 1;
          codeindex += bitsum_table[usebit >> 8] + bitsum_table[usebit & 0xff];
          mb = map_gb18030_index_to_multibyte[codeindex];
          stat->mb[0] = mb >> 8;
          stat->mb[1] = mb & 0xff;
          ADVANCE_UC_MB(1, 2);
          continue;
        }
      }
    }

    return CONV_ILLEGAL_UNICODE;
  }

  return CONV_OK;
}

conv_result_t unicode_to_gb18030(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 8) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t uc = *stat->wc;
    int ucsize = 1;
    if ((uc & 0xFC00) == 0xD800) {
      if (stat->wc_left < 2)
        return CONV_ILLEGAL_UNICODE;
      if ((stat->wc[1] & 0xFC00) != 0xDC00)
        return CONV_ILLEGAL_UNICODE;
      uc = ((stat->wc[0] & 0x3FF) << 10) + (stat->wc[1] & 0x3FF) + 0x10000;
      ucsize = 2;
    }

    uint8_t b1, b2, b3, b4;
    uint32_t cp;

    // unicode < U+80 ----------------------------------------------------------
    if (uc < 0x80) {
      *stat->mb = uc;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // unicode < U+10000 -------------------------------------------------------
    if ((uc >= 0x80) && (uc <= 0xffff)) {
      uint16_t mb = 0;

      // Try GBK
      int pageindex;
      int codeindex;
      uint16_t usebit;

      pageindex = map_gb18030_unicode_to_index[uc >> 8];
      if (pageindex > 0) {
        codeindex = pageindex + ((uc & 0xf0) >> 3);
        usebit = map_gb18030_unicode_to_index[codeindex + 1];
        codeindex = map_gb18030_unicode_to_index[codeindex];
        usebit >>= (15 - (uc & 0xf));
        if (usebit & 1) {
          usebit >>= 1;
          codeindex += bitsum_table[usebit >> 8] + bitsum_table[usebit & 0xff];
          mb = map_gb18030_index_to_multibyte[codeindex];
          stat->mb[0] = mb >> 8;
          stat->mb[1] = mb & 0xff;
          ADVANCE_UC_MB(1, 2);
          continue;
        }
      }

      // If uc is not a GBK character,
      // Search gb18030_unicode_range
      int l = 0, r = sizeof(gb18030_unicode_range) / 2 / sizeof(uint16_t), m;

      while (true) {
        m = (l + r) / 2;
        if (uc < gb18030_unicode_range[m * 2])
          r = m;
        else if (uc > gb18030_unicode_range[m * 2 + 1]) {
          if (l == m) {
            return CONV_ILLEGAL_UNICODE;
            break;
          }
          l = m;
        } else
          break;  // I found it!
      }

      cp = gb18030_codepoint_range[m * 2] + uc - gb18030_unicode_range[m * 2];
      stat->mb[3] = (cp % 10) + 0x30;
      cp /= 10;
      stat->mb[2] = (cp % 126) + 0x81;
      cp /= 126;
      stat->mb[1] = (cp % 10) + 0x30;
      cp /= 10;
      stat->mb[0] = cp + 0x81;
      ADVANCE_UC_MB(1, 4);
      continue;
    }

    // unicode U+10000-U+10FFFF ------------------------------------------------
    if ((uc >= 0x10000) && (uc <= 0x10ffff)) {
      uc -= 0x10000;
      stat->mb[3] = (uc % 10) + 0x30;
      uc /= 10;
      stat->mb[2] = (uc % 126) + 0x81;
      uc /= 126;
      stat->mb[1] = (uc % 10) + 0x30;
      uc /= 10;
      stat->mb[0] = uc + 0x90;
      ADVANCE_UC_MB(ucsize, 4);
      continue;
    }

    return CONV_ILLEGAL_UNICODE;
  }

  return CONV_OK;
}

typedef enum {
  CHINESE_CODESTAT_DEFAULT,
  CHINESE_CODESTAT_CHINESE
} chinese_code_stat_t;

conv_result_t unicode_to_gb2312(conv_stat_t * stat) {
  chinese_code_stat_t codestat = CHINESE_CODESTAT_DEFAULT;

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

      pageindex = map_gb18030_unicode_to_index[uc >> 8];
      if (pageindex == 0)
        return CONV_ILLEGAL_UNICODE;

      codeindex = pageindex + ((uc & 0xf0) >> 3);
      usebit = map_gb18030_unicode_to_index[codeindex + 1];
      codeindex = map_gb18030_unicode_to_index[codeindex];
      usebit >>= (15 - (uc & 0xf));
      if ((usebit & 1) == 0)
        return CONV_ILLEGAL_UNICODE;

      usebit >>= 1;
      codeindex += bitsum_table[usebit >> 8] + bitsum_table[usebit & 0xff];
      mb = map_gb18030_index_to_multibyte[codeindex];

      if ((codestat != CHINESE_CODESTAT_CHINESE)
          || (stat->last_charset_g1_used != CHARSET::GB2312)) {
        codestat = CHINESE_CODESTAT_CHINESE;
        stat->last_charset_g1_used = CHARSET::GB2312;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x24;
        stat->mb[2] = 0x29;
        stat->mb[3] = 0x41;
        ADVANCE_UC_MB(0, 4);
      }

      stat->mb[0] = mb >> 8;
      stat->mb[1] = mb & 0xff;
      ADVANCE_UC_MB(1, 2);
    }
  }
  return CONV_OK;
}

}
}
