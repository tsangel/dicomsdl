#
# DICOM software development library (SDL)
# Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
# See copyright.txt for details.
#

# if cmake has a trouble with finding python executable,
#    specify PYTHON_EXECUTABLE for cmake.
# e.g. -DPYTHON_EXECUTABLE=/opt/python/cp36-cp36m/bin/python
# e.g. -DPYTHON_EXECUTABLE:FILEPATH=`which python`

FILE(GLOB CXX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
FILE(GLOB PY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.py")

set(DEPS
	${CXX_SOURCES}
	${PY_SOURCES}
#	${SETUP_PY}
	${DICOMSDL_LIBRARIES}
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/ext/pybind11 build)
pybind11_add_module(_dicomsdl _dicomsdl.cpp _dataset.cpp)
pybind11_add_module(_util _util.cpp)
target_link_libraries(_dicomsdl PRIVATE ${DICOMSDL_LIBRARIES})
target_link_libraries(_util PRIVATE ${DICOMSDL_LIBRARIES})
