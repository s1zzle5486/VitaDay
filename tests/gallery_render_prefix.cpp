#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>
#include "../src/jpeg_pixels.hpp"
#include "../src/png_pixels.hpp"
using SceUID=int;using SceSize=unsigned;
struct vita2d_texture {std::vector<unsigned char> pixels=std::vector<unsigned char>(256*144*4);};
static auto uiThread=std::this_thread::get_id();
static vita2d_texture* vita2d_create_empty_texture(int,int){assert(std::this_thread::get_id()==uiThread);return new vita2d_texture;}
static void* vita2d_texture_get_datap(vita2d_texture* t){return t->pixels.data();}
static int vita2d_texture_get_stride(vita2d_texture*){return 256*4;}
static void vita2d_texture_set_filters(vita2d_texture*,int,int){}
static void vita2d_free_texture(vita2d_texture* t){assert(std::this_thread::get_id()==uiThread);delete t;}
static void vita2d_wait_rendering_done(){}
static constexpr int SCE_GXM_TEXTURE_FILTER_LINEAR=1;
static std::thread task;static int(*entry)(SceSize,void*)=nullptr;static unsigned launched=0;static std::atomic<bool> gate{true};
static SceUID sceKernelCreateThread(const char*,int(*fn)(SceSize,void*),int priority,int,int,int,void*){assert(priority==0x10000100);entry=fn;return 1;}
static int sceKernelStartThread(int,int,void*){++launched;task=std::thread([]{while(!gate.load())std::this_thread::yield();entry(0,nullptr);});return 0;}
static int sceKernelWaitThreadEnd(int,void*,void*){task.join();return 0;}
static void sceKernelDeleteThread(int){}
static int page=2,settingsTab=6,fileFocus=0,fileScroll=0;static bool closing=false,galleryRescan=false;static unsigned galleryGeneration=0;
static std::vector<std::string> backgroundFiles;static std::map<std::string,vita2d_texture*> thumbnails;
static std::string upper(std::string s){for(char& c:s)c=toupper((unsigned char)c);return s;}
static std::string galleryPath(int i){return i==0?"":i<=10?"app0:assets/background"+std::to_string(i<5?i:i+1)+".jpg":backgroundFiles.at(i-11);}
static std::string previewPhoto;static const std::string& previewLook(){return previewPhoto;}static std::string appearancePath(const std::string& s){return s;}
