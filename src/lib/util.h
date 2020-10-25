/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * util.h
 */

#include <stdarg.h>
#include <stdio.h>

#include <string>

#include "dicom.h"

#ifndef DICOMSDL_UTIL_H__
#define DICOMSDL_UTIL_H__

namespace dicom {

#define MESSAGE_BUFFER_SIZE 256

std::string sprint(const char *format, ...);

void swap2(uint8_t *p, size_t size);
void swap4(uint8_t *p, size_t size);
void swap8(uint8_t *p, size_t size);

void copyswap2(uint8_t *dst, uint8_t *src, size_t size);
void copyswap4(uint8_t *dst, uint8_t *src, size_t size);
void copyswap8(uint8_t *dst, uint8_t *src, size_t size);

/// Return number of '\' character in data
int count_delimiters(const uint8_t *p, const size_t size);

}  // namespace dicom

#endif  // DICOMSDL_UTIL_H__
