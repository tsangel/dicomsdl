# -*- coding: utf-8 -*-
from __future__ import print_function

import os
import glob
import numpy as np

import dicomsdl as dicom

BASEPATH = '../sample/nema/WG04/IMAGES'
fnlist = glob.glob(os.path.join(BASEPATH, '*', '*_*'))

def filename_to_sampletype(fn):
    # ''./IMAGES/REF/XA1_UNC' -> 'XA1', 'UNC'
    return os.path.split(fn)[-1].split('_')

sampletypes = set()
enctypes = set()
for fn in fnlist:
    typ, enctyp = filename_to_sampletype(fn)
    sampletypes.add(typ)
    enctypes.add(enctyp)
enctypes.remove('UNC')  # remove reference
enctypes = list(enctypes) # ['J2KR', 'J2KI', 'JPLY', 'JLSN', 'JLSL', 'JPLL', 'RLE']


# read reference images to `refs`
refs = dict()
fnlist = glob.glob(os.path.join(BASEPATH, '*', '*_UNC'))

for fn in fnlist:
    print('Reading', fn)
    typ, enctyp = filename_to_sampletype(fn)
    dset = dicom.open_file(fn)
    imarr = dset.pixelData()
    refs[typ] = imarr


# read test images
fnlist = [i for i in glob.glob(os.path.join(BASEPATH, '*', '*'))
          if not i.endswith('_UNC')]

for fn in fnlist:
  print('Reading', fn, end='\t')
  typ, enctyp = filename_to_sampletype(fn)
  dset = dicom.open_file(fn)
  imarr = dset.pixelData()
  print(np.abs(np.float32(refs[typ]) - np.float32(imarr)).max())
