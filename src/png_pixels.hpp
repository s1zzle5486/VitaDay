#pragma once
#include "lodepng.h"
#include <fstream>
#include <vector>
#include <string>
// Decode independently of libvita2d's libpng/setjmp boundary.
// The file and dimension limits apply before allocating decoded pixels.
static bool readPngPixels(const std::string& path,std::vector<unsigned char>& rgba,unsigned& width,unsigned& height){
    rgba.clear();width=height=0;
    std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)return false;
    auto size=file.tellg();if(size<33||size>16*1024*1024)return false;
    std::vector<unsigned char> bytes(static_cast<size_t>(size));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))return false;
    lodepng::State decoder;
    if(lodepng_inspect(&width,&height,&decoder,bytes.data(),bytes.size())||!width||!height||width>2048||height>2048)return false;
    if(lodepng::decode(rgba,width,height,decoder,bytes)){rgba.clear();return false;}
    return rgba.size()==size_t(width)*height*4;
}
