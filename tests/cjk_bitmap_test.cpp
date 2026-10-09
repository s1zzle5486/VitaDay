#include "../src/cjk_bitmap.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>
using namespace day;
static void put32(std::vector<unsigned char>& v,unsigned n){for(int i=0;i<4;++i)v.push_back((n>>(i*8))&255);}
static void put16(std::vector<unsigned char>& v,int n){v.push_back(n&255);v.push_back((n>>8)&255);}
static std::vector<unsigned char> pack(){std::vector<unsigned char> v={'V','D','A','Y','B','F','0','1'};
 put32(v,48);put32(v,1);put32(v,28);put32(v,52);
 unsigned char pixels[]={0,64,128,255};unsigned char zipped[64];uLongf length=64;assert(compress(zipped,&length,pixels,4)==Z_OK);
 put32(v,0x65e5);put32(v,52);put32(v,length);put32(v,4);put16(v,2);put16(v,2);put16(v,-2);put16(v,3);put16(v,48*64);put16(v,0);v.insert(v.end(),zipped,zipped+length);return v;
}
static void write(const char* path,const std::vector<unsigned char>& v){std::ofstream f(path,std::ios::binary);f.write((char*)v.data(),v.size());}
int main(){const char* path="work/cjk-fixture.bin";auto valid=pack();write(path,valid);CjkBitmapFont f;assert(f.open(path)&&f.has(0x65e5)&&!f.has(0x4e2d));assert(f.count()==1);
 CjkGlyph g;assert(f.glyph(48,0x65e5,g)&&g.width==2&&g.height==2&&g.left==-2&&g.top==3&&g.advance==48);assert((g.alpha==std::vector<unsigned char>{0,64,128,255}));
 assert(f.glyph(24,0x65e5,g)&&g.width==1&&g.height==1&&g.alpha[0]==112&&g.advance==24);
 assert(f.glyph(96,0x65e5,g)&&g.width==4&&g.height==4&&g.alpha[0]==0&&g.alpha[15]==255);assert(!f.glyph(0,0x65e5,g)&&!f.glyph(129,0x65e5,g));assert(!f.glyph(18,0x4e2d,g));f.close();assert(!f.ready()&&!f.has(0x65e5)&&f.advance(18,0x65e5)==0);
 for(size_t n=0;n<valid.size();++n){auto broken=valid;broken.resize(n);write(path,broken);assert(!f.open(path));}
 for(int byte:{0,8,12,16,20,28,32,36,40,42}){auto broken=valid;broken[byte]=255;write(path,broken);assert(!f.open(path));}
 auto broken=valid;broken.back()^=255;write(path,broken);assert(f.open(path)&&!f.glyph(18,0x65e5,g));f.close();std::remove(path);
 assert(f.open("assets/cjk-bitmap.bin"));assert(f.count()>43000);for(unsigned cp:{0x65e5u,0x4e2du,0xd55cu,0x7e41u,0x20000u}){if(cp==0x20000&&!f.has(cp))continue;assert(f.has(cp)&&f.glyph(18,cp,g)&&g.advance>0&&!g.alpha.empty());}
 std::cout<<"Bitmap CJK: Unicode lookup, little-endian records, exact masks, area scaling, metrics, missing glyphs, malformed/truncated data and compressed payload checks passed\n";
}
