#!/usr/bin/env python3
"""Keep the launch screen's native text label in sync with VERSION."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET
root=Path(__file__).resolve().parent.parent
version=(root/'VERSION').read_text().strip()
if not re.fullmatch(r'\d+\.\d+\.\d+',version):raise SystemExit('Invalid VERSION')
path=root/'sce_sys/livearea/contents/template.xml'
tree=ET.parse(path);livearea=tree.getroot()
livearea.set('content-rev',str(int(version.split('.')[1])*100+int(version.split('.')[2])))
frame=livearea.find("frame[@id='frame1']")
if frame is None:frame=ET.SubElement(livearea,'frame',{'id':'frame1'})
frame.clear();frame.set('id','frame1')
item=ET.SubElement(frame,'liveitem')
text=ET.SubElement(item,'text',{'valign':'bottom','align':'left','text-align':'left','text-valign':'bottom','margin-left':'24','margin-bottom':'24','line-space':'0','ellipsis':'on'})
ET.SubElement(text,'str',{'color':'#55dcc3','size':'22','bold':'on','shadow':'on'}).text='VitaDay '+version
ET.indent(tree,space='  ')
tree.write(path,encoding='utf-8',xml_declaration=True)
print('LiveArea:',version)
