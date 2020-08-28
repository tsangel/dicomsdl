rm -rf libopenjpeg
tar zxvf v2.1.2.tar.gz



mv openjpeg-2.1.2/libopenjpeg .



rm -rf openjpeg-1.5.0
cp openjpeg_mangled.h libopenjpeg/openjpeg_mangled.h
cp openjpeg.h libopenjpeg/openjpeg.h
cp opj_config.h.in libopenjpeg/opj_config.h