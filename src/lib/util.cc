/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * util.cc
 */

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include "dicom.h"
#include "util.h"

namespace dicom { // namespace dicom -------------------------------------------

static void _sprint(char *buf, const char *format, va_list args) {
  vsnprintf(buf, MESSAGE_BUFFER_SIZE, format, args);
  buf[MESSAGE_BUFFER_SIZE - 1] = '\0';
}

std::string sprint(const char * format, ...) {
  char buf[MESSAGE_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  _sprint(buf, format, args);
  va_end(args);
  return std::string(buf);
}

void swap2(uint8_t *p, size_t size) {
  size /= 2;
  uint8_t c;
  while (size--) {
    c = p[1];
    p[1] = p[0];
    p[0] = c;
    p += 2;
  }
}

void swap4(uint8_t *p, size_t size) {
  size /= 4;
  uint8_t c;
  while (size--) {
    c = p[3];
    p[3] = p[0];
    p[0] = c;
    c = p[2];
    p[2] = p[1];
    p[1] = c;
    p += 4;
  }
}

void swap8(uint8_t *p, size_t size) {
  size /= 8;
  uint8_t c;
  while (size--) {
    c = p[7];
    p[7] = p[0];
    p[0] = c;
    c = p[6];
    p[6] = p[1];
    p[1] = c;
    c = p[5];
    p[5] = p[2];
    p[2] = c;
    c = p[4];
    p[4] = p[3];
    p[3] = c;
    p += 8;
  }
}

void copyswap2(uint8_t *dst, uint8_t *src, size_t size) {
  int n = size / 2;
  while (n--) {
    dst[0] = src[1];
    dst[1] = src[0];
    src += 2;
    dst += 2;
  }
  n = size % 2;
  while (n--)
    *src++ = *dst++;
}

void copyswap4(uint8_t *dst, uint8_t *src, size_t size) {
  int n = size / 4;
  while (n--) {
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
    src += 4;
    dst += 4;
  }
  n = size % 4;
  while (n--)
    *src++ = *dst++;
}

void copyswap8(uint8_t *dst, uint8_t *src, size_t size) {
  int n = size / 8;
  while (n--) {
    dst[0] = src[7];
    dst[1] = src[6];
    dst[2] = src[5];
    dst[3] = src[4];
    dst[4] = src[3];
    dst[5] = src[2];
    dst[6] = src[1];
    dst[7] = src[0];
    src += 8;
    dst += 8;
  }
  n = size % 8;
  while (n--)
    *src++ = *dst++;
}

int count_delimiters(const uint8_t *p, const size_t size) {
  int count = 0;
  for (int i = 0; i < (int) size; i++)
    if (p[i] == '\\')
      count++;
  return count;
}

void applyLut() {
  // C.11.2.1.2 Window Center and Window Width
}

} // namespace dicom -----------------------------------------------------------

