#!/usr/bin/env python3
"""Original player glyphs, supersampled for crisp Vita controls."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parent.parent
out=root/'assets/music';out.mkdir(exist_ok=True)
for name in ['play','pause','previous','next','repeat','repeat-one','shuffle','order','refresh','volume']:
 for size in [24,32]:
  scale=size*4/32;im=Image.new('RGBA',(size*4,size*4));d=ImageDraw.Draw(im);white=(255,255,255,255)
  def poly(points):d.polygon([(round(x*scale),round(y*scale)) for x,y in points],fill=white)
  def box(points):d.rectangle(tuple(round(v*scale) for v in points),fill=white)
  def line(points,width=2):d.line([(round(x*scale),round(y*scale)) for x,y in points],fill=white,width=max(1,round(width*scale)),joint='curve')
  if name=='play':poly([(10,6),(26,16),(10,26)])
  elif name=='pause':box((9,7,13,25));box((19,7,23,25))
  elif name=='previous':box((7,7,9,25));poly([(25,7),(11,16),(25,25)])
  elif name=='next':box((23,7,25,25));poly([(7,7),(21,16),(7,25)])
  elif name in ['repeat','repeat-one']:
   d.arc(tuple(round(v*scale) for v in (5,8,15,24)),90,270,fill=white,width=round(2*scale));d.arc(tuple(round(v*scale) for v in (17,8,27,24)),270,450,fill=white,width=round(2*scale));line([(10,8),(26,8)]);line([(6,24),(22,24)]);poly([(23,4),(29,8),(23,12)]);poly([(9,20),(3,24),(9,28)])
  elif name=='shuffle':
   line([(5,8),(10,8),(21,24),(27,24)]);line([(5,24),(10,24),(14,18)]);line([(19,12),(22,8),(27,8)]);poly([(24,4),(30,8),(24,12)]);poly([(24,20),(30,24),(24,28)])
  elif name=='order':
   line([(13,8),(27,8)]);line([(13,16),(27,16)]);line([(13,24),(27,24)]);font=ImageFont.truetype(str(root/'assets/font.ttf'),round(9*scale))
   for i in range(3):d.text((5*scale,(8+i*8)*scale),str(i+1),font=font,fill=white,anchor='mm')
  elif name=='refresh':
   d.arc(tuple(round(v*scale) for v in (6,6,26,26)),45,330,fill=white,width=round(2.3*scale));poly([(25,6),(29,15),(20,13)])
  else:
   poly([(5,12),(10,12),(16,7),(16,25),(10,20),(5,20)]);d.arc(tuple(round(v*scale) for v in (12,7,28,25)),285,435,fill=white,width=round(2*scale));d.arc(tuple(round(v*scale) for v in (15,11,23,21)),285,435,fill=white,width=round(2*scale))
  if name=='repeat-one':
   font=ImageFont.truetype(str(root/'assets/font.ttf'),round(11*scale));d.text((16*scale,16*scale),'1',font=font,fill=white,anchor='mm')
  im.resize((size,size),Image.Resampling.LANCZOS).save(out/f'{name}-{size}.png')
print('20 original antialiased player icon masks generated')
