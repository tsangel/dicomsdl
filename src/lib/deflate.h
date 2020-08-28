/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * deflate.h
 */

#ifndef __DEFLATE_H__
#define __DEFLATE_H__

#include "dicom.h"
#include <sstream>

namespace dicom {  //------------------------------------------------------

void inflate_dicomfile(uint8_t *data, size_t datasize, std::ostringstream &oss,
                       size_t skip_offset);
void deflate_dicomfile(uint8_t *data, size_t datasize, std::ostringstream &oss,
                       size_t skip_offset, int level);

}  // namespace dicom -----------------------------------------------------

#endif // __DEFLATE_H__
