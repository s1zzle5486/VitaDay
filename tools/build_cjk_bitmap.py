#!/usr/bin/env python3
"""Bake OFL Noto CJK glyphs into a portable bitmap pack.
Usage: python build_cjk_bitmap.py SOURCE_FONT
Requires Pillow and fonttools on the build computer, never on Vita.
Each 48px alpha mask is independently zlib-compressed for lazy reads.
"""
import json, struct, sys, zlib
from pathlib import Path
from PIL import ImageFont
from fontTools.ttLib import TTFont
root=Path(__file__).resolve().parent.parent
source=Path(sys.argv[1]);base=48
face=ImageFont.truetype(str(source),base)
cmap=TTFont(source).getBestCmap()
def asian(cp): return 0x2e80<=cp<=0xffff or 0x1100<=cp<=0x11ff or 0x20000<=cp<=0x3ffff
chars=sorted(cp for cp in cmap if asian(cp))
required=set()
def collect(value):
 if isinstance(value,str): required.update(ord(c) for c in value if asian(ord(c)))
 elif isinstance(value,dict):
  for v in value.values(): collect(v)
 elif isinstance(value,list):
  for v in value: collect(v)
for path in (root/'assets/locales').glob('*.json'): collect(json.loads(path.read_text()))
required.discard(0xfe0f)
assert required <= set(chars), sorted(required-set(chars))
header=struct.Struct('<8sIIII');record=struct.Struct('<IIIIhhhhhh')
start=header.size+len(chars)*record.size
records=[];data=bytearray()
for cp in chars:
 mask,offset=face.getmask2(chr(cp),mode='L',anchor='ls')
 w,h=mask.size;raw=bytes(mask);packed=zlib.compress(raw,6) if raw else b''
 assert len(raw)==w*h and w<=256 and h<=256
 advance=round(face.getlength(chr(cp))*64) # 26.6 fixed point
 records.append(record.pack(cp,start+len(data),len(packed),len(raw),w,h,offset[0],-offset[1],advance,0))
 data.extend(packed)
out=root/'assets/cjk-bitmap.bin'
with out.open('wb') as f:
 f.write(header.pack(b'VDAYBF01',base,len(chars),record.size,start))
 for item in records:f.write(item)
 f.write(data)
print(f'{out}: {len(chars)} glyphs, {len(required)} UI symbols, {out.stat().st_size} bytes; base={base}')
