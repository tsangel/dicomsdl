

set(OPENJPEG_NAMESPACE "OPENJPEG")
set(OPENJPEG_STANDALONE 1)
set(OPENJPEG_LIBRARY_NAME openjp2)

set(OPENJPEG_VERSION_MAJOR 2)
set(OPENJPEG_VERSION_MINOR 5)
set(OPENJPEG_VERSION_BUILD 0)
set(OPENJPEG_VERSION
  "${OPENJPEG_VERSION_MAJOR}.${OPENJPEG_VERSION_MINOR}.${OPENJPEG_VERSION_BUILD}")
# set(PACKAGE_VERSION
#   "${OPENJPEG_VERSION_MAJOR}.${OPENJPEG_VERSION_MINOR}.${OPENJPEG_VERSION_BUILD}")
if(NOT OPENJPEG_SOVERSION)
  set(OPENJPEG_SOVERSION 7)
endif(NOT OPENJPEG_SOVERSION)
set(OPENJPEG_LIBRARY_PROPERTIES
  VERSION   "${OPENJPEG_VERSION_MAJOR}.${OPENJPEG_VERSION_MINOR}.${OPENJPEG_VERSION_BUILD}"
  SOVERSION "${OPENJPEG_SOVERSION}"
)
set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/cmake
    ${CMAKE_MODULE_PATH})
# if (NOT EMSCRIPTEN)
# include (${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
# TEST_BIG_ENDIAN(OPJ_BIG_ENDIAN)
# endif()
# if(CMAKE_COMPILER_IS_GNUCC)
# # For all builds, make sure openjpeg is std99 compliant:
# # set(CMAKE_C_FLAGS "-Wall -std=c99 ${CMAKE_C_FLAGS}") # FIXME: this setting prevented us from setting a coverage build.
# # Do not use ffast-math for all build, it would produce incorrect results, only set for release:
# set(OPENJPEG_LIBRARY_COMPILE_OPTIONS ${OPENJPEG_LIBRARY_COMPILE_OPTIONS} "$<$<CONFIG:Release>:-ffast-math>")
# set(OPENJP2_COMPILE_OPTIONS ${OPENJP2_COMPILE_OPTIONS} "$<$<CONFIG:Release>:-ffast-math>" -Wall -Wextra -Wconversion -Wunused-parameter -Wdeclaration-after-statement -Werror=declaration-after-statement)
# endif()
include(EnsureFileInclude)
# These files are mandatory
ensure_file_include("string.h"   HAVE_STRING_H YES)
ensure_file_include("memory.h"   HAVE_MEMORY_H YES)
ensure_file_include("stdlib.h"   HAVE_STDLIB_H YES)
ensure_file_include("stdio.h"    HAVE_STDIO_H  YES)
ensure_file_include("math.h"     HAVE_MATH_H   YES)
ensure_file_include("float.h"    HAVE_FLOAT_H  YES)
ensure_file_include("time.h"     HAVE_TIME_H   YES)
ensure_file_include("stdarg.h"   HAVE_STDARG_H YES)
ensure_file_include("ctype.h"    HAVE_CTYPE_H  YES)
ensure_file_include("assert.h"   HAVE_ASSERT_H YES)
# For the following files, we provide an alternative, they are not mandatory
ensure_file_include("stdint.h"   OPJ_HAVE_STDINT_H   NO)
ensure_file_include("inttypes.h" OPJ_HAVE_INTTYPES_H NO)
# why check this one ? for openjpip ?
include (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
CHECK_INCLUDE_FILE("strings.h"      HAVE_STRINGS_H)
CHECK_INCLUDE_FILE("sys/stat.h"     HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("sys/types.h"    HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE("unistd.h"       HAVE_UNISTD_H)
# Enable Large file support
macro(OPJ_TEST_LARGE_FILES VARIABLE)
    if(NOT DEFINED ${VARIABLE})

        # On most platforms it is probably overkill to first test the flags for 64-bit off_t,
        # and then separately fseeko. However, in the future we might have 128-bit filesystems
        # (ZFS), so it might be dangerous to indiscriminately set e.g. _FILE_OFFSET_BITS=64.

        message(STATUS "Checking for 64-bit off_t")

        # First check without any special flags
        try_compile(FILE64_OK "${PROJECT_BINARY_DIR}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/cmake/TestFileOffsetBits.c")
        if(FILE64_OK)
          message(STATUS "Checking for 64-bit off_t - present")
       	endif()

        if(NOT FILE64_OK)
            # Test with _FILE_OFFSET_BITS=64
            try_compile(FILE64_OK "${PROJECT_BINARY_DIR}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/cmake/TestFileOffsetBits.c"
                        COMPILE_DEFINITIONS "-D_FILE_OFFSET_BITS=64" )
            if(FILE64_OK)
                message(STATUS "Checking for 64-bit off_t - present with _FILE_OFFSET_BITS=64")
                set(_FILE_OFFSET_BITS 64)
            endif()
        endif()

        if(NOT FILE64_OK)
            # Test with _LARGE_FILES
            try_compile(FILE64_OK "${PROJECT_BINARY_DIR}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/cmake/TestFileOffsetBits.c"
                        COMPILE_DEFINITIONS "-D_LARGE_FILES" )
            if(FILE64_OK)
                message(STATUS "Checking for 64-bit off_t - present with _LARGE_FILES")
                set(_LARGE_FILES 1)
            endif()
        endif()
	
        if(NOT FILE64_OK)
            # Test with _LARGEFILE_SOURCE
            try_compile(FILE64_OK "${PROJECT_BINARY_DIR}"
                        "${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/cmake/TestFileOffsetBits.c"
                        COMPILE_DEFINITIONS "-D_LARGEFILE_SOURCE" )
            if(FILE64_OK)
                message(STATUS "Checking for 64-bit off_t - present with _LARGEFILE_SOURCE")
                set(_LARGEFILE_SOURCE 1)
            endif()
        endif()


        #if(NOT FILE64_OK)
        #    # now check for Windows stuff
        #    try_compile(FILE64_OK "${PROJECT_BINARY_DIR}"
        #                "${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/cmake/TestWindowsFSeek.c")
        #    if(FILE64_OK)
        #        message(STATUS "Checking for 64-bit off_t - present with _fseeki64")
        #        set(HAVE__FSEEKI64 1)
        #    endif()
        #endif()

        if(NOT FILE64_OK)
            message(STATUS "Checking for 64-bit off_t - not present")
        endif()

        set(_FILE_OFFSET_BITS ${_FILE_OFFSET_BITS} CACHE INTERNAL "Result of test for needed _FILE_OFFSET_BITS=64")
        set(_LARGE_FILES      ${_LARGE_FILES}      CACHE INTERNAL "Result of test for needed _LARGE_FILES")
        set(_LARGEFILE_SOURCE ${_LARGEFILE_SOURCE} CACHE INTERNAL "Result of test for needed _LARGEFILE_SOURCE")

        # Set the flags we might have determined to be required above
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/cmake/TestLargeFiles.c.cmake.in"
                       "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TestLargeFiles.c")

        message(STATUS "Checking for fseeko/ftello")

	    # Test if ftello/fseeko are	available
	    try_compile(FSEEKO_COMPILE_OK
	                "${PROJECT_BINARY_DIR}"
                    "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TestLargeFiles.c")
	
	    if(FSEEKO_COMPILE_OK)
            message(STATUS "Checking for fseeko/ftello - present")
        endif()

        if(NOT FSEEKO_COMPILE_OK)
                # glibc 2.2 needs _LARGEFILE_SOURCE for fseeko (but not for 64-bit off_t...)
                try_compile(FSEEKO_COMPILE_OK
                            "${PROJECT_BINARY_DIR}"
                            "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TestLargeFiles.c"
                            COMPILE_DEFINITIONS "-D_LARGEFILE_SOURCE" )

                if(FSEEKO_COMPILE_OK)
                    message(STATUS "Checking for fseeko/ftello - present with _LARGEFILE_SOURCE")
                    set(_LARGEFILE_SOURCE ${_LARGEFILE_SOURCE} CACHE INTERNAL "Result of test for needed _LARGEFILE_SOURCE")
                endif()
        endif()

	    if(FSEEKO_COMPILE_OK)
                set(OPJ_HAVE_FSEEKO ON CACHE INTERNAL "Result of test for fseeko/ftello")
        else()
                message(STATUS "Checking for fseeko/ftello - not found")
                set(OPJ_HAVE_FSEEKO OFF CACHE INTERNAL "Result of test for fseeko/ftello")
        endif()

	    if(FILE64_OK AND FSEEKO_COMPILE_OK)
                message(STATUS "Large File support - found")
                set(${VARIABLE} ON CACHE INTERNAL "Result of test for large file support")
        else()
                message(STATUS "Large File support - not found")
                set(${VARIABLE} OFF CACHE INTERNAL "Result of test for large file support")
        endif()

    endif()
endmacro()
OPJ_TEST_LARGE_FILES(OPJ_HAVE_LARGEFILES)
# Allocating Aligned Memory Blocks
include(CheckIncludeFiles)
check_include_files(malloc.h OPJ_HAVE_MALLOC_H)
include(CheckSymbolExists)
# _aligned_alloc https://msdn.microsoft.com/en-us/library/8z34s9c6.aspx
check_symbol_exists(_aligned_malloc malloc.h OPJ_HAVE__ALIGNED_MALLOC)
# posix_memalign (needs _POSIX_C_SOURCE >= 200112L on Linux)
set(CMAKE_REQUIRED_DEFINITIONS -D_POSIX_C_SOURCE=200112L)
check_symbol_exists(posix_memalign stdlib.h OPJ_HAVE_POSIX_MEMALIGN)
unset(CMAKE_REQUIRED_DEFINITIONS)
# memalign (obsolete)
check_symbol_exists(memalign malloc.h OPJ_HAVE_MEMALIGN)
configure_file(
 ${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/src/lib/openjp2/opj_config.h.cmake.in
 ${CMAKE_CURRENT_BINARY_DIR}/openjpeg.git/src/lib/openjp2/opj_config.h
 @ONLY
 )
 configure_file(
 ${CMAKE_CURRENT_SOURCE_DIR}/openjpeg.git/src/lib/openjp2/opj_config_private.h.cmake.in
 ${CMAKE_CURRENT_BINARY_DIR}/openjpeg.git/src/lib/openjp2/opj_config_private.h
 @ONLY
 )

# ------------------------------------------------------------------------------
SET (OPENJP2_SRC openjpeg.git/src/lib/openjp2)
SET (C_CXX_SOURCES
	opj_codec.cc
	${OPENJP2_SRC}/thread.c
	${OPENJP2_SRC}/thread.h
	${OPENJP2_SRC}/bio.c
	${OPENJP2_SRC}/bio.h
	${OPENJP2_SRC}/cio.c
	${OPENJP2_SRC}/cio.h
	${OPENJP2_SRC}/dwt.c
	${OPENJP2_SRC}/dwt.h
	${OPENJP2_SRC}/event.c
	${OPENJP2_SRC}/event.h
	${OPENJP2_SRC}/ht_dec.c
	${OPENJP2_SRC}/image.c
	${OPENJP2_SRC}/image.h
	${OPENJP2_SRC}/invert.c
	${OPENJP2_SRC}/invert.h
	${OPENJP2_SRC}/j2k.c
	${OPENJP2_SRC}/j2k.h
	${OPENJP2_SRC}/jp2.c
	${OPENJP2_SRC}/jp2.h
	${OPENJP2_SRC}/mct.c
	${OPENJP2_SRC}/mct.h
	${OPENJP2_SRC}/mqc.c
	${OPENJP2_SRC}/mqc.h
	${OPENJP2_SRC}/mqc_inl.h
	${OPENJP2_SRC}/openjpeg.c
	${OPENJP2_SRC}/openjpeg.h
	${OPENJP2_SRC}/opj_clock.c
	${OPENJP2_SRC}/opj_clock.h
	${OPENJP2_SRC}/pi.c
	${OPENJP2_SRC}/pi.h
	${OPENJP2_SRC}/t1.c
	${OPENJP2_SRC}/t1.h
	${OPENJP2_SRC}/t2.c
	${OPENJP2_SRC}/t2.h
	${OPENJP2_SRC}/tcd.c
	${OPENJP2_SRC}/tcd.h
	${OPENJP2_SRC}/tgt.c
	${OPENJP2_SRC}/tgt.h
	${OPENJP2_SRC}/function_list.c
	${OPENJP2_SRC}/function_list.h
	${OPENJP2_SRC}/opj_codec.h
	${OPENJP2_SRC}/opj_includes.h
	${OPENJP2_SRC}/opj_intmath.h
	${OPENJP2_SRC}/opj_malloc.c
	${OPENJP2_SRC}/opj_malloc.h
	${OPENJP2_SRC}/opj_stdint.h
	${OPENJP2_SRC}/sparse_array.c
	${OPENJP2_SRC}/sparse_array.h
)
