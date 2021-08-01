/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * pixelseq.cc
 */

#include "pixelseq.h"

#include <iostream>

#include "dicom.h"
#include "instream.h"
#include "imagecodec.h"
#include "codec_common.h"

namespace dicom {

PixelFrame::PixelFrame()
    : startoffset_(0),
      endoffset_(0),
      encoded_data_(nullptr),
      encoded_data_size_(0) {
  LOG_DEBUG("++ @%p\tPixelFrame::PixelFrame()", this);
}

PixelFrame::~PixelFrame() {
  if (startoffset_ || endoffset_) {
    LOG_DEBUG("-- @%p\tPixelFrame::~PixelFrame() start @{%08x}.", this,
              startoffset_);
  } else if (encoded_data_) {
    LOG_DEBUG("-- @%p\tPixelFrame::~PixelFrame() encoded frame @%p.", this,
              encoded_data_);
  } else {
    LOG_DEBUG("-- @%p\tPixelFrame::~PixelFrame()", this, encoded_data_);
  }

  if (encoded_data_) ::free(encoded_data_);
}

PixelSequence::PixelSequence(DataSet *root_dataset, tsuid_t tsuid)
    : root_dataset_(root_dataset),
      transfer_syntax_(tsuid),
      jpeg_transfer_syntex_(
          transfer_syntax_ >= UID::JPEG_BASELINE_PROCESS1 &&
          transfer_syntax_ <=
              UID::JPEG2000_PART2_MULTICOMPONENT_IMAGE_COMPRESSION),
      base_offset_(0)  // I don't know until read Basic Offset Table
{
  LOG_DEBUG("++ @%p\tPixelSequence::PixelSequence(DataSet *, tsuid_t)", this);
}

PixelSequence::~PixelSequence() {
  LOG_DEBUG("-- @%p\tPixelSequence::PixelSequence()", this);
}

PixelFrame* PixelSequence::addPixelFrame() {
    frames_.push_back(
          std::unique_ptr<PixelFrame>(new PixelFrame()));
    return frames_.back().get();
}

void PixelSequence::attachToInstream(InStream *basestream, size_t size)
{
  is_ = std::unique_ptr<InStream>(new InSubStream(basestream, size));
}

bool check_have_ffd9(uint8_t *p, size_t size) {
  // check 0xFFD9 ( End of Image - EOI - (jpeg) or
  // or End of Codestream - EOC - (jpeg2000) marker
  // in the memory block

  int c = int(size) - 2;
  while (c >= 0) {
    if (p[c] == 0xff && p[c + 1] == 0xd9)
      return true;
    c--;
  }
  return false;
}

void PixelFrame::setEncodedData(uint8_t *data, size_t size)
{
  startoffset_ = endoffset_ = 0;
  if (encoded_data_) {
    ::free(encoded_data_);
    encoded_data_ = nullptr;
    encoded_data_size_ = 0;
  }

  encoded_data_size_ = size;
  if (size & 1)
    encoded_data_size_ += 1;

  encoded_data_ = (uint8_t *)::malloc(encoded_data_size_);
  if (!encoded_data_) {
    encoded_data_size_ = 0;
    LOGERROR_AND_THROW(
        "PixelFrame::setEncodedData(uint8_t*, size_t) - "
        "cannot allocate %zd bytes for the encoded_data_.",
        size);
  }
  
  ::memcpy(encoded_data_, data, size);    
  if (size & 1)
    encoded_data_[size] = 0x0;  // padding 0x00 to make length even
}

void PixelFrame::load(InStream *instream, size_t frame_length, uint8_t* buf) {
  if (encoded_data_) {
    // should not occur.
    LOGERROR_AND_THROW(
        "PixelFrame::load - cannot load if encoded_data_ exists.");
  }

  startoffset_ = instream->tell();
  LOG_DEBUG("   @%p\tPixelFrame::load - start loading a frame at {%#x}.", this,
            startoffset_);

  // instream->tell() should locate the position of the start of frame
  tag_t tag;
  size_t length;
  long remaining_bytes = (long)frame_length;
  encoded_data_size_ = 0;

  while (true) {
    if (instream->read(buf, 8) != 8) {
      LOGERROR_AND_THROW(
          "PixelFrame::load - cannot read 8 bytes for tags (FFFE,E000) or "
          "(FFFE,E0DD) from {%#x},",
          instream->tell());
    }

    // Item Tag (FFFE,E000) or (FFFE,E0DD)
    tag = TAG::load_32le(buf);
    // Item Length (4 bytes)
    length = load_le<uint32_t>(buf + 4);
    encoded_data_size_ += length;

    remaining_bytes -= 8;

    if (tag == 0xfffee0dd) {
      // This Sequence of Items is terminated by a Sequence Delimiter Item
      // with the Tag (FFFE,E0DD).
      break;
    }

    size_t frag_offset = instream->tell();
    size_t n = instream->skip(length);
    if (n != length) {
      LOGERROR_AND_THROW("PixelFrame::load - cannot skip %d bytes from {%#x}.",
                         length, instream->tell() - n);
    }
    remaining_bytes -= length;

    // start and end offset of the fragment item's value
    frag_offsets_.push_back(frag_offset);
    frag_offsets_.push_back(frag_offset + n);
    LOG_DEBUG("   @%p\tPixelFrame::load - add a fragment at {%#x} to {%#x}.",
              this, frag_offset, frag_offset + n);

    // I have no more bytes to read.
    if (remaining_bytes <= 0) break;

    // end of image or codestream marker?; end this frame.
    uint8_t *frag_data = (uint8_t *)instream->get_pointer(frag_offset, length);
    if (check_have_ffd9(frag_data, length) && (remaining_bytes < 8)) {
      puts("BREAK!!!");
      break;
      // 8 remaining_bytes may be for (fffe,e0dd)
    }    
  }
  // instream->tell() should locate the position of the end of frame
  endoffset_ = instream->tell();
}

void PixelFrame::addItem(PixelSequenceItem item)
{
  pixseq_items_.push_back(item);
}

void PixelSequence::readItems_(std::vector<PixelSequenceItem> &pixseq_items)
{
  // Table A.4-1. Example for Elements of an Encoded Single-Frame Image Defined
  // as a Sequence of Three Fragments Without Basic Offset Table Item
  // Value. Table A.4-2. Examples of Elements for an Encoded Two-Frame Image
  // Defined as a Sequence of Three Fragments with Basic Table Item
  // Values.

  // `instream->tell()` is located just after Tag (7fe0,0010), VR(== 'OB'), and
  // length(== 0xFFFFFFFF).
  // Read Item Tags, Item Length, and position of Item Value, build
  // `PixelSequenceItem`, then store them into pixseq_items_.

  uint8_t buf[8];
  tag_t tag;
  size_t length;

  InStream *instream = is_.get();

  while (true) {
    if (instream->read(buf, 8) != 8)
      LOGERROR_AND_THROW(
          "PixelSequence::readItems_ - cannot read 8 bytes from {%#x}",
          instream->tell());

    tag = TAG::load_32le(buf);
    length = load_le<uint32_t>(buf + 4);

    // if buf[8] holds Sequence Delimiter Tag and Item Length (0000 0000H).
    if (tag == 0xFFFEE0DD) break;

    pixseq_items.push_back(PixelSequenceItem{instream->tell() - 8, length});
    if (instream->skip(length) != length)
      LOGERROR_AND_THROW(
          "PixelSequence::readItems_ - cannot skip %zu bytes from {%#x}",
          length, instream->tell());
  }

  // TODO>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // std::vector<PixelSequenceItem> pixseq_items;
  // readItems_(pixseq_items);
  // if (pixseq_items[0].length > 0)
  //   assembleItemsWithBasicOffsetTable_(pixseq_items);
  // else
  //   assembleItemsWithoutBasicOffsetTable_(pixseq_items);
}

void PixelSequence::assembleItemsWithoutBasicOffsetTable_(
    std::vector<PixelSequenceItem> &pixseq_items) {
  // pixseq_items[0].length == 0
  PixelFrame *frame = nullptr;
  for (size_t ii = 1; ii < pixseq_items.size(); ii++) {
    if (!frame)
      frame = addPixelFrame();
    frame->addItem(pixseq_items[ii]);

    // Let's check this item holds EOI or EOC marker.
    size_t pos, len;
    pos = pixseq_items[ii].position;
    len = pixseq_items[ii].length;
    uint8_t *ptr = (uint8_t *)is_->get_pointer(pos, len);
    if (!ptr)
      LOGERROR_AND_THROW(
          "PixelSequence::assembleItemsWithoutBasicOffsetTable_ - cannot get "
          "%zu bytes from {%#x}",
          len, pos);
    // If EOI or EOC doesnot exist in this item, next item will be added to
    // current frame.
    if (check_have_ffd9(ptr, len))
      frame = nullptr;  
  }
}

void PixelSequence::assembleItemsWithBasicOffsetTable_(
    std::vector<PixelSequenceItem> &pixseq_items) {
  // pixseq_items[0].length > 0
  pixseq_items;
}

void PixelSequence::loadFrames()
{
  uint8_t buf[8];
  tag_t tag;
  size_t length;

  InStream *instream = is_.get();

  // Assert Tag is 'Item Tag'
  if (instream->read(buf, 8) != 8)
    LOGERROR_AND_THROW(
        "PixelSequence::loadFrames - cannot read 8 bytes from {%#x}",
        instream->tell());

  tag = TAG::load_32le(buf);
  length = load_le<uint32_t>(buf + 4);

  // Assert Tag is 'Item Tag'
  if (tag != 0xfffee000)
    LOGERROR_AND_THROW(
        "PixelSequence::loadFrames - first item's tag should be "
        "(FFFE,E000), but %s is encountered at {%#x}",
        TAG::repr(tag).c_str(), instream->tell() - 8);
  if (length >= instream->bytes_remaining())
    LOGERROR_AND_THROW(
        "PixelSequence::loadFrames - length of Basic Table Item "
        "Values(%u at {%#x}) is too large.",
        length, instream->tell() - 4);

  if (length) {
    // This pixel sequence has 'Basic Offset Table'.
    // Table A.4-2. Examples of Elements for an Encoded Two-Frame Image
    // Defined as a Sequence of Three Fragments with Basic Table Item Values

    size_t offset_table_items = length / 4;
    std::vector<uint32_t> offsets(length / 4);

    // Read offset table
    size_t n;
    n = instream->read((uint8_t *) &offsets[0], length);
    if (n != length) {
      LOGERROR_AND_THROW(
          "PixelSequence::loadFrames - Could not read %u bytes from "
          "{%#x} for basic offset table",
          length, instream->tell() - n);
    }

    // The first frame's base offset is just after basic offset table.
    base_offset_ = instream->tell();

    // Sort offsets in ascending order.
    // Build a map of offset of a frame -> offset of the next frame.
    std::map<uint32_t, uint32_t> temp_offset_map;
    for (int i = 0; i < offset_table_items; i++)
      temp_offset_map[offsets[i]] = 0;
    for (auto it = temp_offset_map.rbegin();;) {
      int n = it->first;
      if (++it == temp_offset_map.rend())
        break;
      it->second = n;
    }
    // temp_offset_map.rbegin()->second = 0; // calculate it later

    // make PixelFrames
    PixelFrame *frame;
    size_t last_endpos = 0;
    for (int i = 0; i < offset_table_items; i++) {
      size_t startpos = offsets[i];
      size_t endpos = temp_offset_map[offsets[i]];
      if (endpos == 0)
        endpos = startpos+instream->bytes_remaining();

      frame = addPixelFrame();

      // frame->startoffset_ <- base_offset_ + startpos
      instream->seek(base_offset_ + startpos);
      frame->load(instream, endpos-startpos, buf);
      if (instream->tell() > last_endpos)
        last_endpos = instream->tell();
    }

    // instream->tell() is located just after item with tag (fffe,e0dd).
    // ; frame->load already ate that item.
    instream->seek(last_endpos);

    LOG_DEBUG("   @%p\tPixelSequence::loadFrames - "
              "basic offset table with %d item(s) at {%#x}",
              this, offset_table_items, base_offset_);
  } else {  // no basic offset table
    // Table A.4-1. Example for Elements of an Encoded Single-Frame Image
    // Defined as a Sequence of Three Fragments
    // Without Basic Offset Table Item Value

    PixelFrame *frame;
    base_offset_ = instream->tell();

    while (true) {
      frame = addPixelFrame();
      frame->load(instream, instream->bytes_remaining(), buf);

      // last frame->load store tag/length information to the buf
      tag = TAG::load_32le(buf);

      if (tag == 0xfffee0dd)
        break;
      if (instream->bytes_remaining() < 8) // a fragment takes a least 8 bytes
        break;
    }

    if (frames_.back()->frag_offsets_.size() == 0) {
      // last frame without fragment  -> delete it
      frames_.pop_back();
    }

    LOG_DEBUG("   @%p\tPixelSequence::loadFrames - "
              "pixel sequence (%d frames) without basic offset table",
              this, frames_.size());
  }
}

// return start and end offset of `frame` with `index`
size_t PixelSequence::frameOffset(size_t index, size_t &end_offset) {
  if (index >= frames_.size())
    LOGERROR_AND_THROW(
        "PixelSequence::frameOffset  - index '%d' is out of range(0..%d)",
        index, (long)frames_.size()-1);
  end_offset = frames_[index].get()->endoffset_;
  return frames_[index].get()->startoffset_;
}

size_t PixelSequence::encodedFrameDataSize(size_t index) {
  if (index >= frames_.size())
    LOGERROR_AND_THROW(
        "PixelSequence::encodedFrameDataSize - index '%d' is out of "
        "range(0..%d)",
        index, (long)frames_.size() - 1)

  return frames_[index].get()->encodedDataSize();
}

std::vector<size_t> PixelSequence::frameFragmentOffsets(size_t index) {
  if (index >= frames_.size())
    LOGERROR_AND_THROW(
        "PixelSequence::frameFragmentOffsets - index '%d' is out of "
        "range(0..%d)",
        index, (long)frames_.size() - 1);

  return frames_[index].get()->frag_offsets_;
}

Buffer<uint8_t> PixelSequence::encodedFrameData(size_t index) {
  if (index >= frames_.size())
    LOGERROR_AND_THROW(
        "PixelSequence::encodedFrameData - index '%d' is out of range(0..%d)",
        index, (long)frames_.size()-1);

  PixelFrame *frame = frames_[index].get();

  if (frame->encoded_data_) {
    return Buffer<uint8_t>(frame->encoded_data_, frame->encoded_data_size_);
  } else {
    if (frame->frag_offsets_.size() == 2) {
      // this frame has one fragment; returned buffer points to internal memory.
      size_t startpos = frame->frag_offsets_[0];
      size_t length = frame->frag_offsets_[1] - startpos;
      return Buffer<uint8_t>(
          (uint8_t *)(is_.get()->get_pointer(startpos, length)), length);
    } else {
      // frame is split into several fragments; allocate memory for joined data.
      size_t nfrags = frame->frag_offsets_.size() / 2;
      size_t length = 0;
      // calculate entire length
      for (size_t i = 0; i < nfrags; i++) {
        length += frame->frag_offsets_[i * 2 + 1] - frame->frag_offsets_[i * 2];
      }
      // assemble splited data
      Buffer<uint8_t> data(length);
      uint8_t *p;
      uint8_t *q = data.data;
      for (size_t i = 0; i < nfrags; i++) {
        size_t frag_startpos = frame->frag_offsets_[i * 2];
        size_t frag_length = frame->frag_offsets_[i * 2 + 1] - frag_startpos;
        p = (uint8_t *)(is_.get()->get_pointer(frag_startpos, frag_length));
        ::memcpy(q, p, frag_length);
        q += frag_length;
      }
      return data;
    }
  }
}

void PixelSequence::setEncodedFrameData(size_t index, uint8_t *data,
                                        size_t datasize) {
  if (index >= frames_.size())
    LOGERROR_AND_THROW(
        "PixelSequence::copyDecodedFrameData - index '%d' is out of "
        "range(0..%d)",
        index, (long)frames_.size() - 1);

  frames_[index].get()->setEncodedData(data, datasize);
}

void PixelSequence::copyDecodedFrameData(size_t index, uint8_t *data,
                                         int datasize, int rowstep) {
  if (index >= frames_.size())
    LOGERROR_AND_THROW(
        "PixelSequence::copyDecodedFrameData - index '%d' is out of "
        "range(0..%d)",
        index, (long)frames_.size() - 1);

  // get encoded data
  Buffer<uint8_t> encdata = encodedFrameData(index);

  // start decompress
  imagecontainer ic;

  ic.rows = root_dataset_->getDataElement(0x00280010)->toLong();
  ic.cols = root_dataset_->getDataElement(0x00280011)->toLong();
  ic.prec = root_dataset_->getDataElement(0x00280100)->toLong();
  ic.ncomps =
      root_dataset_->getDataElement(0x00280002)->toLong();  // SamplesPerPixel
  ic.sgnd = root_dataset_->getDataElement(0x00280103)
                ->toLong();  // PixelRepresentation
  ic.lossy = 0;

  ic.rowstep = rowstep;
  if (!data) {
    LOGERROR_AND_THROW(
        "PixelSequence::copyDecodedFrameData - data for decoded image is "
        "null.");
  }
  if (ic.rows * ic.rowstep != datasize) {
    LOGERROR_AND_THROW(
        "PixelSequence::copyDecodedFrameData - datasize '%d' is not suitable "
        "for decoded data (%d bytes is required)",
        datasize, ic.rows * ic.rowstep);
  }
  ic.datasize = datasize;  // TODO: check if ic.rows * ic.rowstep;
  ic.data = (char *)data;

  DICOMSDL_CODEC_RESULT codec_result =
      decode_pixeldata(UID::to_uidvalue(root_dataset_->getTransferSyntax()),
                       (char *)encdata.data, encdata.size, &ic);
  if (codec_result == DICOMSDL_CODEC_ERROR) {
    LOGERROR_AND_THROW(
        "PixelSequence::copyDecodedFrameData - error in decoding frame data "
        "'%s'",
        ic.info);
  } else if (codec_result == DICOMSDL_CODEC_WARN)
    LOG_WARN("%s", codec_result, ic.info);
  else if (codec_result == DICOMSDL_CODEC_INFO)
    LOG_DEBUG("%s", codec_result, ic.info);

  // check lossy and check DataElement in DataSet...
}

}  // namespace dicom