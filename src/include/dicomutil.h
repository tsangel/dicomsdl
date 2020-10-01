/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * dicomutil.h
 */

#ifndef DICOMSDL_DICOMUTIL_H_
#define DICOMSDL_DICOMUTIL_H_

#include "dicom.h"
#include <type_traits> 

namespace dicom {
/*
C.11.2.1.2 Window Center and Window Width

if (x <= c - 0.5 - (w-1) /2), then y = ymin
else if (x > c - 0.5 + (w-1) /2), then y = ymax
else y = ((x - (c - 0.5)) / (w-1) + 0.5) * (ymax- ymin) + ymin

c=2048, w=4096 becomes:
  if (x <= 0) then y = 0
  else if (x > 4095) then y = 255
  else y = ((x - 2047.5) / 4095 + 0.5) * (255-0) + 0

c=2048, w=1 becomes:
  if (x <= 2047.5) then y = 0
  else if (x > 2047.5) then y = 255
  else // not reached

c=0, w=100 becomes:
  if (x <= -50) then y = 0
  else if (x > 49) then y = 255
  else y = ((x + 0.5) / 99 + 0.5) * (255-0) + 0

c=0, w=1 becomes:
  if (x <= -0.5) then y = 0
  else if (x > -0.5) then y = 255
  else // not reached

* Above case is not working well if x are small numbers, for example
between 0, 1 (common case in PET images). We use xmin/xmax (e.g. 0..4095) than
center/window (2048, 4096).

if (m <= xmin), then y = ymin
else if (m > xmin), then y = ymax
else ((x - xmin) / (xmax - xmin)) * (ymax-ymin) + ymin

if (xmin == xmax)
if (m <= xmin), then y = ymin
else if (m > xmin), then y = ymax

* Conversion between xmin/xmax to center/window.
w = xmax - xmin + 1
c = (xmax + xmin + 1) / 2

xmin = c - 0.5 - (w - 1) / 2
xmax = c - 0.5 + (w - 1) / 2
*/

template <typename T>
void convert_to_uint8(T *src, size_t rows, size_t cols, size_t src_size_bytes,
                      size_t src_rowsize_bytes, uint8_t *dst,
                      size_t dst_size_bytes, size_t dst_rowsize_bytes,
                      float32_t xmin, float32_t xmax) {
  if (xmax < xmin) {
    std::swap(xmax, xmin);
  }

  if (rows * src_rowsize_bytes != src_size_bytes) {
    LOGERROR_AND_THROW(
        "size of source buffer %d bytes != rows %d * size of rows %d bytes",
        src_size_bytes, rows, src_rowsize_bytes);
  }
  if (rows * dst_rowsize_bytes != dst_size_bytes) {
    LOGERROR_AND_THROW(
        "size of destination buffer%d bytes != rows %d * size of rows %d bytes",
        dst_size_bytes, rows, dst_rowsize_bytes);
  }

  uint8_t *p = (uint8_t *)src;
  uint8_t *q = dst;
  const float ymax = 255.0, ymin = 0.0;
  if (xmin < xmax) {
    float a, b;
    a = (ymax - ymin) / (xmax - xmin);
    b = -xmin * (ymax - ymin) / (xmax - xmin) + ymin;

    for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++) {
        T tmp = ((T*)p)[c] * a + b;
        if (tmp > ymax) {
          tmp = ymax;
        }
        else if (tmp < ymin)
          tmp = ymin;
        q[c] = (uint8_t)tmp;
      }
      p += src_rowsize_bytes;
      q += dst_rowsize_bytes;
    }
  } else {  // xmin == xmax
    for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++) {
        T tmp = ((T*)p)[c];
        if (tmp <= xmax)
          q[c] = (uint8_t)ymin;
        else
          q[c] = (uint8_t)ymax;
      }
      p += src_rowsize_bytes;
      q += dst_rowsize_bytes;
    }
  }
}

}  // namespace dicom

#endif // DICOMSDL_DICOMUTIL_H_