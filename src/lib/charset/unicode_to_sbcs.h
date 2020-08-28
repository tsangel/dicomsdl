/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * unicode_to_sbcs.h
 */

#ifndef DICOMSDL_UNICODE_TO_SBCS_H__
#define DICOMSDL_UNICODE_TO_SBCS_H__

namespace dicom {
extern "C" {

#include "unicode_to_sbcs_table.h"

#define ADVANCE_UC_MB(u,m)  do {\
  stat->wc += (u); stat->wc_left -= (u);\
  stat->mb += (m); stat->mb_left -= (m);\
  } while(0);

// -----------------------------------------------------------------------------

conv_result_t unicode_to_ascii(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;

    // G0 Element
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }
    return CONV_ILLEGAL_UNICODE;
  }
  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_latin1(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;

    // G0 Element
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // G1 Element
    if (c < 0x100) {
      if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_100) {
        stat->last_charset_g1_used = CHARSET::ISO_2022_IR_100;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x2d;
        stat->mb[2] = 0x41;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }
    return CONV_ILLEGAL_UNICODE;
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_latin2(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // G0 Element
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // G1 Element
    if (c < 0x00a0)
      b = c;
    else if (c <= 0x017f)
      b = map_unicode_to_latin2_00a0_017f[c - 0xa0];
    else if (c >= 0x02d0 && c <= 0x02df)
      b = map_unicode_to_latin2_02d0_02df[c - 0x02d0];
    else
      return CONV_ILLEGAL_UNICODE;

    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_101) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_101;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x42;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_latin3(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // G0 Element
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // G1 Element
    if (c < 0x00a0)
      b = c;
    else if (c <= 0x017f)
      b = map_unicode_to_latin3_00a0_017f[c - 0xa0];
    else if (c >= 0x02d0 && c <= 0x02df)
      b = map_unicode_to_latin3_02d0_02df[c - 0x02d0];
    else
      return CONV_ILLEGAL_UNICODE;

    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_109) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_109;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x43;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_latin4(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // G0 Element
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // G1 Element
    if (c < 0x00a0)
      b = c;
    else if (c <= 0x017f)
      b = map_unicode_to_latin4_00a0_017f[c - 0xa0];
    else if (c >= 0x02c0 && c <= 0x02df)
      b = map_unicode_to_latin4_02c0_02df[c - 0x02c0];
    else
      return CONV_ILLEGAL_UNICODE;

    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_110) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_110;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x44;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_cyrillic(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // U+0000~U+007F -> 0x00~0x7F (128 chars)
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    if (c <= 0x00a0)
      b = c;
    else if (c >= 0x0400 && c <= 0x045f)
      b = map_unicode_to_cyrillic_0400_045f[c - 0x0400];
    else {
      switch (c) {
        case 0x00a7:
          b = unicode_to_cyrillic_ua7;
          break;
        case 0x00ad:
          b = unicode_to_cyrillic_uad;
          break;
        case 0x2116:
          b = unicode_to_cyrillic_u2116;
          break;
        default:
          return CONV_ILLEGAL_UNICODE;
          break;
      }
    }

    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_144) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_144;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x4c;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_arabic(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // U+0000~U+007F -> 0x00~0x7F (128 chars)
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    if (c <= 0x00a0 || c == 0xa4 || c == 0xad)
      b = c;
    else if (c >= 0x0600 && c <= 0x065f)
      b = map_unicode_to_arabic_0600_065f[c - 0x0600];
    else
      return CONV_ILLEGAL_UNICODE;

    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_127) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_127;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x47;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_greek(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // U+0000~U+007F -> 0x00~0x7F (128 chars)
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    if (c < 0x00a0)
      b = c;
    else if (c <= 0x0bf)
      b = map_unicode_to_greek_00a0_00bf[c - 0xa0];
    else if (c >= 0x0370 && c <= 0x03cf)
      b = map_unicode_to_greek_0370_03cf[c - 0x0370];
    else if (c >= 0x2010 && c <= 0x201f)
      b = map_unicode_to_greek_2010_201f[c - 0x2010];
    else {
      switch (c) {
        case 0x20ac:
          b = unicode_to_greek_u20ac;
          break;
        case 0x20af:
          b = unicode_to_greek_u20ac;
          break;
        default:
          return CONV_ILLEGAL_UNICODE;
          break;
      }
    }
    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_126) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_126;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x46;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_hebrew(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // U+0000~U+007F -> 0x00~0x7F (128 chars)
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    if (c < 0x00a0)
      b = c;
    else if (c <= 0x0bf)
      b = map_unicode_to_hebrew_00a0_00bf[c - 0xa0];
    else if (c >= 0x05d0 && c <= 0x05ef)
      b = map_unicode_to_hebrew_05d0_05ef[c - 0x05d0];
    else if (c >= 0x2010 && c <= 0x201f)
      b = map_unicode_to_hebrew_2010_201f[c - 0x2010];
    else {
      switch (c) {
        case 0x00d7:
          b = unicode_to_hebrew_ud7;
          break;
        case 0x00f7:
          b = unicode_to_hebrew_uf7;
          break;
        case 0x200e:
          b = unicode_to_hebrew_u200e;
          break;
        case 0x200f:
          b = unicode_to_hebrew_u200f;
          break;
        case 0x2017:
          b = unicode_to_hebrew_u2017;
          break;
        default:
          return CONV_ILLEGAL_UNICODE;
          break;
      }
    }
    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_138) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_138;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x48;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_latin5(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // G0 Element
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // G1 Element
    if (c < 0x00d0)
      b = c;
    else if (c <= 0x0ff)
      b = map_unicode_to_latin5_00a0_00ff[c - 0xa0];
    else {
      switch (c) {
        case 0x011e:
          b = unicode_to_latin5_u11e;
          break;
        case 0x011f:
          b = unicode_to_latin5_u11f;
          break;
        case 0x0130:
          b = unicode_to_latin5_u130;
          break;
        case 0x0131:
          b = unicode_to_latin5_u131;
          break;
        case 0x015e:
          b = unicode_to_latin5_u15e;
          break;
        case 0x015f:
          b = unicode_to_latin5_u15f;
          break;
        default:
          return CONV_ILLEGAL_UNICODE;
          break;
      }
    }

    if (b == 0)
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_2022_IR_148) {
      stat->last_charset_g1_used = CHARSET::ISO_2022_IR_148;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x4d;
      ADVANCE_UC_MB(0, 3);
    }

    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_jisx0201(conv_stat_t *stat) {
  int codestat = 0;

  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b = 0;

    // U+0020~U+005B -> 0x20~0x5B (60 chars)
    // U+005D~U+007D -> 0x5D~0x7D (33 chars)
    if ((c >= 0x0020 && c <= 0x005b) || (c >= 0x005d && c <= 0x007d)) {
      if (c == DELIM_CARET || c == DELIM_EQUAL)
        return CONV_DELIMITER;

      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // U+FF61~U+FF9F -> 0xA1~0xDF (63 chars)
    else if (c >= 0xff61 && c <= 0xff9f) {
      if (stat->last_charset_g1_used != CHARSET::KATAKANA) {
        stat->last_charset_g1_used = CHARSET::KATAKANA;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x29;
        stat->mb[2] = 0x49;
        ADVANCE_UC_MB(0, 3);
      }
      *stat->mb = c - (0xff61 - 0xa1);
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // U+00A5~U+00A5 -> 0x5C~0x5C ( 1 chars)
    else if (c == 0xa5)
      b = 0x5c;

    // U+203E~U+203E -> 0x7E~0x7E ( 1 chars)
    else if (c == 0x203e)
      b = 0x7e;

    else
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g0_used != CHARSET::KATAKANA) {
      stat->last_charset_g0_used = CHARSET::KATAKANA;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x28;
      stat->mb[2] = 0x4a;
      ADVANCE_UC_MB(0, 3);
    }

    if (b == 0x5c)  // DELIM_BACKSLASH
      return CONV_DELIMITER;

    *stat->mb = b;  // 0x5c or 0x7e
    ADVANCE_UC_MB(1, 1);
  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

conv_result_t unicode_to_thai(conv_stat_t *stat) {
  while (stat->wc_left > 0) {
    if (stat->mb_left < 4) {
      if (expand_multibyte_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uc4_t c = *stat->wc;
    uint8_t b;

    // U+0000~U+00A0 -> 0x00~0xA0 (161 chars)
    if (c < 0x0080) {
      if (IFDELIM(c))
        return CONV_DELIMITER;

      if (stat->last_charset_g0_used != CHARSET::DEFAULT) {
        stat->last_charset_g0_used = CHARSET::DEFAULT;
        stat->mb[0] = CONTROL_ESCAPE;
        stat->mb[1] = 0x28;
        stat->mb[2] = 0x42;
        ADVANCE_UC_MB(0, 3);
      }
      *stat->mb = c;
      ADVANCE_UC_MB(1, 1);
      continue;
    }

    // U+0000~U+00A0 -> 0x00~0xA0 (161 chars)
    if (c <= 0x00a0) {
      b = c;
    }
    // U+0E01~U+0E3A -> 0xA1~0xDA (58 chars)
    // U+0E3F~U+0E5B -> 0xDF~0xFB (29 chars)
    else if ((c >= 0x0e01 && c <= 0x0e3a) || (c >= 0x0e3f && c <= 0x0e5b))
      b = c - 0xd60;  // 0xd60 == 0xe01 - 0xa1 == 0xe3f - 0xdf

    else
      return CONV_ILLEGAL_UNICODE;

    if (stat->last_charset_g1_used != CHARSET::ISO_IR_166) {
      stat->last_charset_g1_used = CHARSET::ISO_IR_166;
      stat->mb[0] = CONTROL_ESCAPE;
      stat->mb[1] = 0x2d;
      stat->mb[2] = 0x54;
      ADVANCE_UC_MB(0, 3);
    }
    *stat->mb = b;
    ADVANCE_UC_MB(1, 1);

  }

  return CONV_OK;
}

// -----------------------------------------------------------------------------

#undef ADVANCE_UC_MB

}
}

#endif // DICOMSDL_UNICODE_TO_SBCS_H__
