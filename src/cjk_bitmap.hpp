#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <vector>
#include <zlib.h>
namespace day {
struct CjkGlyph {int width=0,height=0,left=0,top=0,advance=0;std::vector<unsigned char> alpha;};
// Portable little-endian index + compressed alpha masks. No font engine or
// system language participates in glyph lookup, metrics or rasterization.
class CjkBitmapFont {
    struct Entry {uint32_t cp,offset,length,raw;int width,height,left,top,advance;};
    std::ifstream file;
    std::vector<Entry> entries;
    unsigned base=0;
    static uint32_t u32(const unsigned char* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
    static int i16(const unsigned char* p){return int(int16_t(uint16_t(p[0])|(uint16_t(p[1])<<8)));}
    const Entry* find(unsigned cp)const{auto it=std::lower_bound(entries.begin(),entries.end(),cp,[](const Entry& e,unsigned value){return e.cp<value;});return it!=entries.end()&&it->cp==cp?&*it:nullptr;}
public:
    void close(){file.close();file.clear();entries.clear();base=0;}
    bool ready()const{return base&&!entries.empty()&&file.is_open();}
    size_t count()const{return entries.size();}
    bool open(const char* path){
        close();file.open(path,std::ios::binary);if(!file)return false;
        file.seekg(0,std::ios::end);auto end=file.tellg();file.seekg(0);
        unsigned char header[24];if(end<24||end>128*1024*1024||!file.read((char*)header,24)){close();return false;}
        const unsigned char magic[]={ 'V','D','A','Y','B','F','0','1' };
        unsigned count=u32(header+12),stride=u32(header+16),start=u32(header+20),pixelBase=u32(header+8);
        if(!std::equal(header,header+8,magic)||pixelBase<16||pixelBase>128||!count||count>200000||stride!=28||uint64_t(count)*stride+24!=start||start>uint64_t(end)){close();return false;}
        entries.reserve(count);uint32_t previous=0;uint64_t previousEnd=start;
        for(unsigned i=0;i<count;++i){unsigned char raw[28];if(!file.read((char*)raw,28)){close();return false;}
            Entry e{u32(raw),u32(raw+4),u32(raw+8),u32(raw+12),i16(raw+16),i16(raw+18),i16(raw+20),i16(raw+22),i16(raw+24)};
            if((i&&e.cp<=previous)||e.cp>0x10ffff||(e.cp>=0xd800&&e.cp<=0xdfff)||e.width<0||e.height<0||e.width>256||e.height>256||e.raw!=unsigned(e.width*e.height)||e.advance<0||e.length>65536||e.offset<previousEnd||uint64_t(e.offset)+e.length>uint64_t(end)||(e.raw&&!e.length)||(!e.raw&&e.length)){close();return false;}
            entries.push_back(e);previous=e.cp;previousEnd=uint64_t(e.offset)+e.length;
        }base=pixelBase;return true;
    }
    bool has(unsigned cp)const{return ready()&&find(cp);}
    int advance(int size,unsigned cp)const{const auto* e=find(cp);return e&&size>0&&size<=128?int(std::lround(double(e->advance)*size/(base*64.))):0;}
    bool glyph(int size,unsigned cp,CjkGlyph& out){
        out=CjkGlyph{};const auto* e=find(cp);if(!ready()||!e||size<1||size>128)return false;
        out.advance=advance(size,cp);out.left=int(std::lround(double(e->left)*size/base));out.top=int(std::lround(double(e->top)*size/base));
        if(!e->raw)return true;
        std::vector<unsigned char> packed(e->length),pixels(e->raw);
        file.clear();file.seekg(e->offset);if(!file.read((char*)packed.data(),packed.size()))return false;
        uLongf length=pixels.size();if(uncompress(pixels.data(),&length,packed.data(),packed.size())!=Z_OK||length!=pixels.size())return false;
        out.width=(e->width*size+base-1)/base;out.height=(e->height*size+base-1)/base;
        out.alpha.resize(out.width*out.height);
        // Area averaging preserves thin strokes while shrinking the 48px mask.
        for(int y=0;y<out.height;++y)for(int x=0;x<out.width;++x){
            double x0=double(x)*base/size,x1=std::min(double(e->width),double(x+1)*base/size);
            double y0=double(y)*base/size,y1=std::min(double(e->height),double(y+1)*base/size),sum=0;
            for(int sy=int(y0);sy<int(std::ceil(y1));++sy)for(int sx=int(x0);sx<int(std::ceil(x1));++sx){double area=(std::min(x1,double(sx+1))-std::max(x0,double(sx)))*(std::min(y1,double(sy+1))-std::max(y0,double(sy)));sum+=pixels[sy*e->width+sx]*area;}
            out.alpha[y*out.width+x]=static_cast<unsigned char>(std::lround(sum/((x1-x0)*(y1-y0))));
        }return true;
    }
};
}
