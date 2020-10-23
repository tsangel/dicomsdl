# encoding=utf-8

from sys import version_info
import os
import numpy as np

IS_PY3 = version_info.major >= 3
from ._dicomsdl import *
from . import util

__version__ = version


def __dataset__getitem__(self, tagstr):
  de = self.getDataElement(tagstr)
  vr = de.vr()
  if vr == VR.SQ:
    return de.toSequence()
  elif vr == VR.PIXSEQ:
    return de.toPixelSequence()
  elif vr == VR.NONE:
    return None
  else:
    return de.value()
DataSet.__getitem__ = __dataset__getitem__

def __dataset__setitem__(self, tag_vr, value):
  if isinstance(tag_vr, tuple):
    tag, vr = tag_vr
    if not isinstance(vr, VR.type):
      vr = VR.from_string(vr)
  else:
    tag = tag_vr
    vr = VR.NONE
  if vr in (VR.PIXSEQ, VR.SQ):
    raise AttributeError("Cannot set value of DataSet or Pixel Sequence")
  self.addDataElement(tag, vr).setValue(value)
DataSet.__setitem__ = __dataset__setitem__

def __dataset__getattr__(self, key):
    tag = TAG.from_keyword(key)
    if tag == 0xffffffff:
        raise AttributeError("'DataSet' object has no attribute '%s'"%(key))
    else:
        return __dataset__getitem__(self, key)
DataSet.__getattr__ = __dataset__getattr__

def __dataset__setattr__(self, key, value):
    tag = TAG.from_keyword(key)
    if tag == 0xffffffff:
        raise AttributeError("'DataSet' object has no attribute '%s'"%(key))
    else:
        return __dataset__setitem__(self, key, value)
DataSet.__setattr__ = __dataset__setattr__


def __dataset____dir__(self):
    """Return attributes of this DataSet with keywords of all DataElements.
    
    This enables word completions when user use 'tab' on an DataSet object
    under interactive console environment. 
    """
    return self.____dir__() +\
        [TAG.keyword(de.tag())
         for de in self
         if de.tag() & 0x10000 == 0 and de.tag() & 0xffff != 0]
DataSet.____dir__ = DataSet.__dir__ if IS_PY3 else lambda _: dir(DataSet)
DataSet.__dir__ = __dataset____dir__


def __dataset__pixelData__(self, index=0, storedvalue=False):
  """Returns pixel values in this DataSet.

  Args:
    index (int): pixelData return index'th image if dataset holds multiframe
                 data.
    storedvalue (bool): True for get stored values; pixel values before LUT
                        transformation using RescaleSlope and RescaleIntercept.

  Returns:
    Numpy array containing pixel values of `index`'th image if dataset holds
    multiframe data. If `storedvalue` is False, RescaleSlope and
    RescaleIntercept are applied to pixel values.
    ( pixel values = stored values * RescsaleSlope + RescaleIntercept )

  Example:
    >>> dset=dicom.open_file('some_CT_image_file')
    >>> dset.RescaleIntercept
    -1024.0
    >>> dset.pixelData(storedvalue=True).min()  # before LUT transformation
    -2000
    >>> dset.pixelData().min()  # after LUT transformation
    -3024.0

  """
  # https://stackoverflow.com/questions/44659924/returning-numpy-arrays-via-pybind11
  info = self.getPixelDataInfo()
  
  dtype = info['dtype']
  if info['PlanarConfiguration'] == 'RRRGGGBBB':
    shape = [3, info['Rows'], info['Cols']]
  elif info['PlanarConfiguration'] == 'RGBRGBRGB':
    shape = [info['Rows'], info['Cols'], 3]
  else:
    shape = [info['Rows'], info['Cols']]

  outarr = np.empty(shape, dtype=dtype)
  self.copyFrameData(index, outarr)

  check = lambda x: x[index] if isinstance(x, list) else x

  if storedvalue is False:
    intercept = check(info['RescaleIntercept'])
    intercept = intercept if intercept is not None else 0.0
    slope = check(info['RescaleSlope'])
    slope = slope if slope is not None else 1.0
    outarr = np.float32(outarr)
    outarr *= slope
    outarr += intercept

  return outarr
DataSet.pixelData = __dataset__pixelData__


def __dataset__to_pil_image(self, index=0):
  """Returns a pillow image.

  Args:
    index (int): pixelData return index'th image if dataset holds multiframe
                 image data.

  Example:
    >>> dset.to_pil_image().show()  # pop up system's default image viewer

  """
  from PIL import Image
  # https://stackoverflow.com/questions/44659924/returning-numpy-arrays-via-pybind11
  info = self.getPixelDataInfo()
  
  dtype = info['dtype']
  if info['PlanarConfiguration'] == 'RRRGGGBBB':
    shape = [3, info['Rows'], info['Cols']]
    assert False # Need Implementation
  elif info['PlanarConfiguration'] == 'RGBRGBRGB':
    shape = [info['Rows'], info['Cols'], 3]

  else:
    shape = [info['Rows'], info['Cols']]

  outarr = np.empty(shape, dtype=dtype)
  self.copyFrameData(index, outarr)

  if len(shape) == 3 and shape[-1] == 3:
    return Image.fromarray(outarr)

  check = lambda x: x[index] if isinstance(x, list) else x

  intercept = check(info['RescaleIntercept'])
  intercept = intercept if intercept is not None else 0.0
  slope = check(info['RescaleSlope'])
  slope = slope if slope is not None else 1.0
  c = check(info['WindowCenter'])
  w = check(info['WindowWidth'])

  if c is not None and w is not None:
    # xmin, xmax after applying rescale intensity
    xmin = c - 0.5 - (w - 1) * 0.5
    xmax = c - 0.5 + (w - 1) * 0.5

    # xmin, xmax before applying rescale intensity
    xmin = (xmin - intercept) / slope
    xmax = (xmax - intercept) / slope
  else:
    xmin = outarr.min()
    xmax = outarr.max()

  data8 = np.empty_like(outarr, dtype=np.uint8)
  util.convert_to_uint8(outarr, data8, xmin, xmax)

  if info['PhotometricInterpretation'] == 'MONOCHROME1':
    data8 = 255 - data8

  return Image.fromarray(data8)
DataSet.to_pil_image = __dataset__to_pil_image
DataSet.toPilImage = __dataset__to_pil_image

