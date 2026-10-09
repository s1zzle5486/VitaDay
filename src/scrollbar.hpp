#pragma once
#include <algorithm>
namespace day {
struct ScrollThumb {bool visible=false;int start=0,length=0;};
inline ScrollThumb scrollThumb(int track,int total,int shown,int first) {
    if(track<=0||shown<=0||total<=shown)return {};
    int length=std::clamp(int(1LL*track*shown/total),std::min(24,track),track);
    int offset=std::clamp(first,0,total-shown);
    return {true,int(1LL*offset*(track-length)/(total-shown)),length};
}
}
