#
# DICOM software development library (SDL)
# Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
# See copyright.txt for details.
#

if (WIN32)
	cmake_minimum_required (VERSION 3.7)
else (WIN32)
	cmake_minimum_required (VERSION 2.8)
endif(WIN32)

project (dicomsdl)

SET(CMAKE_BUILD_TYPE Release)

if (APPLE)
	if ($ENV{OSX_ARCHITECTURES})
		if ($ENV{OSX_ARCHITECTURES} STREQUAL "universal2")
			SET(OSX_ARCHITECTURES "arm64;x86_64")
		elseif ($ENV{OSX_ARCHITECTURES} STREQUAL "arm64")
			SET(OSX_ARCHITECTURES "arm64")
		elseif ($ENV{OSX_ARCHITECTURES} STREQUAL "x86_64")
			SET(OSX_ARCHITECTURES "x86_64")
		else()
			message(FATAL_ERROR "OSX_ARCHITECTURES should be universal2 or arm64 or x86_64.")
		endif()
	else()
		SET(OSX_ARCHITECTURES ${CMAKE_SYSTEM_PROCESSOR})
	endif()
	message(STATUS "Build OSX_ARCHITECTURES == ${OSX_ARCHITECTURES}.")
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

OPTION(PYTHON_BUILD_EXAMPLE
		"Build c++ example" OFF)

OPTION(USE_SSE2
		"Use __SSE2__" ON)
OPTION(USE_AVX2
		"Use __AVX2__" OFF)

ADD_SUBDIRECTORY(src)
