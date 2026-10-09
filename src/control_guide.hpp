#pragma once
#include "localization.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <fstream>
#include <vector>
#include <cstring>
#include <cerrno>

namespace day {
inline std::string controlGuidePath(int language) {
    return std::string("app0:assets/control-guides/VitaDay-controls-") + languageInfo(language).code + ".vgi";
}
// Prepared RGB565 pixels: no PNG decompression or large temporary RGBA buffer on Vita.
inline bool readControlGuide(const std::string& path,std::vector<unsigned char>& pixels,unsigned& width,unsigned& height,std::string& error) {
    pixels.clear();width=height=0;error.clear();
    std::ifstream file(path,std::ios::binary);if(!file){error="open failed errno="+std::to_string(errno);return false;}
    unsigned char header[20]{};if(!file.read((char*)header,sizeof(header))){error="short header";return false;}
    auto u32=[&](int p){return unsigned(header[p])|(unsigned(header[p+1])<<8)|(unsigned(header[p+2])<<16)|(unsigned(header[p+3])<<24);};
    width=u32(8);height=u32(12);unsigned bytes=u32(16);
    if(std::memcmp(header,"VDAYGI01",8)||!width||!height||width>2048||height>2048||bytes!=size_t(width)*height*2){error="invalid header";width=height=0;return false;}
    pixels.resize(bytes);if(!file.read((char*)pixels.data(),bytes)){error="short pixel data";pixels.clear();return false;}
    char extra;if(file.get(extra)){error="trailing pixel data";pixels.clear();return false;}
    return true;
}
struct ControlGuideView {
    unsigned width=960,height=540;
    double scale() const {return std::min(960./width,544./height);}
    double x() const {return (960-width*scale())/2;}
    double y() const {return (544-height*scale())/2;}
};
}
