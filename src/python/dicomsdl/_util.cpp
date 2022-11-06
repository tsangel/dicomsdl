/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * _util.cc
 */

#include <pybind11/buffer_info.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

#include "dicom.h"
#include "dicomutil.h"
using namespace dicom;

static bool is_contiguous(py::array a) {
  auto buf = a.request();
  int stride = buf.itemsize;
  for (int i = buf.ndim - 1; i >= 0; --i) {
    if (buf.strides[i] != stride) return false;
    stride *= buf.shape[i];
  }
  return true;
}

PYBIND11_MODULE(_util, m) {
  m.def(
    "_convert_to_uint8",
      [](py::array inarray, py::array outarray, float xmin, float xmax) {
        if (!outarray.writeable()) {
          py::pybind11_fail("out array is not writeable");
        }

        if (!is_contiguous(inarray)) {
          py::pybind11_fail("inarray is not contiguous");
        }
        if (!is_contiguous(outarray)) {
          py::pybind11_fail("outarray is not contiguous");
        }

        auto inbuf = inarray.request();
        auto outbuf = outarray.request();

        if (inbuf.ndim != 2) {
          py::pybind11_fail("inarray's dimension is not 2");
        }
        if (inbuf.ndim != 2) {
          py::pybind11_fail("outarray's dimension is not 2");
        }

        for (int i = 0; i < inbuf.ndim; i++) {
          if (inbuf.shape[i] != outbuf.shape[i]) {
            py::pybind11_fail("inarray and outarray's shape is different");
          }
        }

        if (py::isinstance<py::array_t<int16_t>>(inarray)) {
          convert_to_uint8<int16_t>(
              (int16_t *)inbuf.ptr, inbuf.shape[0], inbuf.shape[1],
              inbuf.shape[0] * inbuf.strides[0], inbuf.strides[0],
              (uint8_t *)outbuf.ptr, outbuf.shape[0] * outbuf.strides[0],
              outbuf.strides[0], xmin, xmax);
        } else if (py::isinstance<py::array_t<uint16_t>>(inarray)) {
          convert_to_uint8<uint16_t>(
              (uint16_t *)inbuf.ptr, inbuf.shape[0], inbuf.shape[1],
              inbuf.shape[0] * inbuf.strides[0], inbuf.strides[0],
              (uint8_t *)outbuf.ptr, outbuf.shape[0] * outbuf.strides[0],
              outbuf.strides[0], xmin, xmax);
        } else if (py::isinstance<py::array_t<float32_t>>(inarray)) {
          convert_to_uint8<float32_t>(
              (float32_t *)inbuf.ptr, inbuf.shape[0], inbuf.shape[1],
              inbuf.shape[0] * inbuf.strides[0], inbuf.strides[0],
              (uint8_t *)outbuf.ptr, outbuf.shape[0] * outbuf.strides[0],
              outbuf.strides[0], xmin, xmax);
        } else if (py::isinstance<py::array_t<uint8_t>>(inarray)) {
          convert_to_uint8<uint8_t>(
              (uint8_t *)inbuf.ptr, inbuf.shape[0], inbuf.shape[1],
              inbuf.shape[0] * inbuf.strides[0], inbuf.strides[0],
              (uint8_t *)outbuf.ptr, outbuf.shape[0] * outbuf.strides[0],
              outbuf.strides[0], xmin, xmax);
        } else {
          py::pybind11_fail(
              "only int16_t, uint16_t, and float32_t are supported");
        }
      },
      "Convert values in inarray into outarray with dtype uint8_t. `xmin` "
      "and "
      "`xmax` are used to scale intensity between 0..255. If `center` and "
      "`window` are specified, use it instead of `xmin` and `xmax`.");
}
