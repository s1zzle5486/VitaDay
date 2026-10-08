#include "../src/core.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){State s;s.style=2;s.dayLook.style=0;s.dayLook.background=3;s.nightLook.background=4;s.textRgb=0xddeecc;s.accentRgb=0x123456;s.marks["2026-10-07"]={"Событие",4,true};s.countries.push_back({"US","United States",3,true,true,"America/New_York"});auto j=encode(s);State loaded;loaded=decode(j);assert(loaded.style==2&&loaded.textRgb==s.textRgb&&loaded.accentRgb==s.accentRgb);assert(loaded.dayLook.background==3&&loaded.nightLook.background==4);assert(loaded.countries[0].clock&&loaded.countries[0].holidays);assert(loaded.marks.at("2026-10-07").title=="Событие");j["display"].erase("simple");j["display"].erase("textRgb");j["display"].erase("accentRgb");j["display"]["day"]["style"]=1;loaded=decode(j);assert(loaded.style==1&&loaded.textRgb==-1&&loaded.accentRgb==-1);assert(loaded.dayLook.background==3&&loaded.nightLook.background==4);std::cout<<"V4 persistence and V3 migration passed\n";}
