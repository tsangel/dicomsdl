/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * instream.h
 */

#ifndef DICOMSDL_INSTREAM_H__
#define DICOMSDL_INSTREAM_H__

#include <string>
#include "dicom.h"

namespace dicom {  //-----------------------------------------------------------

#define INITIAL_INSTREAM_DATABUFFER_SIZE  1024

// template <typename T>
// class DataValue {
//   T* data_;
//   size_t nitems_;
//   bool own_data_;

//  public:
//   DataValue() : data_(nullptr), nitems_(0), own_data_(false) {
//     LOG_DEBUG("++  DataValue\t%p  default", this);
//   }
//   DataValue(size_t nitems) : nitems_(nitems) {
//     data_ = new T[nitems];
//     own_data_ = true;
//     LOG_DEBUG("++  DataValue\t%p - nitems %d owndata %d data %p", this, nitems_,
//               own_data_, data_);
//   }
//   DataValue(const T* ptr, size_t nitems)
//       : data_(ptr), nitems_(nitems), own_data_(false) {
//     LOG_DEBUG("++  DataValue\t%p - nitems %d owndata %d data %p", this, nitems_,
//               own_data_, data_);
//   }
//   ~DataValue() {
//     LOG_DEBUG("--  DataValue\t%p - nitems %d owndata %d data %p", this, nitems_,
//               own_data_, data_);
//     if (own_data_) delete[] data_;
//   }

//   void clear()  {
//     if (own_data_) delete[] data_;
//     data_ = nullptr;
//     own_data_ = false;
//     nitems_ = 0;
//   }

//   DataValue(const DataValue&) = delete;
//   DataValue& operator=(DataValue&) = delete;

//   DataValue(DataValue&& other): DataValue() {
//     LOG_DEBUG(
//         "++  DataValue&&\t%p - other nitems %d owndata %d data %p  ==> "
//         "this nitems %d owndata %d data %p",
//         this, other.nitems_, other.own_data_, other.data_, nitems_, own_data_,
//         data_);

//     std::swap(data_, other.data_);
//     std::swap(nitems_, other.nitems_);
//     std::swap(own_data_, other.own_data_);
    
//     LOG_DEBUG(
//         "++  DataValue&2\t%p - other nitems %d owndata %d data %p  ==> "
//         "this nitems %d owndata %d data %p",
//         this, other.nitems_, other.own_data_, other.data_, nitems_, own_data_,
//         data_);
//   }

//   DataValue& operator=(DataValue&& other) {
//     LOG_DEBUG(
//         "++  DataValue=\t%p - other nitems %d owndata %d data %p  ==> "
//         "this nitems %d owndata %d data %p",
//         this, other.nitems_, other.own_data_, other.data_, nitems_, own_data_,
//         data_);

//     std::swap(data_, other.data_);
//     std::swap(nitems_, other.nitems_);
//     std::swap(own_data_, other.own_data_);
    
//     LOG_DEBUG(
//         "++  DataValue=2\t%p - other nitems %d owndata %d data %p  ==> "
//         "this nitems %d owndata %d data %p",
//         this, other.nitems_, other.own_data_, other.data_, nitems_, own_data_,
//         data_);
//     return *this;
//   }
// };

// InStream ---------------------------------------------------------------
// read, load, copy*, attachfile may throw exception.
class InStream {
 protected:
  /*
    basestream_->startoffset
    <= this->startoffset_, this->offset_, this->endoffset
    < basestream_->endoffset_

    filesize_ == os.path.getsize(dicom file)
    bytes_read_ <= filesize_

    if offset_ + request data size > basestream_->bytes_read_:
      fill more basestream_->data_ from file
      basestream_->bytes_read_ += read bytes
    process request data size
   */
  size_t startoffset_;  // start position in this InStream
  size_t offset_;     // position of the cursor startoffset_ <= ... < endoffset_
  size_t endoffset_;  // end position of this InStream

  // data_, own_data_, filesize_, loaded_bytes_ are valid only in basestream.
  uint8_t* data_;        // holds entire dicom file image.
  bool own_data_;        // need free data if own_data_ is true.
  size_t filesize_;      // size of the dicom file.
  size_t loaded_bytes_;  // number of bytes that are loaded from disk (e.g. size
                         // of data_)

  InStream* basestream_;  // parent
  InStream* rootstream_;  // parent's parent's ...

  void reset_internal_buffer();

 public:
  InStream();
  InStream(InStream& other) = delete;
  InStream& operator=(InStream&) = delete;
  InStream(InStream&& other) = delete;
  InStream& operator=(InStream&& other) = delete;

  virtual ~InStream();

  inline bool is_valid() const { return (data_ != nullptr); }

  // return current position in the stream
  inline size_t tell() const { return offset_; }

  // return true if current position is at the end of stream
  inline bool is_eof() const { return offset_ == endoffset_; }

  // return number of remaining bytes
  inline size_t bytes_remaining() const { return endoffset_ - offset_; }

  inline size_t begin() const { return startoffset_; }

  inline size_t end() const { return endoffset_; }

  inline size_t loaded_bytes() const { return rootstream_->loaded_bytes_; }

  // copy 'size' bytes from the current position
  // return number of bytes read
  size_t read(uint8_t* ptr, size_t size);

  // advance current position by 'size'
  size_t skip(size_t size);

  // get memory pointer at offset
  // don't change offset_
  // caller can memcpy using returned value and size
  // TODO: write docs .....................................
  void* get_pointer(size_t offset, size_t size);

  // make size of `data_` at least newsize and fill it from stream.
  virtual void prefetch(size_t newsize) = 0;

  // move current position to new 'pos' and return new position.
  // if 'pos' is out of range, current position is not changed...
  size_t seek(size_t pos);

  // rewind current position
  void unread(size_t size) {
    if (offset_ >= size + startoffset_)
      offset_ -= size;
    else
      offset_ = startoffset_;
  }

  // rewind current position to the start
  void rewind() { offset_ = startoffset_; }

  inline InStream* basestream() const { return basestream_; }
  inline InStream* rootstream() const { return rootstream_; }

  inline uint8_t* data() const { return rootstream_->data_; }
  inline size_t datasize() const { return endoffset_ - startoffset_; }
  inline size_t endoffset() const { return endoffset_; }
};

class InStringStream : public InStream {
 public:
  InStringStream();
  virtual ~InStringStream();

  // open file and prepare memory chunk for file data
  // attach to a new file will disrupt contents in DataSet
  void attachmemory(const uint8_t* data, size_t datasize, bool copydata);
  void prefetch(size_t) {} // entire file is already on the memory.
};

class InFileStream : public InStream {
  FILE* fp_;
  std::string filename_;

 public:
  InFileStream();
  virtual ~InFileStream();

  // open file and prepare memory chunk for file data
  // attach to a new file will disrupt contents in DataSet
  void attachfile(const char* filename);
  void detachfile();
  void prefetch(size_t newsize);
};

class InSubStream : public InStream {
  // any operation during InSubStream don't change basestream's offset
 public:
  InSubStream(InStream *basestream, size_t size);
  virtual ~InSubStream();

  void prefetch(size_t newsize);
};


}  // namespace dicom

#endif  // DICOMSDL_INSTREAM_H__
