#include "../src/input.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){State s;s.themeMode=0;assert(!nightTheme(s,23));s.themeMode=1;assert(nightTheme(s,12));s.themeMode=2;assert(!nightTheme(s,7)&&!nightTheme(s,19)&&nightTheme(s,20)&&nightTheme(s,0));s.dayStart=20;s.nightStart=7;assert(!nightTheme(s,23)&&nightTheme(s,12));s.dayStart=s.nightStart;assert(!nightTheme(s,23));s.themeMode=1;s.countries.push_back({"US","United States",2,false,true,"America/New_York","Brooklyn"});State loaded=decode(encode(s));assert(loaded.themeMode==1&&loaded.countries[0].clockCity=="Brooklyn");auto old=encode(s);old["display"].erase("themeMode");old["display"]["automatic"]=true;assert(decode(old).themeMode==2);assert(shortDate({2027,1,1})=="1.01.2027");assert(stickDirection(255,128,128,128,1,2,4,8)==2);assert(stickDirection(128,128,128,0,1,2,4,8)==4);std::cout<<"Theme modes, midnight boundaries, migration, clock city and date passed\n";}
