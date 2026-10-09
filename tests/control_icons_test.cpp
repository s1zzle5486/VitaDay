#include "../src/png_pixels.hpp"
#include <cassert>
#include <iostream>
int main(){for(const auto* name:{"cross","circle","square","triangle","dpad","left-stick","right-stick","shoulders","start","select"}){std::vector<unsigned char> pixels;unsigned w=0,h=0;assert(readPngPixels(std::string("assets/controls/")+name+".png",pixels,w,h));assert(w==64&&h==32);bool clear=false,opaque=false,edge=false;for(size_t i=3;i<pixels.size();i+=4){clear|=pixels[i]==0;opaque|=pixels[i]==255;edge|=pixels[i]>0&&pixels[i]<255;}assert(clear&&opaque&&edge);}std::cout<<"Ten controller glyphs: dimensions, decoding, clear background and antialiased edges passed\n";}
