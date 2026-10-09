#pragma once
#include <string>
#include <vector>
namespace day {
struct TextRun {std::string value;bool cjk;};
inline bool eastAsian(unsigned cp){return (cp>=0x2e80&&cp<=0xffff)||(cp>=0x1100&&cp<=0x11ff)||(cp>=0x20000&&cp<=0x3ffff);}
struct TextCodepoint {unsigned value;size_t bytes;};
inline TextCodepoint textCodepoint(const std::string& s,size_t at){
    unsigned char c=s[at];size_t n=c<128?1:c>=0xc2&&c<0xe0?2:c<0xf0&&c>=0xe0?3:c>=0xf0&&c<=0xf4?4:0;
    if(!n||at+n>s.size())return {0xfffd,1};
    unsigned cp=c&(n==1?127:n==2?31:n==3?15:7);
    for(size_t i=1;i<n;++i){unsigned char next=s[at+i];if((next&0xc0)!=0x80)return {0xfffd,1};cp=(cp<<6)|(next&63);}
    if((n==2&&cp<0x80)||(n==3&&cp<0x800)||(n==4&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return {0xfffd,1};
    return {cp,n};
}
inline std::vector<TextRun> textRuns(const std::string& s){std::vector<TextRun> runs;for(size_t i=0;i<s.size();){auto cp=textCodepoint(s,i);bool asian=eastAsian(cp.value);if(runs.empty()||runs.back().cjk!=asian)runs.push_back({"",asian});runs.back().value+=s.substr(i,cp.bytes);i+=cp.bytes;}return runs;}
}
