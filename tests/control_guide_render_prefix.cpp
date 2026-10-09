#include "../src/control_guide.hpp"
#include "../src/png_pixels.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
using namespace day;
using SceUID=int;using SceSize=unsigned;
static State state;static bool closing=false,controlGuideOpen=false;static ControlGuideView controlGuideView;
static auto uiThread=std::this_thread::get_id();static int liveTextures=0;static bool failTexture=false;
static constexpr int SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE=2,SCE_GXM_TEXTURE_FORMAT_R5G6B5=3;
static int allocationType=1;
static int vita2d_texture_get_alloc_memblock_type(){return allocationType;}
static void vita2d_texture_set_alloc_memblock_type(int value){allocationType=value;}
struct vita2d_texture{unsigned w,h;std::vector<unsigned char> pixels;};
static vita2d_texture* vita2d_create_empty_texture_format(unsigned w,unsigned h,int format){assert(std::this_thread::get_id()==uiThread);assert(allocationType==SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE&&format==SCE_GXM_TEXTURE_FORMAT_R5G6B5);assert(size_t(w)*h*2<4*1024*1024);if(failTexture)return nullptr;++liveTextures;return new vita2d_texture{w,h,std::vector<unsigned char>(size_t((w+7)&~7)*h*2)};}
static void* vita2d_texture_get_datap(vita2d_texture* t){return t->pixels.data();}
static unsigned vita2d_texture_get_stride(vita2d_texture* t){return ((t->w+7)&~7)*2;}
static void vita2d_texture_set_filters(vita2d_texture*,int,int){}
static void vita2d_free_texture(vita2d_texture* t){assert(std::this_thread::get_id()==uiThread);--liveTextures;delete t;}
static void vita2d_wait_rendering_done(){}
static void vita2d_draw_texture_scale(vita2d_texture*,double,double,double,double){assert(std::this_thread::get_id()==uiThread);}
static constexpr int SCE_GXM_TEXTURE_FILTER_LINEAR=1;
static std::thread task;static int(*entry)(SceSize,void*)=nullptr;static std::atomic<bool> gate{true};static int launches=0,failThread=0;
static SceUID sceKernelCreateThread(const char*,int(*fn)(SceSize,void*),int priority,int,int,int,void*){assert(priority==0x10000100);entry=fn;return failThread==1?-1:1;}
static int sceKernelStartThread(int,int,void*){if(failThread==2)return -1;++launches;task=std::thread([]{while(!gate.load())std::this_thread::yield();entry(0,nullptr);});return 0;}
static int sceKernelWaitThreadEnd(int,void*,void*){task.join();return 0;}
static void sceKernelDeleteThread(int){}
static bool missingFile=false;
static std::string guideFile(int language){return missingFile?"/nonexistent-guide.vgi":assetRoot+"/control-guides/VitaDay-controls-"+languageInfo(language).code+".vgi";}

static const std::string dir=assetRoot+"/../work";
