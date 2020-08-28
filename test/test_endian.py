# -*- coding: utf-8 -*-
from __future__ import print_function
import os
import dicomsdl as dicom

os.chdir(os.path.dirname(os.path.abspath(__file__)))

def diff(s1, s2):
  line1 = [l.strip() for l in s1.splitlines()]
  line2 = [l.strip() for l in s2.splitlines()]
  if len(line1) != len(line2):
    return "Number of dump lines are different %d != %d"%(len(line1), len(line2))
  lineno = 1
  for l1, l2 in zip(line1, line2):
    if l1 != l2:
      return "%d:\t  [%s]\n\t!=[%s]"%(lineno, l1, l2)
    lineno += 1
  return None    

def test_using_dump_bigendian():
  dump1 = dicom.open_file('test_be.dcm').dump()
  dump2 = open('test_be.dcm.dump.txt', 'r').read()
  result = diff(dump1, dump2)
  if result != None:
    print(result)
    assert result == None

def test_using_dump_littleendian():
  dump1 = dicom.open_file('test_le.dcm').dump()
  dump2 = open('test_le.dcm.dump.txt', 'r').read()
  result = diff(dump1, dump2)
  if result != None:
    print(result)
    assert result == None
