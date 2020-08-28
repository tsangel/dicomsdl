/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * dataset.cc
 */

#include <stdarg.h>
#include <string>

#include "dicom.h"

namespace dicom {

#define MESSAGE_BUFFER_SIZE 1024
void _sprint(char *buf, const char *format, va_list args) {
  vsnprintf(buf, MESSAGE_BUFFER_SIZE, format, args);
  buf[MESSAGE_BUFFER_SIZE - 1] = '\0';
}

DicomException build_exception(const char *format, ...) {
  char buf[MESSAGE_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  _sprint(buf, format, args);
  va_end(args);
  return DicomException(buf);
}

}  // namespace dicom