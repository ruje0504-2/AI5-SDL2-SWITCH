#!/usr/bin/env python3
"""Export the user's PE cursor resources, preserving palette XOR and AND bits."""
from pathlib import Path
import struct
import sys


def read_resources(exe):
    data = Path(exe).read_bytes()
    def u16(off): return struct.unpack_from('<H', data, off)[0]
    def u32(off): return struct.unpack_from('<I', data, off)[0]
    pe = u32(0x3c)
    if data[:2] != b'MZ' or data[pe:pe+4] != b'PE\0\0' or u16(pe+24) != 0x10b:
        raise ValueError('Expected PE32 executable')
    sections = pe+24+u16(pe+20)
    def offset(rva):
        for i in range(u16(pe+6)):
            s = sections+i*40
            start, size, raw = u32(s+12), u32(s+16), u32(s+20)
            if start <= rva < start+size:
                return raw+rva-start
        raise ValueError('Resource RVA outside file-backed sections')
    base = offset(u32(pe+24+96+16))
    resources = {}
    def visit(relative, keys):
        off = base+relative
        for i in range(u16(off+12)+u16(off+14)):
            name, value = struct.unpack_from('<II', data, off+16+i*8)
            if name & 0x80000000: continue
            if value & 0x80000000:
                if len(keys) >= 3: raise ValueError('Resource directory depth')
                visit(value & 0x7fffffff, keys+(name,))
            else:
                rva, size = struct.unpack_from('<II', data, base+value)
                pos = offset(rva)
                if pos+size > len(data): raise ValueError('Truncated resource')
                resources[keys+(name,)] = data[pos:pos+size]
    visit(0, ())
    return resources


def extract(exe):
    resources = read_resources(exe)
    records = []
    for (kind, identifier, lang), group in sorted(resources.items()):
        if kind != 12: continue
        if len(group) != 20 or struct.unpack_from('<HHH', group) != (0, 2, 1):
            raise ValueError('Expected one image per cursor')
        cur = resources[(1, struct.unpack_from('<H', group, 18)[0], lang)]
        hx, hy = struct.unpack_from('<HH', cur)
        header, w, double_h, planes, bits, compression = struct.unpack_from('<IiiHHI', cur, 4)
        if (header,w,double_h,planes,compression) != (40,32,64,1,0) or bits not in (1,4,8) or hx>=32 or hy>=32:
            raise ValueError('Unsupported cursor DIB')
        colors = struct.unpack_from('<I',cur,36)[0] or 1<<bits
        start = 44+colors*4
        stride = (32*bits+31)//32*4
        if len(cur) != start+stride*32+128: raise ValueError('Cursor size mismatch')
        pixels = bytearray()
        for y in range(32):
            for x in range(32):
                index = cur[start+(31-y)*stride+x*bits//8] >> (8-bits-(x*bits)%8) & ((1<<bits)-1)
                if index >= colors: raise ValueError('Palette index')
                mask = (cur[start+stride*32+(31-y)*4+x//8] >> (7-x%8)) & 1
                pixels.extend(cur[44+index*4:47+index*4]); pixels.append(mask*255)
        records.append(struct.pack('<HHHH',identifier,hx,hy,0)+pixels)
    if not records: raise ValueError('No cursors')
    return b'ISCUR01\0'+struct.pack('<I',len(records))+b''.join(records)

if __name__ == '__main__':
    target = Path(sys.argv[2]); target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(extract(sys.argv[1]))
    print(f'Native cursor resources exported: {target}')
