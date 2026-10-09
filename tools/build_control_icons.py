#!/usr/bin/env python3
"""Original controller glyphs, rasterized at 4x for smooth small icons."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
r=Path(__file__).resolve().parent.parent;out=r/'assets/controls';out.mkdir(exist_ok=True)
for name in ['cross','circle','square','triangle','dpad','left-stick','right-stick','shoulders','start','select']:
 im=Image.new('RGBA',(256,128));d=ImageDraw.Draw(im);white=(255,255,255,255)
 if name in ['cross','circle','square','triangle']:
  d.ellipse((76,12,180,116),outline=white,width=6)
  if name=='cross':d.line((106,42,150,86),fill=white,width=8);d.line((150,42,106,86),fill=white,width=8)
  elif name=='circle':d.ellipse((101,37,155,91),outline=white,width=7)
  elif name=='square':d.rectangle((104,40,152,88),outline=white,width=7)
  else:d.line([(128,34),(98,88),(158,88),(128,34)],fill=white,width=7)
 elif name=='dpad':
  for points in [[(128,18),(108,42),(148,42)],[(128,110),(108,86),(148,86)],[(82,64),(106,44),(106,84)],[(174,64),(150,44),(150,84)]]:d.polygon(points,fill=white)
 elif name in ['left-stick','right-stick']:
  d.ellipse((78,14,178,114),outline=white,width=7);d.ellipse((96,32,160,96),outline=white,width=4)
  font=ImageFont.truetype(str(r/'assets/font.ttf'),48);d.text((128,62),'L' if name=='left-stick' else 'R',font=font,fill=white,anchor='mm')
 else:
  label={'shoulders':'L / R','start':'START','select':'SELECT'}[name];d.rounded_rectangle((10,22,246,106),radius=24,outline=white,width=6)
  font=ImageFont.truetype(str(r/'assets/font.ttf'),42);d.text((128,62),label,font=font,fill=white,anchor='mm')
 im.resize((64,32),Image.Resampling.LANCZOS).save(out/(name+'.png'))
print('10 original antialiased controller icons generated')
