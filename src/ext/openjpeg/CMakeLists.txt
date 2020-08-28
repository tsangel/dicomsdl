# ------------------------------------------------------------------------------
SET (C_CXX_SOURCES
	opj_codec.cc
	openjp2/bio.c
	openjp2/cio.c
	openjp2/dwt.c
	openjp2/event.c
	openjp2/function_list.c
	openjp2/image.c
	openjp2/invert.c
	openjp2/j2k.c
	openjp2/jp2.c
	openjp2/mct.c
	openjp2/mqc.c
	openjp2/openjpeg.c
	openjp2/opj_clock.c
	openjp2/opj_malloc.c
	openjp2/pi.c
	openjp2/raw.c
	openjp2/t1.c
	openjp2/t1_generate_luts.c
	openjp2/t2.c
	openjp2/tcd.c
	openjp2/tgt.c
)

if(BUILD_JPIP)
  add_definitions(-DUSE_JPIP)
  SET (
	${C_CXX_SOURCES}
	openjp2/cidx_manager.c
	openjp2/phix_manager.c
	openjp2/ppix_manager.c
	openjp2/thix_manager.c
	openjp2/tpix_manager.c
  )
endif()

CONFIGURE_FILE(opj_config.h openjp2/opj_config.h COPYONLY) 
CONFIGURE_FILE(opj_config_private.h openjp2/opj_config_private.h COPYONLY) 
