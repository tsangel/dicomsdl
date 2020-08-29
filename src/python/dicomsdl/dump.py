import sys
import dicomsdl as dicom

def dump(dcmfn_list):
    dcmfn = None
    if isinstance(dcmfn_list, list) and len(dcmfn_list) == 1:
        dcmfn = dcmfn_list[0]
    elif not isinstance(dcmfn_list, list):
        dcmfn = dcmfn_list
    
    if dcmfn:
        dset=dicom.open_file(dcmfn)
        print(dset.dump())
        return
    
    for dcmfn in dcmfn_list:
        dset=dicom.open_file(dcmfn)
        for l in dset.dump().splitlines():
            print("%s:\t%s"%(dcmfn, l.strip()))

def main():
    dump(sys.argv[1:])
    
if __name__ == "__main__":
    main()