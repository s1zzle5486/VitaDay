#include "../src/graph_stick.hpp"
#include <cassert>
#include <iostream>
using namespace day;
static double simulate(int fps,int axis=255){GraphStickMotion m;m.poll(axis,128,0);double position=0;for(int frame=1;frame<=fps*10;++frame)position+=m.poll(axis,128,uint64_t(frame)*1000000/fps);return position;}
int main(){GraphStickMotion m;assert(m.poll(255,128,0)==0);double sum=0;
 for(int i=1;i<=24;++i){double move=m.poll(255,128,i*16667);assert(move>0&&move<1);sum+=move;}assert(sum>3&&sum<4);
 assert(m.poll(128,128,400009)==0&&m.direction==0);assert(m.poll(255,128,400010)==0);assert(m.poll(255,128,416677)<.14);
 assert(m.poll(0,128,2033384)==0);assert(m.poll(0,128,2050051)<0);assert(m.poll(128,128,2050052)==0);
 m.reset();assert(m.poll(128,0,0)==0);assert(m.poll(128,0,16667)<0);assert(m.poll(128,0,1000000)==0);assert(m.poll(128,0,1016667)>-.14);
 assert(m.poll(128,0,100000)==0);assert(m.poll(128,128,100001)==0);assert(m.poll(145,128,100002)==0);
 assert(GraphStickMotion::speed(0)==8&&GraphStickMotion::speed(500000)==8&&GraphStickMotion::speed(3000000)==333);
 assert(std::abs(simulate(30)-simulate(60))<.3&&std::abs(simulate(60)-simulate(120))<.3);assert(simulate(60,170)<simulate(60)/5);
 std::cout<<"Continuous graph stick: fractional motion each frame, analog speed, acceleration, frame-rate independence, immediate neutral/reverse reset and no resume jumps passed\n";
}
