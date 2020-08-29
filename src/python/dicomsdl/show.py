import sys
import dicomsdl as dicom

def show(args):
  if len(args) == 1:
    dcmfn = args[0]
    idx = 0
  elif len(args) > 1:
    dcmfn = args[0]
    idx = int(args[1])
    
  dset = dicom.open_file(dcmfn)
  info = dset.getPixelDataInfo()
  if idx >= info['NumberOfFrames']:
    print('File have %d frames.'%(info['NumberOfFrames']))
    return

  dset.to_pil_image(idx).show()

def main():
    show(sys.argv[1:])
    
if __name__ == "__main__":
    main()