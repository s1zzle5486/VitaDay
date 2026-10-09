#include "../src/text_fit.hpp"
#include <cassert>
#include <iostream>
using namespace day;
static int measure(int size,const std::string& s){int glyphs=0;for(unsigned char c:s)if((c&0xc0)!=0x80)++glyphs;return glyphs*size;}
int main(){auto fit=[](const std::string& s,int w){return fitLabel(s,18,w,15,measure);};
 auto shortText=fit("Test",80);assert(shortText.value=="Test"&&shortText.size==18);
 auto shrink=fit("Test",60);assert(shrink.value=="Test"&&shrink.size==15);
 auto longText=fit("Длинное название праздника",90);assert(longText.value=="Длинн…"&&measure(longText.size,longText.value)<=90);
 auto cjk=fit("東京都の天気予報",45);assert(cjk.value=="東京…"&&measure(cjk.size,cjk.value)<=45);
 assert(fit("Test",14).value.empty());assert(fit("Test",0).value.empty());assert(fit("Test",-1).value.empty());assert(fit("",0).value.empty());
 assert(fit("A\nB\rC\tD",150).value=="A B C D");
 std::string huge;for(int i=0;i<10000;++i)huge+="日";auto bounded=fit(huge,100);assert(measure(bounded.size,bounded.value)<=100&&bounded.value.size()<30);
 std::cout<<"Text fitting: readable shrink, bounded ellipsis, Cyrillic/CJK UTF-8 boundaries, tiny boxes, controls and very long strings passed\n";
}
