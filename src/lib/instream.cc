/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * instream.cc
 */

#include "instream.h"

#include <stdio.h>

#ifdef _WIN32
#include <errno.h>
#else
#include <sys/errno.h>
#endif

#include "dicom.h"

namespace dicom {  // ----------------------------------------------------------

// InStream ====================================================================

InStream::InStream()
    : startoffset_(0),
      offset_(0),
      endoffset_(0),
      data_(nullptr),
      own_data_(false),
      filesize_(0),
      loaded_bytes_(0),
      basestream_(this),
      rootstream_(this) {
  LOG_DEBUG("++ @%p\tInStream::InStream()", this);
}

InStream::~InStream() {
  LOG_DEBUG("-- @%p\tInStream::~InStream()", this);
  reset_internal_buffer();
}

void InStream::reset_internal_buffer() {
  startoffset_ = offset_ = endoffset_ = filesize_ = loaded_bytes_ = 0;
  if (own_data_) free(data_);
  own_data_ = false;
  data_ = nullptr;
}

size_t InStream::read(uint8_t *ptr, size_t size) {
  if (offset_ + size > rootstream_->loaded_bytes_) {
    // size of `data_` will be larger than `offset_ + size`.
    rootstream_->prefetch(offset_ + size);

    if (offset_ + size > rootstream_->loaded_bytes_) return 0;
  }

  if (offset_ + size <= endoffset_) {
    memcpy(ptr, rootstream_->data_ + offset_, size);
    offset_ += size;
    return size;
  }

  return 0;
}

size_t InStream::seek(size_t pos) {
  if (pos > rootstream_->loaded_bytes_) {
    rootstream_->prefetch(pos);

    if (pos > rootstream_->loaded_bytes_) return offset_;
  }

  if (pos >= startoffset_ && pos <= endoffset_) {
    offset_ = pos;
  }

  return offset_;
}

size_t InStream::skip(size_t size) {
  if (offset_ + size > rootstream_->loaded_bytes_) {
    // size of `data_` will be larger than `offset_ + size`.
    rootstream_->prefetch(offset_ + size);

    if (offset_ + size > rootstream_->loaded_bytes_) return 0;
  }

  if (offset_ + size <= endoffset_) {
    offset_ += size;
    return size;
  }

  return 0;
}

void *InStream::get_pointer(size_t offset, size_t size) {
  if (offset + size > rootstream_->loaded_bytes_) {
    // size of `data_` will be larger than `offset + size`.
    rootstream_->prefetch(offset + size);

    if (offset + size > rootstream_->loaded_bytes_) return nullptr;
  }

  if (offset + size <= endoffset_) {
    return rootstream_->data_ + offset;
  }

  return nullptr;
}

// InStringStream ==============================================================

InStringStream::InStringStream() {
  LOG_DEBUG("++ @%p\tInStringStream::InStringStream()", this);
}

InStringStream::~InStringStream() {
  LOG_DEBUG("-- @%p\tInStringStream::~InStringStream()", this);
}

void InStringStream::attachmemory(const uint8_t *data, size_t datasize,
                                  bool copydata) {
  // reset data before attach to a memory block
  reset_internal_buffer();

  if (copydata) {
    data_ = (uint8_t *)malloc(datasize);
    if (data_ == NULL) {
      LOGERROR_AND_THROW(
          "cannot malloc %s bytes in InStringStream::attachmemory", datasize);
    }
    memcpy(data_, data, datasize);
    own_data_ = true;
  } else {
    data_ = (uint8_t *)data;
    own_data_ = false;
  }

  startoffset_ = offset_ = 0;
  endoffset_ = filesize_ = loaded_bytes_ = datasize;

  LOG_DEBUG(
      "   @%p\tInStringStream::attachmemory(const uint8_t*,size_t,bool)\t @%p, "
      "%d, %d",
      this, data, datasize, copydata);
}

// InFileStream ================================================================

InFileStream::InFileStream() : fp_(NULL), filename_("") {
  LOG_DEBUG("++ @%p\tFileInStream::FileInStream()", this);
}

InFileStream::~InFileStream() {
  detachfile();
  LOG_DEBUG("-- @%p\tFileInStream::~FileInStream()", this);
}

void InFileStream::attachfile(const char* filename) {
  // reset data before load a new file
  reset_internal_buffer();
  detachfile();

  fp_ = fopen(filename, "rb");

  if (fp_ == NULL) {
    char *errmsg = strerror(errno);
    LOGERROR_AND_THROW("cannot open \"%s\": %s", filename, errmsg);
  }

  filename_ = filename;

  fseek(fp_, 0, SEEK_END);
  long fileLength = ftell(fp_);
  if (fileLength < 0) {
    // -1; unsuccesful ftell()
    detachfile();
    LOGERROR_AND_THROW("cannot get size of \"%s\"", filename);
  }
  fseek(fp_, 0, SEEK_SET);
  endoffset_ = filesize_ = fileLength;
  startoffset_ = offset_ = 0;

  LOG_DEBUG("++ @%p\tFileInStream::attachfile(const char*)\t %s", this,
            filename_.c_str());

  prefetch(INITIAL_INSTREAM_DATABUFFER_SIZE);
  own_data_ = true;
}

// close file -- memory chunk is preserved
void InFileStream::detachfile() {
  if (fp_ != NULL) {
    LOG_DEBUG("-- @%p\tFileInStream::detachfile()\t %s", this,
              filename_.c_str());
    fclose(fp_);
    fp_ = NULL;
  }
  filename_ = "";
}

void InFileStream::prefetch(size_t newsize) {
  // `prefetch` updates `data_` and `loaded_bytes_`.
  // InSubStream should use `rootstream_->data_` rather than it's own `data_`
  // and `loaded_bytes_`.
  if (newsize < loaded_bytes_) return;

  size_t new_loaded_bytes =
      (loaded_bytes_ > 0 ? loaded_bytes_ * 2
                         : INITIAL_INSTREAM_DATABUFFER_SIZE);
  while (new_loaded_bytes < newsize) new_loaded_bytes *= 2;

  if (new_loaded_bytes > filesize_) new_loaded_bytes = filesize_;

  uint8_t *tmpdata = (uint8_t *)realloc(data_, new_loaded_bytes);
  if (tmpdata == NULL) {
    LOGERROR_AND_THROW(
        "cannot realloc %d bytes in InFileStream::prefetch",
        new_loaded_bytes);
  }

#ifdef DEBUG_MESSAGE
  if (data_ && data_ != tmpdata) {
    LOG_DEBUG("   @%p\tInStream::prefetch() old data @%p -> new data @%p", this,
              data_, tmpdata);
  }
#endif

  data_ = tmpdata;
  size_t nread =
      fread(data_ + loaded_bytes_, 1, new_loaded_bytes - loaded_bytes_, fp_);

  if (nread < new_loaded_bytes - loaded_bytes_) {
    LOGERROR_AND_THROW(
        "cannot fread %d bytes from \"%s\":%d in InFileStream::prefetch",
        (new_loaded_bytes - loaded_bytes_), filename_.c_str(), loaded_bytes_);
  }

  LOG_DEBUG(
      "   @%p\tInStream::prefetch() read +%d bytes, data %p, loaded_bytes_ %d, "
      "new_loaded_bytes %d",
      this, new_loaded_bytes - loaded_bytes_, data_, loaded_bytes_,
      new_loaded_bytes);

  loaded_bytes_ = new_loaded_bytes;
}

// InSubStream ==============================================================

InSubStream::InSubStream(InStream *basestream, size_t size) {
  basestream_ = basestream;
  rootstream_ = basestream->rootstream();

  startoffset_ = offset_ = basestream_->tell();
  endoffset_ = startoffset_ + size;
  if (endoffset_ > basestream_->endoffset())
    endoffset_ = basestream->endoffset();
  data_ = nullptr;  // not used in InSubStream;
  own_data_ = false;
  filesize_ = 0;  // not used in InSubStream
  loaded_bytes_ = 0; // not used in InSubStream

  LOG_DEBUG(
      "++ @%p\tInSubStream::InSubStream(InStream *, size_t)\tbase %p, offset "
      "{%08x}, size %d",
      this, basestream, startoffset_, size);
}

InSubStream::~InSubStream() {
  LOG_DEBUG(
      "-- @%p\tInSubStream::~InSubStream()\tbase %p, offset {%08x}, size %d",
      this, basestream_, startoffset_, endoffset_ - startoffset_);
}

void InSubStream::prefetch(size_t newsize) {
  LOGERROR_AND_THROW("InSubStream::prefetch - should not be called ")
}

}  // namespace dicom
