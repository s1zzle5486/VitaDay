#include "localization.hpp"
#include "offline_weather.hpp"
#include "weather_chart.hpp"
#include "weather_cursor.hpp"
#include "weather_status.hpp"
#include "relative_time.hpp"
#include "text_fit.hpp"
#include "text_runs.hpp"
#include "cjk_bitmap.hpp"
#include "network.hpp"
#include "input.hpp"
#include "graph_stick.hpp"
#include "rounded_panel.hpp"
#include "control_guide.hpp"
#include "scrollbar.hpp"
#include "holiday_names.hpp"
#include "png_pixels.hpp"
#include "jpeg_pixels.hpp"
#include "color_picker.hpp"
#include <psp2/io/dirent.h>
#include <psp2/system_param.h>
#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#include <psp2/rtc.h>
#include <psp2/power.h>
#include <psp2/apputil.h>
#include <psp2/ime_dialog.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/io/stat.h>
#include <atomic>
#include <cstring>

#undef RGBA8
#define RGBA8(r,g,b,a) ((unsigned(r)&255u)|((unsigned(g)&255u)<<8)|((unsigned(b)&255u)<<16)|((unsigned(a)&255u)<<24))
extern "C" { unsigned int _newlib_heap_size_user=96*1024*1024; }

using namespace day;
static const std::string dir="ux0:data/VitaDay";
static State state;static int vitaLanguage=1;
static vita2d_font* font;
static std::map<int,vita2d_font*> fonts;static std::vector<unsigned char> latinBytes;
static int weatherDay=-1,forecastFocus=0,settingsTab=0,settingsRow=0,countryScroll=0,countryFocus=0,holidayScroll=0;
static bool refreshPending=false;
static std::string notice="", pendingKey;
static double graphMinute=720.;
static int page=0,settingsScroll=0,lookEdit=0,countryEdit=-1,countryReturnPage=2,graphMetric=0,fileFocus=0,fileScroll=0,colorTarget=0,paletteFocus=0,colorReturnTab=0,colorReturnRow=0,homeClockFocus=-1,clockScroll=0,cityEdit=-1,calendarAction=-1,cityFocus=-1,cityScroll=0,cityTarget=-1;
static int homeBlock=-1,homeFocus=200,wheelMonth=1,wheelYear=2026,wheelColumn=0;static bool monthPicker=false;
static bool calendarReader=false;static int readerScroll=0;static std::string readerText;
static int weatherRegion=-1,weatherPlaceFocus=0,weatherPlaceScroll=0,zoneFocus=0,zoneScroll=0;static bool homeCalendar=false,eventEditor=false;static HSV picker;
static Json cityResults=Json::array();static std::string citySearch,pendingCityQuery;
static int citySearchReturnTab=3,citySearchReturnRow=1;
static std::string countrySearch;static bool circleEnter=false;static Json timeZones=Json::object(),localizedCountries=Json::object(),englishCountries=Json::object(),cityNames=Json::object();
static bool closing=false;
static bool controlGuideOpen=false;static ControlGuideView controlGuideView;
static bool galleryRescan=false;static unsigned galleryGeneration=0;
static std::vector<std::string> backgroundFiles;static vita2d_texture* backgroundTexture=nullptr;static std::string loadedBackground;static std::map<std::string,vita2d_texture*> backgroundCache;static std::vector<std::string> backgroundRecent;
static long long utcNow(){SceDateTime d{};sceRtcGetCurrentClock(&d,0);time_t epoch=0;sceRtcGetTime_t(&d,&epoch);return epoch;}
static const Json& zoneRules(){static const Json empty=Json::object();auto it=timeZones.find("zones");return it==timeZones.end()?empty:*it;}
static int localOffset(){return state.location.valid?zoneOffset(zoneRules(),state.location.zone,utcNow(),state.location.offset):0;}
static SceDateTime clockAt(int offset){SceDateTime d{};sceRtcGetCurrentClock(&d,offset/60);return d;}
static Appearance& editingLook(){return lookEdit?state.nightLook:state.dayLook;}
static const Appearance& currentLook(){static Appearance look;bool preview=page==2&&(settingsTab==6||((settingsTab==7||settingsTab==9)&&(colorTarget==0||colorTarget==4)));SceDateTime d{};if(state.location.valid)d=clockAt(localOffset());else sceRtcGetCurrentClockLocalTime(&d);bool night=preview?lookEdit==1:nightTheme(state,d.hour);look=night?state.nightLook:state.dayLook;look.style=state.style;look.opacity=state.dayLook.opacity;look.cardRgb=-1;look.cardColor=0;look.theme=night?1:0;return look;}
static const Appearance& previewLook(){if(page==2&&settingsTab==0&&state.themeMode==2&&(settingsRow==5||settingsRow==6))return settingsRow==6?state.nightLook:state.dayLook;return currentLook();}
static std::string appearancePath(const Appearance& a){return a.background>=1&&a.background<=11&&a.background!=5?"app0:assets/background"+std::to_string(a.background)+".jpg":a.background==5?a.image:"";}
static void scanBackgrounds(){vita2d_wait_rendering_done();for(auto it=backgroundCache.begin();it!=backgroundCache.end();)if(it->first.rfind("app0:",0)!=0){if(it->second)vita2d_free_texture(it->second);backgroundRecent.erase(std::remove(backgroundRecent.begin(),backgroundRecent.end(),it->first),backgroundRecent.end());it=backgroundCache.erase(it);}else ++it;galleryRescan=true;++galleryGeneration;loadedBackground.clear();backgroundFiles.clear();SceUID fd=sceIoDopen((dir+"/backgrounds").c_str());if(fd<0)return;SceIoDirent entry{};while(sceIoDread(fd,&entry)>0&&backgroundFiles.size()<100){std::string name=entry.d_name;auto dot=name.rfind('.');std::string ext=dot==std::string::npos?"":upper(name.substr(dot));if(ext==".PNG"||ext==".JPG"||ext==".JPEG")backgroundFiles.push_back(dir+"/backgrounds/"+name);memset(&entry,0,sizeof(entry));}sceIoDclose(fd);std::sort(backgroundFiles.begin(),backgroundFiles.end());}
static bool imageSizeAllowed(const std::string& path){std::ifstream f(path,std::ios::binary);if(!f)return false;unsigned char b[24]{};f.read((char*)b,24);if(b[0]==137&&b[1]==80){unsigned w=(unsigned(b[16])<<24)|(b[17]<<16)|(b[18]<<8)|b[19],h=(unsigned(b[20])<<24)|(b[21]<<16)|(b[22]<<8)|b[23];return w>0&&h>0&&w<=2048&&h<=2048;}f.clear();f.seekg(2);if(b[0]!=255||b[1]!=216)return false;for(int n=0;n<1000&&f;++n){unsigned char c=0,code=0;f.read((char*)&c,1);if(c!=255)continue;do{f.read((char*)&code,1);}while(code==255&&f);if(code==0xda||code==0xd9)break;unsigned char len[2];f.read((char*)len,2);int length=(len[0]<<8)|len[1];if(length<2)break;if((code>=0xc0&&code<=0xc3)||(code>=0xc5&&code<=0xc7)||(code>=0xc9&&code<=0xcb)||(code>=0xcd&&code<=0xcf)){unsigned char info[5];f.read((char*)info,5);int h=(info[1]<<8)|info[2],w=(info[3]<<8)|info[4];return h>0&&w>0&&w<=2048&&h<=2048;}f.seekg(length-2,std::ios::cur);}return false;}
static vita2d_texture* loadPngTexture(const char* path){std::vector<unsigned char> rgba;unsigned w=0,h=0;if(!readPngPixels(path,rgba,w,h))return nullptr;auto* tex=vita2d_create_empty_texture(w,h);if(!tex)return nullptr;auto* pixels=static_cast<unsigned char*>(vita2d_texture_get_datap(tex));unsigned stride=vita2d_texture_get_stride(tex);for(unsigned y=0;y<h;++y)memcpy(pixels+y*stride,rgba.data()+size_t(y)*w*4,size_t(w)*4);return tex;}
// Decode one selected language off the rendering thread; never preload twenty posters.
static vita2d_texture* controlGuideTexture=nullptr;
static int controlGuideLanguage=-1,controlGuideJobLanguage=-1;
static SceUID controlGuideWorker=-1;static std::atomic<bool> controlGuideDone{false};
static std::vector<unsigned char> controlGuidePixels;static unsigned controlGuideWidth=0,controlGuideHeight=0;
static bool controlGuideFailed=false;static std::string controlGuideError;
static int controlGuideEntry(SceSize,void*){
    try{readControlGuide(controlGuidePath(controlGuideJobLanguage),controlGuidePixels,controlGuideWidth,controlGuideHeight,controlGuideError);}
    catch(...){controlGuidePixels.clear();controlGuideError="resource allocation failed";}controlGuideDone.store(true,std::memory_order_release);return 0;
}
static void releaseControlGuideTexture(){if(controlGuideTexture){vita2d_wait_rendering_done();vita2d_free_texture(controlGuideTexture);controlGuideTexture=nullptr;}}
static void prepareControlGuide(){
    if(!controlGuideOpen||controlGuideLanguage!=state.language){releaseControlGuideTexture();if(controlGuideLanguage!=state.language){controlGuideLanguage=state.language;controlGuideFailed=false;}}
    if(controlGuideWorker>=0&&controlGuideDone.load(std::memory_order_acquire)){
        sceKernelWaitThreadEnd(controlGuideWorker,nullptr,nullptr);sceKernelDeleteThread(controlGuideWorker);controlGuideWorker=-1;
        if(controlGuideOpen&&controlGuideJobLanguage==state.language){
            controlGuideFailed=controlGuidePixels.empty();
            if(!controlGuideFailed){auto previousType=vita2d_texture_get_alloc_memblock_type();
                vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE);
                controlGuideTexture=vita2d_create_empty_texture_format(controlGuideWidth,controlGuideHeight,SCE_GXM_TEXTURE_FORMAT_R5G6B5);
                vita2d_texture_set_alloc_memblock_type(previousType);
                if(controlGuideTexture){auto* to=(unsigned char*)vita2d_texture_get_datap(controlGuideTexture);unsigned stride=vita2d_texture_get_stride(controlGuideTexture);
                    for(unsigned y=0;y<controlGuideHeight;++y)memcpy(to+y*stride,controlGuidePixels.data()+size_t(y)*controlGuideWidth*2,size_t(controlGuideWidth)*2);
                    vita2d_texture_set_filters(controlGuideTexture,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);
                    controlGuideView.width=controlGuideWidth;controlGuideView.height=controlGuideHeight;
                }else {controlGuideFailed=true;controlGuideError="RGB565 texture allocation failed";}
            }
        }
        if(controlGuideOpen&&controlGuideJobLanguage==state.language){std::ofstream report(dir+"/control-guide-status.txt");if(report)report<<"Language="<<languageInfo(controlGuideJobLanguage).code<<" Path="<<controlGuidePath(controlGuideJobLanguage)<<" Size="<<controlGuideWidth<<"x"<<controlGuideHeight<<" Bytes="<<controlGuidePixels.size()<<" Loaded="<<bool(controlGuideTexture)<<" Error="<<controlGuideError<<"\n";}
        std::vector<unsigned char>().swap(controlGuidePixels);
    }
    if(controlGuideOpen&&!controlGuideTexture&&!controlGuideFailed&&controlGuideWorker<0&&!closing){
        controlGuideJobLanguage=state.language;controlGuideDone.store(false,std::memory_order_relaxed);
        controlGuideWorker=sceKernelCreateThread("VitaDayGuide",controlGuideEntry,0x10000100,256*1024,0,0,nullptr);
        if(controlGuideWorker<0){controlGuideFailed=true;controlGuideError="thread create "+std::to_string(controlGuideWorker);std::ofstream(dir+"/control-guide-status.txt")<<controlGuideError;return;}
        if(sceKernelStartThread(controlGuideWorker,0,nullptr)<0){sceKernelDeleteThread(controlGuideWorker);controlGuideWorker=-1;controlGuideFailed=true;controlGuideError="thread start failed";std::ofstream(dir+"/control-guide-status.txt")<<controlGuideError;}
    }
}
static bool drawControlGuideImage(){if(!controlGuideTexture)return false;float scale=controlGuideView.scale();vita2d_draw_texture_scale(controlGuideTexture,controlGuideView.x(),controlGuideView.y(),scale,scale);return true;}
static vita2d_texture* roundedTexture=nullptr;
static std::map<std::string,vita2d_texture*> controlTextures;
static void prepareUiShapes(){auto pixels=roundedPanelMask();roundedTexture=vita2d_create_empty_texture(32,32);if(roundedTexture){auto* data=(unsigned char*)vita2d_texture_get_datap(roundedTexture);int stride=vita2d_texture_get_stride(roundedTexture);for(int y=0;y<32;++y)memcpy(data+y*stride,pixels.data()+y*32*4,32*4);}
 for(const auto* name:{"cross","circle","square","triangle","dpad","left-stick","right-stick","shoulders","start","select"}){auto* tex=loadPngTexture((std::string("app0:assets/controls/")+name+".png").c_str());if(tex){vita2d_texture_set_filters(tex,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);controlTextures[name]=tex;}}
}
static void drawRoundedCard(int x,int y,int w,int h,unsigned color){if(!roundedTexture||w<16||h<16){vita2d_draw_rectangle(x,y,w,h,color);return;}
 vita2d_draw_rectangle(x+8,y,w-16,h,color);vita2d_draw_rectangle(x,y+8,8,h-16,color);vita2d_draw_rectangle(x+w-8,y+8,8,h-16,color);
 for(int bottom=0;bottom<2;++bottom)for(int right=0;right<2;++right)vita2d_draw_texture_tint_part(roundedTexture,x+(right?w-8:0),y+(bottom?h-8:0),right?24:0,bottom?24:0,8,8,color);
}
static void drawControlIcon(const std::string& name,int x,int y,int w,unsigned color){auto it=controlTextures.find(name);if(it==controlTextures.end())return;if(w==20)vita2d_draw_texture_tint_part_scale(it->second,x,y,16,0,32,32,20/32.f,20/32.f,color);else vita2d_draw_texture_tint_scale(it->second,x,y,w/64.f,20/32.f,color);}
static void prepareBackground(){
    auto path=appearancePath(currentLook());if(path==loadedBackground)return;loadedBackground=path;backgroundTexture=nullptr;if(path.empty())return;
    backgroundRecent.erase(std::remove(backgroundRecent.begin(),backgroundRecent.end(),path),backgroundRecent.end());backgroundRecent.push_back(path);
    auto cached=backgroundCache.find(path);if(cached!=backgroundCache.end()){backgroundTexture=cached->second;return;}
    vita2d_wait_rendering_done();
    // Drop unusually large previous backgrounds before allocating another one.
    for(auto it=backgroundCache.begin();it!=backgroundCache.end();)if(it->second&&uint64_t(vita2d_texture_get_width(it->second))*vita2d_texture_get_height(it->second)>1024*1024){vita2d_free_texture(it->second);backgroundRecent.erase(std::remove(backgroundRecent.begin(),backgroundRecent.end(),it->first),backgroundRecent.end());it=backgroundCache.erase(it);}else ++it;
    if(!imageSizeAllowed(path)){notice="PNG/JPEG: максимум 2048 × 2048";return;}
    auto dot=path.rfind('.');std::string ext=dot==std::string::npos?"":upper(path.substr(dot));backgroundTexture=ext==".PNG"?loadPngTexture(path.c_str()):vita2d_load_JPEG_file(path.c_str());backgroundCache[path]=backgroundTexture;
    if(!backgroundTexture)notice="Не удалось открыть изображение";
    while(backgroundCache.size()>2){auto old=backgroundRecent.front();backgroundRecent.erase(backgroundRecent.begin());auto it=backgroundCache.find(old);if(it!=backgroundCache.end()){if(it->second)vita2d_free_texture(it->second);backgroundCache.erase(it);}}
}
static void drawBackground(){if(!backgroundTexture)return;float w=vita2d_texture_get_width(backgroundTexture),h=vita2d_texture_get_height(backgroundTexture),scale=std::max(960.f/w,544.f/h);vita2d_draw_texture_scale(backgroundTexture,(960-w*scale)/2,(544-h*scale)/2,scale,scale);}

static const char* iconNames[]={"thermometer","droplets","droplet","wind","gauge","cloud","clock","palette","image","sun","cloud-sun","cloud-rain","cloud-snow","cloud-fog","cloud-lightning","calendar-days"};
static std::map<std::string,vita2d_texture*> uiTextures,thumbnails;
static void loadUiIcons(){for(const auto* name:iconNames)for(int size:{24,32}){std::string key=std::string(name)+"-"+std::to_string(size);auto tex=loadPngTexture(("app0:assets/icons/"+key+".png").c_str());if(tex){vita2d_texture_set_filters(tex,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);uiTextures[key]=tex;}}}
static void drawAssetIcon(int x,int y,int kind,int size,unsigned color){std::string key=std::string(iconNames[std::clamp(kind,0,15)])+"-"+std::to_string(size<=24?24:32);auto it=uiTextures.find(key);if(it!=uiTextures.end())vita2d_draw_texture_tint_scale(it->second,x-size/2,y-size/2,(float)size/vita2d_texture_get_width(it->second),(float)size/vita2d_texture_get_height(it->second),color);}
static std::string galleryPath(int index){if(index<=0)return "";if(index<=10)return "app0:assets/background"+std::to_string(galleryBackground(index))+".jpg";return index-11<(int)backgroundFiles.size()?backgroundFiles[index-11]:"";}
static std::map<std::string,uint64_t> thumbnailUsed;
static uint64_t galleryClock=0;
static SceUID imageWorker=-1;static std::atomic<bool> imageDone{false};
static std::string imageJobPath;static std::vector<unsigned char> imageJobPixels;
static unsigned imageJobGeneration=0;
static bool builtInPhoto(const std::string& path){return path.rfind("app0:assets/background",0)==0;}
static vita2d_texture* thumbnailTexture(const std::vector<unsigned char>& pixels){
    if(pixels.size()!=256*144*4)return nullptr;auto* texture=vita2d_create_empty_texture(256,144);if(!texture)return nullptr;
    auto* to=(unsigned char*)vita2d_texture_get_datap(texture);int stride=vita2d_texture_get_stride(texture);
    for(int y=0;y<144;++y)memcpy(to+y*stride,pixels.data()+y*256*4,256*4);
    vita2d_texture_set_filters(texture,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);return texture;
}
static void preloadGallery(){for(int index=1;index<=10;++index){auto path=galleryPath(index);std::string name=path.substr(path.rfind('/')+1);name.replace(name.rfind('.'),4,".rgba");
    std::ifstream file("app0:assets/thumbs/"+name,std::ios::binary);std::vector<unsigned char> pixels(256*144*4);
    if(file.read((char*)pixels.data(),pixels.size()))thumbnails[path]=thumbnailTexture(pixels);
}}
static int imageEntry(SceSize,void*){
    try{std::vector<unsigned char> source;unsigned width=0,height=0;auto dot=imageJobPath.rfind('.');std::string ext=dot==std::string::npos?"":upper(imageJobPath.substr(dot));
        bool ok=ext==".PNG"?readPngPixels(imageJobPath,source,width,height):readJpegPixels(imageJobPath,source,width,height);
        if(ok){imageJobPixels.resize(256*144*4);double scale=std::max(256./width,144./height);
            for(int y=0;y<144;++y)for(int x=0;x<256;++x){int sx=std::clamp(int((x-128)/scale+width/2.),0,int(width)-1),sy=std::clamp(int((y-72)/scale+height/2.),0,int(height)-1);memcpy(imageJobPixels.data()+(y*256+x)*4,source.data()+(size_t(sy)*width+sx)*4,4);}
        }
    }catch(...){imageJobPixels.clear();}imageDone.store(true,std::memory_order_release);return 0;
}
static void prepareGallery(){
    std::vector<std::string> needed;
    if(page==2&&settingsTab==6){fileFocus=std::clamp(fileFocus,0,std::max(0,10+(int)backgroundFiles.size()));if(fileFocus<fileScroll)fileScroll=fileFocus/3*3;if(fileFocus>=fileScroll+6)fileScroll=(fileFocus/3-1)*3;for(int i=fileScroll;i<fileScroll+6&&i<11+(int)backgroundFiles.size();++i){auto path=galleryPath(i);if(!path.empty())needed.push_back(path);}}
    else if(page==2&&settingsTab==0){auto path=appearancePath(previewLook());if(!path.empty())needed.push_back(path);}
    for(const auto& path:needed)thumbnailUsed[path]=++galleryClock;
    if(galleryRescan){vita2d_wait_rendering_done();for(auto it=thumbnails.begin();it!=thumbnails.end();)if(!builtInPhoto(it->first)){if(it->second)vita2d_free_texture(it->second);thumbnailUsed.erase(it->first);it=thumbnails.erase(it);}else ++it;galleryRescan=false;}
    if(imageWorker>=0&&imageDone.load(std::memory_order_acquire)){
        sceKernelWaitThreadEnd(imageWorker,nullptr,nullptr);sceKernelDeleteThread(imageWorker);imageWorker=-1;
        if(imageJobGeneration==galleryGeneration){thumbnails[imageJobPath]=thumbnailTexture(imageJobPixels);thumbnailUsed[imageJobPath]=++galleryClock;}
        imageJobPixels.clear();
    }
    // Keep ten built-ins and up to sixteen recent user previews across pages.
    while(thumbnails.size()>26){auto oldest=thumbnails.end();uint64_t age=UINT64_MAX;
        for(auto it=thumbnails.begin();it!=thumbnails.end();++it)if(!builtInPhoto(it->first)&&std::find(needed.begin(),needed.end(),it->first)==needed.end()&&thumbnailUsed[it->first]<age){oldest=it;age=thumbnailUsed[it->first];}
        if(oldest==thumbnails.end())break;vita2d_wait_rendering_done();if(oldest->second)vita2d_free_texture(oldest->second);thumbnailUsed.erase(oldest->first);thumbnails.erase(oldest);
    }
    if(imageWorker<0&&!closing)for(const auto& path:needed)if(!thumbnails.count(path)){
        imageJobPath=path;imageJobGeneration=galleryGeneration;imageJobPixels.clear();imageDone.store(false,std::memory_order_relaxed);
        imageWorker=sceKernelCreateThread("VitaDayImages",imageEntry,0x10000100,256*1024,0,0,nullptr);
        if(imageWorker<0){thumbnails[path]=nullptr;break;}
        if(sceKernelStartThread(imageWorker,0,nullptr)<0){sceKernelDeleteThread(imageWorker);imageWorker=-1;thumbnails[path]=nullptr;}break;
    }
}
static void drawThumbnail(const std::string& path,int x,int y,int w,int h){auto it=thumbnails.find(path);if(it!=thumbnails.end()&&it->second)vita2d_draw_texture_scale(it->second,x,y,w/256.f,h/144.f);}
static void drawGalleryImage(int index,int x,int y,int w,int h){drawThumbnail(galleryPath(index),x,y,w,h);}
static void drawAppearanceImage(int x,int y,int w,int h){auto it=thumbnails.find(appearancePath(previewLook()));if(it==thumbnails.end()||!it->second)return;float tw=256,th=144,scale=std::max(w/tw,h/th),cw=w/scale,ch=h/scale;vita2d_draw_texture_part_scale(it->second,x,y,(tw-cw)/2,(th-ch)/2,cw,ch,scale,scale);}
static vita2d_texture* pickerTexture=nullptr;static double pickerTextureHue=-1;
static void preparePickerField(){if(page!=2||settingsTab!=9)return;if(!pickerTexture){pickerTexture=vita2d_create_empty_texture(64,64);if(pickerTexture)vita2d_texture_set_filters(pickerTexture,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);}if(!pickerTexture||std::abs(pickerTextureHue-picker.h)<.1)return;vita2d_wait_rendering_done();auto* pixels=(unsigned char*)vita2d_texture_get_datap(pickerTexture);int stride=vita2d_texture_get_stride(pickerTexture);for(int y=0;y<64;++y)for(int x=0;x<64;++x){unsigned rgb=0xff000000u|hsvRgb({picker.h,x/63.,1-y/63.});memcpy(pixels+y*stride+x*4,&rgb,4);}pickerTextureHue=picker.h;}
static void drawPickerField(double){if(pickerTexture)vita2d_draw_texture_scale(pickerTexture,44,142,300.f/64,300.f/64);}
// One reusable chart texture: no per-frame gradient geometry or allocation.
static vita2d_texture* weatherChartTexture=nullptr;
static WeatherChart uploadedChart;static bool chartUploaded=false;
static void uploadWeatherChart(const WeatherChart& chart){
    if(chartUploaded&&chart==uploadedChart)return;
    vita2d_wait_rendering_done();
    if(!weatherChartTexture)weatherChartTexture=vita2d_create_empty_texture(WeatherChart::width,WeatherChart::height);
    if(!weatherChartTexture)return;
    auto pixels=chartPixels(chart);auto* target=(unsigned char*)vita2d_texture_get_datap(weatherChartTexture);
    int stride=vita2d_texture_get_stride(weatherChartTexture);
    for(int y=0;y<WeatherChart::height;++y)memcpy(target+y*stride,pixels.data()+y*WeatherChart::width*4,WeatherChart::width*4);
    uploadedChart=chart;chartUploaded=true;
}
static void drawWeatherChart(){if(weatherChartTexture)vita2d_draw_texture(weatherChartTexture,136,215);}
static Date selected;
static bool deleteConfirm=false;
static SceUID worker=-1;
static std::atomic<bool> done{false};
static Refresh job;
static void* netMemory=nullptr;
static bool netReady=false,wifiConnected=false,offlinePopup=false;
static uint16_t imeInput[256]{},imeInitial[256]{},imeTitle[64]{};
static int imeKind=0;
struct Palette {unsigned bg,card,ink,muted,accent,line;};
static Palette p;
static unsigned colors[]={0,RGBA8(77,163,245,255),RGBA8(75,189,143,255),RGBA8(236,179,69,255),RGBA8(236,119,143,255),RGBA8(161,131,236,255),RGBA8(234,130,78,255),RGBA8(64,191,202,255),RGBA8(203,161,111,255),RGBA8(213,77,82,255),RGBA8(156,193,67,255)};
static const char* months[]={"Январь","Февраль","Март","Апрель","Май","Июнь","Июль","Август","Сентябрь","Октябрь","Ноябрь","Декабрь"};
static const char* weekdays[]={"Пн","Вт","Ср","Чт","Пт","Сб","Вс"};
static const char* styles[]={"Минимализм","Цифровые","Аналоговые"};
static const char* t(const char* ru,const char* en){return localized(state.language,ru,en);}
static const char* monthName(int m){static const char* en[]={"January","February","March","April","May","June","July","August","September","October","November","December"};return localeDate(state.language,"months",m-1,state.language?en[m-1]:months[m-1]);}
[[maybe_unused]] static const char* dateMonth(int m){static const char* names[]={"января","февраля","марта","апреля","мая","июня","июля","августа","сентября","октября","ноября","декабря"};return localeDate(state.language,"dateMonths",m-1,state.language?monthName(m):names[m-1]);}
static std::string dateWords(Date d){return localizedDate(d,state.language);}
static const char* weekName(int w){static const char* en[]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};return localeDate(state.language,"weekdays",w,state.language?en[w]:weekdays[w]);}
static const char* styleName(int v){static const char* en[]={"Minimal","Digital","Analog"};return t(styles[v],en[v]);}
static bool readFont(const char* path,std::vector<unsigned char>& bytes){std::ifstream f(path,std::ios::binary);if(!f)return false;f.seekg(0,std::ios::end);auto n=f.tellg();if(n<=0||n>24*1024*1024)return false;f.seekg(0);bytes.resize((size_t)n);return bool(f.read((char*)bytes.data(),n));}
static vita2d_font* fontFor(int size){auto it=fonts.find(size);if(it!=fonts.end())return it->second;auto f=vita2d_load_font_mem(latinBytes.data(),latinBytes.size());if(!f)return font;fonts[size]=f;return f;}
static CjkBitmapFont cjkFace;
struct CjkAtlas {vita2d_texture* texture=nullptr;int x=1,y=1,row=0;};
struct CjkEntry {int page=-1,x=0,y=0,w=0,h=0,left=0,top=0,advance=0;};
static std::vector<CjkAtlas> cjkAtlases;
static std::map<uint64_t,CjkEntry> cjkGlyphs;
static std::map<uint64_t,int> cjkAdvances;
static bool cjkReset=false,cjkAttempted=false;
static void clearCjkAtlas(){for(auto& atlas:cjkAtlases)vita2d_free_texture(atlas.texture);cjkAtlases.clear();cjkGlyphs.clear();cjkAdvances.clear();cjkReset=false;}
static bool loadCjk(){
    if(cjkAttempted)return cjkFace.ready();cjkAttempted=true;
    bool ready=cjkFace.open("app0:assets/cjk-bitmap.bin");
    std::ofstream report(dir+"/font-status.txt",std::ios::trunc);
    if(report)report<<"CJK bitmap=1 Glyphs="<<cjkFace.count()<<" Ready="<<ready<<" Japanese="<<cjkFace.has(0x65e5)<<" Chinese="<<cjkFace.has(0x4e2d)<<" Korean="<<cjkFace.has(0xd55c)<<"\n";
    return ready;
}
static int cjkWidth(int size,const std::string& s){
    int width=0;if(!loadCjk())return vita2d_font_text_width(fontFor(size),size,s.c_str());
    for(size_t i=0;i<s.size();){auto cp=textCodepoint(s,i);uint64_t key=(uint64_t(size)<<32)|cp.value;auto it=cjkAdvances.find(key);
        if(it!=cjkAdvances.end())width+=it->second;
        else{int advance=cjkFace.has(cp.value)?cjkFace.advance(size,cp.value):vita2d_font_text_width(fontFor(size),size,s.substr(i,cp.bytes).c_str());
            width+=advance;if(cjkAdvances.size()<32768)cjkAdvances.emplace(key,advance);
        }i+=cp.bytes;
    }return width;
}
static const CjkEntry* cjkEntry(int size,unsigned cp){
    uint64_t key=(uint64_t(size)<<32)|cp;auto found=cjkGlyphs.find(key);if(found!=cjkGlyphs.end())return &found->second;
    CjkGlyph glyph;if(!loadCjk()||!cjkFace.glyph(size,cp,glyph))return nullptr;
    CjkEntry entry;entry.w=glyph.width;entry.h=glyph.height;entry.left=glyph.left;entry.top=glyph.top;entry.advance=glyph.advance;
    if(glyph.width&&glyph.height){
        constexpr int side=512;
        if(glyph.width+2>side||glyph.height+2>side)return nullptr;
        int page=0;
        for(;page<int(cjkAtlases.size());++page){auto& atlas=cjkAtlases[page];if(atlas.x+glyph.width+1>side){atlas.x=1;atlas.y+=atlas.row+1;atlas.row=0;}if(atlas.y+glyph.height+1<=side)break;}
        if(page==int(cjkAtlases.size())){
            if(cjkAtlases.size()>=8){cjkReset=true;return nullptr;}
            CjkAtlas atlas;atlas.texture=vita2d_create_empty_texture(side,side);if(!atlas.texture)return nullptr;
            auto stride=vita2d_texture_get_stride(atlas.texture);std::memset(vita2d_texture_get_datap(atlas.texture),0,stride*side);
            cjkAtlases.push_back(atlas);
        }
        auto& atlas=cjkAtlases[page];entry.page=page;entry.x=atlas.x;entry.y=atlas.y;
        auto* data=static_cast<unsigned char*>(vita2d_texture_get_datap(atlas.texture));unsigned stride=vita2d_texture_get_stride(atlas.texture);
        for(int y=0;y<glyph.height;++y)for(int x=0;x<glyph.width;++x){auto* pixel=data+(entry.y+y)*stride+(entry.x+x)*4;pixel[0]=pixel[1]=pixel[2]=255;pixel[3]=glyph.alpha[y*glyph.width+x];}
        atlas.x+=glyph.width+1;atlas.row=std::max(atlas.row,glyph.height);
    }
    return &cjkGlyphs.emplace(key,entry).first->second;
}
static int drawCjk(int x,int y,int size,const std::string& s,unsigned color){
    int start=x;
    for(size_t i=0;i<s.size();){auto cp=textCodepoint(s,i);auto* glyph=cjkEntry(size,cp.value);
        if(glyph){if(glyph->page>=0)vita2d_draw_texture_tint_part(cjkAtlases[glyph->page].texture,x+glyph->left,y-glyph->top,glyph->x,glyph->y,glyph->w,glyph->h,color);x+=glyph->advance;}
        else{std::string fallback=s.substr(i,cp.bytes);vita2d_font_draw_text(fontFor(size),x,y,color,size,fallback.c_str());x+=vita2d_font_text_width(fontFor(size),size,fallback.c_str());}
        i+=cp.bytes;
    }
    return x-start;
}

static void prepareFonts(){static int loadedLanguage=-1;if(loadedLanguage==state.language&&!cjkReset)return;vita2d_wait_rendering_done();clearCjkAtlas();if(loadedLanguage!=state.language){for(const auto& entry:fonts)vita2d_free_font(entry.second);fonts.clear();loadedLanguage=state.language;}}
static int textWidth(int size,const std::string& value){static std::map<std::pair<int,std::string>,int> widths;auto key=std::make_pair(size,value);auto found=widths.find(key);if(found!=widths.end())return found->second;int width=0;for(const auto& r:textRuns(value))width+=r.cjk?cjkWidth(size,r.value):vita2d_font_text_width(fontFor(size),size,r.value.c_str());if(widths.size()>=4096)widths.clear();widths.emplace(std::move(key),width);return width;}

static void palette(){
    if(currentLook().theme==0)p={RGBA8(240,239,232,255),RGBA8(251,250,245,255),RGBA8(28,44,45,255),RGBA8(92,110,111,255),RGBA8(25,125,112,255),RGBA8(213,219,211,255)};
    else if(currentLook().theme==1)p={RGBA8(13,22,30,255),RGBA8(23,35,46,255),RGBA8(235,242,247,255),RGBA8(151,175,189,255),RGBA8(90,222,193,255),RGBA8(47,65,80,255)};
    else p={RGBA8(26,24,20,255),RGBA8(40,36,27,255),RGBA8(246,222,165,255),RGBA8(169,152,115,255),RGBA8(255,183,67,255),RGBA8(76,66,45,255)};
    const auto& a=currentLook();if(a.background==0&&a.bgColor)p.bg=colors[a.bgColor];if(a.bgRgb>=0&&a.background==0)p.bg=0xff000000u|a.bgRgb;if(state.textRgb>=0){p.ink=0xff000000u|state.textRgb;p.muted=(p.ink&0xffffffu)|0xff000000u;}if(state.accentRgb>=0)p.accent=0xff000000u|state.accentRgb;p.card=(p.card&0xffffffu)|((unsigned)(a.opacity*255/100)<<24);
}
static void rect(float x,float y,float w,float h,unsigned color){vita2d_draw_rectangle(x,y,w,h,color);}
static void text(int x,int y,int size,const std::string& s,unsigned color=0){unsigned ink=color?color:p.ink;for(const auto& r:textRuns(s)){if(r.cjk)x+=drawCjk(x,y,size,r.value,ink);else{auto f=fontFor(size);vita2d_font_draw_text(f,x,y,ink,size,r.value.c_str());x+=vita2d_font_text_width(f,size,r.value.c_str());}}}
static void fit(int x,int y,int size,std::string s,int width,unsigned color=0){auto fitted=fitLabel(std::move(s),size,width,std::min(size,std::max(11,size-3)),textWidth);text(x,y,fitted.size,fitted.value,color);}
static std::vector<std::string> textLines(const std::string& source,int size,int width){
    std::vector<std::string> out;size_t start=0;
    do{size_t end=source.find('\n',start);std::string line=source.substr(start,end==std::string::npos?end:end-start);line.erase(std::remove(line.begin(),line.end(),'\r'),line.end());for(char& c:line)if(c=='\t')c=' ';line=trim(line);
        if(line.empty())out.push_back("");
        while(!line.empty()){if(textWidth(size,line)<=width){out.push_back(line);break;}size_t last=0;for(size_t n=0;n<line.size();){unsigned char c=line[n];size_t step=c<128?1:c<224?2:c<240?3:4;n=std::min(line.size(),n+step);if(textWidth(size,line.substr(0,n))>width)break;last=n;}if(!last){unsigned char c=line[0];last=std::min(line.size(),size_t(c<128?1:c<224?2:c<240?3:4));}size_t space=line.rfind(' ',last);size_t take=space!=std::string::npos&&space>0?space:last;out.push_back(line.substr(0,take));line=trim(line.substr(take));}
        if(end==std::string::npos)break;start=end+1;
    }while(start<=source.size());return out;
}
static void wrap(int x,int y,int size,const std::string& s,int width,int lines,unsigned color=0){auto rows=textLines(s,size,width);for(int row=0;row<lines&&row<(int)rows.size();++row){auto line=rows[row];if(row==lines-1&&rows.size()>(size_t)lines)line+="…";fit(x,y+row*(size+6),size,line,width,color);}}
static SceDateTime now(){SceDateTime n{};if(state.location.valid)n=clockAt(localOffset());else sceRtcGetCurrentClockLocalTime(&n);return n;}
static Date today(){auto n=now();return {n.year,n.month,n.day};}
static void persist(){if(!save(dir,state))notice="Не удалось сохранить: проверь место на карте";}
static int networkEntry(SceSize,void*){refresh(job);done.store(true,std::memory_order_release);return 0;}
static bool networkLoading(){return worker>=0||refreshPending;}
static bool checkConnection(){int status=SCE_NETCTL_STATE_DISCONNECTED;return netReady&&sceNetCtlInetGetState(&status)>=0&&status==SCE_NETCTL_STATE_CONNECTED;}
static void updateNetwork(bool forceIP=false,const std::string& query=""){
    if(worker>=0||imeKind){if(!query.empty())pendingCityQuery=query;else refreshPending=true;return;}refreshPending=false;if(!checkConnection()){wifiConnected=false;notice.clear();return;}wifiConnected=true;
    for(auto& c:state.countries)if(c.cityEnabled&&c.zone.empty()&&timeZones.contains("countries")&&timeZones["countries"].contains(c.code)&&!timeZones["countries"][c.code].empty())c.zone=defaultZone(c.code,timeZones["countries"][c.code].get<std::vector<std::string>>());
    job=Refresh{};job.state=state;job.origin=state.location;job.year=selected.y;job.query=query;
    if(forceIP)job.state.location.automatic=true;
    done.store(false);worker=sceKernelCreateThread("VitaDayNetwork",networkEntry,0x10000100,512*1024,0,0,nullptr);
    if(worker<0){notice="Не удалось запустить обновление";return;}
    if(sceKernelStartThread(worker,0,nullptr)<0){sceKernelDeleteThread(worker);worker=-1;notice="Ошибка запуска сети";return;}
    notice="Загрузка места, погоды и праздников…";
}
static void collectNetwork(){
    if(worker<0||!done.load(std::memory_order_acquire))return;
    sceKernelWaitThreadEnd(worker,nullptr,nullptr);sceKernelDeleteThread(worker);worker=-1;
    if(!job.query.empty()){cityResults=job.cities;cityFocus=cityResults.empty()?-1:0;cityScroll=0;notice=job.cities.empty()?job.message:"";}
    bool followToday=page==0&&key(selected)==key(today());
    if(job.query.empty()){state.networkReport=job.report;{std::ofstream f(std::string(dir)+"/network-last.txt");f<<state.networkReport;}if(job.any){mergeRefresh(state,job);notice=job.message==localized(job.state.language,"Обновлено","Updated")?"":job.message;persist();if(followToday)selected=today();}else {notice=job.message;persist();}}
    if(!pendingCityQuery.empty()){auto query=pendingCityQuery;pendingCityQuery.clear();updateNetwork(false,query);return;}
    if((job.year!=selected.y||refreshPending)&&!closing)updateNetwork();
}
static std::u16string utf16(const std::string& s){
    std::u16string out;for(size_t i=0;i<s.size();){unsigned c=(unsigned char)s[i++];int n=0;if(c>=0xf0){c&=7;n=3;}else if(c>=0xe0){c&=15;n=2;}else if(c>=0xc0){c&=31;n=1;}while(n--&&i<s.size())c=(c<<6)|((unsigned char)s[i++]&63);if(c>0xffff){c-=0x10000;out.push_back(0xd800+(c>>10));out.push_back(0xdc00+(c&1023));}else out.push_back(c);}return out;
}
static std::string utf8(const uint16_t* s){
    std::string out;for(int i=0;s[i]&&i<255;++i){unsigned c=s[i];if(c>=0xd800&&c<=0xdbff&&s[i+1]>=0xdc00&&s[i+1]<=0xdfff)c=0x10000+((c-0xd800)<<10)+(s[++i]-0xdc00);if(c<128)out+=(char)c;else if(c<2048){out+=(char)(0xc0|(c>>6));out+=(char)(0x80|(c&63));}else if(c<65536){out+=(char)(0xe0|(c>>12));out+=(char)(0x80|((c>>6)&63));out+=(char)(0x80|(c&63));}else {out+=(char)(0xf0|(c>>18));out+=(char)(0x80|((c>>12)&63));out+=(char)(0x80|((c>>6)&63));out+=(char)(0x80|(c&63));}}return out;
}
static void keyboard(int kind,const std::string& initial,const std::string& title){
    if(imeKind)return;auto a=utf16(initial),b=utf16(title);memset(imeInput,0,sizeof(imeInput));memset(imeInitial,0,sizeof(imeInitial));memset(imeTitle,0,sizeof(imeTitle));memcpy(imeInitial,a.data(),std::min(a.size(),size_t(120))*2);memcpy(imeTitle,b.data(),std::min(b.size(),size_t(63))*2);
    SceImeDialogParam param;sceImeDialogParamInit(&param);param.supportedLanguages=0xfffffULL;param.languagesForced=SCE_FALSE;param.type=SCE_IME_TYPE_DEFAULT;param.title=imeTitle;param.maxTextLength=120;param.initialText=imeInitial;param.inputTextBuffer=imeInput;
    if(sceImeDialogInit(&param)>=0)imeKind=kind;else notice="Не удалось открыть клавиатуру";
}
static void edit(){pendingKey=editKey(state,selected);keyboard(1,state.marks.count(pendingKey)?state.marks[pendingKey].title:"",t("Название события","Event title"));}
static void color(int value=-1){std::string k=editKey(state,selected);Mark& m=state.marks[k];m.color=value>=0?value:(m.color+1)%7;if(m.title.empty()&&m.color==0)state.marks.erase(k);persist();}
static void repeat(){auto k=editKey(state,selected);if(!state.marks.count(k)){notice="Сначала добавь событие или цвет";return;}state.marks[k].yearly=!state.marks[k].yearly;persist();}
static void deleteMark(){auto k=editKey(state,selected);if(!state.marks.count(k))return;if(!deleteConfirm){deleteConfirm=true;notice=t("Подтверди удаление ещё раз","Confirm deletion again");return;}state.marks.erase(k);deleteConfirm=false;notice="Отметка удалена";persist();}
static void pollKeyboard(){
    if(!imeKind||sceImeDialogGetStatus()!=SCE_COMMON_DIALOG_STATUS_FINISHED)return;SceImeDialogResult r{};sceImeDialogGetResult(&r);int kind=imeKind;imeKind=0;sceImeDialogTerm();if(r.button!=SCE_IME_DIALOG_BUTTON_ENTER)return;std::string value=trim(clipUtf8(utf8(imeInput),240));
    if(kind==1){Mark& m=state.marks[pendingKey];m.title=value;if(!m.color&&!m.title.size())state.marks.erase(pendingKey);persist();}
    else if(kind==4){countrySearch=value;countryFocus=countryScroll=0;}
    else if(kind==2&&!value.empty()){citySearch=value;settingsTab=8;page=2;cityResults=Json::array();cityFocus=-1;cityScroll=0;updateNetwork(false,value+(cityTarget>=0?", "+state.countries[cityTarget].code:usesPostal(state.location.country)&&validPostal(state.location.country,normalizePostal(state.location.country,value))?", "+state.location.country:""));}
    else if(kind==3){auto code=resolveRegion(state.location.country,value,state.language);if(!value.empty()&&code.empty()){notice=t("Регион не найден. Введи название или код","Region not found. Enter a name or code");return;}state.location.region=code;state.location.automatic=false;state.holidayYear=0;persist();updateNetwork();}
    else if(kind==5){value=normalizePostal(state.location.country,value);if(!validPostal(state.location.country,value)){notice=std::string(t("Неверный формат. Например: ","Invalid format. Example: "))+postalExample(state.location.country);return;}if(value.empty()){state.location.postal.clear();persist();return;}citySearchReturnTab=3;citySearchReturnRow=3;cityTarget=-1;citySearch=value;settingsTab=8;page=2;cityResults=Json::array();cityFocus=-1;cityScroll=0;updateNetwork(false,value+", "+state.location.country);}

}
static void button(int x,int y,int w,const std::string& title,bool active=false){rect(x,y,w,42,active?p.accent:p.line);auto fitted=fitLabel(title,18,w-24,15,textWidth);text(x+(w-textWidth(fitted.size,fitted.value))/2,y+28,fitted.size,fitted.value,active?p.bg:p.ink);}
static void header(){rect(0,0,960,65,p.card);text(20,39,24,"VitaDay",p.accent);const char* labels[]={t("Часы","Clock"),t("Календарь","Calendar"),t("Погода","Weather"),t("Настройки","Settings")};int pages[]={0,1,3,2};for(int i=0;i<4;++i){int x=350+i*148;if(page==pages[i])rect(x,13,138,38,p.line);auto label=fitLabel(labels[i],18,126,15,textWidth);text(x+(138-textWidth(label.size,label.value))/2,39,label.size,label.value);}rect(20,62,920,1,p.line);}
static void digitalDigit(int x,int y,int number,float scale){
    static const unsigned masks[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};unsigned bits=masks[number];
    const int seg[7][4]={{7,0,36,6},{43,7,6,36},{43,50,6,36},{7,87,36,6},{0,50,6,36},{0,7,6,36},{7,43,36,6}};
    for(int i=0;i<7;++i)rect(x+seg[i][0]*scale,y+seg[i][1]*scale,seg[i][2]*scale,seg[i][3]*scale,bits&(1<<i)?p.accent:p.line);
}
#include "ui.hpp"
int main(){
    sceIoMkdir("ux0:data",0777);sceIoMkdir(dir.c_str(),0777);sceIoMkdir((dir+"/backgrounds").c_str(),0777);state=load(dir);loadLanguages("app0:assets/locales");try{std::ifstream f("app0:assets/zones.json");f>>timeZones;}catch(...){}try{std::ifstream f("app0:assets/country-names-ru.json");f>>localizedCountries;}catch(...){}try{std::ifstream f("app0:assets/country-names-en.json");f>>englishCountries;}catch(...){}try{std::ifstream f("app0:assets/city-names.json");f>>cityNames;}catch(...){}try{if(state.catalog.empty()){std::ifstream f("app0:assets/countries.json");Json j;f>>j;for(const auto& c:j)state.catalog.push_back({c.at("countryCode"),c.at("name"),1});}}catch(...){}
    if(timeZones.contains("countries")&&pruneLegacyDefaults(state,timeZones["countries"]))persist();
    SceAppUtilInitParam init{};SceAppUtilBootParam boot{};sceAppUtilInit(&init,&boot);SceCommonDialogConfigParam config{};int enterButton=SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON,&enterButton);circleEnter=enterButton==SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE;config.enterButtonAssign=(SceSystemParamEnterButtonAssign)enterButton;int systemLanguage=SCE_SYSTEM_PARAM_LANG_ENGLISH_US;sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG,&systemLanguage);vitaLanguage=systemLanguageId(systemLanguage);applySystemLanguage(state,systemLanguage);config.language=(SceSystemParamLang)systemLanguage;sceCommonDialogSetConfigParam(&config);
    sceSysmoduleLoadModule(SCE_SYSMODULE_IME);sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,SCE_TOUCH_SAMPLING_STATE_START);
    vita2d_init();font=readFont("app0:assets/font.ttf",latinBytes)?vita2d_load_font_mem(latinBytes.data(),latinBytes.size()):nullptr;if(!font){vita2d_fini();sceKernelExitProcess(1);return 1;}
    prepareFonts();loadCjk();for(const auto& language:languages())for(const auto& run:textRuns(language.native))if(run.cjk)for(size_t i=0;i<run.value.size();){auto cp=textCodepoint(run.value,i);cjkEntry(18,cp.value);i+=cp.bytes;}
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);netMemory=malloc(4*1024*1024);SceNetInitParam np{};np.memory=netMemory;np.size=4*1024*1024;
    if(netMemory&&sceNetInit(&np)>=0){if(sceNetCtlInit()>=0&&curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK)netReady=true;}
    loadUiIcons();prepareUiShapes();preloadGallery();for(int size=11;size<=32;++size)fontFor(size);selected=today();wifiConnected=checkConnection();offlinePopup=!wifiConnected;updateNetwork();NavigationRepeat repeatInput,leftRepeat,rightRepeat;GraphStickMotion graphStick;bool heldTouch=false,dragging=false,continuousTouch=false;int touchX=0,touchY=0,lastTouchY=0;uint64_t nextRefresh=sceKernelGetProcessTimeWide()+900000000ULL,nextPowerTick=0,nextConnectionCheck=0,lastTouchStamp=0,lastFrame=sceKernelGetProcessTimeWide();
    while(true){
        if(state.keepScreen&&sceKernelGetProcessTimeWide()>=nextPowerTick){sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);nextPowerTick=sceKernelGetProcessTimeWide()+1000000ULL;}
        if(sceKernelGetProcessTimeWide()>=nextConnectionCheck){bool was=wifiConnected;wifiConnected=checkConnection();nextConnectionCheck=sceKernelGetProcessTimeWide()+2000000ULL;if(wifiConnected&&!was){offlinePopup=false;notice.clear();updateNetwork();}else if(!wifiConnected&&was)notice.clear();}
        collectNetwork();pollKeyboard();if(closing&&worker<0)break;
        if(!closing&&worker<0&&!imeKind&&sceKernelGetProcessTimeWide()>=nextRefresh){updateNetwork();nextRefresh=sceKernelGetProcessTimeWide()+900000000ULL;}
        SceCtrlData ctrl{};sceCtrlPeekBufferPositive(0,&ctrl,1);unsigned buttons=ctrl.buttons;
        unsigned leftStick=stickDirection(ctrl.lx,ctrl.ly,128,128,SCE_CTRL_LEFT,SCE_CTRL_RIGHT,SCE_CTRL_UP,SCE_CTRL_DOWN),rightStick=stickDirection(128,128,ctrl.rx,ctrl.ry,SCE_CTRL_LEFT,SCE_CTRL_RIGHT,SCE_CTRL_UP,SCE_CTRL_DOWN);
        unsigned pressed=repeatInput.poll(mappedButtons(buttons,circleEnter,SCE_CTRL_CROSS,SCE_CTRL_CIRCLE),SCE_CTRL_UP|SCE_CTRL_DOWN|SCE_CTRL_LEFT|SCE_CTRL_RIGHT,sceKernelGetProcessTimeWide());
        if((ctrl.buttons&(SCE_CTRL_START|SCE_CTRL_SELECT))==(SCE_CTRL_START|SCE_CTRL_SELECT)&&!imeKind){closing=true;notice="Закрытие…";}
        else{uint64_t frame=sceKernelGetProcessTimeWide();double dt=(frame-lastFrame)/1000000.;lastFrame=frame;controls(pressed);auto left=leftRepeat.poll(leftStick,SCE_CTRL_UP|SCE_CTRL_DOWN|SCE_CTRL_LEFT|SCE_CTRL_RIGHT,frame),right=rightRepeat.poll(rightStick,SCE_CTRL_UP|SCE_CTRL_DOWN|SCE_CTRL_LEFT|SCE_CTRL_RIGHT,frame);if(page==2&&settingsTab==9&&!imeKind){graphStick.reset();pickerAnalog(ctrl.lx,ctrl.ly,ctrl.rx,ctrl.ry,dt);}else{stickControls(left,false);if((page==0||page==3)&&weatherDay>=0&&!imeKind&&!offlinePopup){double move=graphStick.poll(ctrl.rx,ctrl.ry,frame);if(move)graphMinute=std::clamp(graphMinute+move,0.,double(graphMinuteLimit()));}else{graphStick.reset();stickControls(right,true);}}}
        SceTouchData samples[64]{};int count=sceTouchPeek(SCE_TOUCH_PORT_FRONT,samples,64);if(count>0)std::sort(samples,samples+count,[](const SceTouchData& a,const SceTouchData& b){return a.timeStamp<b.timeStamp;});
        for(int sample=0;sample<count;++sample){const auto& td=samples[sample];if(td.timeStamp<=lastTouchStamp)continue;lastTouchStamp=td.timeStamp;
            if(td.reportNum){int x=td.report[0].x/2,y=td.report[0].y/2;bool field=page==2&&settingsTab==9&&((x>=44&&x<=392&&y>=142&&y<=442)||(x>=458&&y>=151&&y<400));bool chart=(page==0||page==3)&&weatherDay>=0&&x>=136&&x<=607&&y>=174&&y<=395;
                if(!heldTouch){if(monthPicker)wheelColumn=x<520?0:1;touchX=x;touchY=lastTouchY=y;dragging=false;continuousTouch=field||chart;if(continuousTouch)touch(x,y);}
                else if(controlGuideOpen){if(std::abs(y-touchY)>8||std::abs(x-touchX)>8)dragging=true;}
                else{if(field||chart)touch(x,y);if(std::abs(y-touchY)>18||std::abs(x-touchX)>24)dragging=true;if(dragging&&!chart&&(monthPicker||!(page==2&&settingsTab==9))&&std::abs(y-lastTouchY)>=18){scrollGesture((lastTouchY-y)/18,touchX);lastTouchY=y;}}
            }else if(heldTouch&&!dragging&&!continuousTouch)touch(touchX,touchY);heldTouch=td.reportNum>0;
        }
        prepareControlGuide();prepareBackground();prepareGallery();preparePickerField();prepareFonts();palette();prepareWeatherChart();vita2d_set_clear_color(p.bg);vita2d_start_drawing();vita2d_clear_screen();if(!controlGuideOpen){drawBackground();header();if(page==0){if(weatherDay>=0)weatherDetails();else home();}else if(page==1){if(calendarReader)drawCalendarText();else if(eventEditor)eventSettings();else calendar();}else if(page==3){if(weatherDay>=0)weatherDetails();else weatherPlaces();}else settings();
        if(monthPicker)drawMonthPicker();if(offlinePopup)drawOfflineNotice();}
        if(!controlGuideOpen){if(!wifiConnected)fit(20,536,14,t("Офлайн: сохранённый прогноз. Обновление и поиск недоступны.","Offline: saved forecast. Updates and search unavailable."),920,p.accent);else if(!notice.empty())fit(20,536,14,displayNotice(),920,p.muted);else if(state.hints)drawControlHints();}else drawControlGuide();
        vita2d_end_drawing();if(imeKind)vita2d_common_dialog_update();vita2d_swap_buffers();
    }
    if(controlGuideWorker>=0){sceKernelWaitThreadEnd(controlGuideWorker,nullptr,nullptr);sceKernelDeleteThread(controlGuideWorker);}releaseControlGuideTexture();
    if(imageWorker>=0){sceKernelWaitThreadEnd(imageWorker,nullptr,nullptr);sceKernelDeleteThread(imageWorker);imageWorker=-1;}persist();if(netReady){curl_global_cleanup();sceNetCtlTerm();sceNetTerm();}free(netMemory);vita2d_wait_rendering_done();if(roundedTexture)vita2d_free_texture(roundedTexture);for(auto& entry:controlTextures)vita2d_free_texture(entry.second);if(pickerTexture)vita2d_free_texture(pickerTexture);if(weatherChartTexture)vita2d_free_texture(weatherChartTexture);for(const auto& entry:fonts)vita2d_free_font(entry.second);clearCjkAtlas();cjkFace.close();vita2d_free_font(font);for(auto& v:backgroundCache)if(v.second)vita2d_free_texture(v.second);for(auto& v:uiTextures)vita2d_free_texture(v.second);for(auto& v:thumbnails)if(v.second)vita2d_free_texture(v.second);vita2d_fini();sceAppUtilShutdown();sceKernelExitProcess(0);return 0;
}
