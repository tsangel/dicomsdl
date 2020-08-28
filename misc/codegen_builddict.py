#-*- encoding: utf-8 -*-
'''Bulid data dictionary by parsing "DICOM PS3.6 2020c - Data Dictionary" in xml format.
The file can be retrived from http://dicom.nema.org/medical/dicom/current/source/docbook/part06/part06.xml
                           or http://dicom.nema.org/medical/dicom/
$ cd misc
$ python3 codegen_builddict.py [path_to_part06.xml]/part06.xml
'''

import os
import sys
import re
import datetime

from bs4 import BeautifulSoup

DICOMDICT_INC_FILENAME = '../src/lib/datadict.inc.cxx'
UIDCONST1_FILENAME = 'uidconst1.inc.cxx'
UIDCONST2_FILENAME = 'uidconst2.inc.cxx'
DICOM_H_FILENAME = '../src/include/dicom.h'
DICOMSDL_WRAPPER_FILENAME = '../src/python/_dicomsdl.cpp'
DATADICTIONARY_VERSION = 'DICOM PS3.6 2020c'

# utility functions ------------------------------------------------------------

def tidy(word):
    """Filter out non-printable characters such as unicode characters."""
    word = word.strip()
    word = ''.join([i for i in word if i.isprintable()])
    return word

def fnv1(data):
    """Fowler–Noll–Vo hash function; FNV-1 hash."""
    hash_ = 2166136261
    data = bytearray(data)
    for c in data:
        hash_ = ((hash_ * 16777619) & 0xffffffff) ^ c
    return hash_

def fnv1a(data):
    """Fowler–Noll–Vo hash function; FNV-1a hash."""
    hash_ = 2166136261
    data = bytearray(data)
    for c in data:
        hash_ = ((hash_ ^ c) * 16777619) & 0xffffffff
    return hash_

def uid2val(uid):
    """uid -> val"""
    val = list(map(int, uid.split('.')[-2:]))
    return val[0] * 0x100 + val[1]

def name2key(uid):
    """Convert UID name into c-variable compatible name."""
    uid = uid.upper().replace(' & ', 'AND')
    uid = uid.replace('(RETIRED)', '')
    uid = re.sub('[' + re.escape(',@/()') + ']', ' ', uid)
    uid = re.sub('[' + re.escape('-.') + ']', '', uid)
    uid = uid.split(':')[0]
    uid = uid.split('[')[0]
    uid = '_'.join(uid.split())
    # uid = 'UID_' + uid   # will use UID:: instead UID_
    uid = uid.replace('PROCESS_', 'PROCESS')
    uid = uid.replace('JPEG_2000', 'JPEG2000')
    uid = uid.replace('PART_', 'PART')  # Part 2 -> PART2
    uid = uid.replace('LEVEL_4', 'LEVEL4') # MPEG-4 AVC/H.264 BD-compatible High Profile / Level 4.1
    return uid


# ------------------------------------------------------------------------------

def build_uid_registry(soup):
    """Process table. 8-1"""

    fout = open(DICOMDICT_INC_FILENAME, 'a')
    fout2 = open(UIDCONST1_FILENAME, 'w')
    fout3 = open(UIDCONST2_FILENAME, 'w')
    enc = lambda s: s.encode('latin8', errors='ignore')

    # A Registry of DICOM Unique Identifiers (UIDs) (Normative)
    # Table A-1. UID Values

    table_a = soup.find('table', {"xml:id":"table_A-1"})
    table_a = table_a.find('tbody')

    # table_a = [t for t in soup.findAll(b'div', {'class':'table'})
    #            if t.find(b'a', {'id':'table_A-1'})]
    # assert len(table_a) == 1  # there is only one table with id = 'table_A-1'.
    # table_a = table_a[0].find('tbody')

    line_0 = [] # uid table
    line_1 = [] # hash table for UID type == Transfer Syntax
    line_2 = [] # hash table for UID type != Transfer Syntax
    line_3 = [] # for UID constant variable's name
    line_4 = [] # for UID constant variable's name for swig generated wrapper


    rows = [[tidy(i.text) for i in r.find_all('para')] for r in table_a.findAll('tr')]
    rows = [r for r in rows if r[0]]

    for i, row in enumerate(table_a.findAll('tr')):
        row = [tidy(i.text) for i in row.find_all('para')]

        line = '/* %4d */ "%s", "%s", "%s"' % (i, row[0], row[1], row[2])
        line_0.append(line)
        h = fnv1a(enc(row[0])) & 0xffff
        line = '/* %-32s */ 0x%04x, %4d' % (row[0], h, i)
        if row[2] == 'Transfer Syntax' or i == 0:
            line_1.append(line)
        if row[2] != 'Transfer Syntax' and i != 0:
            line_2.append(line)
        if row[2] == 'Transfer Syntax' or i == 0:
            line = '    %s = %i,' % (name2key(row[1]), i)
            line_3.append(line)
            line = '      .value("%s", UID::%s)' % (name2key(row[1]), name2key(row[1]))
            line_4.append(line)

    line_1.sort(key=lambda x: x[38:])
    line_2.sort(key=lambda x: x[38:])

    print('static const char* uid_registry[] = {', file=fout)
    print(',\n'.join(line_0), file=fout)
    print('''};

    static const int tsuid_hash_index[] = {''', file=fout)
    print(',\n'.join(line_1), file=fout)
    print('''};

    static const int uid_hash_index[] = {''', file=fout)
    print(',\n'.join(line_2), file=fout)
    print('};', file=fout)

    print('\n'.join(line_3), file=fout2)
    print('\n'.join(line_4), file=fout3)

    print('write to', DICOMDICT_INC_FILENAME)
    fout.close()
    print('write to', UIDCONST1_FILENAME)
    fout2.close()
    print('write to', UIDCONST2_FILENAME)
    fout3.close()

def build_elements_registry(soup):
    """Process tables 6-1, 7-1, 8-1"""

    datadict = []

    # Table 6-1. Registry of DICOM Data Elements
    table_6 = soup.find('chapter', {'xml:id':'chapter_6'}).find('tbody')
    rows = [[tidy(c.text) for c in r.find_all('td')]
            for r in table_6.find_all('tr')]
    datadict.extend(rows)

    # Table 7-1. Registry of DICOM File Meta Elements
    table_7 = soup.find('chapter', {'xml:id':'chapter_7'}).find('tbody')
    rows = [[tidy(c.text) for c in r.find_all('td')]
            for r in table_7.find_all('tr')]
    datadict.extend(rows)

    # Table 8-1. Registry of DICOM Directory Structuring Elements
    table_8 = soup.find('chapter', {'xml:id':'chapter_8'}).find('tbody')
    rows = [[tidy(c.text) for c in r.find_all('td')]
            for r in table_8.find_all('tr')]
    datadict.extend(rows)

    # Table 9-1. Registry of DICOM Dynamic RTP Payload Elements​
    table_9 = soup.find('chapter', {'xml:id':'chapter_9'}).find('tbody')
    rows = [[tidy(c.text) for c in r.find_all('td')]
            for r in table_9.find_all('tr')]
    datadict.extend(rows)

    datadict.sort(key=lambda r: r[0])
    datadict[:100]

    print('MERGE TABLE 1 DONE', file=sys.stderr)

    fout3 = open(DICOMDICT_INC_FILENAME, 'w')

    # Data dictionary with key 'Tag'

    lines = []
    lines_with_x = []
    tags = []
    keywords = []

    min_kw = 10000
    max_kw = 0

    for idx, row in enumerate(datadict):
        tag_s = row[0]
        tag = tag_s.replace('x', 'f')
        tag = int(tag[1:5], 16) * 0x10000 + int(tag[6:10], 16)
        name, keyword, vr, vr_s, vm, retired = \
            (row[1], row[2], row[3][:2], row[3], row[4], row[5] if len(row) > 5 else '')
        keyword = keyword.strip()

        vr = vr if (vr.strip() and vr != 'Se') else 'NONE'  # "See Note 2" for (FFFE,E0??)
        vr_s = vr_s if vr != 'NONE' else ''
        line = '/* %4d */ { "%s", "%s", "%s", VR::%s, "%s", "%s", "%s" }'%\
            (idx, tag_s, name, keyword, vr, vr_s, vm, retired)
        lines.append(line)
        line = ' /* %4d */ 0x%08x' % (idx, tag)
        tags.append(line)

        if keyword:
            line = '/* %-62s */ %d' % (keyword, idx)
            keywords.append(line)

        if 'x' in row[0]:
            line = ' /* %s */ %d' % (tag_s, idx)
            lines_with_x.append(line)

        if keyword:
            if len(keyword) > max_kw:
                max_kw = len(keyword)
            if len(keyword) < min_kw:
                min_kw = len(keyword)

    print('MERGE TABLE 2 DONE', file=sys.stderr)

    lines_with_x.sort()
    keywords.sort()

    print('C++ TABLE READY', file=sys.stderr)

    print('''/* This data dictionary is generated from 'part06.html' by 'codegen_builddict.py' at %s.
 * 'part06.xml' is available at 'http://dicom.nema.org/medical/dicom/current/source/docbook/part06/part06.xml'.
 */

const char* DATADICTIONARY_VERSION = "%s";
const int SIZE_ELEMENT_REGISTRY = %d;
const int SIZE_INDEX_TAGS_WITH_XX = %d;
const int SIZE_INDEX_KEYWORD = %d;

static const ElementRegistry element_registry[] = {
%s
};

static const tag_t tags_registry[] = {
%s
};

static const int tags_xx_index[] = {
%s
};

static const int keyword_index[] = {
%s
};

/* Length of longest keyword = %d bytes,
            shortest keyword = %d bytes. */

    ''' % (
        datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y"),
        DATADICTIONARY_VERSION,
        len(lines),
        len(lines_with_x),
        len(keywords),
        ',\n'.join(lines),
        ',\n'.join(tags),
        ',\n'.join(lines_with_x),
        ',\n'.join(keywords),
        min_kw, max_kw), file=fout3)

    print('C++ TABLE DONE', file=sys.stderr)

    print('write to', DICOMDICT_INC_FILENAME)
    fout3.close()


# main -------------------------------------------------------------------------

def insert_generated_code(filename, codelet, startword, endword):
    """Insert codelet into a file, between 'startword' and 'endword'"""
    fragment1 = []
    fragment2 = []
    fragment3 = []

    data = open(filename, 'r').read()
    open(filename+'.bak', 'w').write(data)
    lines = data.splitlines()

    frag = fragment1
    for line in lines:
        frag.append(line)
        if startword.lower() in line.lower():
            frag = fragment2
        elif endword.lower() in line.lower():
            frag = fragment3
            frag.append(fragment2.pop())

    fout = open(filename, 'w')
    print(('\n'.join(fragment1)), file=fout)
    print(codelet, file=fout, end='')
    print(('\n'.join(fragment3)), file=fout)
    fout.close()


def do_parse(part6filename):
    # part6filename = '/Users/tsangel/Documents/workspace.dev/dicomsdl.reference/part06.xml'

    """Bulid dictionary by Parse DICOM part 6 in html format."""
    # Read part6.html
    raw = open(part6filename, 'rb').read()
    soup = BeautifulSoup(raw, 'lxml')

    print('PARSE %s DONE'%part6filename, file=sys.stderr)

    # bulid_tables
    build_elements_registry(soup)
    build_uid_registry(soup)
    
    # place generated tsuid_t constants into dicom.h and dicomsdl.i.in
    codelet = open(UIDCONST1_FILENAME, 'r').read()
    assert(len(codelet) >= 2266)
    insert_generated_code(DICOM_H_FILENAME, codelet, '$$generated_uid', '$$end_uid')
    os.remove(UIDCONST1_FILENAME)
    
    codelet = open(UIDCONST2_FILENAME, 'r').read()
    assert(len(codelet) >= 4000)
    insert_generated_code(DICOMSDL_WRAPPER_FILENAME, codelet, '$$generated_uid', '$$end_uid')
    os.remove(UIDCONST2_FILENAME)

if __name__ == "__main__":
    do_parse(sys.argv[1])
