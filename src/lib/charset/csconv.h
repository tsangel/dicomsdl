/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * csconv.h
 */

#ifndef DICOMSDL__CSCONV_H__
#define DICOMSDL__CSCONV_H__

#include "dicom.h"

namespace dicom {
// Character Set Converter
// PS3.5 C.12.1.1.2. Specific Character Set
// convert values with a specific character set to unicode
// SH, LO, ST, PN, LT, UC or UT
typedef uint32_t uc4_t;  // unicode in 32 bit
typedef uint16_t uc2_t;  // unicode in 16 bit
//typedef unsigned char uint8_t;
//typedef int error_t;

typedef enum {
  // All bytes from source has been consumed.
  CONV_OK,

  // Converter need more buffer for output.
  // mb_remaining or uc_remaining may not be zero.
  CONV_CONTINUE,

  // Converter need more buffer for input.
  // mb_remaining or uc_remaining may not be zero.
  CONV_TOOFEW,

  // Error during expand output buffer
  CONV_MEMERROR,

  // Delimiter encountered. '^', '=', '\'
  // *mb pointing at delimiter.
  CONV_DELIMITER,

  // Escape character(0x1b) encountered.
  // *mb pointing at 0x1b.
  CONV_ESCAPE,

  // Illegal sequence of bytes
  CONV_ILLEGAL_SEQUENCE,

  // Illegal unicode
  CONV_ILLEGAL_UNICODE,

  // Wrong argument
  CONV_ARGUMENT_ERROR
} conv_result_t;

typedef struct conv_stat_t {
  uint8_t *mb;  // current pointer in multibyte buffer
  int mb_left;  // number of bytes left in multibyte buffer
  wchar_t *wc;  // current pointer in unicode buffer
  int wc_left;  // number of characters left in unicode buffer (1 char = 4 bytes)

  uint8_t *mb_start;  // pointer to multibyte buffer
  wchar_t *wc_start;  // pointer to unicode buffer

  uint16_t *set_g0;  // code set G0
  uint16_t *set_g1;  // code set G1

  // Set by encoder if it encode any character successfully.
  // Next encoder may need output escape sequence according to this.
  charset_t last_charset_g0_used;
  charset_t last_charset_g1_used;

  // Pointer to list of character sets for encoding.
  // The last element should be UNKNOWN (-1)
  charset_t *charsets;
} conv_stat_t;

#define DELIM_EQUAL  '='
#define DELIM_CARET  '^'

// Backslash or YEN sign
#define DELIM_BACKSLASH  '\x5c'

// part05. 6.1.3 Control Characters
// Table 6.1-1. DICOM Control Characters and Their Encoding
// LF \x0a, FF \x0c, CR \x0d, ESC \x1b, TAB \x09
#define CONTROL_LF  '\x0a'
#define CONTROL_FF  '\x0c'
#define CONTROL_CR  '\x0d'
#define CONTROL_TAB  '\x09'
#define CONTROL_ESCAPE  '\x1b'

#define IFDELIM(c) \
    ((c) == DELIM_BACKSLASH || (c) == DELIM_CARET || (c) == DELIM_EQUAL \
     || (c) == CONTROL_LF || (c) == CONTROL_FF || (c) == CONTROL_CR \
     || (c) == CONTROL_TAB)


// Expand multibyte buffer by factor 2.
// Returns new stat->mb.
// If an error occurred on realloc buffer, it return NULL.
uint8_t *expand_multibyte_buffer(conv_stat_t *stat);

// Expand unicode buffer by factor 2.
// Returns new stat->uc.
// If an error occurred on realloc buffer, it return NULL.
wchar_t *expand_unicode_buffer(conv_stat_t *stat);

conv_result_t utf8_to_unicode(conv_stat_t *stat);
conv_result_t unicode_to_utf8(conv_stat_t *stat);

typedef conv_result_t (*func_mbtouc_t)(conv_stat_t *stat);
typedef conv_result_t (*func_uctomb_t)(conv_stat_t *stat);

#endif
}
