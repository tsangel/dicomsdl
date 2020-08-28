/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * ijg_codec.h
 */

#include "dicom.h"
#include "imagecodec.h"

#ifndef DICOMSDL_IJG_CODEC_H__
#define DICOMSDL_IJG_CODEC_H__

namespace dicom {  //------------------------------------------------------

extern "C" DICOMSDL_CODEC_RESULT ijg_decoder(const char *tsuid, char *data,
                                             long datasize,
                             imagecontainer *ic);
/*
 * jpeg encoder using ijg library
 *
 * acceptable arguments in ic->args are ...
 *  quality=[ 0 < int value <= 100% ]
 *
 *  example) 'quality=50'
 */
extern "C" DICOMSDL_CODEC_RESULT ijg_encoder(const char *tsuid,
                                             imagecontainer *ic, char **data,
                                             long *datasize,
                                             free_memory_fnptr *free_memory_fn);

extern "C" void ijg_codec_free_memory(char *data);

}  // namespace dicom ------------------------------------------------------

#endif // DICOMSDL_IJG_CODEC_H__
