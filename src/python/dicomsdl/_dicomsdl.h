/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * _dicomsdl.h
 */

#ifndef __DICOMSDL_H_
#define __DICOMSDL_H_

#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "dicom.h"

namespace dicom {

void _DataElement_setValue(DataElement &de, py::object &obj);
py::object _DataElement_value(DataElement &de);

}

#endif  // __DICOMSDL_H_