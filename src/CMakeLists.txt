#
# DICOM software development library (SDL)
# Copyright (c) 2010-2017, Kim, Tae-Sung. All rights reserved.
# See copyright.txt for details.
#

if (WIN32)
	cmake_minimum_required (VERSION 3.4)
else (WIN32)
	cmake_minimum_required (VERSION 2.8)
endif(WIN32)

# ------------------------------------------------------------------------------
# Platform specfic settings

if (APPLE)
  set (CMAKE_MACOSX_RPATH ON)
endif(APPLE)

# ------------------------------------------------------------------------------
# Compiler flags

IF(MSVC)
	# Suppressed warnings:
	# C4250: 'class1' : inherits 'class2::member' via dominance
	# C4355: 'this' : used in base member initializer list
	# C4503: 'function' : decorated name length exceeded, name was truncated
	# C4800: 'type' : forcing value to bool 'true' or 'false'
	# C4996: 'function': was declared deprecated

	SET(CMAKE_CXX_FLAGS
		"${CMAKE_CXX_FLAGS} /Ox /wd4250 /wd4355 /wd4503 /wd4800 /wd4996 -DWIN32")
	SET(CMAKE_C_FLAGS
		"${CMAKE_C_FLAGS} /Ox -wd4996 -wd4819 -wd4018 -wd4101 -wd4013 -DWIN32 ")

	IF (USE_DEBUG_MESSAGE)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D DEBUG_MESSAGE")
	ENDIF (USE_DEBUG_MESSAGE)

	IF (BUILD_SHARED_LIBS)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _BUILD_DLL")
	ENDIF(BUILD_SHARED_LIBS)

	IF(USE_SSE)
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SSE__")
		SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__SSE__")
	ENDIF(USE_SSE)
ENDIF(MSVC)

IF(UNIX)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -fvisibility=hidden")

	IF (USE_DEBUG_MESSAGE)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG_MESSAGE")
	ENDIF (USE_DEBUG_MESSAGE)

	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-fPIC" HAVE_FPIC_FLAG)
	IF(HAVE_FPIC_FLAG)
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
		SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	ENDIF(HAVE_FPIC_FLAG)

	if(NOT APPLE)
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lpthread")
		SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ldl -lpthread")
	ENDIF(NOT APPLE)

	IF(USE_SSE)
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__SSE__")
		SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__SSE__")
	ENDIF(USE_SSE)
ENDIF(UNIX)

# ------------------------------------------------------------------------------
# setting C, CXX sources

MACRO (ADD_SOURCES SUBDIRNAME)
	SET (C_CXX_SOURCES )
	ADD_SUBDIRECTORY(${SUBDIRNAME})

	GET_DIRECTORY_PROPERTY(
		SUB_SOURCES DIRECTORY ${SUBDIRNAME}
		DEFINITION C_CXX_SOURCES
	)
	FOREACH (FN ${SUB_SOURCES})
		SET (C_CXX_SOURCES ${C_CXX_SOURCES} ${SUBDIRNAME}/${FN})
	ENDFOREACH(FN)

	GET_DIRECTORY_PROPERTY(
		EXT_SOURCES DIRECTORY ${SUBDIRNAME}
		DEFINITION EXT_C_CXX_SOURCES
	)
	FOREACH (FN ${EXT_SOURCES})
		STRING(REGEX REPLACE "\\\\" "/" FN ${FN})
		SET (C_CXX_SOURCES ${C_CXX_SOURCES} ${FN})
	ENDFOREACH(FN)

	GET_DIRECTORY_PROPERTY(
		ADD_CMAKE_CXX_FLAGS DIRECTORY ${SUBDIRNAME}
		DEFINITION ADD_CMAKE_CXX_FLAGS
	)
	FOREACH (FLAG ${ADD_CMAKE_CXX_FLAGS})
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
	ENDFOREACH(FLAG)

	GET_DIRECTORY_PROPERTY(
		ADD_CMAKE_C_FLAGS DIRECTORY ${SUBDIRNAME}
		DEFINITION ADD_CMAKE_C_FLAGS
	)
	FOREACH (FLAG ${ADD_CMAKE_C_FLAGS})
		SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
	ENDFOREACH(FLAG)
ENDMACRO (ADD_SOURCES)


# ------------------------------------------------------------------------------


include_directories("${PROJECT_SOURCE_DIR}/src/lib"
					"${PROJECT_SOURCE_DIR}/src/include"
					"${PROJECT_SOURCE_DIR}/src/ext"
					"${PROJECT_SOURCE_DIR}/src/ext/zlib"
					"${PROJECT_SOURCE_DIR}/src/ext/openjpeg"
					"${PROJECT_SOURCE_DIR}/src/ext/ijg"
					"${PROJECT_SOURCE_DIR}/src/ext/charls"
					"${CMAKE_BINARY_DIR}/src/ext/zlib"
					)


# ------------------------------------------------------------------------------
# external modules

# IJG library ----------------------------------------------------------

SET (DICOMSDL_LIBRARIES dicomsdl)

IF (USE_IJG_CODEC)
	ADD_SOURCES(ext/ijg)
	ADD_LIBRARY(dicomsdl_ijg ${C_CXX_SOURCES})
	SET (DICOMSDL_LIBRARIES ${DICOMSDL_LIBRARIES} dicomsdl_ijg)
ENDIF (USE_IJG_CODEC)


# OpenJpeg library ----------------------------------------------------------

IF (USE_OPENJPEG_CODEC)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOPJ_STATIC")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOPJ_STATIC")
	ADD_SOURCES(ext/openjpeg)
	ADD_LIBRARY(dicomsdl_openjpeg ${C_CXX_SOURCES})
	SET (DICOMSDL_LIBRARIES ${DICOMSDL_LIBRARIES} dicomsdl_openjpeg)
ENDIF (USE_OPENJPEG_CODEC)


# CharLS library ----------------------------------------------------------

IF (USE_OPENJPEG_CODEC)
	ADD_SOURCES(ext/charls)
	ADD_LIBRARY(dicomsdl_charls ${C_CXX_SOURCES})
	SET (DICOMSDL_LIBRARIES ${DICOMSDL_LIBRARIES} dicomsdl_charls)
ENDIF (USE_OPENJPEG_CODEC)


# zlib -------------------------------------------------------------------------

IF (USE_ZLIB)
	ADD_SOURCES(ext/zlib)
	ADD_LIBRARY(dicomsdl_zlib ${C_CXX_SOURCES})
	SET (DICOMSDL_LIBRARIES ${DICOMSDL_LIBRARIES} dicomsdl_zlib)
ENDIF (USE_ZLIB)


# ------------------------------------------------------------------------------
# Define library 'dicomsdl'

FILE(GLOB C_CXX_SOURCES "lib/*.cc")
FILE(GLOB C_CXX_SOURCES ${C_CXX_SOURCES} "lib/charset/*.cc")
SET (SOURCES ${SOURCES} ${C_CXX_SOURCES})
ADD_LIBRARY(dicomsdl ${SOURCES})


# ------------------------------------------------------------------------------
# Build wrapper

IF (PYTHON_BUILD_EXT)
	ADD_SUBDIRECTORY(python)
ENDIF (PYTHON_BUILD_EXT)


# ------------------------------------------------------------------------------
# Build examples

IF (PYTHON_BUILD_EXAMPLE)
	ADD_SUBDIRECTORY(example)
ENDIF (PYTHON_BUILD_EXAMPLE)
