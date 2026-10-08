import json
from pathlib import Path
from datetime import datetime,timezone
from zoneinfo import ZoneInfo
countries={};zones={};start=int(datetime(2025,1,1,tzinfo=timezone.utc).timestamp());end=int(datetime(2037,1,1,tzinfo=timezone.utc).timestamp())
for line in Path('/usr/share/zoneinfo/zone.tab').read_text().splitlines():
 if line.startswith('#') or not line:continue
 cc,coord,name,*_=line.split('\t');countries.setdefault(cc,[]).append(name)
 for country in cc.split(','):countries.setdefault(country,[]);countries[country]=list(dict.fromkeys(countries[country]+[name]))
 z=ZoneInfo(name)
 def offset(t):return int(datetime.fromtimestamp(t,timezone.utc).astimezone(z).utcoffset().total_seconds())
 last=offset(start);points=[[start,last]]
 for t in range(start+43200,end,43200):
  cur=offset(t)
  if cur!=last:
   lo=t-43200;hi=t
   while hi-lo>1:
    mid=(lo+hi)//2
    if offset(mid)==last:lo=mid
    else:hi=mid
   points.append([hi,cur]);last=cur
 zones[name]=points
preferred={'US':'America/New_York','CA':'America/Toronto','RU':'Europe/Moscow','AU':'Australia/Sydney','BR':'America/Sao_Paulo','CN':'Asia/Shanghai','GB':'Europe/London'}
for cc,name in preferred.items():
 if name in countries.get(cc,[]):countries[cc].remove(name);countries[cc].insert(0,name)
(Path(__file__).resolve().parent.parent/'assets/zones.json').write_text(json.dumps({'countries':countries,'zones':zones,'start':start,'end':end},separators=(',',':')))

