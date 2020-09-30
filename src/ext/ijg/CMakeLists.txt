cmake_minimum_required(VERSION 2.8)

MACRO (COPY_SOURCES _BITS_IN_JSAMPLE)
	SET (BITS_IN_JSAMPLE ${_BITS_IN_JSAMPLE})
	FOREACH (FN ${IJG_C_SOURCES})
		CONFIGURE_FILE(ljpeg-6b/${FN} ${BITS_IN_JSAMPLE}/${FN} COPYONLY)
		SET(C_CXX_SOURCES ${C_CXX_SOURCES} ${BITS_IN_JSAMPLE}/${FN}) 
	ENDFOREACH(FN)

	FOREACH (FN ${IJG_HEADERS})
		CONFIGURE_FILE(ljpeg-6b/${FN} ${BITS_IN_JSAMPLE}/${FN} COPYONLY) 
	ENDFOREACH(FN)
	
	CONFIGURE_FILE(jdatasrc.c.patch ${BITS_IN_JSAMPLE}/jdatasrc.c COPYONLY)
	CONFIGURE_FILE(jdatadst.c.patch ${BITS_IN_JSAMPLE}/jdatadst.c COPYONLY)
	
	CONFIGURE_FILE(jconfig.h.in ${BITS_IN_JSAMPLE}/jconfig.h)
	CONFIGURE_FILE(jmorecfg.h.in ${BITS_IN_JSAMPLE}/jmorecfg.h)
	CONFIGURE_FILE(jpeglib.h.in ${BITS_IN_JSAMPLE}/jpeglib.h)
ENDMACRO (COPY_SOURCES)

SET (IJG_C_SOURCES
	jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c
    jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c
    jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c
    jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c
    jdinput.c jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c
    jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c jfdctfst.c
    jfdctint.c jidctflt.c jidctfst.c jidctint.c jidctred.c jquant1.c
    jquant2.c jutils.c jmemmgr.c
    
	jcodec.c jcshuff.c jcdiffct.c jclhuff.c
	jclossy.c jclossls.c jdlossy.c jdlossls.c jdlhuff.c
	jdpred.c jdscale.c jddiffct.c jdshuff.c jcscale.c jcpred.c
    
    jmemnobs.c #jmemansi.c jmemname.c jmemdos.c jmemmac.c
)

SET (IJG_HEADERS
	cderror.h cdjpeg.h jchuff.h jdct.h jdhuff.h jerror.h jinclude.h
	jlossls.h jlossy.h jmemsys.h jmorecfg.h jpegint.h jpeglib.h
	jversion.h transupp.h
)

include(CheckIncludeFile)
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(stdlib.h HAVE_STDLIB_H)

SET (C_CXX_SOURCES )
COPY_SOURCES("8")
COPY_SOURCES("12")
COPY_SOURCES("16")

SET(C_CXX_SOURCES
	${C_CXX_SOURCES}
	ijg_codec8.cc
	ijg_codec12.cc
	ijg_codec16.cc
	ijg_codec.cc
)
