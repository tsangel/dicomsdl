/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * dicomcfg.h
 */

#ifndef DICOMSDL_DICOMCFG_H_
#define DICOMSDL_DICOMCFG_H_

/* stdint.h style int type names */
#ifdef _WIN32
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else

#include <stdint.h>
#include <stdlib.h>  // for size_t

#endif

typedef unsigned int uint;
typedef float float32_t;
typedef double float64_t;

namespace dicom {

const char *const DICOMSDL_VERSION = "0.109.2";

const char *const DICOMSDL_UIDPREFIX = "1.3.6.1.4.1.56559";
const char *const DICOMSDL_IMPLCLASSUID = "1.3.6.1.4.1.56559.1";
const char *const DICOMSDL_IMPLVERNAME = "DICOMSDL 2022OCT";
const char *const DICOMSDL_FILESETID = "DICOMDIR";
const char *const DICOMSDL_SOURCEAETITLE = "DICOMSDL";

/* determine machine and compiler ------------------------------------------- */

#if !defined (__linux__) && !defined(__APPLE__) && !defined(_WIN32)
#error Cannot determine machine \
    (One of __linux__, __APPLE__ or _WIN32 should be defined)
#endif

#if !defined (__GNUC__) && !defined(_MSC_VER)
#error Cannot determine compiler \
    (Neither __GNUC__ nor _MSC_VER is defined)
#endif

#if defined(_MSC_VER) && defined(BUILD_SHARED_LIBS)
#ifdef _BUILD_DLL
#pragma warning(disable: 4251)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif  // _BUILD_DLL
#else  // defined(_MSC_VER) && defined(BUILD_SHARED_LIBS)
#define DLLEXPORT
#endif

/* define types and macros -------------------------------------------------- */

/* wchar */
#ifdef __GNUC__
#include <stdint.h>
#if defined(__WCHAR_WIDTH__)
#define WCHAR_WIDTH __WCHAR_WIDTH__
#elif defined(WCHAR_MAX) and (WCHAR_MAX > 0x10001)
#define WCHAR_WIDTH 32
#elif defined(WCHAR_MAX) and (WCHAR_MAX < 0x10001)
#define WCHAR_WIDTH 16
#else
#error cannot determine WCHAR_WIDTH
#endif
#endif

#ifdef _WIN32
#include <stddef.h>   /* uintptr_t is declared here */
#define WCHAR_WIDTH 16
#endif

// /* null */
// #undef NULL
// #define NULL 0

/* from Google C++ Style Guide */
#ifdef _LP64
#define __PRIS_PREFIX "z"
#else
#define __PRIS_PREFIX
#endif

/* Use these macros after a % in a printf format string
 to get correct 32/64 bit behavior, like this:
 size_t size = records.size();
 printf("%"PRIuS"\n", size); */
#define PRIdS __PRIS_PREFIX "d"
#define PRIxS __PRIS_PREFIX "x"
#define PRIuS __PRIS_PREFIX "u"
#define PRIXS __PRIS_PREFIX "X"
#define PRIoS __PRIS_PREFIX "o"

/* PRIxx macros */
#if defined(__linux) || defined(__APPLE__)
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#elif defined(_WIN32)
#define PRId64 "I64d"
#define PRIu64 "I64u"
#define PRIx64 "I64x"
#endif



/* define endianess-----------------------------------------------------------*/

#ifdef __linux__
#include <endian.h>
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN    __BIG_ENDIAN
#endif

#ifdef __APPLE__
#include <machine/endian.h>
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __BIG_ENDIAN    BIG_ENDIAN
#define __BYTE_ORDER    BYTE_ORDER
#endif

#ifdef _WIN32
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER    __LITTLE_ENDIAN
#define LITTLE_ENDIAN   __LITTLE_ENDIAN
#define BIG_ENDIAN      __BIG_ENDIAN
#endif

#if defined (_MSC_VER)
#define bswap16(x) _byteswap_ushort(x);
#define bswap32(x) _byteswap_ulong(x);
#define bswap64(x) _byteswap_uint64(x);
#elif defined (__GNUC__)
#define bswap16(x) __builtin_bswap16(x);
#define bswap32(x) __builtin_bswap32(x);
#define bswap64(x) __builtin_bswap64(x);
#endif

#if __BYTE_ORDER==__LITTLE_ENDIAN
template <typename T>
inline T load_le(void *p) {
  return *(T *)(p);
}
template <typename T>
inline void store_le(void *p, T v) {
  *(T *)(p) = v;
}

#pragma warning( push )
#pragma warning( disable : 4739 )
// turn off warning: reference to variable 't' exceeds its storage space

template <typename T>
inline T load_be(void *p) {
  if (sizeof(T) == 2) {
    uint16_t t = bswap16(*(uint16_t *)p);
    return *(T *)(&t);
  }
  if (sizeof(T) == 4) {
    uint32_t t = bswap32(*(uint32_t *)p);
    return *(T *)(&t);
  }
  if (sizeof(T) == 8) {
    uint64_t t = bswap64(*(uint64_t *)p);
    return *(T *)(&t);
  }
}
template <typename T>
inline void store_be(void *p, T v) {
  if (sizeof(T) == 2) {
    uint16_t t;
    *(T *)&t = v;
    *(T *)(p) = bswap16(t);
  }
  if (sizeof(T) == 4) {
    uint32_t t;
    *(T *)&t = v;
    *(T *)(p) = bswap32(t);
  }
  if (sizeof(T) == 8) {
    uint64_t t;
    *(T *)&t = v;
    *(T *)(p) = bswap64(t);
  }
}

#elif __BYTE_ORDER==__BIG_ENDIAN
template <typename T>
inline T load_be(void *p) {
  return *(T *)(p);
}
template <typename T>
inline void store_be(void *p, T v) {
  *(T *)(p) = v;
}

template <typename T>
inline T load_le(void *p) {
  if (sizeof(T) == 2) {
    uint16_t t = bswap16(*(uint16_t *)p);
    return *(T *)(&t);
  }
  if (sizeof(T) == 4) {
    uint32_t t = bswap32(*(uint32_t *)p);
    return *(T *)(&t);
  }
  if (sizeof(T) == 8) {
    uint64_t t = bswap64(*(uint64_t *)p);
    return *(T *)(&t);
  }
}
template <typename T>
inline void store_le(void *p, T v) {
  if (sizeof(T) == 2) {
    uint16_t t;
    *(T *)&t = v;
    *(T *)(p) = bswap16(t);
  }
  if (sizeof(T) == 4) {
    uint32_t t;
    *(T *)&t = v;
    *(T *)(p) = bswap32(t);
  }
  if (sizeof(T) == 8) {
    uint64_t t;
    *(T *)&t = v;
    *(T *)(p) = bswap64(t);
  }
}
#error cannot determine machine endianness
#endif

#pragma warning( pop )

template <typename T>
inline T load_e(void *p, bool is_little_endian) {
  return (is_little_endian ? load_le<T>(p) : load_be<T>(p));
}
template <typename T>
inline void store_e(void *p, T v, bool is_little_endian) {
  if (is_little_endian)
    store_le<T>(p, v);
  else
    store_be<T>(p, v);
}

/* pointer to value macro --------------------------------------------------- */

#define UINT8(p)    (*(uint8_t *)(p))
#define UINT16(p)   (*(uint16_t *)(p))
#define UINT32(p)   (*(uint32_t *)(p))
#define UINT64(p)   (*(uint64_t *)(p))
#define INT8(p)     (*(uint8_t *)(p))
#define INT16(p)    (*(uint16_t *)(p))
#define INT32(p)    (*(uint32_t *)(p))
#define INT64(p)    (*(uint64_t *)(p))

// #define GET_UINT16LE(p)     (*((uint16_t *)(p)))
// #define GET_UINT32LE(p)     (*((uint32_t *)(p)))
// #define GET_INT16LE(p)      (*((int16_t *)(p)))
// #define GET_INT32LE(p)      (*((int32_t *)(p)))

// #define GET_UINT32BE(x)   (((*(uint32_t *)(x) & 0xff) << 24) + \
//                            ((*(uint32_t *)(x) & 0xff00) << 8) + \
//                            ((*(uint32_t *)(x) & 0xff0000) >> 8) + \
//                            ((*(uint32_t *)(x) & 0xff000000) >> 24) )
// #define GET_UINT16BE(x)   (((*(uint16_t *)(x) & 0xff00) >> 8) +\
//                            ((*(uint16_t *)(x) & 0xff) << 8))

// #define GET_INT32BE(x)  int32_t(((*(uint32_t *)(x) & 0xff) << 24) + \
//                                 ((*(uint32_t *)(x) & 0xff00) << 8) + \
//                                 ((*(uint32_t *)(x) & 0xff0000) >> 8) + \
//                                 ((*(uint32_t *)(x) & 0xff000000) >> 24))
// #define GET_INT16BE(x)  int16_t(((*(uint16_t *)(x) & 0xff00) >>8) + \
//                                 ((*(uint16_t *)(x) & 0xff) <<8))

#define PUT_UINT32LE(p, v)  ((*(uint32_t *)(p) = uint32_t(v)))
#define PUT_UINT16LE(p, v)  ((*(uint16_t *)(p) = uint16_t(v)))
#define PUT_INT32LE(p, v)   ((*(int32_t *)(p) = int32_t(v)))
#define PUT_INT16LE(p, v)   ((*(int16_t *)(p) = int16_t(v)))

#define PUT_UINT32BE(p, v)  (*(uint32_t *)(p) = \
				((uint32_t(v)&0xff) << 24) + ((uint32_t(v)&0xff00) << 8) + \
				((uint32_t(v)&0xff0000) >> 8) + ((uint32_t(v)&0xff000000) >> 24))
#define PUT_UINT16BE(p, v) (*(uint16_t *)(p) = \
				((uint16_t(v)&0xff)<<8) + ((uint32_t(v)&0xff00) >> 8))
#define PUT_INT32BE(p, v) (*(int32_t *)(p) = \
				((int32_t(v)&0xff) << 24) + ((int32_t(v)&0xff00) << 8) + \
				((int32_t(v)&0xff0000) >> 8) + ((int32_t(v)&0xff000000) >> 24))
#define PUT_INT16BE(p, v) (*(int16_t *)(p) = \
				((int16_t(v)&0xff)<<8) + ((int32_t(v)&0xff00) >> 8))

#define GET_UINT16E(p,ISLITTLEENDIAN) ((ISLITTLEENDIAN)?GET_UINT16LE(p):GET_UINT16BE(p))
#define GET_UINT32E(p,ISLITTLEENDIAN) ((ISLITTLEENDIAN)?GET_UINT32LE(p):GET_UINT32BE(p))

/* misc --------------------------------------------------------------------- */

#define MAKE_EVEN(x)        ((x)+((x)&1))

#ifdef _MSC_VER
#define snprintf _snprintf
#define strtok_r strtok_s
#endif

#ifdef _WIN32
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

}  // end of namespace dicom

#endif /* DICOMSDL_DICOMCFG_H_ */
