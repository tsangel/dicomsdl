/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * charls_codec.h
 */

#ifndef DICOMSDL_CHARLS_CODEC_H__
#define DICOMSDL_CHARLS_CODEC_H__

#include "dicom.h"
#include "imagecodec.h"

namespace dicom {  //-----------------------------------------------------------

extern "C" DICOMSDL_CODEC_RESULT charls_decoder(const char *tsuid, char *data,
                                                long datasize,
                                                imagecontainer *ic);

extern "C" DICOMSDL_CODEC_RESULT charls_encoder(
    const char *tsuid, imagecontainer *ic, char **data, long *datasize,
    free_memory_fnptr *free_memory_fn);

extern "C" void charls_codec_free_memory(char *data);

}  // namespace dicom ----------------------------------------------------------

#endif // DICOMSDL_CHARLS_CODEC_H__
