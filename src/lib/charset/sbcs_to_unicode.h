/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * sbcs_to_unicode.h
 */

#ifndef DICOMSDL_SBCS_TO_UNICODE_H__
#define DICOMSDL_SBCS_TO_UNICODE_H__

namespace dicom {
extern "C" {

#include "sbcs_to_unicode_table.h"

conv_result_t __to_unicode(conv_stat_t *stat) {
  while (stat->mb_left > 0) {
    if (stat->wc_left < 1) {
      if (expand_unicode_buffer(stat) == NULL)
        return CONV_MEMERROR;
    }

    uint8_t b = *stat->mb;
    uc4_t c;

    if (b < 0x80) {
      // use set G0
      c = stat->set_g0[b];
      if (c == 0) {
        if (b == CONTROL_ESCAPE)
          return CONV_ESCAPE;
        else
          // control characters
          return CONV_DELIMITER;
      }
    } else
      // use set G1
      c = stat->set_g1[b - 0x80];

    if (c == 0xfffd)
      return CONV_ILLEGAL_SEQUENCE;

    *stat->wc++ = c;
    stat->mb++;
    stat->wc_left--;
    stat->mb_left--;
  }
  return CONV_OK;
}

conv_result_t ascii_to_unicode(conv_stat_t *stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_null_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t latin1_to_unicode(conv_stat_t *stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_latin1_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t latin2_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_latin2_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t latin3_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_latin3_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t latin4_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_latin4_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t cyrillic_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_cyrillic_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t arabic_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_arabic_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t greek_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_greek_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t hebrew_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_hebrew_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t latin5_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_latin5_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t jisx0201_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_jisx0201_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_jisx0201_to_unicode_g1;
  }
  return __to_unicode(stat);
}

conv_result_t thai_to_unicode(conv_stat_t * stat) {
  if (stat->set_g0 == NULL) {
    stat->set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
    stat->set_g1 = (uint16_t *) map_thai_to_unicode_g1;
  }
  return __to_unicode(stat);
}

}
}

#endif // DICOMSDL_SBCS_TO_UNICODE_H__
