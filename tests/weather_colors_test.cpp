#include "../src/weather_chart.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){
 assert(chartColor(0,-10)!=chartColor(0,35));assert(chartColor(3,5)!=chartColor(3,90));
 assert(chartColor(8,0)==chartColor(8,60));assert(chartColor(8,100)!=chartColor(8,180));
 assert(chartColor(9,0)==chartColor(9,5));assert(chartColor(9,15)!=chartColor(9,90));
 for(double bound:{3.,6.,8.,11.})assert(uvValueColor(bound-.001)!=uvValueColor(bound));
 for(unsigned bg:{0xfff5fafbu,0xff3d2f20u,0xff000000u,0xffffffffu,0xffaaaaaau,0xff88ff00u})for(int metric=0;metric<10;++metric)for(double value:{-40.,0.,5.,15.,60.,100.,180.,1000.}){auto c=readableWeatherColor(chartColor(metric,value),bg);assert(weatherContrast(c,bg)>=4.499);assert((c>>24)==255);}
 std::cout<<"Weather value colours: pollutant/UV boundaries and readable light/dark/custom panels passed\n";
}
