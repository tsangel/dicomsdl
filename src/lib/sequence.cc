/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * sequence.cc
 */

#include <iostream>

#include "dicom.h"
#include "instream.h"

namespace dicom {

Sequence::Sequence(DataSet *root_dataset): root_dataset_(root_dataset)
{
  LOG_DEBUG("++ @%p\tSequence::Sequence(DataSet *)", this);

  transfer_syntax_ = root_dataset_->getTransferSyntax();
  is_little_endian_ = root_dataset_->is_little_endian();
  is_vr_explicit_ = root_dataset_->is_vr_explicit();
}

Sequence::~Sequence()
{
  LOG_DEBUG("-- @%p\tSequence::~Sequence()", this);
}

DataSet* Sequence::addDataSet()
{
  seq_.push_back(std::unique_ptr<DataSet>(new DataSet(root_dataset_)));
  return seq_.back().get();
}

DataSet* Sequence::getDataSet(size_t index) {
  if (index < size())
    return seq_[index].get();
  else
    return nullptr;
}

DataSet* Sequence::operator[](size_t index) {
  if (index < size())
    return seq_[index].get();
  else
    return nullptr;
}

void Sequence::load(InStream *instream) {
  uint8_t buf[8];
  size_t n;

  while (!instream->is_eof()) {
    n = instream->read(buf, 8);

    if (n < 8)
      LOGERROR_AND_THROW(
          "Sequence::load - "
          "cannot read 8 bytes for Item Tag and length at {%x}",
          instream->tell());

    tag_t tag;
    size_t length, offset;

    tag = TAG::load_32e(buf, is_little_endian_);

    // Sequence Delimitation Item
    if (tag == 0xfffee0dd)  // Seq. Delim. Tag (FFFE, E0DD)
      break;

    if (tag != 0xfffee000) {  // Item Tag (FFFE, E000)
      instream->unread(8);
      LOG_ERROR(
          "Sequence::load - "
          "(FFFE,E000) is expected for item data element "
          "but %s appeared at {%#x}; "
          "quit this sequence which starts from {%#x}.",
          TAG::repr(tag).c_str(), instream->tell(), instream->begin());
      break;
    }

    // PS3.3 Table F.3-3. Directory Information Module Attributes
    // This offset includes the File Preamble and the DICM Prefix.
    offset = instream->tell();
    length = load_e<uint32_t>(buf + 4, is_little_endian_);

    if (length == 0xffffffff) length = instream->bytes_remaining();
    
    DataSet *dataset = addDataSet();
    if (length) {      
      dataset->attachToInstream(instream, length);
      dataset->setOffset(offset);
      InStream* subs = dataset->instream();
      size_t offset_start, offset_end;
      offset_start = subs->tell();
      dataset->load(0xffffffff, subs);
      offset_end = subs->tell();
      instream->skip(offset_end - offset_start);
    }
  }

}

}  // namespace dicom
