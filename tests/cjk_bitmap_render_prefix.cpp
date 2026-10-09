#include <fstream>
#include <algorithm>
#include <map>
#include <cstring>
#include <cassert>
#include <set>
#include <iostream>
#include "../src/cjk_bitmap.hpp"
#include "../src/text_runs.hpp"
using namespace day;
struct vita2d_texture{std::vector<unsigned char> pixels;vita2d_texture(int w,int h):pixels(w*h*4){}};
vita2d_texture* vita2d_create_empty_texture(int w,int h){return new vita2d_texture(w,h);}
void vita2d_free_texture(vita2d_texture* p){delete p;}
unsigned vita2d_texture_get_stride(vita2d_texture*){return 512*4;}
void* vita2d_texture_get_datap(vita2d_texture* p){return p->pixels.data();}
static std::vector<unsigned char> canvas(960*240*4,255);
void vita2d_draw_texture_tint_part(vita2d_texture* tex,float fx,float fy,float fsx,float fsy,float fw,float fh,unsigned color){
 for(int y=0;y<int(fh);++y)for(int x=0;x<int(fw);++x){int dx=int(fx)+x,dy=int(fy)+y;if(dx<0||dx>=960||dy<0||dy>=240)continue;
 auto* src=tex->pixels.data()+((int(fsy)+y)*512+int(fsx)+x)*4;auto* dst=canvas.data()+(dy*960+dx)*4;unsigned a=src[3];
 for(int ch=0;ch<3;++ch)dst[ch]=(dst[ch]*(255-a)+((color>>(ch*8))&255)*a)/255;
 }
}
struct vita2d_font{};vita2d_font* fontFor(int){return nullptr;}
int vita2d_font_text_width(vita2d_font*,int,const char*){return 0;}
void vita2d_font_draw_text(vita2d_font*,int,int,unsigned,int,const char*){}
static const std::string dir="work";
