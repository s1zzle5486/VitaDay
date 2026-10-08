#include "../src/png_pixels.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
int main(int argc,char** argv){assert(argc==2);unsigned w=0,h=0;std::vector<unsigned char> pixels;int count=0;for(const auto& entry:std::filesystem::directory_iterator(argv[1])){if(entry.path().extension()!=".png")continue;assert(readPngPixels(entry.path().string(),pixels,w,h));bool small=entry.path().stem().string().find("-24")!=std::string::npos;assert(w==(small?24u:32u)&&h==w);assert(pixels.size()==w*h*4);bool transparent=false,visible=false;for(size_t i=3;i<pixels.size();i+=4){transparent|=pixels[i]==0;visible|=pixels[i]>0;}assert(transparent&&visible);++count;}assert(count==32);assert(!readPngPixels("missing-image.png",pixels,w,h));assert(pixels.empty());std::cout<<"All 32 shipped PNG icons decode to RGBA, dimensions and transparency passed\n";}
