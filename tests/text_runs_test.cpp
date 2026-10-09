#include "../src/text_runs.hpp"
#include <cassert>
#include <iostream>
int main(){using namespace day;
 for(const std::string s:{"日本語","简体中文","繁體中文","한국어","\xf0\xa0\x80\x80"}){auto runs=textRuns(s);assert(runs.size()==1&&runs[0].cjk&&runs[0].value==s);}
 auto mixed=textRuns("RU 日本語 20°C 한국어");assert(mixed.size()==4&&!mixed[0].cjk&&mixed[1].cjk&&!mixed[2].cjk&&mixed[3].cjk);
 auto astral=textCodepoint("\xf0\xa0\x80\x80",0);assert(astral.value==0x20000&&astral.bytes==4);
 for(const std::string s:{"\xf0\x80\x80\x80","\xe3\x81","\x80","\xed\xa0\x80"}){size_t n=0;for(size_t at=0;at<s.size();){auto cp=textCodepoint(s,at);assert(cp.bytes>0);at+=cp.bytes;n+=cp.bytes;}assert(n==s.size());}
 std::cout<<"CJK routing: Japanese, Chinese, Korean, supplementary Unicode, mixed runs and malformed UTF-8 passed\n";
}
