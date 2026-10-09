#pragma once
#include <vector>
namespace day {
inline std::vector<unsigned char> roundedPanelMask(){constexpr int side=32,radius=8,samples=8;std::vector<unsigned char> pixels(side*side*4,255);
 for(int y=0;y<side;++y)for(int x=0;x<side;++x){int covered=0;for(int sy=0;sy<samples;++sy)for(int sx=0;sx<samples;++sx){double px=x+(sx+.5)/samples,py=y+(sy+.5)/samples;double dx=px<radius?radius-px:px>side-radius?px-(side-radius):0,dy=py<radius?radius-py:py>side-radius?py-(side-radius):0;covered+=dx*dx+dy*dy<=radius*radius;}pixels[(y*side+x)*4+3]=(covered*255+samples*samples/2)/(samples*samples);}return pixels;
}
}
