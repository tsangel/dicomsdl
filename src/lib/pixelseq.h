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

struct PixelFrame {
  // start and endoffset pair of fragments
  // [start] [end] [start] [end] ...
  std::vector<size_t> frag_offsets_;

  size_t startoffset_;  // start offset in InStream.
  size_t endoffset_;    // end offset == startoffset of next frame

  PixelFrame(size_t startoffset) : startoffset_(startoffset), endoffset_(0) {
    LOG_DEBUG("++ @%p\tPixelFrame::PixelFrame(size_t) start @{%08x}", this,
              startoffset_);
  }

  ~PixelFrame() {
    LOG_DEBUG("-- @%p\tPixelFrame::~PixelFrame() start @{%08x}", this,
              startoffset_);
  }

  // buf should be uint8_t[8];
  void load(InStream *instream, size_t frame_length, uint8_t* buf);
};

}

#endif // DICOMSDL_PIXELSEQ_H
