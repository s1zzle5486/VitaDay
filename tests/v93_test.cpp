#include "../src/core.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){State s;s.userRgb=0xa123ef;s.marks["2026-10-08"]={"Своя дата: поездка, встреча с друзьями и праздничный ужин",3,true};auto j=encode(s);auto restored=decode(j);assert(restored.userRgb==s.userRgb&&restored.marks.begin()->second.title==s.marks.begin()->second.title);assert(marksFor(restored,{2027,10,8}).size()==1);j["preferences"].erase("userRgb");j["preferences"].erase("userColor");auto legacy=decode(j);assert(legacy.userRgb==0xe077bb&&legacy.userColor==8);assert(legacy.marks.begin()->second.title==s.marks.begin()->second.title&&legacy.marks.begin()->second.color==3&&legacy.marks.begin()->second.yearly);j["preferences"]["userRgb"]=0xffffff+1;assert(decode(j).userRgb==0xffffff);std::cout<<"Personal date color migration preserves title, fill and yearly recurrence: PASS\n";}
