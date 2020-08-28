/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * imagecodec.cc
 */

#include <sstream>
#include <stdio.h>
#include <string.h>
#include <list>

#include "dicom.h"
#include "imagecodec.h"
#include "util.h"

#include "rle_codec.h"
#include "ijg/ijg_codec.h"
#include "openjpeg/opj_codec.h"
#include "charls/charls_codec.h"
#include "imagecodec.h"

namespace dicom {  //------------------------------------------------------
extern "C" {

// codes to load and unload shared libraries -----------------------------

#if defined(_MSC_VER) // Microsoft compiler
#include <windows.h>
#elif defined(__GNUC__) // GNU compiler
#include <dlfcn.h>
#else
#error cannot define compiler
#endif

#include <string>

void* loadlib(const char *dllname) {
#if defined(_MSC_VER)
  return (void*) LoadLibrary(dllname);
#elif defined(__GNUC__)
  return dlopen(dllname, RTLD_LAZY);
#endif
}

void* getfunc(void *lib, const char *func) {
#if defined(_MSC_VER)
  return (void*) GetProcAddress((HINSTANCE) lib, func);
#elif defined(__GNUC__)
  return dlsym(lib,func);
#endif
}

int freelib(void *hdll) {
  // freelib return 0 on success
#if defined(_MSC_VER)
  return (FreeLibrary((HINSTANCE) hdll) ? 0 : -1);
  // FreeLibrary return non-zero value on success
#elif defined(__GNUC__)
  return (dlclose(hdll)?-1:0);
  // dlclose return 0 on success
#endif
}

#define ERROR_MSG_SIZE  1024

//////////////////
//////// TODO:::::: CHANGE TO LINKED LIST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
struct t_codec {
  std::string codec_name;
  void* codec_handle;
  encoder_fnptr encoder;
  decoder_fnptr decoder;
  char errmsg[ERROR_MSG_SIZE];

  t_codec() {
    codec_handle = NULL;
    encoder = NULL;
    decoder = NULL;
  }
  ;
  ~t_codec() {
    unload_codec();
  }

  DICOMSDL_CODEC_RESULT load_codec(const char *_codec_name,
                                   encoder_fnptr _encoder,
                                   decoder_fnptr _decoder) {
    if (_decoder || _encoder) {
      codec_name = _codec_name;
      codec_handle = NULL;
      decoder = _decoder;
      encoder = _encoder;
      LOG_DEBUG(
          "t_codec::load_codec(%s,{%p},{%p})", _codec_name, encoder, decoder);
      return DICOMSDL_CODEC_OK;
    }
    codec_handle = loadlib(_codec_name);

    decoder = NULL;
    encoder = NULL;
    if (!codec_handle) {
      snprintf(errmsg, ERROR_MSG_SIZE, "load_codec(): "
               "cannot load '%s'",
               codec_name.c_str());
      return DICOMSDL_CODEC_ERROR;
    }

    decoder = (decoder_fnptr) getfunc(codec_handle, "decode_pixeldata");
    encoder = (encoder_fnptr) getfunc(codec_handle, "encode_pixeldata");

    if (!decoder || !encoder) {
      freelib(codec_handle);
      snprintf(errmsg, ERROR_MSG_SIZE, "load_codec(): "
               "cannot GetProcAddress/dlsym from codec '%s'",
               codec_name.c_str());
      return DICOMSDL_CODEC_ERROR;
    }

    LOG_DEBUG("t_codec::load_codec(%s,{%p},{%p}):handle=%p", _codec_name,
              encoder, decoder, codec_handle);

    codec_name = _codec_name;
    return DICOMSDL_CODEC_OK;
  }

  int unload_codec() {
    LOG_DEBUG("t_codec::unload_codec():%s,%p", codec_name.c_str(),
              codec_handle);
    if (!codec_handle)
      return DICOMSDL_CODEC_OK;

    if (freelib(codec_handle) == 0) {
      codec_handle = NULL;
      return DICOMSDL_CODEC_OK;
    } else {
      snprintf(errmsg, ERROR_MSG_SIZE, "unload_codec():"
               "cannot unload codec");
      return DICOMSDL_CODEC_ERROR;
    }
  }
};

struct t_codec_registry {
  std::list<t_codec *> codecs;
  char errmsg[ERROR_MSG_SIZE];

  t_codec_registry() {
    load_codec("rle", rle_encoder, rle_decoder);
    load_codec("jpeg", ijg_encoder, ijg_decoder);
    load_codec("jpegls", charls_encoder, charls_decoder);
    load_codec("jpeg2000", opj_encoder, opj_decoder);
  }

  ~t_codec_registry() {
    unload_all_codec();
  }

  DICOMSDL_CODEC_RESULT load_codec(const char *codec_name,
                                   encoder_fnptr encoder,
                                   decoder_fnptr decoder) {
    t_codec *c = new t_codec();
    DICOMSDL_CODEC_RESULT ret = c->load_codec(codec_name, encoder, decoder);
    if (ret == DICOMSDL_CODEC_OK)
      codecs.push_back(c);
    else {
      snprintf(errmsg, ERROR_MSG_SIZE, "%s", c->errmsg);
      delete c;
    }
    return ret;
  }

  DICOMSDL_CODEC_RESULT unload_codec(char *codec_name) {
    std::list<t_codec *>::iterator it;

    for (it = codecs.begin(); it != codecs.end(); it++) {
      if ((*it)->codec_name == codec_name) {
        if ((*it)->codec_handle) {
          if ((*it)->unload_codec() == DICOMSDL_CODEC_OK) {
            delete *it;
            codecs.erase(it);
            return DICOMSDL_CODEC_OK;
          } else {
            snprintf(errmsg, ERROR_MSG_SIZE, "unload_codec():"
                     "cannot unload '%s'",
                     codec_name);
            return DICOMSDL_CODEC_ERROR;
          }
        }
        break;
      }
    }

    snprintf(errmsg, ERROR_MSG_SIZE, "unload_codec():"
             "codec '%s' have not been loaded",
             codec_name);
    return DICOMSDL_CODEC_ERROR;
  }

  void unload_all_codec() {
    std::list<t_codec *>::iterator it;

    for (it = codecs.begin(); it != codecs.end(); it++)
      delete (*it);
    codecs.clear();
  }

  DICOMSDL_CODEC_RESULT encode_pixeldata(const char *tsuid, imagecontainer *ic,
                                         char **data, long *datasize,
                                         free_memory_fnptr *free_memory_fn) {
    DICOMSDL_CODEC_RESULT ret = DICOMSDL_CODEC_NOTSUPPORTED;
    std::stringstream ss;
    char *dst = NULL;
    *data = dst;
    int dstlen = 0;
    *datasize = dstlen;

    for (auto rit = codecs.rbegin(); rit != codecs.rend(); rit++) {
      ret = (*rit)->encoder(tsuid, ic, data, datasize, free_memory_fn);
      if (ret == DICOMSDL_CODEC_NOTSUPPORTED)  // not supported; try next codec
        continue;
      break;
    }

    if (ret == DICOMSDL_CODEC_NOTSUPPORTED)  // not supported
        {
      snprintf(ic->info, ARGBUF_SIZE, "encode_pixeldata(...): "
               "no codec for '%s'",
               tsuid);
      return DICOMSDL_CODEC_ERROR;  // NO AVAILABLE CODEC
    } else if (ret == DICOMSDL_CODEC_ERROR)  // some error
        {
      // error string set in ic->info;
      return DICOMSDL_CODEC_ERROR;
    }

    // return DICOMSDL_CODEC_OK or DICOMSDL_CODEC_INFO or DICOMSDL_CODEC_WARN
    return ret;
  }

  DICOMSDL_CODEC_RESULT decode_pixeldata(const char *tsuid, char *data,
                                         long datasize, imagecontainer *ic) {
    DICOMSDL_CODEC_RESULT ret = DICOMSDL_CODEC_ERROR;

    for (auto rit = codecs.rbegin(); rit != codecs.rend(); rit++) {
      ret = (*rit)->decoder(tsuid, data, datasize, ic);
      if (ret == DICOMSDL_CODEC_NOTSUPPORTED)  // not supported; try next codec
        continue;
      break;
    }

    if (ret == DICOMSDL_CODEC_NOTSUPPORTED)  // not supported
        {
      snprintf(ic->info, ARGBUF_SIZE, "decode_pixeldata(...):"
               "no codec for '%s'",
               tsuid);
      return DICOMSDL_CODEC_ERROR;  // NO AVAILABLE CODEC
    } else if (ret == DICOMSDL_CODEC_ERROR)  // some error
        {
      // error string set in ic->info;
      return DICOMSDL_CODEC_ERROR;
    }

    // return DICOMSDL_CODEC_OK or DICOMSDL_CODEC_INFO or DICOMSDL_CODEC_WARN
    return ret;
  }
};

static t_codec_registry codec_registery;

DICOMSDL_CODEC_RESULT encode_pixeldata(const char *tsuid, imagecontainer *ic,
                                       char **data, long *datasize,
                                       free_memory_fnptr *free_memory_fn) {
  return codec_registery.encode_pixeldata(tsuid, ic, data, datasize,
                                          free_memory_fn);
}

DICOMSDL_CODEC_RESULT decode_pixeldata(const char *tsuid, char *data,
                                       long datasize, imagecontainer *ic) {
  return codec_registery.decode_pixeldata(tsuid, data, datasize, ic);
}

} // extern "C"

#if defined (_MSC_VER)
// cause error at LogLevel::ERROR
#undef ERROR
#endif

void load_codec(char *codec_filename) {
  DICOMSDL_CODEC_RESULT ret;
  ret = codec_registery.load_codec(codec_filename, NULL, NULL);
  if (ret != DICOMSDL_CODEC_OK)
    LOGERROR_AND_THROW("%s", codec_registery.errmsg);
}

void unload_codec(char *codec_filename) {
  DICOMSDL_CODEC_RESULT ret = codec_registery.unload_codec(codec_filename);
  if (ret != DICOMSDL_CODEC_OK)
    LOGERROR_AND_THROW("%s", codec_registery.errmsg);
}


}  // namespace dicom -----------------------------------------------------
