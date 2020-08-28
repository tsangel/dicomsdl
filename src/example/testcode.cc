#include <dicom.h>
#include <stdio.h>
#include <iostream>
int main(int argc, char **argv) {
  // dicom::set_loglevel(dicom::LogLevel::DEBUG);
  // auto dset = dicom::open_file("../mazda/I2571498.dcm", 0xffffffff);
  // auto vr = (*dset)[0x00080008].vr();
  // printf("%d", vr)

  auto dset = dicom::open_file(argv[1]);
  std::wcout << dset->dump();
}




// import _dicomsdl as dicom

// dicom.set_loglevel(dicom.LogLevel.DEBUG)

// # dset = dicom.DataSet()
// # de = dset.addDataElement(0x00010001, dicom.VR.SQ)
// # se = de.toSequence()
// # dsetlist = [se.addDataSet() for i in range(20)]

// dset = dicom.open_file("../mazda/I2571498.dcm", load_until=0xffffffff)

// # data = open("../mazda/I2571498.dcm", "rb").read()
// # dset = dicom.open_memory(data, copydata=True)
