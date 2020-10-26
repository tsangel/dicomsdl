/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * pixelseq.h
 */

#include <map>
#include <vector>
#include "dicom.h"
#include "instream.h"

#ifndef DICOMSDL_PIXELSEQ_H
#define DICOMSDL_PIXELSEQ_H

namespace dicom {

bool check_have_ffd9(uint8_t *p, size_t size);

}

#endif // DICOMSDL_PIXELSEQ_H
