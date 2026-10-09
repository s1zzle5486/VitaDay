#include "../src/rounded_panel.hpp"
#include <cassert>
#include <iostream>
int main(){auto p=day::roundedPanelMask();assert(p.size()==32*32*4);bool fractional=false;for(int y=0;y<32;++y)for(int x=0;x<32;++x){auto a=p[(y*32+x)*4+3];assert(a==p[(y*32+31-x)*4+3]&&a==p[((31-y)*32+x)*4+3]);assert(p[(y*32+x)*4]==255);fractional|=a>0&&a<255;if(x>=8&&x<24||y>=8&&y<24)assert(a==255);}assert(fractional&&p[3]==0&&p[(8*32+8)*4+3]==255);std::cout<<"Rounded panel: fractional edge coverage, opaque interior, clear corners and symmetric mask passed\n";}
