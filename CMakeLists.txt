#
# DICOM software development library (SDL)
# Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
# See copyright.txt for details.
#

if (WIN32)
	cmake_minimum_required (VERSION 3.4)
else (WIN32)
	cmake_minimum_required (VERSION 2.8)
endif(WIN32)

project (dicomsdl)

SET(CMAKE_BUILD_TYPE Release)

if (APPLE)
	SET(OSX_ARCHITECTURES "x86_64")
	SET(CMAKE_OSX_ARCHITECTURES "${OSX_ARCHITECTURES}")
endif (APPLE)

# ------------
# User options

OPTION(USE_OPENJPEG_CODEC
		"Use OpenJPEG to decode/encode JPEG2000 images." ON)
OPTION(USE_IJG_CODEC
		"Use IJG library to decode/encode JPEG images." ON)
OPTION(USE_ZLIB
		"Use zlib to decode/encode deflated explicit LE images." ON)
OPTION(USE_CHARLS_CODEC
		"Use CharLS to decode/encode JPEG-LS images." ON)

OPTION(USE_DEBUG_MESSAGE
		"Display debug messages" OFF)


OPTION(PYTHON_BUILD_EXT
		"Build python extension" ON)

OPTION(USE_SSE
		"Use __SSE__" ON)

ADD_SUBDIRECTORY(src)
