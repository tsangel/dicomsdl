/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * csconv.cc
 */

/* TODO:

 If within a textual value a character set
 other than the one specified in value 1 of the Attribute Specific Character Set (0008,0005),
 or the default character repertoire if value 1 is missing,
 has been invoked,
 the character set specified in the value 1
 (, or the default character repertoire if value 1 is missing, )
 shall be active in the following instances:

 6.1.2.5.3 Requirements

 Before CR/LF, FF, and any other control characters other than ESC
 Before 0x5c (backslash or Yen sign)
 Before the "^" and "=" delimiters in Data Elements with a VR of PN.

 Before the first use of the character set in the line
 Before the first use of the character set in the page
 Before the first use of the character set in the Data Element value
 Before the first use of the character set in the name component and name component group in Data Element with a VR of PN Note
 */

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "dicom.h"
#include "util.h"
// #include "outstream.h"

#include "csconv.h"

#include "sbcs_to_unicode.h"
#include "unicode_to_sbcs.h"
#include "ksx1001.h"
#include "jisx0208.h"
#include "jisx0212.h"
#include "gb18030.h"

namespace dicom {

typedef struct {
  const charset_t codeset;
  const charset_t charset_g0;
  const charset_t charset_g1;
  const char *term;
  const char *description;

  func_mbtouc_t func_mbtouc;
  func_uctomb_t func_uctomb;
} codeset_struct_t;

const codeset_struct_t CODESETS[] = {
    //
    // Table C.12-2. Defined Terms for Single-Byte Character Sets Without Code
    // Extensions
    //
    // Default repertoire -- ISO-IR 6
    {CHARSET::DEFAULT, CHARSET::DEFAULT, CHARSET::UNKNOWN, "",
     "Default repertoire", ascii_to_unicode, unicode_to_ascii},
    // Default repertoire -- ISO-IR 6
    {CHARSET::ISO_IR_6, CHARSET::ISO_IR_6, CHARSET::UNKNOWN, "ISO_IR 6",
     "Default repertoire", ascii_to_unicode, unicode_to_ascii},
    // Latin alphabet No. 1
    {CHARSET::ISO_IR_100, CHARSET::DEFAULT, CHARSET::ISO_IR_100, "ISO_IR 100",
     "Latin alphabet No. 1", latin1_to_unicode, unicode_to_latin1},
    // Latin alphabet No. 2
    {CHARSET::ISO_IR_101, CHARSET::DEFAULT, CHARSET::ISO_IR_101, "ISO_IR 101",
     "Latin alphabet No. 2", latin2_to_unicode, unicode_to_latin2},
    // Latin alphabet No. 3
    {CHARSET::ISO_IR_109, CHARSET::DEFAULT, CHARSET::ISO_IR_109, "ISO_IR 109",
     "Latin alphabet No. 3", latin3_to_unicode, unicode_to_latin3},
    // Latin alphabet No. 4
    {CHARSET::ISO_IR_110, CHARSET::DEFAULT, CHARSET::ISO_IR_110, "ISO_IR 110",
     "Latin alphabet No. 4", latin4_to_unicode, unicode_to_latin4},
    // Cyrillic
    {CHARSET::ISO_IR_144, CHARSET::DEFAULT, CHARSET::ISO_IR_144, "ISO_IR 144",
     "Cyrillic", cyrillic_to_unicode, unicode_to_cyrillic},
    // Arabic
    {CHARSET::ISO_IR_127, CHARSET::DEFAULT, CHARSET::ISO_IR_127, "ISO_IR 127",
     "Arabic", arabic_to_unicode, unicode_to_arabic},
    // Greek
    {CHARSET::ISO_IR_126, CHARSET::DEFAULT, CHARSET::ISO_IR_126, "ISO_IR 126",
     "Greek", greek_to_unicode, unicode_to_greek},
    // Hebrew
    {CHARSET::ISO_IR_138, CHARSET::DEFAULT, CHARSET::ISO_IR_138, "ISO_IR 138",
     "Hebrew", hebrew_to_unicode, unicode_to_hebrew},
    // Latin alphabet No. 5
    {CHARSET::ISO_IR_148, CHARSET::DEFAULT, CHARSET::ISO_IR_148, "ISO_IR 148",
     "Latin alphabet No. 5", latin5_to_unicode, unicode_to_latin5},
    // JIS X 0201
    // charset_g0 'ISO_IR_13' actually activate code set ISO_IR_14 JIX X 0201:
    // Romanji
    {CHARSET::ISO_IR_13, CHARSET::ISO_IR_13, CHARSET::ISO_IR_13, "ISO_IR 13",
     "Japanese", jisx0201_to_unicode, unicode_to_jisx0201},
    // Thai
    {CHARSET::ISO_IR_166, CHARSET::DEFAULT, CHARSET::ISO_IR_166, "ISO_IR 166",
     "Thai", thai_to_unicode, unicode_to_thai},

    //
    // Table C.12-3. Defined Terms for Single-Byte Character Sets with Code
    // Extensions
    //
    // Default repertoire
    {CHARSET::ISO_2022_IR_6, CHARSET::DEFAULT, CHARSET::UNKNOWN,
     "ISO 2022 IR 6", "Default repertoire", ascii_to_unicode, unicode_to_ascii},
    // Latin alphabet No. 1
    {CHARSET::ISO_2022_IR_100, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_100,
     "ISO 2022 IR 100", "Latin alphabet No. 1", latin1_to_unicode,
     unicode_to_latin1},
    // Latin alphabet No. 2
    {CHARSET::ISO_2022_IR_101, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_101,
     "ISO 2022 IR 101", "Latin alphabet No. 2", latin2_to_unicode,
     unicode_to_latin2},
    // Latin alphabet No. 3
    {CHARSET::ISO_2022_IR_109, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_109,
     "ISO 2022 IR 109", "Latin alphabet No. 3", latin3_to_unicode,
     unicode_to_latin3},
    // Latin alphabet No. 4
    {CHARSET::ISO_2022_IR_110, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_110,
     "ISO 2022 IR 110", "Latin alphabet No. 4", latin4_to_unicode,
     unicode_to_latin4},
    // Cyrillic
    {CHARSET::ISO_2022_IR_144, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_144,
     "ISO 2022 IR 144", "Cyrillic", cyrillic_to_unicode, unicode_to_cyrillic},
    // Arabic
    {CHARSET::ISO_2022_IR_127, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_127,
     "ISO 2022 IR 127", "Arabic", arabic_to_unicode, unicode_to_arabic},
    // Greek
    {CHARSET::ISO_2022_IR_126, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_126,
     "ISO 2022 IR 126", "Greek", greek_to_unicode, unicode_to_greek},
    // Hebrew
    {CHARSET::ISO_2022_IR_138, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_138,
     "ISO 2022 IR 138", "Hebrew", hebrew_to_unicode, unicode_to_hebrew},
    // Latin alphabet No. 5
    {CHARSET::ISO_2022_IR_148, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_148,
     "ISO 2022 IR 148", "Latin alphabet No. 5", latin5_to_unicode,
     unicode_to_latin5},
    // JIS X 0201
    {CHARSET::ISO_2022_IR_13, CHARSET::ISO_2022_IR_13, CHARSET::ISO_2022_IR_13,
     "ISO 2022 IR 13", "Japanese", jisx0201_to_unicode, unicode_to_jisx0201},
    // Thai
    {CHARSET::ISO_2022_IR_166, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_166,
     "ISO 2022 IR 166", "Thai", thai_to_unicode, unicode_to_thai},

    //
    // Table C.12-4. Defined Terms for Multi-Byte Character Sets with Code
    // Extensions
    //
    // JIS X 0208:Kanji
    {CHARSET::ISO_2022_IR_87, CHARSET::ISO_2022_IR_87, CHARSET::UNKNOWN,
     "ISO 2022 IR 87", "Japanese", jisx0208_to_unicode, unicode_to_jisx0208},
    // JIS X 0212: Supplementary Kanji set
    {CHARSET::ISO_2022_IR_159, CHARSET::ISO_2022_IR_159, CHARSET::UNKNOWN,
     "ISO 2022 IR 159", "Japanese", jisx0212_to_unicode, unicode_to_jisx0212},
    // KS X 1001: Hangul and Hanja
    {CHARSET::ISO_2022_IR_149, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_149,
     "ISO 2022 IR 149", "Korean", ksx1001_to_unicode, unicode_to_ksx1001},
    // GB 2312-80 China Association for Standardization
    {CHARSET::ISO_2022_IR_58, CHARSET::DEFAULT, CHARSET::ISO_2022_IR_58,
     "ISO 2022 IR 58", "Simplified Chinese", gb2312_to_unicode,
     unicode_to_gb2312},

    //
    // Table C.12-5. Defined Terms for Multi-Byte Character Sets Without Code
    // Extensions
    //
    // Unicode
    {CHARSET::ISO_IR_192, CHARSET::UNKNOWN, CHARSET::UNKNOWN, "ISO_IR 192",
     "Unicode in UTF-8", utf8_to_unicode, unicode_to_utf8},
    // GB18030
    {CHARSET::GB18030, CHARSET::UNKNOWN, CHARSET::UNKNOWN, "GB18030", "GB18030",
     gb18030_to_unicode, unicode_to_gb18030},
    // GBK
    {CHARSET::GBK, CHARSET::UNKNOWN, CHARSET::UNKNOWN, "GBK", "GBK",
     gb18030_to_unicode, unicode_to_gbk}};

#define CONSUME_BYTES_FROM_MB(n) do { cs.mb += (n); cs.mb_left -= (n); } while(0)

static conv_result_t _convert_to_unicode(const char *inbuf, size_t inbuflen,
                                         wchar_t **outbuf, size_t *outbuflen,
                                         charset_t charset, std::string &errmsg) {
  if (int(charset) > CHARSET::GBK) {
    errmsg.append(
        sprint("convert_to_unicode - wrong CHARSET number (%d)", charset));
    return CONV_ARGUMENT_ERROR;
  }

  if (inbuf == NULL || inbuflen == 0) {
    *outbuf = NULL;
    *outbuflen = 0;
    return CONV_OK;
  }

  func_mbtouc_t func = CODESETS[charset].func_mbtouc;
  conv_stat_t cs;
  conv_result_t result;

  *outbuflen = inbuflen + 8;
  *outbuf = (wchar_t *)::malloc(*outbuflen * sizeof(wchar_t));
  if (*outbuf == NULL) {
    *outbuflen = 0;
    errmsg.append(sprint("convert_to_unicode - cannot malloc %d bytes",
                         *outbuflen * sizeof(wchar_t)));
    return CONV_MEMERROR;
  }

  cs.mb_start = cs.mb = (uint8_t *)inbuf;
  cs.mb_left = inbuflen;
  cs.wc_start = cs.wc = *outbuf;
  cs.wc_left = *outbuflen;
  cs.set_g0 = NULL;  // set_g0 and set_g1 will be set in func_mbtouc

  while (true) {
    result = func(&cs);
    if (result == CONV_OK)
      break;
    else if (result == CONV_ESCAPE) {
      cs.mb++;
      cs.mb_left--;

      if (cs.mb_left < 2)  // just ignore escape sequence
        continue;
      uint8_t c1, c2, c3;
      c1 = cs.mb[0];
      c2 = cs.mb[1];
      if (c1 == 0x28) {
        // Single Byte Character Sets G0 Escape
        if (c2 == 0x42) {
          // G0 Escape 02/08 04/02
          cs.set_g0 = (uint16_t *) map_ascii_to_unicode_g0;
          func = CODESETS[CHARSET::DEFAULT].func_mbtouc;
          CONSUME_BYTES_FROM_MB(2);
        } else if (c2 == 0x4a) {
          // G0 Escape 02/08 04/10
          cs.set_g0 = (uint16_t *) map_jisx0201_to_unicode_g0;
          func = CODESETS[CHARSET::KATAKANA].func_mbtouc;
          CONSUME_BYTES_FROM_MB(2);
        }
      } else if (c1 == 0x2d) {
        // Single Byte Character Sets G1 Escape
        switch (c2) {
          case 0x41:
            // G1 Escape 02/13 04/01 -- LATIN1
            cs.set_g1 = (uint16_t *) map_latin1_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_100].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x42:
            // G1 Escape 02/13 04/02 -- LATIN2
            cs.set_g1 = (uint16_t *) map_latin2_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_101].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x43:
            // G1 Escape 02/13 04/03 -- LATIN3
            cs.set_g1 = (uint16_t *) map_latin3_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_109].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x44:
            // G1 Escape 02/13 04/04 -- LATIN4
            cs.set_g1 = (uint16_t *) map_latin4_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_110].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x4c:
            // G1 Escape 02/13 04/12 -- CYRILLIC
            cs.set_g1 = (uint16_t *) map_cyrillic_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_144].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x47:
            // G1 Escape 02/13 04/07 -- ARABIC
            cs.set_g1 = (uint16_t *) map_arabic_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_127].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x46:
            // G1 Escape 02/13 04/06 -- GREEK
            cs.set_g1 = (uint16_t *) map_greek_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_126].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x48:
            // G1 Escape 02/13 04/08 -- HEBREW
            cs.set_g1 = (uint16_t *) map_hebrew_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_138].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x4d:
            // G1 Escape 02/13 04/01 -- LATIN5
            cs.set_g1 = (uint16_t *) map_latin5_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_148].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          case 0x54:
            // G1 Escape 02/13 05/04 -- THAI
            cs.set_g1 = (uint16_t *) map_thai_to_unicode_g1;
            func = CODESETS[CHARSET::ISO_2022_IR_166].func_mbtouc;
            CONSUME_BYTES_FROM_MB(2);
            break;
          default:
            break;
        }
      } else if (c1 == 0x29 && c2 == 0x49) {
        // G1 Escape 02/09 04/09 -- KATAKANA
        cs.set_g1 = (uint16_t *) map_jisx0201_to_unicode_g1;
        func = CODESETS[CHARSET::ISO_2022_IR_13].func_mbtouc;
        CONSUME_BYTES_FROM_MB(2);
      } else if (c1 == 0x24) {
        if (c2 == 0x42) {
          // G0 Escape 02/04 04/02 -- KANJI
          func = CODESETS[CHARSET::ISO_2022_IR_87].func_mbtouc;
          CONSUME_BYTES_FROM_MB(2);
        } else {
          if (cs.mb_left < 3)  // just ignore escape sequence
            continue;
          c3 = cs.mb[2];
          if (c2 == 0x28 && c3 == 0x44) {
            // G0 Escape 02/04 02/08 04/04
            func = CODESETS[CHARSET::KANJISUPP].func_mbtouc;
            CONSUME_BYTES_FROM_MB(3);
          } else if (c2 == 0x29 && c3 == 0x43) {
            // G1 Escape 02/04 02/09 04/03
            func = CODESETS[CHARSET::KOREAN].func_mbtouc;
            CONSUME_BYTES_FROM_MB(3);
          } else if (c2 == 0x29 && c3 == 0x41) {
            // G1 Escape 02/04 02/09 04/01
            func = CODESETS[CHARSET::GB2312].func_mbtouc;
            CONSUME_BYTES_FROM_MB(3);
          }
        }
      }
      result = CONV_OK;
      // END OF ESCAPE SEQ PROCESSING
    } else if (result == CONV_DELIMITER) {
      // Before CR/LF, FF, and any other control characters other than ESC
      // Before 0x5c (backslash or Yen sign)
      // Before the "^" and "=" delimiters in Data Elements with a VR of PN.
      // part05. 6.1.3 Control Characters
      // Table 6.1-1. DICOM Control Characters and Their Encoding
      // LF \x0a, FF \x0c, CR \x0d, ESC \x1b, TAB \x09

      // reset code sets.

      uint8_t b = *cs.mb++;
      cs.mb_left--;

      if (IFDELIM(b)) {
        cs.set_g0 = NULL;
        func = CODESETS[charset].func_mbtouc;
        *cs.wc++ = b;
        cs.wc_left--;
      } else {
        // ignore other control characters
      }

      result = CONV_OK;
    } else
      break; // any other error
  }

  if (result == CONV_OK) {
    // cs.wc_start can be changed during expand_multibyte_buffer
    *outbuflen = cs.wc - cs.wc_start;
    *outbuf = cs.wc_start;
    return CONV_OK;
  } else {
    // TODO: catch CONV_MEMERROR!
    errmsg.append(sprint("convert_to_unicode - can't convert byte 0x%02x at {%ld}", *cs.mb,
               inbuflen - cs.mb_left));
    ::free(*outbuf);
    *outbuf = NULL;
    *outbuflen = 0;
    return result;
  }
}

std::wstring convert_to_unicode(const char *inbuf, size_t inbuflen,
                                charset_t charset) {
  wchar_t *outbuf;
  size_t outbuflen;
  std::string errmsg("");
  conv_result_t result =
      _convert_to_unicode(inbuf, inbuflen, &outbuf, &outbuflen, charset, errmsg);

  if (result != CONV_OK) {
    LOGERROR_AND_THROW(errmsg.c_str());
  }

  std::wstring ws = std::wstring(outbuf, outbuflen);
  ::free(outbuf);
  return ws;
}

static conv_result_t _convert_from_unicode(const wchar_t *inbuf, size_t inbuflen,
                                          char **outbuf, size_t *outbuflen,
                                          charset_t charset,
                                          std::string &errmsg) {
  charset_t charsets_ex[20];

  if (int(charset) > CHARSET::GBK) {
    errmsg.append(
        sprint("convert_from_unicode - wrong CHARSET number (%d)", charset));
    return CONV_ARGUMENT_ERROR;
  }

  if (inbuf == NULL || inbuflen == 0) {
    *outbuf = NULL;
    *outbuflen = 0;
    return CONV_OK;
  }

  conv_stat_t cs;
  conv_result_t result;

  *outbuflen = inbuflen * 4;
  *outbuf = (char *)::malloc(*outbuflen * sizeof(char));

  if (*outbuf == NULL) {
    *outbuflen = 0;
    errmsg.append(sprint("convert_from_unicode - cannot malloc %d bytes",
                         *outbuflen * sizeof(char)));
    return CONV_MEMERROR;
  }

  cs.wc_start = cs.wc = (wchar_t *)inbuf;
  cs.wc_left = inbuflen;
  cs.mb_start = cs.mb = (uint8_t *)*outbuf;
  cs.mb_left = *outbuflen;

  switch (charset) {
    case CHARSET::KOREAN:
      charsets_ex[0] = CHARSET::DEFAULT;
      charsets_ex[1] = CHARSET::KOREAN;
      charsets_ex[2] = CHARSET::UNKNOWN;
      break;
    case CHARSET::KATAKANA:
    case CHARSET::KANJI:
    case CHARSET::KANJISUPP:
      charsets_ex[0] = CHARSET::DEFAULT;
      charsets_ex[1] = CHARSET::KATAKANA;
      charsets_ex[2] = CHARSET::JISX0208;
      charsets_ex[3] = CHARSET::JISX0212;
      charsets_ex[4] = CHARSET::UNKNOWN;
      break;
    case CHARSET::GB2312:
      charsets_ex[0] = CHARSET::DEFAULT;
      charsets_ex[1] = CHARSET::GB2312;
      charsets_ex[2] = CHARSET::UNKNOWN;
      break;
    case CHARSET::UNKNOWN: {
      int i = 0;
      charsets_ex[i++] = CHARSET::DEFAULT;
      charsets_ex[i++] = CHARSET::LATIN1;
      charsets_ex[i++] = CHARSET::LATIN2;
      charsets_ex[i++] = CHARSET::LATIN3;
      charsets_ex[i++] = CHARSET::LATIN4;
      charsets_ex[i++] = CHARSET::CYRILLIC;
      charsets_ex[i++] = CHARSET::ARABIC;
      charsets_ex[i++] = CHARSET::GREEK;
      charsets_ex[i++] = CHARSET::HEBREW;
      charsets_ex[i++] = CHARSET::LATIN5;
      charsets_ex[i++] = CHARSET::KATAKANA;
      charsets_ex[i++] = CHARSET::THAI;
      charsets_ex[i++] = CHARSET::JISX0208;
      charsets_ex[i++] = CHARSET::JISX0212;
      charsets_ex[i++] = CHARSET::KOREAN;
      charsets_ex[i++] = CHARSET::GB2312;
      charsets_ex[i] = CHARSET::UNKNOWN;
    }
      break;

    default:
      charsets_ex[0] = charset;
      charsets_ex[1] = CHARSET::UNKNOWN;
      break;
  }

  cs.last_charset_g0_used = CODESETS[charsets_ex[0]].charset_g0;
  cs.last_charset_g1_used = CODESETS[charsets_ex[0]].charset_g1;

  // skip first character if is BOM
  if (cs.wc_left && *cs.wc == 0xfeff) {
    cs.wc++;
    cs.wc_left--;
  }

  func_uctomb_t func;

  int ci = 0;  // charset index
  int lastci = 0;  // last charset index with any successful encoded characters.
  wchar_t *lastwc;  // last decoded character.

  while (true) {
    func = CODESETS[charsets_ex[ci]].func_uctomb;
    result = func(&cs);
    if (result == CONV_OK)
      break;
    else if (result == CONV_ILLEGAL_UNICODE) {
      if (lastwc != cs.wc) {
        lastci = ci;
        lastwc = cs.wc;
      }
      ci++;
      if (charsets_ex[ci] == CHARSET::UNKNOWN)
        ci = 0;
      if (ci == lastci && lastwc == cs.wc)
        // Tried all encoders, but no progression.
        break;
    } else if (result == CONV_DELIMITER) {
      // Reset to default character set.
      // 'cs.mb_left' should be >= 4 by 'expand_multibyte_buffer'
      // at each unicdeo_to_xxxx function

      ci = 0;
      lastci = 0;

      wchar_t u = *cs.wc;

      if (u == 0xa5) {  // 0x5c for YEN in JIS X 0201
        u = 0x5c;
      } else {
        if (cs.last_charset_g0_used != CHARSET::DEFAULT) {
          cs.mb[0] = CONTROL_ESCAPE;
          cs.mb[1] = 0x28;
          cs.mb[2] = 0x42;
          cs.mb += 3;
        }
      }

      *cs.mb++ = u;
      cs.wc++;
      cs.mb_left--;
      cs.wc_left--;

      // reset to default
      cs.last_charset_g0_used = CODESETS[charsets_ex[0]].charset_g0;
      cs.last_charset_g1_used = CODESETS[charsets_ex[0]].charset_g1;
    }
  }
  if (result == CONV_OK) {
    // cs.mb_start can be changed during expand_multibyte_buffer
    *outbuflen = cs.mb - cs.mb_start;
    *outbuf = (char *)cs.mb_start;
    return CONV_OK;
  } else {
    // TODO: catch CONV_MEMERROR!
    errmsg.append(sprint("convert_from_unicode - can't convert unicode char U+%04x at {%ld}", *cs.wc,
               inbuflen - cs.wc_left));
    ::free(*outbuf);
    *outbuf = NULL;
    *outbuflen = 0;
    return result;
  }
}

std::string convert_from_unicode(const wchar_t *inbuf, size_t inbuflen,
                                charset_t charset) {
  char *outbuf;
  size_t outbuflen;
  std::string errmsg("");
  conv_result_t result =
      _convert_from_unicode(inbuf, inbuflen, &outbuf, &outbuflen, charset, errmsg);

  if (result != CONV_OK) {
    LOGERROR_AND_THROW(errmsg.c_str());
  }

  std::string s = std::string(outbuf, outbuflen);
  ::free(outbuf);
  return s;
}

uint8_t *expand_multibyte_buffer(conv_stat_t *stat) {
  uint8_t *oldptr, *newptr;
  int oldsize, newsize;
  int offset;  // how far stat->mb has been advanced.

  oldptr = stat->mb_start;
  offset = stat->mb - oldptr;
  oldsize = offset + stat->mb_left;
  newsize = oldsize * 2;  // simply double space
  newptr = (uint8_t *) ::realloc(oldptr, newsize);
  if (newptr == NULL) {
    // error; oldptr should be released and should raise an error.
    return NULL;
  }

  stat->mb_start = newptr;
  stat->mb = newptr + offset;
  stat->mb_left += newsize - oldsize;
  return stat->mb;
}

wchar_t *expand_unicode_buffer(conv_stat_t *stat) {
  wchar_t *oldptr, *newptr;
  int oldsize, newsize;
  int offset;  // how far stat->uc has been advanced.

  oldptr = stat->wc_start;
  offset = stat->wc - oldptr;
  oldsize = offset + stat->wc_left;
  newsize = oldsize * 2;  // simply double space
  newptr = (wchar_t *) ::realloc(oldptr, newsize * sizeof(wchar_t));
  if (newptr == NULL) {
    // error; oldptr should be released and should raise an error.
    return NULL;
  }

  stat->wc_start = newptr;
  stat->wc = newptr + offset;
  stat->wc_left += newsize - oldsize;
  return stat->wc;
}

// Get character set from string; maybe (0008,0005)
charset_t CHARSET::from_string(const std::string &s){
  return CHARSET::from_string(s.c_str(), s.size());
}
charset_t CHARSET::from_string(const char *s, size_t size) {
  size_t i;

  // Default repertoire
  if (size == 0)
    return CHARSET::DEFAULT;

  // take first code string
  for (i = 0; i < size; i++) {
    if (s[i] == '\\') {
      size = i;
      break;
    }
  }

  // remove leading space
  for (i = 0; i < size; i++) {
    if (s[i] != ' ')
      break;
  }
  if (i != 0) {
    s += i;
    size -= i;
  }

  // remove trailing space
  for (i = size; i > 0; i--) {
    if (s[i - 1] != ' ')
      break;
  }
  size = i;

  charset_t charset = CHARSET::DEFAULT;
  if (size >= 13) {
    int i = (s[size - 2] - '0') * 10 + s[size - 1] - '0';
    switch (i) {
      case 0:
        charset = CHARSET::ISO_2022_IR_100;
        break;
      case 1:
        charset = CHARSET::ISO_2022_IR_101;
        break;
      case 9:
        charset = CHARSET::ISO_2022_IR_109;
        break;
      case 10:
        charset = CHARSET::ISO_2022_IR_110;
        break;
      case 44:
        charset = CHARSET::ISO_2022_IR_144;
        break;
      case 27:
        charset = CHARSET::ISO_2022_IR_127;
        break;
      case 26:
        charset = CHARSET::ISO_2022_IR_126;
        break;
      case 38:
        charset = CHARSET::ISO_2022_IR_138;
        break;
      case 48:
        charset = CHARSET::ISO_2022_IR_148;
        break;
      case 13:
        charset = CHARSET::ISO_2022_IR_13;
        break;
      case 66:
        charset = CHARSET::ISO_2022_IR_166;
        break;
      case 87:
        charset = CHARSET::ISO_2022_IR_87;
        break;
      case 59:
        charset = CHARSET::ISO_2022_IR_159;
        break;
      case 49:
        charset = CHARSET::ISO_2022_IR_149;
        break;
      case 58:
        charset = CHARSET::ISO_2022_IR_58;
        break;
      default:
        charset = CHARSET::ISO_2022_IR_6;
        break;
    }
  } else if (size >= 9) {
    int i = (s[size - 2] - '0') * 10 + s[size - 1] - '0';
    switch (i) {
      case 0:
        charset = CHARSET::ISO_IR_100;
        break;
      case 1:
        charset = CHARSET::ISO_IR_101;
        break;
      case 9:
        charset = CHARSET::ISO_IR_109;
        break;
      case 10:
        charset = CHARSET::ISO_IR_110;
        break;
      case 44:
        charset = CHARSET::ISO_IR_144;
        break;
      case 27:
        charset = CHARSET::ISO_IR_127;
        break;
      case 26:
        charset = CHARSET::ISO_IR_126;
        break;
      case 38:
        charset = CHARSET::ISO_IR_138;
        break;
      case 48:
        charset = CHARSET::ISO_IR_148;
        break;
      case 13:
        charset = CHARSET::ISO_IR_13;
        break;
      case 66:
        charset = CHARSET::ISO_IR_166;
        break;
      case 92:
        charset = CHARSET::ISO_IR_192;
        break;
      default:
        charset = CHARSET::DEFAULT;
        break;
    }
  } else if (size >= 8)
    charset = CHARSET::ISO_IR_6;
  else if (size >= 7)
    charset = CHARSET::GB18030;
  else if (size == 3)
    charset = CHARSET::GBK;

  // confirm string has Specific Character Set's Defined Term.
  if (strncmp(CODESETS[charset].term, s, size) == 0)
    return charset;
  else
    return CHARSET::UNKNOWN;
}

}
