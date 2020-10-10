/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * dataset.cc
 */

#include <stdio.h>

#include <memory>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <string.h>
#include <wchar.h>
#include <string>
#include <sstream>

#include "dicom.h"
#include "instream.h"
#include "deflate.h"

namespace dicom {

DataSet::DataSet()
    : root_dataset_(this),
      transfer_syntax_(UID::EXPLICIT_VR_LITTLE_ENDIAN),
      is_little_endian_(true),
      is_vr_explicit_(true),
      specific_charset0_(CHARSET::UNKNOWN) {
  // 0xffffffff for last_tag_loaded_ will prevent getDataElement try to load()
  // from an empty DataSet.
  last_tag_loaded_ = 0xffffffff;
  UINT64(buf8_) = 0;
  LOG_DEBUG("++ @%p\tDataSet::DataSet()", this);
}

DataSet::DataSet(DataSet* parent)
    : root_dataset_(parent),
      transfer_syntax_(parent->transfer_syntax()),
      is_little_endian_(parent->transfer_syntax() !=
                        UID::EXPLICIT_VR_BIG_ENDIAN),
      is_vr_explicit_(parent->transfer_syntax() !=
                      UID::IMPLICIT_VR_LITTLE_ENDIAN) {
  last_tag_loaded_ = 0x0;
  UINT64(buf8_) = 0;
  specific_charset0_ = CHARSET::UNKNOWN; // use root_dataset's charset
  LOG_DEBUG("++ @%p\tDataSet::DataSet(DataSet*) parent @p", this, parent);
}

DataSet::~DataSet() { LOG_DEBUG("-- @%p\t~DataSet::~DataSet()", this); }

DataElement* DataSet::addDataElement(tag_t tag, vr_t vr, uint32_t length,
                                     size_t offset) {
  if (tag != 0 && vr == VR::NONE) {
    vr = TAG::get_vr(tag);
    if (vr == VR::NONE) {
      LOGERROR_AND_THROW(
          "DataSet::addDataElement - cannot find VR for a DataElement with tag "
          "%s (don't omit parameter VR)",
          TAG::repr(tag).c_str());
    }
  }
  removeDataElement(tag);
  return (edict_[tag] = std::unique_ptr<DataElement>(
              new DataElement(tag, vr, length, offset, this)))
      .get();
}

DataElement* DataSet::getDataElement(tag_t tag)
{
  if (this == root_dataset_ && tag > last_tag_loaded_) load(tag, NULL);

  auto it = edict_.find(tag);
  if (it != edict_.end())
    return it->second.get();
  else
    return DataElement::NullElement();
}

DataElement* DataSet::getDataElement(const char *tagstr) {
  char *_tagstr = (char *) tagstr;
  char *nextptr;
  char *endptr = (char *) tagstr + strlen(tagstr);

  tag_t tag, gggg, eeee;
  tag_t block;
  int seqidx;
  DataElement *el;
  DataSet* dataset = this;

  while (1) {
    // ignore starting (
    if (_tagstr[0] == '(')
      _tagstr++;

    if ((_tagstr[0] >= '0' && _tagstr[0] <= '9') ||   //
        ((_tagstr[0] == 'F' || _tagstr[0] == 'f') &&  //
         (_tagstr[1] == 'F' || _tagstr[1] == 'f'))) {
      // string starts with number - read gggg or ggggeeee
      tag = tag_t(strtol(_tagstr, &nextptr, 16));

      if (nextptr && (*nextptr == ',' || *nextptr == '{')) {
        // if string->hex conversion is stopped by ',' or '{',
        // first portion is group number
        gggg = tag;

        // if ",{" string sequence, skip ','
        if (*nextptr == ',' && nextptr[1] == '{') nextptr++;

        // string between { and } is creator id
        // e.g. 0000,{CREATOR}00
        if ((*nextptr == '{') && (tag & 1)) {
          _tagstr = nextptr + 1;

          nextptr++;
          int n = 0;
          while (nextptr[n] && nextptr[n] != '}') n++;

          if (nextptr[n] == '\0') {
            LOGERROR_AND_THROW(
                "DataSet::getDataElement(\"%s\"): "
                "malformed string -- no matching '}'",
                tagstr);
            el = DataElement::NullElement();
            break;
          }
          std::string creator(nextptr, n);

          // find element from gggg0001 to gggg00ff
          for (block = 0x10; block <= 0xff; block++) {
            DataElement* creator_de = getDataElement(TAG::build(gggg, block));
            if (!creator_de->isValid()) continue;
            if (creator_de->toBytes() == creator) break;
          }

          if (block > 0xff) {
            // no such block of elements from private creator
            el = DataElement::NullElement();
            break;
          }

          _tagstr = nextptr + n + 1;

          // Get element number in the block
          eeee = tag_t(strtol(_tagstr, &nextptr, 16));
          eeee = (block << 8) + (eeee & 0xff);
        } else {
          _tagstr = nextptr + 1;

          // Get element number in eeee form
          eeee = tag_t(strtol(_tagstr, &nextptr, 16));
        }

        if (eeee == 0 && _tagstr == nextptr)
          LOGERROR_AND_THROW(
              "DataSet::getDataElement - "
              "malformed string '%s'; no number after ',' or '}'",
              tagstr);

        tag = TAG::build(gggg, eeee);

        if (*nextptr == ')') nextptr++;
      }
    } else {
      // tagstr is keyword string
      int n = 0;

      while (_tagstr[n] && _tagstr[n] != '.')  // take until next '.'
        n++;
      tag = TAG::from_keyword(std::string(_tagstr, n).c_str());
      if (tag == 0xffffffff)
        LOGERROR_AND_THROW(
            "DataSet::getDataElement - error in string '%s'; no such keyword "
            "'%s'",
            tagstr, std::string(_tagstr, n).c_str());

      nextptr = (char *) _tagstr + n;
    }

    _tagstr = nextptr + 1;

    el = dataset->getDataElement(tag);
    if (!el->isValid())
      break;

    if (_tagstr >= endptr)
      break;

    if (el->vr() != VR::SQ)
      LOGERROR_AND_THROW(
          "DataSet::getDataElement - error in string '%s'; VR of element %s "
          "(VR::%s) is not VR::SQ",
          tagstr, TAG::repr(tag).c_str(), VR::repr(el->vr()));

    // number after '.' is DataSet sequence number, starting from 0
    seqidx = int(strtol(_tagstr, &nextptr, 10));

    dataset = el->toSequence()->getDataSet(seqidx);

    _tagstr = nextptr + 1;
    if (dataset == NULL || _tagstr >= endptr) {
      // no DataSet with index in the Sequence
      el = DataElement::NullElement();
      break;
    }
  }
  return el;
}

void DataSet::removeDataElement(tag_t tag) { edict_.erase(tag); }

charset_t DataSet::getSpecificCharset(int index) {
  if (this != root_dataset_)
    return root_dataset_->getSpecificCharset();

  if (specific_charset0_ == CHARSET::UNKNOWN) {
    DataElement* de = root_dataset_->getDataElement(0x00080005);

    {
      char* valueptr = (char*)de->value_ptr();
      size_t valuesize = de->length();

      char* firstdelim = strchr(valueptr, '\\');
      if (firstdelim == NULL) {
        // no delim, only one character set
        specific_charset1_ = specific_charset0_ =
            CHARSET::from_string(valueptr, valuesize);
      } else {
        const char* lastdelim = strrchr(valueptr, '\\');
        specific_charset0_ =
            CHARSET::from_string(valueptr, firstdelim - valueptr);
        specific_charset1_ = CHARSET::from_string(
            lastdelim + 1, valuesize - (lastdelim + 1 - valueptr));
      }
    }
    if (specific_charset0_ == CHARSET::UNKNOWN ||
        specific_charset1_ == CHARSET::UNKNOWN) {
      LOG_WARN("   DataSet::specific_charset - unknown CHARSET \"%s\"",
               std::string((char*)de->value_ptr(), de->length()).c_str());
      specific_charset0_ = CHARSET::DEFAULT;
      specific_charset1_ = CHARSET::DEFAULT;
    }
  }

  if (index == 0)
    return specific_charset0_;
  else
    return specific_charset1_;
}

void DataSet::setSpecificCharset(charset_t charset)
{
  if (int(charset) > CHARSET::GBK)
    LOGERROR_AND_THROW("DataSet::setSpecificCharset - unknown charset (%d).",
                       charset);

  char tmpterm[32];
  int tmpterm_len;

  switch (charset) {
    // C.12.1.1.2 Specific Character Set​
    // Defined Terms for Multi-Byte Character Sets with Code Extensions
    // ISO 2022 IR 87, ISO 2022 IR 159​, ISO 2022 IR 149, ISO 2022 IR 58​
    case CHARSET::ISO_2022_IR_87:   // japanese jis x 0208
    case CHARSET::ISO_2022_IR_159:  // japanese jis x 0212
    case CHARSET::ISO_2022_IR_149:  // korean
    case CHARSET::ISO_2022_IR_58:   // simplified chinese
      tmpterm_len =
          snprintf(tmpterm, sizeof(tmpterm), "\\%s", CHARSET::term(charset));
      specific_charset0_ = CHARSET::DEFAULT;  // for convert_to_unicode
      specific_charset1_ = charset;           // for convert_from_unicode
      break;
    default:
      // C.12.1.1.2 Specific Character Set​
      // Defined Terms for Single-Byte Character Sets Without Code Extensions​
      // Defined Terms for Single-Byte Character Sets with Code Extensions​
      // Defined Terms for Multi-Byte Character Sets Without Code Extensions​
      // - GB18030, ISO_IR 192, GBK
      tmpterm_len =
          snprintf(tmpterm, sizeof(tmpterm), "%s", CHARSET::term(charset));
      specific_charset0_ = specific_charset1_ = charset;
      break;
  }

  addDataElement(0x00080005, VR::CS)->fromBytes(tmpterm, tmpterm_len);
}

void DataSet::attachToMemory(const uint8_t* data, size_t datasize,
                             bool copy_data) {
  // only root DataSet may have InStream
  if (this != root_dataset_) {
    LOGERROR_AND_THROW(
        "only root dataset can call DataSet::attachToMemory");
  }
  detach();

  is_ = std::unique_ptr<InStream>(new InStringStream);
  InStringStream* iss = dynamic_cast<InStringStream*>(is_.get());
  iss->attachmemory(data, datasize, copy_data);
}

void DataSet::attachToFile(const char* filename) {
  // only a root DataSet may have InStream
  if (this != root_dataset_) {
    LOGERROR_AND_THROW(
        "only root dataset can call DataSet::attachToFile");
  }
  detach();

  is_ = std::unique_ptr<InStream>(new InFileStream);
  InFileStream* ifs = dynamic_cast<InFileStream*>(is_.get());
  ifs->attachfile(filename);
}

void DataSet::attachToInstream(InStream* basestream, size_t size) {
  is_ = std::unique_ptr<InStream>(new InSubStream(basestream, size));
}

// - First call from DataSet::loadDicomFile()
//   tag_t can be any value and instream should be valid
// - Second and further calls from DataSet::loadDicomFile()
//   tag_t can be any value and instream should be NULL;
//   load() uses baseinstream_ in this case.
// - Call from Sequence::load()
//   tag_t should be (ffff,ffff) and instream should be valid.
void DataSet::load(tag_t load_until, InStream *instream) {
  uint8_t buf4[4];
  size_t n;

  tag_t gggg, eeee, tag;
  vr_t vr;
  size_t length, offset;  // value's length and offset

  if (instream == nullptr) {
    // instream == nullptr; second call from DataSet::loadDicomFile() or calls
    // from Sequence::load()
    instream = is_.get();

    if (instream == nullptr) {
      // a DataSet is created from scratch without attaching stream.
      LOGERROR_AND_THROW("attach an instream before call DataSet::load");
    }
  }

  while (!instream->is_eof()) {
    if (UINT32(buf8_) == 0) {  // buffer is empty
      n = instream->read(buf8_, 8);
      if (n < 8) {
        LOGERROR_AND_THROW(
            "DataSet::load - cannot read 8 bytes for Tag and VR at {%x}",
            instream->tell());
      }

      if (UINT32(buf8_) == 0) {
        // may be trailing zeros?
        while (!instream->is_eof()) {
          instream->read(buf8_, 1);  // just consume zeros
          if (buf8_[0] != '\0') {
            instream->unread(1);
            break;
          }
        }
        if (instream->is_eof()) break;
      }
    }

    if (is_little_endian_) {
      gggg = load_le<uint16_t>(buf8_);
      eeee = load_le<uint16_t>(buf8_ + 2);
    } else {
      gggg = load_be<uint16_t>(buf8_);
      eeee = load_be<uint16_t>(buf8_ + 2);
    }
    tag = TAG::build(gggg, eeee);

    // PS3.5 Table 7.5-3, Item Delim. Tag
    if (gggg == 0xfffe) {
      if (tag == 0xfffee00d) {
        if (this != root_dataset_)
          break;
        else {
          LOG_ERROR(
              "DataSet::load - "
              "Item Delim. Tag (FFFE,E00D) encountered out of sequence at "
              "{%#x}; "
              "just ignore this.",
              instream->tell() - 8);
          UINT32(buf8_) = 0;  // consume instream byte.
          continue;  // Item Delim. Tag out of sequence... just ignore it.
        }
      } else if (tag == 0xfffee0dd) {
        LOG_ERROR(
            "DataSet::load - "
            "Sequence Delim. Tag (FFFE,E0DD) encountered at {%#x} "
            "while parsing DataSet; just ignore this.",
            instream->tell() - 8);
        UINT32(buf8_) = 0;  // consume instream byte.
        if (this != root_dataset_)
          break;
        else
          continue;
      }
    }

    if (this == root_dataset_ && tag > load_until) break;

    if (last_tag_loaded_ > tag && this != root_dataset_) {
      // Encounter a tag which is smaller than last loaded tag.
      // May be unexpected end in the sequence in an erroneous DICOM file.
      LOG_ERROR(
          "DataSet::load - "
          "This tag %s at {%#x} is smaller than the last tag %s; "
          "quit this sequence.",
          TAG::repr(tag).c_str(), instream->tell(),
          TAG::repr(last_tag_loaded_).c_str());
      instream->unread(8);
      break;
    }

    // DATA ELEMENT STRUCTURE WITH EXPLICIT VR
    if (is_vr_explicit_) {
      vr = VR::from_uint16le(uint16_t(buf8_[4])+uint16_t(buf8_[5])*256);
      length = load_e<uint16_t>(buf8_ + 6, is_little_endian_);

      switch (vr) {
        case VR::FL:
        case VR::FD:
        case VR::SL:
        case VR::UL:
        case VR::SS:
        case VR::US:

        case VR::AE:
        case VR::AS:
        case VR::AT:
        case VR::CS:
        case VR::DA:
        case VR::DS:
        case VR::DT:
        case VR::IS:
        case VR::LO:
        case VR::LT:
        case VR::PN:
        case VR::SH:
        case VR::ST:
        case VR::TM:
        case VR::UI:
          // PS 3.5-2020, Table 7.1-2
          // Data Element with Explicit VR of AE, AS, AT, CS, DA, DS, DT, FL,
          // FD, IS, LO, LT, PN, SH, SL, SS, ST, TM, UI, UL and US
          break;

        case VR::OB:
        case VR::OD:
        case VR::OF:
        case VR::OL:
        case VR::OV:
        case VR::OW:
        case VR::SQ:
        case VR::UN:

        case VR::SV:
        case VR::UC:
        case VR::UR:
        case VR::UV:
          // PS3.5-2020, Table 7.1-1. Data Element with Explicit VR other than
          // as shown in Table 7.1-2
          // OB, OD, OF, OL, OV, OW, SQ and UN
          // VRs of SV, UC, UR, UV and UT may not have an Undefined Length

          n = instream->read(buf4, 4);
          if (n < 4)
            LOGERROR_AND_THROW(
                "DataSet::load - cannot read 4 bytes for data element value's "
                "length at {%x}",
                instream->tell());
          length = load_e<uint32_t>(buf4, is_little_endian_);
          break;

        case VR::UT:
          // In a strange implementation, VR 'UT' takes 2 bytes for 'len'
          n = instream->read(buf4, 4);
          if (n < 4)
            LOGERROR_AND_THROW(
                "DataSet::load - cannot read 4 bytes for data element value's "
                "length at {%x}",
                instream->tell());
          length = load_e<uint32_t>(buf4, is_little_endian_);
          if (length > instream->bytes_remaining()) {
            instream->unread(4);
            length = load_e<uint16_t>(buf8_ + 6, is_little_endian_);
          }
          break;

        default:
          // salvage codes some unusual VR
          // A non-standard VR 'UK' in RAYPAX file; may contains a short string
          // and have 2 byte length
          if (buf8_[4] == 'U' && buf8_[5] == 'K') {
            vr = VR::UN;
          } else {
            // assume this Data Element has implicit vr
            // PS 3.5-2009, Table 7.1-3
            // DATA ELEMENT STRUCTURE WITH IMPLICIT VR
            length = load_le<uint32_t>(buf8_ + 4);

            vr = TAG::get_vr(tag);
            if (vr == VR::NONE) vr = VR::UN;
          }
          break;
      }
    } else {
      // PS 3.5-2009, Table 7.1-3
      // DATA ELEMENT STRUCTURE WITH IMPLICIT VR

      // always little endian if vr is implicit.
      length = load_le<uint32_t>(buf8_ + 4);

      vr = TAG::get_vr(tag);
      if (vr == VR::NONE) vr = VR::UN;
    }

    // Data Element's values position
    offset = instream->tell();

    if (vr == VR::SQ) {
      if (length == 0xffffffff) length = instream->bytes_remaining();

      DataElement *de = addDataElement(tag, vr, length, offset);
      Sequence *seq = de->toSequence();
      InSubStream subs(instream, length);

      size_t offset_end;
      seq->load(&subs);
      offset_end = subs.tell();
      length = offset_end - offset;
      de->setLength(length);

      instream->skip(length);
    }

    else if (tag == 0x7fe00010) {
      if (length != 0xffffffff) {
        if ((getDataElement(0x00280100)->toLong()) > 8 && (vr == VR::OB))
          vr = VR::OW;
        addDataElement(tag, vr, length, offset);
        if (instream->skip(length) != length) {
          LOGERROR_AND_THROW(
              "DataSet::load - "
              "cannot process %d bytes for tag %s, vr %s from {%08x}",
              length, TAG::repr(tag).c_str(), (vr), offset);
        }
      }

      else {
        vr = VR::PIXSEQ;
        DataElement* de = addDataElement(tag, vr, length, offset);
        PixelSequence* pixseq = de->toPixelSequence();
        // process basic offset table of the pixel sequence
        pixseq->attachToInstream(instream, instream->bytes_remaining());
        size_t offset_end;

        pixseq->readOffsetTable();
        offset_end = pixseq->instream()->tell();
        length = offset_end - offset;
        de->setLength(length);
        instream->skip(length);
      }
    }

    else if (vr == VR::OFFSET) {
      break;
      //      if (len != 4) {
      //        set_error_message(
      //            "DataSet::load():"
      //            "length of DataElement %08x with VR_OFFSET should be 4");
      //        throw DICOM_INSTREAM_ERROR;
      //      }
      //
      //      uint32_t offset = s->read32u(endian);
      //      recordoffset *recoff = new recordoffset(offset, base_dcmfile);
      //
      //      e = add_dataelement(tag, vr, len, (void *) recoff, endian, true);
    }

    else {
      if (length != 0xffffffff) {
        addDataElement(tag, vr, length, offset);
        long n = instream->skip(length);

        // TODO: throw ///////////////
        if (n != length)
          LOGERROR_AND_THROW(
              "DataSet::load - "
              "cannot process %lu bytes for tag=%08x, vr=%s from {%x}",
              length, tag, VR::repr(vr), offset);
      } else {
        // PROBABLY SEQUENCE ELEMENT WITH IMPLICIT VR WITH ...
        length = instream->bytes_remaining();
        vr = VR::SQ;
        DataElement *de = addDataElement(tag, vr, length, offset);
        Sequence* seq = de->toSequence();
        InSubStream subs(instream, length);

        size_t offset_end;
        seq->load(&subs);
        offset_end = subs.tell();
        length = offset_end - offset;
        de->setLength(length);

        instream->skip(length);
      }
    }

    UINT32(buf8_) = 0;  // clear temporary buffer for tag, vr and length

    LOG_DEBUG("   DataSet::load - process (%08x) %s length=%4d at {%x}", tag,
              VR::repr(vr), length, offset);

    last_tag_loaded_ = tag;

    if (tag == load_until) break;

  }  // END OF WHILE -----------------------------------------------------------

  LOG_DEBUG(
      "   DataSet::load(load_until = %08x) - "
      "last_tag = %08x endpos = {%x}",
      load_until, tag, instream->tell());

  last_tag_loaded_ = load_until;
  if (instream->is_eof()) last_tag_loaded_ = 0xFFFFFFFF;
}

void DataSet::loadDicomFile(tag_t load_until){
  try {
    uint8_t buf[16];

    // only a base DataSet may have InStream
    if (this != root_dataset_) {
      LOGERROR_AND_THROW("only base dataset can call DataSet::loadDicomFile");
    }

    // check source stream
    if (is_.get() == nullptr || !is_->is_valid()) {
      LOGERROR_AND_THROW(
          "attach an instream before call DataSet::loadDicomFile");
    }

    // do clean before load a new file
    if (!edict_.empty()) {
      // TODO: clean all dataset!!!!!!!!!!!!!! except is_
    }

    // try read preamble
    is_->rewind();
    is_->skip(128);
    is_->read(buf, 4);

    if (memcmp(buf, "DICM", 4)) {
      // in some file, preamble in DICOM PART 10 FILE is stripped.
      is_->rewind();
    }

    // parse meta information
    // see PS3.10 Table 7.1-1. DICOM File Meta Information
    // Data elements in meta information should be little endian
    // and explicit VR.
    last_tag_loaded_ = 0x0;
    load(0x0002ffff, is_.get());

    std::string t = getDataElement(0x00020010)->toBytes();
    transfer_syntax_ = UID::from_uidvalue(t.c_str());
    is_little_endian_ = (transfer_syntax_ != UID::EXPLICIT_VR_BIG_ENDIAN);
    is_vr_explicit_ = (transfer_syntax_ != UID::IMPLICIT_VR_LITTLE_ENDIAN);

    if (transfer_syntax_ == UID::DEFLATED_EXPLICIT_VR_LITTLE_ENDIAN) {
      DataElement* de = getDataElement(0x00020000);
      size_t zipped_start_offset = de->toLong() + de->offset() + de->length();

      void* ptr =
          is_->get_pointer(0, is_->end());  // pointer to whole DICOM file image
      size_t zipped_len = is_->end();       // size of whole DICOM file image

      // Inflated zipped data
      std::ostringstream oss(std::ostringstream::binary);
      inflate_dicomfile((uint8_t*)ptr, zipped_len, oss, zipped_start_offset);

      LOG_DEBUG(
          "   @%p\tDataSet::loadDicomFile(tag_t)  unzip deflated file %d -> %d",
          this, zipped_len, is_->endoffset());

      // AttachToMemory to new stream on memory
      std::string inflated_image = oss.str();
      attachToMemory((const uint8_t*)inflated_image.c_str(),
                     inflated_image.size(), true);

      // continue on new stream on memory
      is_->seek(zipped_start_offset);
      UINT32(buf8_) = 0;  // clear temporary buffer for tag, vr, and length
    }

    // Parsing remaining Data Elements
    load(load_until, nullptr);
  } catch (DicomException& e) {
    last_tag_loaded_ = 0xFFFFFFFF;  // prevent from trying reloading
    throw e;
  }
}

void DataSet::detach() { is_.reset(); }

std::unique_ptr<DataSet> open_file(const char* filename, tag_t load_until,
                                   bool keep_on_error) {
  std::unique_ptr<DataSet> dset(new DataSet);
  try {
    dset->attachToFile(filename);
    dset->loadDicomFile(load_until);
  } catch (DicomException&) {
    if (!keep_on_error) throw;
    // if keep_on_error is true, ignore exception and return partially decoded
    // DataSet.
  }
  return dset;
}

std::unique_ptr<DataSet> open_memory(const uint8_t* data, size_t datasize,
                                     bool copy_data, tag_t load_until,
                                     bool keep_on_error) {
  std::unique_ptr<DataSet> dset(new DataSet);
  try {
    dset->attachToMemory(data, datasize, copy_data);
    dset->loadDicomFile(load_until);
  } catch (DicomException& ) {
    if (!keep_on_error) throw;
    // if keep_on_error is true, ignore exception and return partially decoded
    // DataSet.
  }
  return dset;
}

static void wordswap(uint8_t* ptr, size_t size) {
  uint16_t *word = (uint16_t *)ptr;
  size /= 2;
  for (size_t i = 0; i < size; ++i)
    word[i] = bswap16(word[i]);
}

void DataSet::copyFrameData(size_t index, uint8_t* data, int datasize,
                            int rowstep) {
  DataElement *de = getDataElement(0x7fe00010);
  if (de->vr() == VR::PIXSEQ) {
    de->toPixelSequence()->copyDecodedFrameData(index, data, datasize, rowstep);
  } else {
    if (!data) {
      LOGERROR_AND_THROW(
          "DataSet::copyFrameData - data for decoded image is "
          "null.");
    }
    int rows = getDataElement(0x00280010)->toLong();
    int cols = getDataElement(0x00280011)->toLong();
    int bitsalloc = getDataElement(0x00280100)->toLong();
    int ncomps = getDataElement(0x00280002)->toLong();  // SamplesPerPixel
    int bytesalloc = (bitsalloc > 8 ? 2 : 1);
    // PlanarConfiguration(0: RGBRGBRGB..., 1:RRR..GGG...BBB...)
    int planarconfig = getDataElement(0x00280006)->toLong();
    int nframes = getDataElement(0x00280008)->toLong(1);  // NumberOfFrames

#if __BYTE_ORDER == __BIG_ENDIAN
    bool needswap = (is_little_endian_ == true);
#else
    bool needswap = (is_little_endian_ != true);
#endif

    if (index >= nframes)
      LOGERROR_AND_THROW(
          "PixelSequence::copyDecodedFrameData - index '%d' is out of "
          "range(0..%d)",
          index, nframes - 1);

    if (rows * rowstep != datasize) {
      LOGERROR_AND_THROW(
          "DataSet::copyFrameData - datasize '%d' is not suitable to copy (a) "
          "frame data (%d bytes is required)",
          datasize, rows * rowstep);
    }

    if (ncomps == 1 || planarconfig == 0) { // gray image or RGBRGBRGB...
      int src_rowstep = cols * bytesalloc * ncomps;
      int src_pagestep = src_rowstep * rows;
      uint8_t *p, *q;
      p = (uint8_t*)de->value_ptr();
      p += src_pagestep * index;
      q = data;
      for (int r = 0; r < rows; r++) {
        ::memcpy(q, p, src_rowstep);
        if (needswap) wordswap(q, src_rowstep);
        p += src_rowstep;
        q += rowstep;
      }
    } else { // RRR... GGG... BBB...
      int src_rowstep = cols * bytesalloc;
      int src_pagestep = src_rowstep * rows * ncomps;
      uint8_t *p, *q;
      for (int c = 0; c < ncomps; c++) {
        p = (uint8_t*)de->value_ptr();
        p += src_pagestep * index + c * src_rowstep * rows;
        q = data + rowstep * rows * c;
        for (int r = 0; r < rows; r++) {
          ::memcpy(q, p, src_rowstep);
          if (needswap) wordswap(q, src_rowstep);
          p += src_rowstep;
          q += rowstep;
        }
      }
    }
  }
}

std::wstring DataSet::dump(size_t max_length) {
  std::wstringstream wss;
  wss << L"TAG\tVR\tLEN\tVM\tOFFSET\tKEYWORD\n";

  std::function<void(DataSet*, std::wstring)> _dump;
  _dump = [&wss, &_dump, &max_length](DataSet* ds, std::wstring prefix) {
    wchar_t buf[1024];
    buf[1023] = '\0';

    for (auto it = ds->begin(); it != ds->end(); it++) {
      wss << prefix;
      tag_t tag = it->first;
      DataElement* de = it->second.get();

      swprintf(buf, 1023, L"%08x'\t%hs\t%zu\t%d\t%#zx", tag,
               VR::repr(de->vr()), de->length(), de->vm(),
               de->offset());
      wss << buf;
      
      wss << L"\t" << de->repr(max_length);

      switch (tag) {
        case 0x00020002:
        case 0x00020010:
        case 0x00080016: {
          std::string s = de->toBytes("");
          wss << L" = "
              << (s.empty() ? "(Unknown UID)" : UID::uidvalue_to_uidname(s.c_str()));
        } break;
        default:
          break;
      }

      swprintf(buf, 1023, L"\t# %hs\n", TAG::keyword(de->tag()));
      wss << buf;

      if (de->vr() == VR::SQ) {
        Sequence* seq = de->toSequence();
        for (int i = 0; i < seq->size(); i++) {
          swprintf(buf, 1023, L"%ls%08x.%d.", prefix.c_str(), tag, i);
          _dump(seq->getDataSet(i), std::wstring(buf));
        }
      } else if (de->vr() == VR::PIXSEQ) {
        PixelSequence* pixseq = de->toPixelSequence();
        for (size_t frame_index = 0; frame_index < pixseq->numberOfFrames();
             frame_index++) {
          size_t start, end;
          start = pixseq->frameOffset(frame_index, end);
          std::vector<size_t> frag_offsets =
              pixseq->frameFragmentOffsets(frame_index);
          size_t nfrags = frag_offsets.size();
          swprintf(
              buf, 1023,
              L"\t\tFRAME #%zu (%zu BYTES) WITH %zu FRAGMENTS {%#zx - %#zx}\n",
              frame_index + 1, pixseq->frameEncodedDataLength(frame_index),
              nfrags / 2, start, end);
          wss << buf;
          for (size_t frag_index = 0; frag_index < nfrags / 2; frag_index++) {
            swprintf(buf, 1023, L"\t\t\t\tFRAGMENT #%zu {%#zx - %#zx}\n",
                     frag_index, frag_offsets[frag_index * 2],
                     frag_offsets[frag_index * 2 + 1]);
            wss << buf;
          }
        }
      }
    }
  };

  _dump(this, std::wstring(L"'"));
  return wss.str();
}

}  // namespace dicom
