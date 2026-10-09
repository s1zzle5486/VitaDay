#include "../src/scrollbar.hpp"
#include <cassert>
#include <iostream>
int main(){using day::scrollThumb;assert(!scrollThumb(362,0,6,0).visible);assert(!scrollThumb(362,6,6,0).visible);assert(!scrollThumb(0,21,6,0).visible);assert(!scrollThumb(362,21,0,0).visible);
 for(int track:{30,158,309,362,392,516})for(int shown:{2,3,5,6,8,9})for(int total:{7,16,21,250,10000})if(total>shown){auto first=scrollThumb(track,total,shown,0),last=scrollThumb(track,total,shown,total-shown);assert(first.visible&&first.start==0&&first.length>=24&&first.length<=track);assert(last.start+last.length==track);int previous=0;for(int i=0;i<=total-shown;++i){auto t=scrollThumb(track,total,shown,i);assert(t.start>=previous&&t.start+t.length<=track);previous=t.start;}assert(scrollThumb(track,total,shown,-10).start==0);assert(scrollThumb(track,total,shown,total+1).start==last.start);}
 std::cout<<"Scrollbar: short/empty lists hidden, visible fraction, monotone motion, clamping and exact end positions passed\n";}
