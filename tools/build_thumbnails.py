#!/usr/bin/env python3
"""Bake small gallery previews so Vita never decodes full built-in photos."""
from pathlib import Path
from PIL import Image,ImageOps
root=Path(__file__).resolve().parent.parent
out=root/'assets/thumbs';out.mkdir(exist_ok=True)
for path in sorted(root.joinpath('assets').glob('background*.jpg')):
 image=ImageOps.fit(Image.open(path).convert('RGBA'),(256,144),method=Image.Resampling.LANCZOS)
 (out/(path.stem+'.rgba')).write_bytes(image.tobytes())
print('10 gallery thumbnails baked at 256x144 RGBA')
