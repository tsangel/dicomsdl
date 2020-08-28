#
# DICOM software development library (SDL)
# Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
# See copyright.txt for details.
#

SET (EXAMPLE_SOURCES
    testcode
)

SET (PY_EXAMPLE_SOURCES
)

FOREACH (FN ${EXAMPLE_SOURCES})
    ADD_EXECUTABLE (${FN} ${FN}.cc)
    TARGET_LINK_LIBRARIES (${FN}
    	${DICOMSDL_LIBRARIES}
    )
ENDFOREACH (FN)

#INSTALL (TARGETS ${EXAMPLE_SOURCES}
#    RUNTIME DESTINATION bin
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib)

#INSTALL (FILES ${PY_EXAMPLE_SOURCES} DESTINATION bin)
