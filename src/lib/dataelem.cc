/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * dataelement.cc
 */

#include <iostream>
#include <sstream>
#include <type_traits>

#include "dicom.h"
#include "instream.h"

namespace dicom {

template <typename AVT, typename SVT>
static int _value_to_string(AVT value, char tmp[32]) {
  // convert 'AVT' type value' into string in 'SVT' type format.
  // AVT - argument value type - long, long long, or double
  // SVT - store value type, either long long for string in integer format
  // (VR::IS, e.g. "1234") or in decimal format (VR::DS, e.g. "12.34"). Returns
  // length of the resultant string.
  int len;

  if (std::is_integral<SVT>::value) {  // for VR::IS
    if (std::is_integral<AVT>::value) {
      len = snprintf((char *)tmp, 31, "%lld", (long long)value);
    } else {  // type value == double
      long long value_ = (long long)value;
      len = snprintf((char *)tmp, 31, "%lld", (long long)value_);
    }
  } else {  // for VR::DS
    // Maximum length of value if 16 bytes for DS (Decimal String).
    static const char *decimal_formats[] = {"%.15g", "%.14g", "%.13g", "%.12g",
                                            "%.11g", "%.10g", "%.9g",  "%.8g",
                                            "%.7g",  "%.6g",  "%.5g",  NULL};
    // "%.9g" is sufficient actually...
    const char **fmt = decimal_formats;
    do {
      // value with any type will be cast into double
      len = snprintf(tmp, 32, *fmt, (double) value);
      if (len <= 16) break;
      ++fmt;
    } while (*fmt);
  }

  return len;
}

DataElement::DataElement(tag_t tag, vr_t vr, size_t length, size_t offset, DataSet* parent)
    : tag_(tag),
      vr_(vr),
      length_(length),
      offset_(offset),
      parent_(parent) {
  // DataElement should be constructed by DataSet::addDataElement
  // assert parent != nullptr
  if (vr_ == VR::SQ) {
    seq_ = new Sequence(parent_);
  } else if (vr_ == VR::PIXSEQ) {
    tsuid_t tsuid = parent->transfer_syntax();
    pixseq_ = new PixelSequence(parent_, tsuid);
  } else {
    ptr_ = nullptr;
  }
  LOG_DEBUG(
      "++ @%p\tDataElement::DataElement(...)\t%s, %s, len %d, offset {%08x}, "
      "ptr %p, parent %p",
      this, TAG::repr(tag_).c_str(), VR::repr(vr_), length_, offset_, ptr_,
      parent_);
}

DataElement::~DataElement() {
  if (ptr_) {
    switch (vr_) {
      case VR::SQ:
        delete seq_;
        break;
      case VR::PIXSEQ:
        delete pixseq_;
        break;
      default:
        _free_ptr();
        break;
    }
  }
  LOG_DEBUG(
      "-- @%p\tDataElement::DataElement(...)\t%s, %s, len %d, offset {%08x}, "
      "ptr %p, parent %p",
      this, TAG::repr(tag_).c_str(), VR::repr(vr_), length_, offset_, ptr_,
      parent_);

    length_ = 0;
}

void* DataElement::value_ptr() {
  if (ptr_) return ptr_;
  if (parent_ && parent_->instream()) {
    return parent_->instream()->get_pointer(offset_, length_);
  }
  return nullptr;
}

long DataElement::toLong(long default_value) {
  if (!isValid() || length_ == 0) return default_value;

  void *ptr = value_ptr();
  if (ptr == nullptr) return default_value;

  long value;

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::SS:
      value = (length_ >= 2
                   ? (is_little_endian ? load_le<int16_t>(ptr) : load_be<int16_t>(ptr))
                   : default_value);
      break;
    case VR::US:
      value = (length_ >= 2
                   ? (is_little_endian ? load_le<uint16_t>(ptr) : load_be<uint16_t>(ptr))
                   : default_value);
      break;
    case VR::SL:
      value = (length_ >= 4
                   ? (is_little_endian ? load_le<int32_t>(ptr) : load_be<int32_t>(ptr))
                   : default_value);
      break;
    case VR::UL:
      value = (length_ >= 4
                   ? (is_little_endian ? load_le<uint32_t>(ptr) : load_be<uint32_t>(ptr))
                   : default_value);
      break;
    case VR::AT:
      value = (length_ >= 4
                   ? (is_little_endian ? load_le<uint32_t>(ptr) : load_be<uint32_t>(ptr))
                   : default_value);
      if (is_little_endian) {
        uint16_t hi = value >> 16;
        uint16_t lo = value & 0xffff;
        value = hi + (lo << 16);
      }
      break;
    case VR::SV:
      value = (length_ >= 8
                   ? (is_little_endian ? load_le<int64_t>(ptr) : load_be<int64_t>(ptr))
                   : default_value);
      break;
    case VR::UV:
      value = (length_ >= 8
                   ? (is_little_endian ? load_le<uint64_t>(ptr) : load_be<uint64_t>(ptr))
                   : default_value);
      break;
    case VR::IS:
      if (length_ == 0)
        value = default_value;
      else {
        std::string s((const char*) (ptr), length_);
        value = strtol(s.c_str(), NULL, 10);
      }
      break;
    default:
      value = default_value;
      break;
  }

  return value;
}

long long DataElement::toLongLong(long long default_value) {
  if (!isValid() || length_ == 0) return default_value;

  void *ptr = value_ptr();
  if (ptr == nullptr) return default_value;

  long long value;

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::SS:
      value = (length_ >= 2
                   ? (is_little_endian ? load_le<int16_t>(ptr) : load_be<int16_t>(ptr))
                   : default_value);
      break;
    case VR::US:
      value = (length_ >= 2
                   ? (is_little_endian ? load_le<uint16_t>(ptr) : load_be<uint16_t>(ptr))
                   : default_value);
      break;
    case VR::SL:
      value = (length_ >= 4
                   ? (is_little_endian ? load_le<int32_t>(ptr) : load_be<int32_t>(ptr))
                   : default_value);
      break;
    case VR::UL:
      value = (length_ >= 4
                   ? (is_little_endian ? load_le<uint32_t>(ptr) : load_be<uint32_t>(ptr))
                   : default_value);
      break;
    case VR::AT:
      value = (length_ >= 4 ? (is_little_endian ? load_le<uint32_t>(ptr)
                                                : load_be<uint32_t>(ptr))
                            : default_value);
      if (is_little_endian) {
        uint16_t hi = value >> 16;
        uint16_t lo = value & 0xffff;
        value = hi + (lo << 16);
      }
      break;
    case VR::SV:
      value = (length_ >= 8
                   ? (is_little_endian ? load_le<int64_t>(ptr) : load_be<int64_t>(ptr))
                   : default_value);
      break;
    case VR::UV:
      value = (length_ >= 8
                   ? (is_little_endian ? load_le<uint64_t>(ptr) : load_be<uint64_t>(ptr))
                   : default_value);
      break;
    case VR::IS:
      if (length_ == 0)
        value = default_value;
      else {
        std::string s((const char*) (ptr), length_);
        value = strtoll(s.c_str(), NULL, 10);
      }
      break;
    default:
      value = default_value;
      break;
  }

  return value;
}



#define PUSHBACK(TYPE)                                  \
  {                                                     \
    nvalues = length_ / sizeof(TYPE);                   \
    vec.reserve(nvalues);                                \
    if (is_little_endian) {                             \
      for (int i = 0; i < nvalues; i++) {               \
        vec.push_back(load_le<TYPE>(((TYPE *)ptr) + i)); \
      }                                                 \
    } else {                                            \
      for (int i = 0; i < nvalues; i++) {               \
        vec.push_back(load_be<TYPE>(((TYPE *)ptr) + i)); \
      }                                                 \
    }                                                   \
  }


std::vector<long> DataElement::toLongVector() {
  std::vector<long> vec;
  if (!isValid() || length_ == 0) return vec;

  void *ptr = value_ptr();
  if (ptr == nullptr) return vec;
  int nvalues;

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::SS:
      PUSHBACK(int16_t)
      break;
    case VR::US:
    case VR::OW:
      PUSHBACK(uint16_t)
      break;
    case VR::SL:
      PUSHBACK(int32_t)
      break;
    case VR::UL:
    case VR::OL:
      PUSHBACK(uint32_t)
      break;
    case VR::AT:
      PUSHBACK(uint32_t)
      // switch high and lower word
      {
        if (is_little_endian) {
          for (int i = 0; i < nvalues; i++) {
            uint16_t hi = vec[i] >> 16;
            uint16_t lo = vec[i] & 0xffff;
            vec[i] = hi + (lo << 16);
          }
        }
      }
      break;
    case VR::SV:
      PUSHBACK(uint64_t)
      break;
    case VR::UV:
    case VR::OV:
      PUSHBACK(uint64_t)
      break;
    case VR::IS: {
      char *startp, *p, *nextp;
      startp = p = (char *)(ptr);
      nextp = NULL;
      long value;
      while (1) {
        value = strtol(p, &nextp, 10);
        if (value == 0 && errno == EINVAL) break;  // no conversion is performed
        vec.push_back(value);
        p = nextp + 1;  // skip delim ('/')
        if (p - startp >= (int)length_) break;
      }
    } break;
    default:
      LOGERROR_AND_THROW(
          "DataElement::toLongVector - "
          "Value of a DataElement %s, VR %s cannot be convert "
          "to long values.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  return vec;
}
std::vector<long long> DataElement::toLongLongVector() {
  std::vector<long long> vec;
  if (!isValid() || length_ == 0) return vec;

  void *ptr = value_ptr();
  if (ptr == nullptr) return vec;
  int nvalues;

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::SS:
      PUSHBACK(int16_t)
      break;
    case VR::US:
    case VR::OW:
      PUSHBACK(uint16_t)
      break;
    case VR::SL:
      PUSHBACK(int32_t)
      break;
    case VR::UL:
    case VR::OL:
      PUSHBACK(uint32_t)
      break;
    case VR::AT:
      PUSHBACK(uint32_t)
      // switch high and lower word
      {
        if (is_little_endian) {
          for (int i = 0; i < nvalues; i++) {
            uint16_t hi = vec[i] >> 16;
            uint16_t lo = vec[i] & 0xffff;
            vec[i] = hi + (lo << 16);
          }
        }
      }
      break;      
    case VR::SV:
      PUSHBACK(uint64_t)
      break;
    case VR::UV:
    case VR::OV:
      PUSHBACK(uint64_t)
      break;
    case VR::IS: {
      char *startp, *p, *nextp;
      startp = p = (char *)(ptr);
      nextp = NULL;
      long long value;
      while (1) {
        value = strtoll(p, &nextp, 10);
        if (value == 0 && errno == EINVAL) break;  // no conversion is performed
        vec.push_back(value);
        p = nextp + 1;  // skip delim ('/')
        if (p - startp >= (int)length_) break;
      }
    } break;
    default:
      LOGERROR_AND_THROW(
          "DataElement::toLongLongVector - "
          "Value of a DataElement %s, VR %s cannot be convert "
          "to long long values.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  return vec;
}

double DataElement::toDouble(double default_value) {
  if (!isValid() || length_ == 0) return default_value;

  void *ptr = value_ptr();
  if (ptr == nullptr) return default_value;

  double value;

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::FL:
    case VR::OF:
      value = (length_ >= 4
                   ? (is_little_endian ? load_le<float32_t>(ptr) : load_be<float32_t>(ptr))
                   : default_value);
      break;
    case VR::FD:
    case VR::OD:
      value = (length_ >= 8
                   ? (is_little_endian ? load_le<float64_t>(ptr) : load_be<float64_t>(ptr))
                   : default_value);
      break;
    case VR::DS:
      if (length_ == 0)
        value = default_value;
      else {
        std::string s((const char*) (ptr), length_);
        return atof(s.c_str());
      }
      break;

    case VR::SS:
    case VR::US:
    case VR::OW:
    case VR::SL:
    case VR::UL:
    case VR::OL:
    case VR::SV:
    case VR::UV:
    case VR::OV:
    case VR::IS:
      value = (double) toLongLong((int) default_value);
      break;

    default:
      value = default_value;
      break;
  }
  return value;
}

std::vector<double> DataElement::toDoubleVector() {
  std::vector<double> vec;
  if (!isValid() || length_ == 0) return vec;

  void *ptr = value_ptr();
  if (ptr == nullptr) return vec;
  int nvalues;

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::FL:
    case VR::OF:
      PUSHBACK(float32_t)
      break;
    case VR::FD:
    case VR::OD:
      PUSHBACK(float64_t)
      break;

    case VR::DS: {
      char *startp, *p, *nextp;
      startp = p = (char *)(ptr);
      nextp = NULL;
      double value;
      while (1) {
        value = strtod(p, &nextp);
        if (p == nextp) break;  // no conversion is performed
        vec.push_back(value);
        p = nextp + 1;  // skip delim ('/')
        if (p - startp >= (int)length_) break;
      }
    } break;
    default:
      LOGERROR_AND_THROW("DataElement::toDoubleVector",
                         "Value of a DataElement %s, VR %s cannot be convert "
                         "to double values.",
                         TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  return vec;
}

int DataElement::vm() {
  // PS 3.5, 6.4 VALUE MULTIPLICITY (VM) AND DELIMITATION
  if (length_ == 0) return 0;

  switch (vr_) {
    case VR::FD:
    case VR::SV:
    case VR::UV:
      return int(length_) / 8;
    case VR::AT:
    case VR::FL:
    case VR::UL:
    case VR::SL:
      return int(length_) / 4;
    case VR::US:
    case VR::SS:
      return int(length_) / 2;

    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DA:
    case VR::DS:
    case VR::DT:
    case VR::IS:
    case VR::LO:
    case VR::PN:
    case VR::SH:
    case VR::TM:
    case VR::UC:
    case VR::UI: {
      uint8_t* p = static_cast<uint8_t*>(value_ptr());
      if (p) {
        int delims = 0;
        for (size_t i = 0; i < length_; i++)
          if (p[i] == '\\') delims++;
        return delims + 1;
      } else
        return 0;
    }

      // text
    case VR::ST:
    case VR::UT:
    case VR::LT:

    // LT, OB, OD, OF, OL, OW, SQ, ST, UN, UR or UT -> always 1

    case VR::OB:
    case VR::OD:
    case VR::OF:
    case VR::OL:
    case VR::OW:
    case VR::OV:
    case VR::SQ:
    case VR::UN:
    case VR::UR:

    default:
      return 1;
  }
}

static void de_value_rstrip(char **s, int *n) {
  char *p = *s;
  p += (*n - 1);
  while (*n > 0) {
    if (isspace(*p) || *p == '\0') {
      --p;
      --(*n);
    } else
      break;
  }
  if (*n < 0) *n += 1;
}

static void de_value_lstrip(char **s, int *n) {
  while (*n > 0) {
    if (isspace(**s)) {
      ++(*s);
      --(*n);
    } else
      break;
  }
  if (*n < 0) *n += 1;
}

std::string DataElement::toBytes(const char *default_value) {
  if (!isValid() || length_ == 0) return std::string(default_value);

  char *s = (char *)value_ptr();
  int n = (int)length_;

  if (s) {
    switch (vr_) {
      // ignore leading and trailing spaces
      case VR::AE:  // leading and trailing spaces (20H) being non-significant.
      case VR::AS:  // 4 bytes fixed
      case VR::CS:  // leading and trailing spaces (20H) being non-significant.
      case VR::DS:  // may be padded with leading or trailing spaces
      case VR::DT:  // fixed format YYYYMMDDHHMMSS.FFFFFF&ZZXX
      case VR::IS:  // may be padded with leading and/or trailing spaces.
      case VR::LO:  // may be padded with leading and/or trailing spaces.
      case VR::PN:  // leading and trailing spaces and considers them
                    // insignificant.
      case VR::SH:  // may be padded with leading and/or trailing spaces.
        de_value_lstrip(&s, &n);
        de_value_rstrip(&s, &n);
        break;

      // ignore trailing spaces
      case VR::DA:  // a trailing SPACE character is allowed for
                    // padding.
      case VR::TM:  // may be padded with trailing spaces. Leading and
                    // embedded spaces are not allowed.

      case VR::UC:  // may be padded with trailing spaces.
      case VR::UI:  // the Value Field shall be padded with a single trailing
                    // NULL (00H) character to ensure that the Value Field is
                    // an even number of bytes in length
      case VR::UR:  // Trailing spaces shall be ignored
        de_value_rstrip(&s, &n);
        break;

      // remove trailing spaces, but keep leading spaces.
      case VR::LT:
      case VR::ST:
      case VR::UT:  // It may be padded with trailing spaces, which may be
                    // ignored, but leading spaces are considered to be
                    // significant
        de_value_rstrip(&s, &n);
        break;

      default:
        break;
    }
    return std::string(s, (size_t)n);
  } else
    return std::string("");
}

std::wstring DataElement::repr(size_t max_length)
{
  // TODO: \n \t ...
  // TODO: LO - strip heading/trailing spaces!!!!!!!!!!!!!!!!!?????????????
  if (!isValid()) return L"n/a";
  if (length_ == 0) return L"(no value)";

  std::wstringstream oss;
  std::wstring reprstr;
  auto _crop = [max_length](std::wstring s, std::wstring postfix) -> std::wstring {
    size_t maxlen = max_length - postfix.size();
    if (max_length > 0 && s.size() > maxlen)
      s = s.substr(0, maxlen) + postfix;
    return s;
  };

  switch (vr_) {
    case VR::AE:  case VR::AS:  case VR::CS:  case VR::DA:  case VR::DS:
    case VR::DT:  case VR::IS:  case VR::TM:  case VR::UI:  case VR::UR:
    case VR::LO:  case VR::LT:  case VR::PN:  case VR::SH:  case VR::ST:
    case VR::UC:  case VR::UT:
      try {
        reprstr = _crop(L"'" + toString() + L"'", L"...");
        return reprstr;
      } catch (DicomException &) {
        // failed during unicode conversion. treat this like OB or UN
      }
      break;
    default:
      break;
  }
  switch (vr_) {
    case VR::AT: {
      std::wostringstream oss;
      wchar_t buf[32];
      std::vector<long> val = toLongVector();
      for (size_t i = 0;;) {
        swprintf(buf, 32, L"(%04x,%04x)", val[i] >> 16, val[i] & 0xffff);
        oss << buf;
        if (++i >= val.size()) break;
        oss << L"\\";
      }
      reprstr = _crop(oss.str(), L"...");
    } break;
    case VR::FD:
    case VR::FL:
    case VR::OD:
    case VR::OF: {
      std::vector<double> val = toDoubleVector();
      for (size_t i = 0;;) {
        oss << val[i];
        if (++i >= val.size() || i > max_length) break;
        oss << L"\\";
      }
      reprstr = _crop(oss.str(), L"...");
    } break;
    case VR::SS:
    case VR::US:
    case VR::SL:
    case VR::UL:
    case VR::SV:
    case VR::UV: {
      std::vector<long long> val = toLongLongVector();
      for (size_t i = 0;;) {
        oss << val[i];
        if (++i >= val.size() || i > max_length) break;
        oss << L"\\";
      }
      reprstr = _crop(oss.str(), L"...");
    } break;
    case VR::OW: {
      wchar_t buf[32];
      std::vector<long> val = toLongVector();
      for (size_t i = 0;;) {
        swprintf(buf, 32, L"%04lx", val[i]);
        oss << buf;
        if (++i >= val.size() || i > max_length) break;
        oss << L"\\";
      }
      reprstr = _crop(oss.str(), L"...");
    } break;
    case VR::OL: {
      wchar_t buf[32];
      std::vector<long> val = toLongVector();
      for (size_t i = 0;;) {
        swprintf(buf, 32, L"%08lx", val[i]);
        oss << buf;
        if (++i >= val.size() || i > max_length) break;
        oss << L"\\";
      }
      reprstr = _crop(oss.str(), L"...");
    } break;
    case VR::OV: {
      wchar_t buf[32];
      std::vector<long long> val = toLongLongVector();
      for (size_t i = 0;;) {
        swprintf(buf, 32, L"%016llx", val[i]);
        oss << buf;
        if (++i >= val.size() || i > max_length) break;
        oss << L"\\";
      }
      reprstr = _crop(oss.str(), L"...");
    } break;
    case VR::OFFSET: {
      return (L"VR_OFFSET");  // TODO: -----------------------------------------------
    } break;
    case VR::SQ: {
      oss << L"SEQUENCE WITH " << toSequence()->size() << L" DATASET(s)";
      reprstr = oss.str();
    } break;
    case VR::PIXSEQ: {
      oss << L"PIXEL SEQUENCE WITH " << toPixelSequence()->numberOfFrames()
          << L" FRAME(S)";
      reprstr = oss.str();
    } break;

    case VR::OB:
    case VR::UN:
    // error during unicode conversion - treat as OB or UN
    // case VR::AE:  case VR::AS:  case VR::CS:  case VR::DA:  case VR::DS:
    // case VR::DT:  case VR::IS:  case VR::TM:  case VR::UI:  case VR::UR:
    // case VR::LO:  case VR::LT:  case VR::PN:  case VR::SH:  case VR::ST:
    // case VR::UC:  case VR::UT:
    default: {
      wchar_t buf[32];

      oss << "'";
      char *p = (char *)value_ptr();
      for (size_t i = 0; i < length_; i++) {
        if (isprint(*p)) {
          swprintf(buf, 32, L"%hc", *p);
          oss << buf;
        } else {
          swprintf(buf, 32, L"\\x%02x", *(uint8_t *)p);
          oss << buf;
        }
        if (i >= max_length) break;
        p++;
      }
      oss << "'";
      reprstr = _crop(oss.str(), L"...");
    } break;
  }

  return reprstr;
}

std::vector<std::wstring> DataElement::toStringVector() {
  std::vector<std::wstring> vec;

  if (!isValid() || length_ == 0)
    return vec;

  int vecsize = vm();
  vec.reserve(vecsize);
  
  if (vecsize == 1) {
    vec.push_back(toString());
    return vec;
  }

  charset_t charset;

  switch (vr_) {
    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DA:
    case VR::DS:
    case VR::DT:
    case VR::IS:
    case VR::TM:
    case VR::UI:
    case VR::UR:
      charset = CHARSET::DEFAULT;
      break;

    case VR::LO:
    case VR::PN:
    case VR::SH:
    case VR::UC:
      charset = parent_->getSpecificCharset();
      break;
    default:
      LOGERROR_AND_THROW(
          "DataElement::toStringVector - Value of a DataElement %s, VR %s "
          "cannot be converted to unicode string vector.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }

  bool need_strip_leading_space = false;

  switch (vr_){
    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DS:
    case VR::DT:
    case VR::IS:
    case VR::LO:
    case VR::PN:
    case VR::SH:

    case VR::DA:
    case VR::TM:
    case VR::UI: 
      need_strip_leading_space = true;
      break;
    // case VR::UC:
    default:
      // other VR (i.e. VR::UC), keep leading spaces.
      break;
  }

  char *p = (char *)value_ptr();
  char *nextp;
  int n = (int) length_; // remaining bytes
  int i;

  while (n > 0) {
    for (i = 0; i < n; i++) {
      if (p[i] == '\\')
        break;
    }
    // i==n (end of string) or p[i]=='\\' (end of an item)
    if (i == n) { // end of string
      nextp = p + i;
      n -= i;
    } else { // end of an item
      nextp = p + i + 1;
      n -= (i + 1);
    }

    if (need_strip_leading_space) de_value_lstrip(&p, &i);
    de_value_rstrip(&p, &i);
    
    vec.push_back(convert_to_unicode(p, i, charset));  // may throw error
    p = nextp;
  }

  return vec;
}

std::wstring DataElement::toString(const wchar_t *default_value) {
  if (!isValid())
    return default_value;

  if (length_ == 0)
    return std::wstring(L"");

  charset_t charset;
  std::wstring ws;

  char *p = (char *)value_ptr();
  int n = (int) length_;

  switch (vr_) {
    // ignore leading and trailing spaces
    case VR::AE:  // leading and trailing spaces (20H) being non-significant.
    case VR::AS:  // 4 bytes fixed
    case VR::CS:  // leading and trailing spaces (20H) being non-significant.
    case VR::DS:  // may be padded with leading or trailing spaces
    case VR::DT:  // fixed format YYYYMMDDHHMMSS.FFFFFF&ZZXX
    case VR::IS:  // may be padded with leading and/or trailing spaces.
    case VR::LO:  // may be padded with leading and/or trailing spaces.
    case VR::PN:  // leading and trailing spaces and considers them
                  // insignificant.
    case VR::SH:  // may be padded with leading and/or trailing spaces.
      de_value_lstrip(&p, &n);
      de_value_rstrip(&p, &n);
      break;

    // ignore trailing spaces
    case VR::DA:  // a trailing SPACE character is allowed for padding.
    case VR::TM:  // may be padded with trailing spaces. Leading and embedded
                  // spaces are not allowed.

    case VR::UC:  // may be padded with trailing spaces.
    case VR::UI:  // the Value Field shall be padded with a single trailing
                  // NULL (00H) character to ensure that the Value Field is
                  // an even number of bytes in length
    case VR::UR:  // Trailing spaces shall be ignored
      de_value_rstrip(&p, &n);
      break;

    // remove trailing spaces, but keep leading spaces.
    case VR::LT:
    case VR::ST:
    case VR::UT:  // It may be padded with trailing spaces, which may be
                  // ignored, but leading spaces are considered to be
                  // significant
      de_value_rstrip(&p, &n);
      break;
      
    default:
      break;
  }

  switch (vr_) {
    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DA:
    case VR::DS:
    case VR::DT:
    case VR::IS:
    case VR::TM:
    case VR::UI:
    case VR::UR:
      charset = CHARSET::DEFAULT;
      ws = convert_to_unicode(p, n, charset);  // may throw error
      break;

    case VR::LO:
    case VR::LT:
    case VR::PN:
    case VR::SH:
    case VR::ST:
    case VR::UC:
    case VR::UT:
      charset = parent_->getSpecificCharset();
      ws = convert_to_unicode(p, n, charset);  // may throw error
      break;

    default:
      LOGERROR_AND_THROW(
          "DataElement::toString - Value of a DataElement %s, VR %s cannot be "
          "converted to unicode string.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;

      return L"";
      break;
  }
  return ws;
}

void DataElement::fromLong(const long value) {
  uint8_t tmp[32];

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::SS:
      store_e<int16_t>(tmp, (int16_t)value, is_little_endian);
      length_ = 2;
      break;
    case VR::US:
      store_e<uint16_t>(tmp, (uint16_t)value, is_little_endian);
      length_ = 2;
      break;
    case VR::SL:
      store_e<int32_t>(tmp, (int32_t)value, is_little_endian);
      length_ = 4;
      break;
    case VR::UL:
      store_e<uint32_t>(tmp, (uint32_t)value, is_little_endian);
      length_ = 4;
      break;
    case VR::AT: {
      uint16_t hi = value >> 16;
      uint16_t lo = value & 0xffff;
      store_e<uint16_t>(tmp, hi, is_little_endian);
      store_e<uint16_t>(tmp, lo, is_little_endian);
    }
      length_ = 4;
      break;
    case VR::SV:
      store_e<int64_t>(tmp, (int64_t)value, is_little_endian);
      length_ = 8;
      break;
    case VR::UV:
      store_e<uint64_t>(tmp, (uint64_t)value, is_little_endian);
      length_ = 8;
      break;
    case VR::IS:
      length_ = snprintf((char *)tmp, 31, "%ld", value);
      if (length_ & 1) {
        tmp[length_] = ' ';  // trailing space to make length even.
        length_++;
      }
      break;
    case VR::FL:
    case VR::FD:
    case VR::DS:
      fromDouble((double)value);
      return;
      break;      
    default:
      LOGERROR_AND_THROW(
          "DataElement::setValue(long) - "
          "cannot set long value to the DataElement %s, VR %s.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  alloc_ptr_(length_);
  memcpy(ptr_, tmp, length_);
}

void DataElement::fromLongLong(const long long value) {
  uint8_t tmp[32];

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::SS:
      store_e<int16_t>(tmp, (int16_t)value, is_little_endian);
      length_ = 2;
      break;
    case VR::US:
      store_e<uint16_t>(tmp, (uint16_t)value, is_little_endian);
      length_ = 2;
      break;
    case VR::SL:
      store_e<int32_t>(tmp, (int32_t)value, is_little_endian);
      length_ = 4;
      break;
    case VR::UL:
      store_e<uint32_t>(tmp, (uint32_t)value, is_little_endian);
      length_ = 4;
      break;
    case VR::AT: {
      uint16_t hi = value >> 16;
      uint16_t lo = value & 0xffff;
      store_e<uint16_t>(tmp, hi, is_little_endian);
      store_e<uint16_t>(tmp, lo, is_little_endian);
    }
      length_ = 4;
      break;
    case VR::SV:
      store_e<int64_t>(tmp, (int64_t)value, is_little_endian);
      length_ = 8;
      break;
    case VR::UV:
      store_e<uint64_t>(tmp, (uint64_t)value, is_little_endian);
      length_ = 8;
      break;
    case VR::IS:
      length_ = snprintf((char *)tmp, 31, "%lld", value);
      if (length_ & 1) {
        tmp[length_] = ' ';  // trailing space to make length even.
        length_++;
      }
      break;
    case VR::FL:
    case VR::FD:
    case VR::DS:
      fromDouble((double)value);
      return;
      break;
    default:
      LOGERROR_AND_THROW(
          "DataElement::setValue(long long) - "
          "cannot set long long value to the DataElement %s, VR %s.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  alloc_ptr_(length_);
  memcpy(ptr_, tmp, length_);
}

template<typename AVT, typename SVT>
void DataElement::_fromNumberVectorToBytes(const std::vector<AVT> &value)
{
  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;
  
  alloc_ptr_(value.size() *sizeof(SVT));
  SVT *p = (SVT *)ptr_;
  for (auto v: value) {
    store_e<SVT>(p, (SVT)v, is_little_endian);
    ++p;
  }
}

template <typename AVT>
void DataElement::_fromNumberVectorToAttrTags(const std::vector<AVT> &value) {
  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  alloc_ptr_(value.size() * sizeof(tag_t));
  uint16_t *p = (uint16_t *)ptr_;
  for (auto v : value) {
    long _v = (long)v;
    store_e<uint16_t>(p, _v >> 16, is_little_endian);
    store_e<uint16_t>(p + 1, _v & 0xffff, is_little_endian);
    p += 2;
  }
}

void DataElement::fromDouble(const double value) {
  uint8_t tmp[32];

  bool is_little_endian =
      parent_->is_little_endian() || TAG::group(tag_) == 0x0002;

  switch (vr_) {
    case VR::FL:
      store_e<float32_t>(tmp, (float32_t)value, is_little_endian);
      length_ = 4;
      break;
    case VR::FD:
      store_e<float64_t>(tmp, (float64_t)value, is_little_endian);
      length_ = 8;
      break;
    case VR::DS:
      length_ = _value_to_string<double, double>(value, (char *)tmp);
      if (length_ & 1) {
        tmp[length_] = ' ';  // trailing space to make length even.
        length_++;
      }
      break;
    case VR::SL:
    case VR::SS:
    case VR::SV:
    case VR::UL:
    case VR::US:
    case VR::UV:
    case VR::IS:
      fromLong((long)value);
      return;
      break;
    default:
      LOGERROR_AND_THROW(
          "DataElement::setValue(double) - "
          "cannot set double value to the DataElement %s, VR %s.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  alloc_ptr_(length_);
  memcpy(ptr_, tmp, length_);
}

// AVT; argument type - long, long long or double
// SVT; stored value type - uint16_t, int16_t, uint32_t, ....
// SVT; stored value type - double for DS, long for IS
template <typename AVT, typename SVT>
void DataElement::_fromNumberVectorToString(const std::vector<AVT> &value) {
  char tmp[32];
  std::string tmpstr;
  int len;
  tmpstr.reserve(value.size() * 8);
  for (auto v : value) {
    len = _value_to_string<AVT, SVT>(v, tmp);
    tmpstr.append(tmp, len);
    tmpstr.push_back('\\');
  }
  tmpstr.pop_back();
  fromBytes(tmpstr);
}

// AVT; argument value type - long, long long or double
template <typename AVT>
bool DataElement::_fromNumberVector(const std::vector<AVT> &value) {
  if (value.size() == 0)
    return true;

  switch (vr_) {
    case VR::SS:
      _fromNumberVectorToBytes<AVT, int16_t>(value);
      break;
    case VR::SL:
      _fromNumberVectorToBytes<AVT, int32_t>(value);
      break;
    case VR::SV:
      _fromNumberVectorToBytes<AVT, int64_t>(value);
      break;
    case VR::US:
    case VR::OW:    
      _fromNumberVectorToBytes<AVT, uint16_t>(value);
      break;
    case VR::UL:
    case VR::OL:
      _fromNumberVectorToBytes<AVT, uint32_t>(value);
      break;
    case VR::UV:
    case VR::OV:
      _fromNumberVectorToBytes<AVT, uint64_t>(value);
      break;
    case VR::AT:
      _fromNumberVectorToAttrTags<AVT>(value);
      break;
    case VR::FL:
      _fromNumberVectorToBytes<AVT, float32_t>(value);
      break;
    case VR::FD:
      _fromNumberVectorToBytes<AVT, float64_t>(value);
      break;
    case VR::IS:
      _fromNumberVectorToString<AVT, long long>(value);
      break;
    case VR::DS:
      _fromNumberVectorToString<AVT, double>(value);
      break;
    default:
      return false;
      break;
  }
  return true;
}

void DataElement::fromLongVector(const std::vector<long> &value) {
  if (_fromNumberVector<long>(value))
    return;
  else
    LOGERROR_AND_THROW(
        "DataElement::fromLongVector - "
        "cannot set long vector value to the DataElement %s, VR %s.",
        TAG::repr(tag_).c_str(), VR::repr(vr_));
}

void DataElement::fromLongLongVector(const std::vector<long long> &value) {
  if (_fromNumberVector<long long>(value))
    return;
  else
    LOGERROR_AND_THROW(
        "DataElement::fromLongLongVector - "
        "cannot set long long vector value to the DataElement %s, VR %s.",
        TAG::repr(tag_).c_str(), VR::repr(vr_));
}

void DataElement::fromDoubleVector(const std::vector<double> &value) {
  if (_fromNumberVector<double>(value))
    return;
  else
    LOGERROR_AND_THROW(
        "DataElement::fromDoubleVector - "
        "cannot set double vector value to the DataElement %s, VR %s.",
        TAG::repr(tag_).c_str(), VR::repr(vr_));
}

void DataElement::fromString(const wchar_t *value, size_t length) {
  if (value == nullptr || *value == L'\0')
    return;

  if (length == -1) {
    length = wcslen(value);
  }

  charset_t charset;
  DataSet *dataset = parent_;
  std::string bytes;

  switch (vr_) {
    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DA:
    case VR::DS:
    case VR::DT:
    case VR::IS:
    case VR::TM:
    case VR::UI:
    case VR::UR:
      charset = CHARSET::DEFAULT;
      break;

    case VR::LO:
    case VR::LT:
    case VR::PN:
    case VR::SH:
    case VR::ST:
    case VR::UC:
    case VR::UT:
      // get character set for encoding
      charset = parent_->getSpecificCharset(1);
      break;

    default:
      LOGERROR_AND_THROW(
          "DataElement::fromString - cannot set unicode value to the "
          "DataElement %s, VR %s",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  bytes = convert_from_unicode(value, length, charset);  // may throw error
  fromBytes(bytes);
}

void DataElement::fromString(const std::wstring &value) {
  fromString(value.c_str(), value.size());
}

void DataElement::fromStringVector(const std::vector<std::wstring> &value) {
  if (value.size() == 0) return;
  switch (vr_) {
    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DA:
    case VR::DS:
    case VR::DT:
    case VR::IS:
    case VR::LO:
    case VR::PN:
    case VR::SH:
    case VR::TM:
    case VR::UC:
    case VR::UI:
      break;
    default:
      LOGERROR_AND_THROW(
          "DataElement::fromStringVector - "
          "DataElement %s, VR %s does not take multiple string values.",
          TAG::repr(tag_).c_str(), VR::repr(vr_));
      break;
  }
  std::wstring valstr;
  size_t totalsize = 0;
  for (auto v: value) {
    totalsize += value.size() + 1;
  }
  valstr.reserve(totalsize);
  for (auto v: value) {
    valstr.append(v);
    valstr.push_back(L'\\');
  }
  valstr.pop_back();
  fromString(valstr);
}

void  DataElement::fromBytes(const char *value, size_t length) {
  if (length == 0)
    return;

  if (length == -1) {
    length = strlen(value);
  }

  alloc_ptr_((length & 1) ? length + 1 : length);
  memcpy(ptr_, value, length);

  if (length & 1) {
    switch (vr_) {
      // add a trailing space
      case VR::AE:
      case VR::AS:
      case VR::CS:
      case VR::DS:
      case VR::DT:
      case VR::IS:
      case VR::LO:
      case VR::PN:
      case VR::SH:
      case VR::DA:
      case VR::TM:
      case VR::UC:
      case VR::UR:
      case VR::LT:
      case VR::ST:
      case VR::UT:
        ((char *)ptr_)[length] = ' ';

        break;
        // a single trailing NULL (00H) character
      case VR::UI:
      default:
        ((char *)ptr_)[length] = '\0';
        break;
    }
  }
}

void DataElement::fromBytes(const std::string &value) {
  fromBytes(value.c_str(), value.length());
}

void DataElement::alloc_ptr_(size_t size) {
  if (size & 1) {
    LOGERROR_AND_THROW(
        "DataElement::alloc_(size_t) - "
        "Size = %zd bytes is not even number for the DataElement %s, VR %s.",
        size, TAG::repr(tag_).c_str(), VR::repr(vr_));
  }

  _free_ptr();
  if (size == 0) return;
  ptr_ = ::malloc(size);
  if (!ptr_) {
    LOGERROR_AND_THROW(
        "DataElement::alloc_(size_t) - "
        "cannot allocate %zd bytes for the DataElement %s, VR %s.",
        size, TAG::repr(tag_).c_str(), VR::repr(vr_));
  }
  length_ = size;
}

void DataElement::_free_ptr()
{
  if (ptr_) {
    ::free(ptr_);
    ptr_ = nullptr;
    length_ = 0;
  }
}

}  // namespace dicom
