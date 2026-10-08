#include "../src/core.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){State s;for(int i=0;i<20;++i)rememberColor(s,i);assert(s.recentColors.size()==12&&s.recentColors.front()==19&&s.recentColors.back()==8);rememberColor(s,12);assert(s.recentColors.front()==12&&s.recentColors.size()==12);s.dayLook.background=5;s.dayLook.image="custom.png";s.nightLook.background=11;auto decoded=decode(encode(s));assert(decoded.recentColors==s.recentColors&&decoded.dayLook.background==5&&decoded.dayLook.image=="custom.png"&&decoded.nightLook.background==11);for(int i=0;i<=10;++i)assert(backgroundGallery(galleryBackground(i))==i);auto old=encode(s);old["display"].erase("recentColors");assert(decode(old).recentColors.empty());std::cout<<"Recent color limit, deduplication, photo mapping and legacy custom background passed\n";}
