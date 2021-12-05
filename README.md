## DICOMSDL
[![Build status](https://ci.appveyor.com/api/projects/status/cbaefp3pyvie8ilp?svg=true)](https://ci.appveyor.com/project/tsangel/dicomsdl)

A fast and light-weighted DICOM software development library.

## Introduction

Digital Imaging and Communications in Medicine (DICOM) is a standard for managing informations in medical imaging developed by American College of Radiology (ACR) and National Electrical Manufacturers Association (NEMA). It defines a file format and a communication protocol over network.

DICOM Software Development Library (DICOM SDL) is a software developed libraries for easy and quick development of an application managing DICOM formatted files. DICOM SDL allows to make programs that read DICOM formatted files without in depth knowledge of DICOM.

DICOM SDL is intended to process a huge bunch of DICOM formatted image in high speed. For example, processing 1M+ dicom files for deep learning, speed really matters. To achieve the processing speed, DICOM SDL use an extension written in C++. DICOM SDL works much faster than pure DICOM implementation(see tutorials/timeit_test.ipynb). If your job is simple such as image extraction or get study date, DICOM SDL may be what you want.

DICOM SDL can

* read DICOM formatted files.
* read medical images in DICOM file, if file encodes in raw/jpeg/jpeg2000/RLE/JPEG-LS format.

DICOM SDL is especially optimized for reading lots of DICOM formatted files quickly, and would be very useful for scanning and processing huge numbers of DICOM files.

DICOM SDL cannot

* modify/write function (will be add.)
* send/receive DICOM over network.

## Install

To install on python,
```
$ pip install -U dicomsdl
```

To get code and compile,
```
$ git clone --recursive https://github.com/tsangel/dicomsdl
$ python setup.py install
```

DICOMSDL was successfully compiled and ran on following environments

* Microsoft Windows 7/10/11 (x86, x64)
* Linux x86/x64 (Ubuntu)
* MacOS 10.9+ (x86_64, arm64)
* Python 2.7, 3.5-3.10

If you have `ImportError` in Microsoft Windows, install [Microsoft Visual C++ Redistributable](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).

# Some examples

Please look ipynb files in tutorial folder for detailed explanations.

```python
>>> import dicomsdl as dicom

>>> dset = dicom.open("somefile.dcm")

>>> print (dset.Rows, dset.Columns)
512 512

>>> dset.getPixelDataInfo()
{'Rows': 512, 'Cols': 512, 'NumberOfFrames': 1, 'SamplesPerPixel': 1, 'PlanarConfiguration': None, 'BitsAllocated': 16, 'BytesAllocated': 2, 'BitsStored': 16, 'PixelRepresentation': True, 'dtype': 'h', 'PhotometricInterpretation': 'MONOCHROME2', 'WindowCenter': None, 'WindowWidth': None, 'RescaleIntercept': -1024.0, 'RescaleSlope': 1.0}

>>> dset.pixelData()
array([[-3024., -3024., -3024., ..., -3024., -3024., -3024.],
       [-3024., -3024., -3024., ..., -3024., -3024., -3024.],
       [-3024., -3024., -3024., ..., -3024., -3024., -3024.],
       ...,
       [-3024., -3024., -3024., ..., -3024., -3024., -3024.],
       [-3024., -3024., -3024., ..., -3024., -3024., -3024.],
       [-3024., -3024., -3024., ..., -3024., -3024., -3024.]],
      dtype=float32)
      
>>> dset.pixelData(storedvalue=True)
array([[-2000, -2000, -2000, ..., -2000, -2000, -2000],
       [-2000, -2000, -2000, ..., -2000, -2000, -2000],
       [-2000, -2000, -2000, ..., -2000, -2000, -2000],
       ...,
       [-2000, -2000, -2000, ..., -2000, -2000, -2000],
       [-2000, -2000, -2000, ..., -2000, -2000, -2000],
       [-2000, -2000, -2000, ..., -2000, -2000, -2000]], dtype=int16)

>>> dset.RescaleIntercept
-1024.0
>>> dset['RescaleIntercept']  # another way to get value
-1024.0
>>> dset[0x00281052]  # yet another way to get value
-1024.0
>>> dset.getDataElement('RescaleSlope')
<dicomsdl._dicomsdl.DataElement object at 0x10ff5d0f0>
>>> dset.getDataElement('RescaleIntercept').toDouble() # yet yet another
-1024.0
>>> dset.getDataElement('RescaleIntercept').value() # yet yet yet another
-1024.0

>>> print(dset['00540016.0.00180031'])  # from another example
FDG -- fluorodeoxyglucose

>>> print(dset.WindowCenter)  # return None if dicom file doesn't have value.
None

>>> dset.toPilImage().show()
 ... pop up image ...

>>> print(dset.dump())
TAG	VR	LEN	VM	OFFSET	KEYWORD
'00020000'	UL	4	1	0x8c	212	# FileMetaInformationGroupLength
'00020001'	OB	2	1	0x9c	'\x00\x01'	# FileMetaInformationVersion
'00020002'	UI	26	1	0xa6	'1.2.840.10008.5.1.4.1.1.2' = CT Image Storage	# MediaStorageSOPClassUID
...

 
```
