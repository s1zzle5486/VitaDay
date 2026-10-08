#include "../src/core.hpp"
#include "../src/color_picker.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){for(int color:{0,0xffffff,0xff,0xff00,0xff0000,0x639ab2,0x114d81})assert(hsvRgb(rgbHsv(color))==color);for(int i=0;i<72;++i)assert(presetRgb(i)>=0&&presetRgb(i)<=0xffffff);State s;Location l;l.valid=true;l.automatic=false;l.city="Tokyo";l.country="JP";l.zone="Asia/Tokyo";l.lat=35.68;l.lon=139.76;Weather w;w.valid=true;w.temperature=24;w.code=2;w.dates={"2026-10-07"};w.highs={25};w.lows={18};w.details={{"current",{{"temperature_2m",24}}}};s.regions.push_back({l,w,123});State loaded=decode(encode(s));assert(loaded.regions.size()==1&&loaded.regions[0].location.city=="Tokyo"&&loaded.regions[0].weather.code==2&&loaded.regions[0].weather.details==w.details);auto old=encode(s);old.erase("regions");assert(decode(old).regions.empty());std::cout<<"HSV roundtrip, 72 swatches, regional weather persistence and legacy migration passed\n";}
