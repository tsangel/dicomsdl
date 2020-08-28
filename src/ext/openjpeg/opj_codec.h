/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * opj_codec.h
 */

#ifndef __OPJ_CODEC_H__
#define __OPJ_CODEC_H__

#include "dicom.h"
#include "imagecodec.h"

namespace dicom {  // ----------------------------------------------------------

extern "C" DICOMSDL_CODEC_RESULT opj_decoder(const char *tsuid, char *data,
                                             long datasize, imagecontainer *ic);

/*
 * jpeg2k encoder using openjpeg library
 *
 * acceptable arguments in ic->args are ...
 * 	rate=[ double value >= 1]
 * 	quality=[ 0 < double value <= 100% ]
 * 	layer=[ int value >= 1 ]
 * 	level=[ int value >= 1 ]
 * 	reversible=[ y|n ]
 *
 *	example) reversible=y
 *	example) rate=20;layer=3;level=5
 */
extern "C" DICOMSDL_CODEC_RESULT opj_encoder(const char *tsuid,
                                             imagecontainer *ic, char **data,
                                             long *datasize,
                                             free_memory_fnptr *free_memory_fn);

extern "C" void opj_codec_free_memory(char *data);

}  // namespace dicom ----------------------------------------------------------

#endif
