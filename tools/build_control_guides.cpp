// Area-resample original posters to Vita's display width, then store RGB565.
#include "../src/png_pixels.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
int main(int argc,char** argv){
 if(argc!=3)return 2;std::vector<unsigned char> rgba;unsigned sw=0,sh=0;
 if(!readPngPixels(argv[1],rgba,sw,sh))return 3;
 constexpr unsigned w=960,h=540;std::ofstream f(argv[2],std::ios::binary);f.write("VDAYGI01",8);
 for(unsigned n:{w,h,w*h*2}){char b[4];for(int i=0;i<4;++i)b[i]=n>>(i*8);f.write(b,4);}
 for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
  double x0=double(x)*sw/w,x1=double(x+1)*sw/w,y0=double(y)*sh/h,y1=double(y+1)*sh/h,sum[3]={},area=(x1-x0)*(y1-y0);
  for(unsigned sy=floor(y0);sy<std::min(sh,unsigned(ceil(y1)));++sy)for(unsigned sx=floor(x0);sx<std::min(sw,unsigned(ceil(x1)));++sx){
   double weight=(std::min(x1,double(sx+1))-std::max(x0,double(sx)))*(std::min(y1,double(sy+1))-std::max(y0,double(sy)));
   for(int c=0;c<3;++c)sum[c]+=rgba[(size_t(sy)*sw+sx)*4+c]*weight;
  }
  unsigned r=std::clamp(int(lround(sum[0]/area)),0,255),g=std::clamp(int(lround(sum[1]/area)),0,255),b=std::clamp(int(lround(sum[2]/area)),0,255);
  uint16_t rgb=((r>>3)<<11)|((g>>2)<<5)|(b>>3);char bytes[2]={char(rgb),char(rgb>>8)};f.write(bytes,2);
 }
 return f?0:4;
}
