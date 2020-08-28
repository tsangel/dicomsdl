mkdir 8
mkdir 12
mkdir 16

unzip ljpegsrc.v6b.zip

cp ljpeg-6b/jcapimin.c ljpeg-6b/jcapistd.c ljpeg-6b/jccoefct.c \
ljpeg-6b/jccolor.c ljpeg-6b/jcdctmgr.c ljpeg-6b/jchuff.c \
ljpeg-6b/jcinit.c ljpeg-6b/jcmainct.c ljpeg-6b/jcmarker.c \
ljpeg-6b/jcmaster.c ljpeg-6b/jcomapi.c ljpeg-6b/jcparam.c \
ljpeg-6b/jcphuff.c ljpeg-6b/jcprepct.c ljpeg-6b/jcsample.c \
ljpeg-6b/jctrans.c ljpeg-6b/jdapimin.c ljpeg-6b/jdapistd.c \
ljpeg-6b/jdatadst.c ljpeg-6b/jdatasrc.c ljpeg-6b/jdcoefct.c \
ljpeg-6b/jdcolor.c ljpeg-6b/jddctmgr.c ljpeg-6b/jdhuff.c \
ljpeg-6b/jdinput.c ljpeg-6b/jdmainct.c ljpeg-6b/jdmarker.c \
ljpeg-6b/jdmaster.c ljpeg-6b/jdmerge.c ljpeg-6b/jdphuff.c \
ljpeg-6b/jdpostct.c ljpeg-6b/jdsample.c ljpeg-6b/jdtrans.c \
ljpeg-6b/jerror.c ljpeg-6b/jfdctflt.c ljpeg-6b/jfdctfst.c \
ljpeg-6b/jfdctint.c ljpeg-6b/jidctflt.c ljpeg-6b/jidctfst.c \
ljpeg-6b/jidctint.c ljpeg-6b/jidctred.c ljpeg-6b/jquant1.c \
ljpeg-6b/jquant2.c ljpeg-6b/jutils.c ljpeg-6b/jmemmgr.c \
ljpeg-6b/jcodec.c ljpeg-6b/jcshuff.c ljpeg-6b/jcdiffct.c \
ljpeg-6b/jclhuff.c ljpeg-6b/jclossy.c ljpeg-6b/jclossls.c \
ljpeg-6b/jdlossy.c ljpeg-6b/jdlossls.c ljpeg-6b/jdlhuff.c \
ljpeg-6b/jdpred.c ljpeg-6b/jdscale.c ljpeg-6b/jddiffct.c \
ljpeg-6b/jdshuff.c ljpeg-6b/jcscale.c ljpeg-6b/jcpred.c \
ljpeg-6b/jmemnobs.c ljpeg-6b/cderror.h ljpeg-6b/cdjpeg.h \
ljpeg-6b/jchuff.h ljpeg-6b/jdct.h ljpeg-6b/jdhuff.h \
ljpeg-6b/jerror.h ljpeg-6b/jinclude.h ljpeg-6b/jlossls.h \
ljpeg-6b/jlossy.h ljpeg-6b/jmemsys.h ljpeg-6b/jmorecfg.h \
ljpeg-6b/jpegint.h ljpeg-6b/jpeglib.h ljpeg-6b/jversion.h 8

cp ljpeg-6b/jcapimin.c ljpeg-6b/jcapistd.c ljpeg-6b/jccoefct.c \
ljpeg-6b/jccolor.c ljpeg-6b/jcdctmgr.c ljpeg-6b/jchuff.c \
ljpeg-6b/jcinit.c ljpeg-6b/jcmainct.c ljpeg-6b/jcmarker.c \
ljpeg-6b/jcmaster.c ljpeg-6b/jcomapi.c ljpeg-6b/jcparam.c \
ljpeg-6b/jcphuff.c ljpeg-6b/jcprepct.c ljpeg-6b/jcsample.c \
ljpeg-6b/jctrans.c ljpeg-6b/jdapimin.c ljpeg-6b/jdapistd.c \
ljpeg-6b/jdatadst.c ljpeg-6b/jdatasrc.c ljpeg-6b/jdcoefct.c \
ljpeg-6b/jdcolor.c ljpeg-6b/jddctmgr.c ljpeg-6b/jdhuff.c \
ljpeg-6b/jdinput.c ljpeg-6b/jdmainct.c ljpeg-6b/jdmarker.c \
ljpeg-6b/jdmaster.c ljpeg-6b/jdmerge.c ljpeg-6b/jdphuff.c \
ljpeg-6b/jdpostct.c ljpeg-6b/jdsample.c ljpeg-6b/jdtrans.c \
ljpeg-6b/jerror.c ljpeg-6b/jfdctflt.c ljpeg-6b/jfdctfst.c \
ljpeg-6b/jfdctint.c ljpeg-6b/jidctflt.c ljpeg-6b/jidctfst.c \
ljpeg-6b/jidctint.c ljpeg-6b/jidctred.c ljpeg-6b/jquant1.c \
ljpeg-6b/jquant2.c ljpeg-6b/jutils.c ljpeg-6b/jmemmgr.c \
ljpeg-6b/jcodec.c ljpeg-6b/jcshuff.c ljpeg-6b/jcdiffct.c \
ljpeg-6b/jclhuff.c ljpeg-6b/jclossy.c ljpeg-6b/jclossls.c \
ljpeg-6b/jdlossy.c ljpeg-6b/jdlossls.c ljpeg-6b/jdlhuff.c \
ljpeg-6b/jdpred.c ljpeg-6b/jdscale.c ljpeg-6b/jddiffct.c \
ljpeg-6b/jdshuff.c ljpeg-6b/jcscale.c ljpeg-6b/jcpred.c \
ljpeg-6b/jmemnobs.c ljpeg-6b/cderror.h ljpeg-6b/cdjpeg.h \
ljpeg-6b/jchuff.h ljpeg-6b/jdct.h ljpeg-6b/jdhuff.h \
ljpeg-6b/jerror.h ljpeg-6b/jinclude.h ljpeg-6b/jlossls.h \
ljpeg-6b/jlossy.h ljpeg-6b/jmemsys.h ljpeg-6b/jmorecfg.h \
ljpeg-6b/jpegint.h ljpeg-6b/jpeglib.h ljpeg-6b/jversion.h 12

cp ljpeg-6b/jcapimin.c ljpeg-6b/jcapistd.c ljpeg-6b/jccoefct.c \
ljpeg-6b/jccolor.c ljpeg-6b/jcdctmgr.c ljpeg-6b/jchuff.c \
ljpeg-6b/jcinit.c ljpeg-6b/jcmainct.c ljpeg-6b/jcmarker.c \
ljpeg-6b/jcmaster.c ljpeg-6b/jcomapi.c ljpeg-6b/jcparam.c \
ljpeg-6b/jcphuff.c ljpeg-6b/jcprepct.c ljpeg-6b/jcsample.c \
ljpeg-6b/jctrans.c ljpeg-6b/jdapimin.c ljpeg-6b/jdapistd.c \
ljpeg-6b/jdatadst.c ljpeg-6b/jdatasrc.c ljpeg-6b/jdcoefct.c \
ljpeg-6b/jdcolor.c ljpeg-6b/jddctmgr.c ljpeg-6b/jdhuff.c \
ljpeg-6b/jdinput.c ljpeg-6b/jdmainct.c ljpeg-6b/jdmarker.c \
ljpeg-6b/jdmaster.c ljpeg-6b/jdmerge.c ljpeg-6b/jdphuff.c \
ljpeg-6b/jdpostct.c ljpeg-6b/jdsample.c ljpeg-6b/jdtrans.c \
ljpeg-6b/jerror.c ljpeg-6b/jfdctflt.c ljpeg-6b/jfdctfst.c \
ljpeg-6b/jfdctint.c ljpeg-6b/jidctflt.c ljpeg-6b/jidctfst.c \
ljpeg-6b/jidctint.c ljpeg-6b/jidctred.c ljpeg-6b/jquant1.c \
ljpeg-6b/jquant2.c ljpeg-6b/jutils.c ljpeg-6b/jmemmgr.c \
ljpeg-6b/jcodec.c ljpeg-6b/jcshuff.c ljpeg-6b/jcdiffct.c \
ljpeg-6b/jclhuff.c ljpeg-6b/jclossy.c ljpeg-6b/jclossls.c \
ljpeg-6b/jdlossy.c ljpeg-6b/jdlossls.c ljpeg-6b/jdlhuff.c \
ljpeg-6b/jdpred.c ljpeg-6b/jdscale.c ljpeg-6b/jddiffct.c \
ljpeg-6b/jdshuff.c ljpeg-6b/jcscale.c ljpeg-6b/jcpred.c \
ljpeg-6b/jmemnobs.c ljpeg-6b/cderror.h ljpeg-6b/cdjpeg.h \
ljpeg-6b/jchuff.h ljpeg-6b/jdct.h ljpeg-6b/jdhuff.h \
ljpeg-6b/jerror.h ljpeg-6b/jinclude.h ljpeg-6b/jlossls.h \
ljpeg-6b/jlossy.h ljpeg-6b/jmemsys.h ljpeg-6b/jmorecfg.h \
ljpeg-6b/jpegint.h ljpeg-6b/jpeglib.h ljpeg-6b/jversion.h 16

cp jdatasrc.c.patch 8/jdatasrc.c
cp jdatasrc.c.patch 12/jdatasrc.c
cp jdatasrc.c.patch 16/jdatasrc.c
cp jdatadst.c.patch 8/jdatadst.c
cp jdatadst.c.patch 12/jdatadst.c
cp jdatadst.c.patch 16/jdatadst.c
cp jconfig8.h 8/jconfig.h
cp jconfig12.h 12/jconfig.h
cp jconfig16.h 16/jconfig.h
cp jmorecfg8.h 8/jmorecfg.h
cp jmorecfg12.h 12/jmorecfg.h
cp jmorecfg16.h 16/jmorecfg.h

rm -rf ljpeg-6b

