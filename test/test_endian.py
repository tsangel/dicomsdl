# -*- coding: utf-8 -*-
from __future__ import print_function
import os
import dicomsdl as dicom

os.chdir(os.path.dirname(os.path.abspath(__file__)))

def diff(s1, s2):
  line1 = [l.strip() for l in s1.splitlines()]  # test
  line2 = [l.strip() for l in s2.splitlines()]  # reference
  if len(line1) != len(line2):
    return "Number of dump lines are different %d != %d (reference)"%(len(line1), len(line2))
  lineno = 1
  for l1, l2 in zip(line1, line2):
    if l1 != l2:
      return "Line %d:\n  (this)   [%s]\n   (ref) !=[%s]"%(lineno, l1, l2)
    lineno += 1
  return None    

def test_using_dump_bigendian():
  dump = dicom.open_file('test_be.dcm').dump()
  dump_ref = open('test_be.dcm.dump.txt', 'r').read()
  result = diff(dump, dump_ref)
  if result != None:
    print(result)
    assert result == None

def test_using_dump_littleendian():
  dump = dicom.open_file('test_le.dcm').dump()
  dump_ref = open('test_le.dcm.dump.txt', 'r').read()
  result = diff(dump, dump_ref)
  if result != None:
    print(result)
    assert result == None
