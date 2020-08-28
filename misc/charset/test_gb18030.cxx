#include <stdio.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#include "gb18030.h"

typedef struct {
  uint8_t *ptr;
  size_t size; // number of items (= bytes)
  bool isalloc;  // true if *data needs to be 'free()'.
}  uint8_array_t;

typedef struct {
  uint32_t *ptr;
  size_t size; // number of items (= bytes/sizeof(uint32_t))
  bool isalloc;  // true if *data needs to be 'free()'.
}  uint32_array_t;


typedef struct conv_stat_t {
  uint8_t *mb;  // current pointer in multibyte buffer
  int mb_left;  // number of bytes left in multibyte buffer
  uc4_t *uc;  // current pointer in unicode buffer
  int uc_left;  // number of characters left in unicode buffer (1 char = 4 bytes)

  uint8_t *mb_start;  // pointer to multibyte buffer
  uc4_t *uc_start;  // pointer to unicode buffer

  uint16_t *set_g0;  // code set G0
  uint16_t *set_g1;  // code set G1

  // Set by encoder if it encode any character successfully.
  // Next encoder may need output escape sequence.
  CODESET::type charset;

  // Pointer to list of character sets for encoding.
  // The last element should be UNKNOWN (-1)
  CODESET::type *charsets;
} conv_stat_t;

int test()
{
  uint32_t uc;
  int uc_left;
  uint8_t buf[16];

  conv_stat_t cs;
  // set uc, ...
  
  CODESET::type ci, lastci;

  encoder = CODESETS[ci].func_uctomb;
  while (cs.uc_left) {
    uc = *cs.uc;
    int n = encoder(uc, buf);
    if (n > 0) {

      // write
    }


  }
}

uint32_t gb18030_to_unicode(uint8_t *b) {
  uint8_t b1, b2, b3, b4;
  uint32_t u;

  b1 = b[0];
  b2 = b[1];
  b3 = b[2];
  b4 = b[3];

  // GBK area ------------------------------------------------------------------
  // unicode < U+0080

  if (b1 < 0x80) {
    u = b1;
  } else

  // GB2312 / GBK area ---------------------------------------------------------

  if (b1 >= 0x81 && b1 <= 0xfe && b2 >= 0x40 && b2 <= 0xfe && b2 != 0x7f) {
    int index;

    index = (b1 - 0x81) * 190 + b2 - 0x40; // each row has 190 characters
    if (b2 > 0x7f)
      index --;

    u = map_gb18030_to_unicode[index];
  } else

  // unicode < U+FFFF ----------------------------------------------------------

  if ((b1 >= 0x81) && (b1 <= 0x84) && (b2 >= 0x30) && (b2 <= 0x39)) {
    if ((b3 < 0x81) || (b3 > 0xfe) || (b4 < 0x30) || (b4 > 0x39))
      return 0;  // ILLEGAL SEQUENCE

    // Calculate using gb18030_codepoint_range and gb18030_translate_offset.

    unsigned int cp = (b4 - 0x30) + (b3 - 0x81) * 10 + (b2 - 0x30) * 1260
        + (b1 - 0x81) * 12600;

    if (cp > 0x99fb)
      return 0;  // ILLEGAL SEQUENCE

    // Search gb18030_codepoint_range
    int l = 0, r = sizeof(gb18030_codepoint_range) / 2 / sizeof(uint16_t), m;

    while (true) {
      m = (l + r) / 2;

      if (cp < gb18030_codepoint_range[m * 2])
        r = m;
      else if (cp > gb18030_codepoint_range[m * 2 + 1])
        l = m;
      else
        break;
    }

    u = gb18030_unicode_range[m * 2] + cp - gb18030_codepoint_range[m * 2];
  } else

  // unicode U+10000-U+10FFFF --------------------------------------------------

  if ((b1 >= 0x90) && (b1 <= 0xe3) && (b2 >= 0x30) && (b2 <= 0x39)) {
    if ((b3 < 0x81) || (b3 > 0xfe) || (b4 < 0x30) || (b4 > 0x39))
      return 0;  // ILLEGAL SEQUENCE
    u = (b4 - 0x30) + (b3 - 0x81) * 10 + (b2 - 0x30) * 1260
        + (b1 - 0x90) * 12600 + 0x10000;
    if (u > 0x10ffff) {
      return 0;  // ILLEGAL SEQUENCE
    }
  }

  return u;
}

void unicode_to_gb18030(uint32_t u, uint8_t *b) {
  uint8_t b1, b2, b3, b4;
  uint32_t cp;


  // unicode < U+80 ------------------------------------------------------------
  if (u < 0x80) {
    b1 = u;
    b[0] = b1;
  } else

  // unicode < U+10000 ---------------------------------------------------------
  if (u >= 0x80 && u <= 0xffff) {

    // Try GBK -------------------------------------------------------------
    do {
      int pageindex;
      int codeindex;
      uint16_t usebit;
      uint16_t mb = 0;

      pageindex = map_gb18030_unicode_to_index[u >> 8];
      if (pageindex == 0)
        break; // try GB-18030

      codeindex = pageindex + ((u & 0xf0) >> 3);
      usebit = map_gb18030_unicode_to_index[codeindex + 1];
      codeindex = map_gb18030_unicode_to_index[codeindex];
      usebit >>= (15 - (u & 0xf));
      if (usebit & 1) {
        usebit >>= 1;
        codeindex += bitsum_table[usebit >> 8] + bitsum_table[usebit & 0xff];
        mb = map_gb18030_index_to_multibyte[codeindex] | 0x8080;
        b1 = mb >> 8;
        b2 = mb & 0xff;

        b[0] = b1;
        b[1] = b2;
        return; // OK
      }
    } while (0);

    // Search gb18030_unicode_range ---------------------------------------
    do {
      int l = 0, r = sizeof(gb18030_unicode_range) / 2 / sizeof(uint16_t), m;

      while (true) {
        m = (l + r) / 2;
        if (u < gb18030_unicode_range[m * 2])
          r = m;
        else if (u > gb18030_unicode_range[m * 2 + 1]) {
          if (l == m) {
            m = -1; // ILLEGAL SEQUENCE
            break;
          }
          l = m;
        } else
          break;  // I found it!
      }

      if (m >= 0) {
        cp = gb18030_codepoint_range[m * 2] + u - gb18030_unicode_range[m * 2];
        b4 = (cp % 10) + 0x30;
        cp /= 10;
        b3 = (cp % 126) + 0x81;
        cp /= 126;
        b2 = (cp % 10) + 0x30;
        cp /= 10;
        b1 = cp + 0x81;

        b[0] = b1;
        b[1] = b2;
        b[2] = b3;
        b[3] = b4;
        return;  // OK
      }
    } while (0);

    return; // ILLEGAL SEQUENCE
  } else

  // unicode U+10000-U+10FFFF --------------------------------------------------
  if ((u >= 0x10000) && (u <= 0x10ffff)) {
    u -= 0x10000;
    b4 = (u % 10) + 0x30;
    u /= 10;
    b3 = (u % 126) + 0x81;
    u /= 126;
    b2 = (u % 10) + 0x30;
    u /= 10;
    b1 = u + 0x90;

    b[0] = b1;
    b[1] = b2;
    b[2] = b3;
    b[3] = b4;
    return;  // OK
  }

  return;  // ILLEGAL SEQUENCE
}

int main() {
//  uint8_t b[4] = { 0x81, 0xc2, 0x00, 0x00 }; // GBK area
//  uint8_t b[4] = { 0x81, 0x30, 0x89, 0x37 }; // GB-18030 area
  uint8_t b[4] = { 0x93, 0x39, 0xf4, 0x39 }; // > U+10000

  printf("%02x %02x %02x %02x\n", b[0], b[1], b[2], b[3]);

  uint32_t u = gb18030_to_unicode(b);

  printf("%04x\n", u);

  b[0] = b[1] = b[2] = b[3] = 0;

  unicode_to_gb18030(u, b);

  printf("%02x %02x %02x %02x\n", b[0], b[1], b[2], b[3]);
}
