#!/usr/bin/env python3
"""Test the actual gallery/worker code with desktop CPU decoding and GPU mocks.
Set JPEG_INCLUDE and JPEG_LIBRARY for another libjpeg installation.
"""
import os, shutil, subprocess
from pathlib import Path
root=Path(__file__).resolve().parent.parent
project=root.parent.parent
out=root/'work';out.mkdir(exist_ok=True)
include=Path(os.environ.get('JPEG_INCLUDE',project/'work/jpeginclude'))
if 'JPEG_LIBRARY' in os.environ:library=Path(os.environ['JPEG_LIBRARY'])
else:
 import PIL
 library=next((Path(PIL.__file__).parent/'.dylibs').glob('libjpeg.*.dylib'))
prefix=(root/'tests/gallery_render_prefix.cpp').read_text().replace('"../src/','"'+str(root/'src')+'/')
main=(root/'src/main.cpp').read_text().split('static std::map<std::string,uint64_t> thumbnailUsed;',1)[1].split('static void drawThumbnail(',1)[0]
main=('static std::map<std::string,uint64_t> thumbnailUsed;'+main).replace('app0:assets/thumbs/',str(root/'assets/thumbs')+'/')
fixtures=[]
for i in range(22):
 path=out/f'photo-{i}.jpg';shutil.copyfile(root/'assets/background1.jpg',path);fixtures.append(str(path))
setup='backgroundFiles={'+','.join('"'+p+'"' for p in fixtures)+'};'
suffix=(root/'tests/gallery_render_suffix.cpp').read_text().replace('assert(launched==0);','assert(launched==0);'+setup)
source=out/'gallery_test.cpp';source.write_text(prefix+'static const std::string assetRoot=\"'+str(root/'assets')+'\";'+main+suffix)
binary=out/'gallery_test'
subprocess.run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-I'+str(include),str(source),str(root/'src/lodepng.cpp'),'-DLODEPNG_NO_COMPILE_ENCODER',str(library),'-o',str(binary)],check=True)
subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',DYLD_LIBRARY_PATH=str(library.parent)))
