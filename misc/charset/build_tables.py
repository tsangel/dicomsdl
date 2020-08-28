# encoding=utf-8

from bs4 import BeautifulSoup 

# ------------------------------------------------------------------------------

FORMAT16 = """
static const uint16_t {name}[{size}] = {{
{code}
}};"""

FORMAT8 = """
static const uint8_t {name}[{size}] = {{
{code}
}};"""

# ------------------------------------------------------------------------------

def read_table(filename, col_mb, col_uc):
    """ Read table from table files at http://www.unicode.org/Public/MAPPINGS/
        and pick colume for multibyte character and unicode.
    """
    filename = 'data/' + filename

    if filename.endswith('.gz'):
        import gzip
        data = gzip.open(filename, 'rb').read()
    else:
        data = open(filename, 'rb').read()

    lines = [i.strip() for i in data.splitlines()]
    lines = [i for i in lines if not i.startswith(b'#') and i]
    lines = [i.split() for i in lines]

    m2u = [(int(l[col_mb], 16), int(l[col_uc], 16)) for l in lines] 
    u2m = [(int(l[col_uc], 16), int(l[col_mb], 16)) for l in lines] 
    return dict(m2u), dict(u2m)

def read_gb18030_xml(filename):
    # read gb-18030-2000.xml file from
    # http://source.icu-project.org/repos/icu/data/trunk/charset/data/xml/
    if filename.endswith('.gz'):
        import gzip
        xml = gzip.open(filename, 'rb').read()
    else:
        xml = open(filename, 'rb').read()
    return BeautifulSoup(gzip.open(filename), "lxml")

# ------------------------------------------------------------------------------

class CCodeBuilder(object):
    def __init__(self, indent=4, margin=80):
        self.rows = []
        self.row = []
        self.indent = ' '*indent
        self.margin = margin
        self.nitems = 0

    def addRow(self, s):
        # add a row (such as comment)
        # ex) stmt.append("// page %d"%(pageno))
        self.flush()
        self.rows.append(s)

    def addItem(self, s):
        # add an item to current row
        # ex) stmt.addItem('%d,'%(number))
        tmp = ''.join(self.row)
        if len(self.indent + tmp + s) > self.margin:
            self.flush()
        self.row.append(s)
        self.nitems += 1

    def flush(self):
        # append all items in row to rows,
        # then empty row.
        if self.row:
            lastline = ''.join(self.row)
            self.rows.append(lastline)
            self.row = []

    def joinRows(self):
        # retuen all rows in one string
        self.flush()
        # remove trailing comma
        self.rows[-1] = self.rows[-1].rstrip()[:-1]
        return '\n'.join(
            self.indent + r for r in self.rows)

    def numberOfItems(self):
        return self.nitems
        
# -----------------------------------------------------------------------------

def ccode_bitsum_table():
    def popcount(i):
        c = 0
        while i:
            c += i & 1
            i //= 2
        return c

    code = CCodeBuilder()
    varname = 'bitsum_table'

    for i in range(256):
        code.addItem("%d,"%(popcount(i)))

    return FORMAT16.format(name=varname, size=code.numberOfItems(),
                         code=code.joinRows())



def generate_ccode_map_unicode_to_multibyte(u2m, loc_name):
    """ Build a table for converting unicode to index """
    result = []

    code = CCodeBuilder()
    varname = 'map_%s_unicode_to_index'%(loc_name)

    # number of used codes in each page
    num_codes_in_page = [0]*256
    for u in u2m.keys():
        num_codes_in_page[u >> 8] += 1

    pagesize = 16 * 2  # 32 words per page

    # 1 if one or more codes are assigned in the page.
    # 0 if no code exists in the page.
    code_exist_in_page = [1 if num_codes_in_page[i] else 0 for i in range(256)]

    # start of builing table
    code.addRow('// Offsets to page table (256 words)')
    for i in range(256):
        offset = sum(code_exist_in_page[:i]) * pagesize + 256
        code.addItem('%d,'%(offset if code_exist_in_page[i] else 0))

    code.flush()

    offset = 256  # offset for table unicode_to_index
    index = 0  # index for table index_to_multibyte
    for page in range(256):
        # skip page without code.
        if not code_exist_in_page[page]:
            continue
        code.addRow('// page %d (offset %d-)'%(page, offset))
        # iteraging ??0? ~ ??F?
        for nib in range(16):
            uc0 = (page << 8) + (nib << 4)
            uc1 = uc0 + 16
            code.addItem('%d,'%(index))
            # set use bits
            usebit = 0
            for i in range(uc0, uc1):
                usebit *= 2
                if i in u2m:
                    usebit += 1
                    index += 1
            code.addItem('0x%x,'%(usebit))
            offset += 2

    result.append(FORMAT16.format(name=varname, size=code.numberOfItems(),
                                code=code.joinRows()))

    """ Build a table for converting index into multibytes """
    code = CCodeBuilder()
    varname = 'map_%s_index_to_multibyte'%(loc_name)

    unicodes = sorted(u2m.keys())
    for i, u in enumerate(unicodes):
        if (i % 100 == 0):
            code.addRow('// %d-'%(i))
        code.addItem('0x%04x,'%(u2m[u]))

    result.append(FORMAT16.format(name=varname, size=code.numberOfItems(),
                                code=code.joinRows()))

    # bitsum table
    result.append(r"""
#ifndef BITSUM_TABLE
#define BITSUM_TABLE
    """)
    result.append(ccode_bitsum_table())
    result.append("""#endif // BITSUM_TABLE""")

    return '\n'.join(result)

def generate_ccode_map_multibyte_to_unicode(m2u, loc_name):
    result = []

    code = CCodeBuilder()
    varname = 'map_%s_multibyte_to_unicode'%(loc_name)

    pagelist = set(mb // 256 for mb in m2u.keys())

    code.addRow('// Offset to each page table (94 words, 0x21 ~ 0x7e)')

    # 94 words of offset to each page table
    # -- 0 for non-existing page
    offset = 94 + 94
    for page in range(0x21, 0x7e+1):
        if page in pagelist:
            code.addItem('%d,'%offset)
            offset +=  94  # 94 words per each page
        else:
            code.addItem('94,')

    # generate items for non-existing page
    code.addRow('// A page for illegal characters')
    for c in range(0x21, 0x7e+1):
        code.addItem('0,')

    # generate items for each page
    for page in sorted(pagelist):
        code.addRow('// page 0x%02x'%(page))
        for c in range(0x21, 0x7e+1):
            mb = page * 256 + c
            if mb in m2u:
                code.addItem('0x%04x,'%m2u[mb])
            else:
                code.addItem('     0,')
    
    result.append(FORMAT16.format(name=varname, size=code.numberOfItems(),
                  code=code.joinRows()))

    return '\n'.join(result)


# ------------------------------------------------------------------------------

def to_18030_codepoint(b='', m=0):
    """ Convert 4 bytes sequence to an GB-18030 code point.
    
    Args:
      b: 4 hexadecimal number in string form of "xx xx xx xx"
      m: 32bit intger
    Returns
      18030 code points starts from 0
    """

    if m != 0:
        b1 = (m >> 24)
        b2 = (m >> 16) & 0xff
        b3 = (m >> 8) & 0xff
        b4 = (m) & 0xff
    elif b != '':
        b = b.split()
        b1 = int(b[0], 16)
        b2 = int(b[1], 16)
        b3 = int(b[2], 16)
        b4 = int(b[3], 16)

    if b2 < 0x30 or b2 > 0x39 or b3 < 0x81 or b3 > 0xfe or b4 < 0x30 or b4 > 0x39:
        raise RuntimeError('%02x %02x %02x %02x is not in range' % (b1, b2, b3, b4))

    if b1 >= 0x90 and b1 <= 0xe3:
        u = (b4 - 0x30) + (b3 - 0x81) * 10 + (b2 - 0x30) * 1260 + (b1 - 0x90) * 12600 + 0x10000
    elif b1 >= 0x81 and b1 <= 0x84:
        u = (b4 - 0x30) + (b3 - 0x81) * 10 + (b2 - 0x30) * 1260 + (b1 - 0x81) * 12600
    else:
        raise RuntimeError('%02x %02x %02x %02x is not in range' % (b1, b2, b3, b4))

    return u


def offset_table_from_xml(soup):
    """Generate offset table to translate codes points
    between unicode and "4 byte sequence" or vice versa,
    using 'gb-18030-2000.xml' from 'icu-project'.
    
    Returns: (offset, first unicode, last unicode,
                      first 18030 code point, last 18030 code point)
    
    ref) https://en.wikipedia.org/wiki/GB_18030
    ref) http://source.icu-project.org/repos/icu/data/trunk/charset/data/xml/gb-18030-2000.xml
    """

    offset_table = []
    
    # one to one mapping
    # <a u="FF00" b="84 31 95 34"/>
    mapping = [a for a in soup.findAll('a') if len(a['b']) > 8]

    offsets = dict()
    for a in mapping:
        u = int(a['u'], 16)
        b = to_18030_codepoint(a['b'])
        off = u - b
        if off not in offsets:
            offsets[off] = []
        offsets[off].append((u, b))

    for d in sorted(offsets.keys()):
        u = [i[0] for i in offsets[d]]
        b = [i[1] for i in offsets[d]]

        # Assure that numbers in unicode and multibyte with same offset
        # are sequencial without gap.
        assert u[-1] - u[0] == b[-1] - b[0]
        assert u[-1] - u[0] + 1 == len(u)
        offset_table.append((d, u[0], u[-1], b[0], b[-1]))

    # ranged mapping
    # <range uFirst="0452" uLast="200F"
    #        bFirst="81 30 D3 30" bLast="81 36 A5 31"  
    #        bMin="81 30 81 30" bMax="FE 39 FE 39"/>
    mapping = [r for r in soup.findAll('range')]
    
    for r in mapping:
        ufirst = int(r['ufirst'], 16)
        ulast = int(r['ulast'], 16)
        bfirst = to_18030_codepoint(r['bfirst'])
        blast = to_18030_codepoint(r['blast'])
        assert ulast - blast == ufirst - bfirst
        offset_table.append((ulast - blast, ufirst, ulast, bfirst, blast))
    
    # sort offset table in ascending order
    offset_table.sort(key=lambda x:x[1])
    return offset_table[:-1] # except last item (0x10000,0x10ffff)


def generate_ccode_offset_table(soup):
    """ Generate C code fragments for static array data. """   
    offset_table = offset_table_from_xml(soup)
    
    result = []

    # gb18030 code points
    code = CCodeBuilder()
    code.addRow('// Ranges of gb18030 code point in 4 bytes.')
    code.addRow('// Code points for bytes sequence with 81-fe | 30-39 | 81-fe | 30-39')
    varname = 'gb18030_codepoint_range'
    for o in offset_table:
        code.addItem('%#x,%#x, '%(o[3], o[4]))
        if code.numberOfItems() % 5 == 0: # line break
            code.flush()
    result.append(FORMAT16.format(name=varname, size=code.numberOfItems()*2,
                                code=code.joinRows()))

    # unicode
    code = CCodeBuilder()
    varname = 'gb18030_unicode_range'
    code.addRow('// Ranges of unicodes.')
    for o in offset_table:
        code.addItem('%#x,%#x, ' % (o[1], o[2]))
        if code.numberOfItems() % 5 == 0: # line break
            code.flush()
    result.append(FORMAT16.format(name=varname, size=code.numberOfItems() * 2,
                                code=code.joinRows()))
   
    return '\n'.join(result)
   

# ------------------------------------------------------------------------------

def translate_map_from_xml(soup):
    """Generate a translate map between unicode and "2 byte sequence" or vice versa,
    using 'gb-18030-2000.xml' from 'icu-project'.
    
    Returns: dict(multibyte -> unicode), dict(unicode -> multibyte)
    
    ref) https://en.wikipedia.org/wiki/GB_18030
    ref) http://source.icu-project.org/repos/icu/data/trunk/charset/data/xml/gb-18030-2000.xml
    """

    int16 = lambda x: int(x[0], 16) * 256 + int(x[1], 16) if len(x) == 2 else int(x[0], 16)
    mapping = [(int(a['u'], 16),
                int16(a['b'].split()))
               for a in soup.findAll('a') if len(a['b']) < 8]
    return dict((i[1],i[0]) for i in mapping), dict((i[0],i[1]) for i in mapping)

def generate_ccode_map_gb18030_to_unicode(soup):
    """ Generate C code fragments for static array data. """

    m2u, _ = translate_map_from_xml(soup)
    result = []
    code = CCodeBuilder()
    varname = 'map_gb18030_to_unicode'

    for b1 in range(0x81, 0xfe+1):
        code.addRow('// %#x40-%#x7e, %#x80-%#xfe'%(b1, b1, b1, b1))
        for b2 in range(0x40, 0xfe+1):
            if b2 == 0x7f:
                code.flush()
                continue
            code.addItem('%#x,'%m2u[b1*256+b2])
        code.flush()

    result.append(FORMAT16.format(name=varname, size=code.numberOfItems(),
                                code=code.joinRows()))

    return '\n'.join(result)

# ------------------------------------------------------------------------------


def generate_ccode_map_unicode_to_gb18030(soup):
    """ Generate C code fragments for static array data.

    generate_ccode_map_unicode_to_.... will build two tables.

    1. map_gb18030_unicode_to_index
    2. map_gb18030_index_to_multibyte
    """

    _, u2m = translate_map_from_xml(soup)
    result = []

    # ----------------------------------------------
    # Build a table for converting unicode to index.

    code = CCodeBuilder()
    varname = 'map_gb18030_unicode_to_index'

    # number of used codes in each page
    num_codes_in_page = [0]*256
    for u in u2m.keys():
        num_codes_in_page[u >> 8] += 1

    pagesize = 16 * 2  # 32 words per page

    # 1 if one or more codes are assigned in the page.
    # 0 if no code exists in the page.
    code_exist_in_page = [1 if num_codes_in_page[i] else 0 for i in range(256)]

    # start of builing table
    code.addRow('// Offsets to page table (256 words)')
    for i in range(256):
        offset = sum(code_exist_in_page[:i]) * pagesize + 256
        code.addItem('%d,'%(offset if code_exist_in_page[i] else 0))

    code.flush()

    offset = 256  # offset for tbl_uc2idx
    index = 0  # index for tbl_idx2mb
    for page in range(256):
        # skip page without code.
        if not code_exist_in_page[page]:
            continue
        code.addRow('// page %d (offset %d-)'%(page, offset))
        # iteraging ??0? ~ ??F?
        for nib in range(16):
            uc0 = (page << 8) + (nib << 4)
            uc1 = uc0 + 16
            code.addItem('%d,'%(index))
            # set use bits
            usebit = 0
            for i in range(uc0, uc1):
                usebit *= 2
                if i in u2m:
                    usebit += 1
                    index += 1
            code.addItem('0x%x,'%(usebit))
            offset += 2

    result.append(FORMAT16.format(name=varname, size=code.numberOfItems(),
                                code=code.joinRows()))

    # ---------------------------------------------------
    # Build a table for converting index into multibytes.
    code = CCodeBuilder()
    varname = 'map_gb18030_index_to_multibyte'

    unicodes = sorted(u2m.keys())
    for i, u in enumerate(unicodes):
        if (i % 100 == 0):
            code.addRow('// %d-'%(i))
        code.addItem('0x%04x,'%(u2m[u]))

    result.append(FORMAT16.format(name=varname, size=code.numberOfItems(),
                                code=code.joinRows()))

    # --------------------------------
    # Build a table for bit summation.

    result.append(r"""
#ifndef BITSUM_TABLE
#define BITSUM_TABLE
    """)
    result.append(ccode_bitsum_table())
    result.append("""#endif // BITSUM_TABLE""")

    return '\n'.join(result)

# ------------------------------------------------------------------------------

def chinese():    
    # CHINESE CODESETS
    filename = 'data/gb-18030-2000.xml.gz'
    soup = read_gb18030_xml(filename)
    
    codes = [r"""// This code was generated using "misc/charset/build_tables.py".
#ifndef DICOMSDL_GB18030_TABLE_H__
#define DICOMSDL_GB18030_TABLE_H__"""]

    # GBK including GB2312 (81 – FE, 40 - FE except 7F)
    # GBK/GB2312 -> unicode
    code = generate_ccode_map_gb18030_to_unicode(soup)
    codes.append(code)
        
    # unicode -> GBK/GB2312
    code = generate_ccode_map_unicode_to_gb18030(soup)
    codes.append(code)
    
    # GB-18030 <-> unicode
    code = generate_ccode_offset_table(soup)
    codes.append(code)

    codes.append("\n#endif // DICOMSDL_GB18030_TABLE_H__")

    open("gb18030_table.h", "w").write("\n".join(codes))

def korean():
    codes = [r"""// This code was generated using "misc/charset/build_tables.py".
#ifndef DICOMSDL_KSX1001_TABLE_H__
#define DICOMSDL_KSX1001_TABLE_H__"""]

    m2u, u2m = read_table('KSX1001.TXT.gz', 0, 1)
    code = generate_ccode_map_unicode_to_multibyte(u2m, 'ksx1001')
    codes.append(code)

    code = generate_ccode_map_multibyte_to_unicode(m2u, 'ksx1001')
    codes.append(code)

    codes.append("\n#endif // DICOMSDL_KSX1001_TABLE_H__")

    open("ksx1001_table.h", "w").write("\n".join(codes))

def japanese():

    codes = [r"""// This code was generated using "misc/charset/build_tables.py".
#ifndef DICOMSDL_JISX0208_TABLE_H__
#define DICOMSDL_JISX0208_TABLE_H__"""]

    m2u, u2m = read_table('JIS0208.TXT.gz', 1, 2)
    code = generate_ccode_map_unicode_to_multibyte(u2m, 'jisx0208')
    codes.append(code)

    code = generate_ccode_map_multibyte_to_unicode(m2u, 'jisx0208')
    codes.append(code)

    codes.append("\n#endif // DICOMSDL_JISX0208_TABLE_H__")

    open("jisx0208_table.h", "w").write("\n".join(codes))


    codes = [r"""// This code was generated using "misc/charset/build_tables.py".
#ifndef DICOMSDL_JISX0212_TABLE_H__
#define DICOMSDL_JISX0212_TABLE_H__"""]

    m2u, u2m = read_table('JIS0212.TXT.gz', 0, 1)
    code = generate_ccode_map_unicode_to_multibyte(u2m, 'jisx0212')
    codes.append(code)

    code = generate_ccode_map_multibyte_to_unicode(m2u, 'jisx0212')
    codes.append(code)

    codes.append("\n#endif // DICOMSDL_JISX0212_TABLE_H__")

    open("jisx0212_table.h", "w").write("\n".join(codes))


# -----------------------------------------------------------------------------

def generate_ccode_map_singlebyte_to_unicode(m2u, codecname, area):
    """ Generate table for converting Single Byte Character Sets
    to unicode.
    
    0 for mapped unicode -> control characters or delimiters
    U+FFFD -> illegal character
    """

    # part05. 6.1.3 Control Characters
    # Table 6.1-1. DICOM Control Characters and Their Encoding
    # LF \x0a, FF \x0c, CR \x0d, ESC \x1b, TAB \x09

    if area == 'g0':
        for i in range(0, 0x21):
            m2u[i] = 0
        m2u[ord(b'\\')] = 0  # delimiters
        m2u[ord(b'=')] = 0
        m2u[ord(b'^')] = 0
        m2u[0x7f] = 0  # control character DEL
    
    result = ['\n// %s (%s set)'%(codecname.upper(), area.upper())]

    code = CCodeBuilder()
    varname = 'map_%s_to_unicode_%s'%(
        codecname.lower(), area.lower())

    char_range = range(0, 0x80) if area == 'g0' else range(0x80, 0x100)
    charsets = [m2u[c] for c in char_range if c in m2u]

    if len([m2u[c] for c in char_range if c in m2u]) != 128:
        fmt = '%-8s'
    else:
        m = max(charsets)
        if m >= 0x1000:
            fmt = '%-8s'
        elif m >= 0x100:
            fmt = '%-7s'
        else:
            fmt = '%-6s'

    for c in char_range:
        if c in m2u:
            item = '0x%x,'%m2u[c]
        else:
            item = '0xfffd,'
        code.addItem(fmt%(item))
        if c % 8 == 7:
            code.flush()

    result.append(FORMAT16.format(name=varname, size=code.numberOfItems(),
                            code=code.joinRows()))

    return ''.join(result)

def sbcs_to_unicode():
    """ """
    MAPPING_FILENAME_LIST = r"""
        8859-1.txt        latin1     0  1  g1
        8859-2.TXT        latin2     0  1  g1
        8859-3.TXT        latin3     0  1  g1
        8859-4.TXT        latin4     0  1  g1
        8859-5.TXT        cyrillic   0  1  g1
        8859-6.TXT        arabic     0  1  g1
        8859-7.TXT        greek      0  1  g1
        8859-8.TXT        hebrew     0  1  g1
        8859-9.TXT        latin5     0  1  g1
        8859-11.TXT       thai       0  1  g1
        JIS0201.TXT       jisx0201   0  1  g0
        JIS0201.TXT       jisx0201   0  1  g1
    """.strip().splitlines()

    codes = [r"""// This code was generated using "misc/charset/build_tables.py".
#ifndef DICOMSDL_SBCS_TO_UNICODE_TABLE_H__
#define DICOMSDL_SBCS_TO_UNICODE_TABLE_H__"""]

    # ascii to unicode
    m2u = {i:i for i in range(0, 128)}
    code = generate_ccode_map_singlebyte_to_unicode(m2u, 'ascii', 'g0')
    codes.append(code)

    # none to unicode
    m2u = {}
    m2u = {'\x1b': 0}
    code = generate_ccode_map_singlebyte_to_unicode(m2u, 'null', 'g0')
    codes.append(code)

    code = generate_ccode_map_singlebyte_to_unicode(m2u, 'null', 'g1')
    codes.append(code)

    for r in MAPPING_FILENAME_LIST:
        mapfn, codec, mcol, ucol, area = r.split()
        if codec in ('ksx1001', 'jisx0208', 'jisx0212'):
            continue
        m2u, u2m = read_table(mapfn+'.gz', int(mcol), int(ucol))
        code = generate_ccode_map_singlebyte_to_unicode(m2u, codec, area)
        codes.append(code)

    codes.append("\n#endif // DICOMSDL_SBCS_TO_UNICODE_TABLE_H__")

    open('sbcs_to_unicode_table.h', 'w').write('\n'.join(codes))


# -----------------------------------------------------------------------------


def table_unicode_to_singlebyte(codec, u2m, ucfrom, ucto):
    """ """
    code = CCodeBuilder()
    varname = 'map_unicode_to_%s_%04x_%04x'%(codec, ucfrom, ucto - 1)

    for i in range(ucfrom, ucto):
        if i % 16 == 0:
            code.addRow('/* U+%04X - U+%04X */'%(i, i+15))
        if i in u2m:
            code.addItem('0x%02x, '%(u2m[i]))
        else:
            code.addItem('0x00, ')
        if i % 8 == 7:
            code.flush()

    return FORMAT8.format(name=varname, size=code.numberOfItems(),
                                code=code.joinRows())


def const_unicode_to_singlebyte(codec, u2m, uc):
    """ """

    s = "const static uint8_t unicode_to_%s_u%x = 0x%02x;"%(codec, uc, u2m[uc])
    return s
    


def unicode_to_sbcs():
    """ """
    codes = [r"""// This code was generated using "misc/charset/build_tables.py".
#ifndef DICOMSDL_UNICODE_TO_SBCS_TABLE_H__
#define DICOMSDL_UNICODE_TO_SBCS_TABLE_H__"""]

    # latin2
    _, u2m = read_table('8859-2.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('latin2', u2m, 0xa0, 0x180))
    codes.append(table_unicode_to_singlebyte('latin2', u2m, 0x2d0, 0x2e0))

    _, u2m = read_table('8859-3.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('latin3', u2m, 0xa0, 0x180))
    codes.append(table_unicode_to_singlebyte('latin3', u2m, 0x2d0, 0x2e0))

    _, u2m = read_table('8859-4.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('latin4', u2m, 0xa0, 0x180))
    codes.append(table_unicode_to_singlebyte('latin4', u2m, 0x2c0, 0x2e0))

    _, u2m = read_table('8859-5.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('cyrillic', u2m, 0x400, 0x460))
    codes.append(const_unicode_to_singlebyte('cyrillic', u2m, 0xa7))
    codes.append(const_unicode_to_singlebyte('cyrillic', u2m, 0xad))
    codes.append(const_unicode_to_singlebyte('cyrillic', u2m, 0x2116))

    _, u2m = read_table('8859-6.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('arabic', u2m, 0x600, 0x660))
    # 0xA4, 0xAD

    _, u2m = read_table('8859-7.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('greek', u2m, 0xa0, 0xc0))
    codes.append(table_unicode_to_singlebyte('greek', u2m, 0x370, 0x3d0))
    codes.append(table_unicode_to_singlebyte('greek', u2m, 0x2010, 0x2020))
    codes.append(const_unicode_to_singlebyte('greek', u2m, 0x20ac))
    codes.append(const_unicode_to_singlebyte('greek', u2m, 0x20af))

    _, u2m = read_table('8859-8.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('hebrew', u2m, 0xa0, 0xc0))
    codes.append(table_unicode_to_singlebyte('hebrew', u2m, 0x5d0, 0x5f0)) # 05d0~05ea
    codes.append(table_unicode_to_singlebyte('hebrew', u2m, 0x2010, 0x2020))
    codes.append(const_unicode_to_singlebyte('hebrew', u2m, 0xd7))
    codes.append(const_unicode_to_singlebyte('hebrew', u2m, 0xf7))
    codes.append(const_unicode_to_singlebyte('hebrew', u2m, 0x200e))
    codes.append(const_unicode_to_singlebyte('hebrew', u2m, 0x200f))
    codes.append(const_unicode_to_singlebyte('hebrew', u2m, 0x2017))

    _, u2m = read_table('8859-9.TXT.gz', 0, 1)
    codes.append(table_unicode_to_singlebyte('latin5', u2m, 0xa0, 0x100))
    codes.append(const_unicode_to_singlebyte('latin5', u2m, 0x11e))
    codes.append(const_unicode_to_singlebyte('latin5', u2m, 0x11f))
    codes.append(const_unicode_to_singlebyte('latin5', u2m, 0x130))
    codes.append(const_unicode_to_singlebyte('latin5', u2m, 0x131))
    codes.append(const_unicode_to_singlebyte('latin5', u2m, 0x15e))
    codes.append(const_unicode_to_singlebyte('latin5', u2m, 0x15f))

    codes.append("\n#endif // DICOMSDL_UNICODE_TO_SBCS_TABLE_H__")
    open('unicode_to_sbcs_table.h', 'w').write('\n'.join(codes))
    
if __name__ == "__main__":
    chinese()
    japanese()
    korean()

    sbcs_to_unicode()
    unicode_to_sbcs()

    """
    unicode_to_sbcs_table.h
    unicode_to_sbcs.h
    sbcs_to_unicode_table.h
    sbcs_to_unicode.h
    """
