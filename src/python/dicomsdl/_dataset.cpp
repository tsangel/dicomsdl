/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * _dataset.cc
 */

#include <pybind11/buffer_info.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

#include "_dicomsdl.h"
#include "dicom.h"

namespace dicom {

void _DataElement_setValue(DataElement &de, py::object &obj) {
  switch (de.vr()) {
    case VR::SS:
    case VR::US:
    case VR::SL:
    case VR::UL:
    case VR::SV:
    case VR::UV:
    case VR::IS:
      if (py::isinstance<py::list>(obj) || py::isinstance<py::tuple>(obj) ||
          py::isinstance<py::set>(obj)) {
        std::vector<long> vec;
        for (auto it : py::iterator(obj)) vec.push_back(py::cast<py::int_>(it));
        de.fromLongVector(vec);
      } else {
        de.fromLong(py::cast<py::int_>(obj));
      }
      break;
    case VR::FL:
    case VR::FD:
    case VR::DS:
      if (py::isinstance<py::list>(obj) || py::isinstance<py::tuple>(obj) ||
          py::isinstance<py::set>(obj)) {
        std::vector<double> vec;
        for (auto it : py::iterator(obj))
          vec.push_back(py::cast<py::float_>(it));
        de.fromDoubleVector(vec);
      } else {
        de.fromDouble(py::cast<py::float_>(obj));
      }
      break;
    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DA:
    case VR::DT:
    case VR::TM:
    case VR::UI:
    case VR::UR:
    case VR::LO:
    case VR::LT:
    case VR::PN:
    case VR::SH:
    case VR::ST:
    case VR::UC:
    case VR::UT:
      if (py::isinstance<py::list>(obj) || py::isinstance<py::tuple>(obj) ||
          py::isinstance<py::set>(obj)) {
        std::vector<std::wstring> vec;
        for (auto it : py::iterator(obj)) {
          if (!py::isinstance<py::str>(it))
            THROW_ERROR("setValue() need a list of string objects.")
          vec.push_back(py::cast<std::wstring>(it));
        }
        de.fromStringVector(vec);
      } else {
        if (!py::isinstance<py::str>(obj))
          THROW_ERROR("setValue() need a string object.")
        de.fromString(py::cast<std::wstring>(obj));
      }
      break;
    default:
      if (py::isinstance<py::str>(obj) || py::isinstance<py::bytes>(obj))
        de.fromBytes(py::cast<std::string>(obj));
      else
        THROW_ERROR("setValue() need a byte or string object.")
      break;
  }
}

py::object _DataElement_value(DataElement &de) {
  py::object o;
  switch (de.vr()) {
    case VR::SS:
    case VR::US:
    case VR::SL:
    case VR::UL:
    case VR::SV:
    case VR::UV:
    case VR::IS:
      if (de.vm() > 1)
        o = py::list(py::cast(de.toLongLongVector()));
      else
        o = py::cast(de.toLongLong());
      break;
    case VR::FL:
    case VR::FD:
    case VR::DS:
      if (de.vm() > 1)
        o = py::list(py::cast(de.toDoubleVector()));
      else
        o = py::cast(de.toDouble());
      break;
    case VR::AE:
    case VR::AS:
    case VR::CS:
    case VR::DA:
    case VR::DT:
    case VR::TM:
    case VR::UI:
    case VR::UR:
    case VR::LO:
    case VR::LT:
    case VR::PN:
    case VR::SH:
    case VR::ST:
    case VR::UC:
    case VR::UT:
      if (de.vm() > 1)
        o = py::cast(de.toStringVector());
      else
        o = py::cast(de.toString());
      break;
    default:
      o = py::bytes(de.toBytes());
  }
  return o;
}

}  // namespace dicom
