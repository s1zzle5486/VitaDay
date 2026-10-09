#!/usr/bin/env python3
"""Exercise the actual Vita CJK atlas helpers against portable bitmap reader.
Runs the same portable lookup, decompression, resampling and GPU atlas helpers
that are compiled for Vita. No font engine is used in this test.
"""
import json, os, subprocess
from pathlib import Path
root=Path(__file__).resolve().parent.parent
project=root.parent.parent
os.chdir(root)
(root/"work").mkdir(exist_ok=True)
chars=set()
def collect(x):
 if isinstance(x,str):chars.update(ord(c) for c in x if 0x2e80<=ord(c)<=0xffff or 0x1100<=ord(c)<=0x11ff or 0x20000<=ord(c)<=0x3ffff)
 elif isinstance(x,(list,dict)):
  for v in (x.values() if isinstance(x,dict) else x): collect(v)
for p in root.joinpath('assets/locales').glob('*.json'):collect(json.loads(p.read_text()))
chars.difference_update([0x3000,0xfe0f])
root.joinpath('work/cjk-codepoints.txt').write_text('\n'.join(map(str,sorted(chars))))
native=root.joinpath('src/main.cpp').read_text().split('static CjkBitmapFont cjkFace;',1)[1].split('static void prepareFonts()',1)[0]
prefix=root.joinpath('tests/cjk_bitmap_render_prefix.cpp').read_text().replace('"../src/', '"'+str(root/'src')+'/')
source=root/'work/cjk_native_test.cpp'
source.write_text(prefix+'static CjkBitmapFont cjkFace;'+native.replace('app0:assets/cjk-bitmap.bin',str(root/'assets/cjk-bitmap.bin'))+root.joinpath('tests/cjk_bitmap_render_suffix.cpp').read_text())
binary=root/'work/cjk_native_test'
subprocess.run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined',str(source),'-lz','-o',str(binary)],check=True)
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
subprocess.run([str(binary)],check=True,env=env)
