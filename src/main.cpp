#include "network.hpp"
#include "input.hpp"
#include "holiday_names.hpp"
#include "png_pixels.hpp"
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
#include <psp2/sysmodule.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/io/stat.h>
#include <atomic>
#include <cstring>

#undef RGBA8
#define RGBA8(r,g,b,a) ((unsigned(r)&255u)|((unsigned(g)&255u)<<8)|((unsigned(b)&255u)<<16)|((unsigned(a)&255u)<<24))
extern "C" { unsigned int _newlib_heap_size_user=64*1024*1024; }

using namespace day;
static const std::string dir="ux0:data/VitaDay";
static State state;
static vita2d_font* font;
static std::map<int,vita2d_font*> fonts;
static int weatherDay=-1,forecastFocus=0,settingsTab=0,settingsRow=0,countryScroll=0,countryFocus=0,holidayScroll=0;
static bool refreshPending=false;
static std::string notice="", pendingKey;
static int page=0,settingsScroll=0,lookEdit=0,countryEdit=-1,countryReturnPage=2,graphMetric=0,graphHour=12,fileFocus=0,fileScroll=0,colorTarget=0,paletteFocus=0,colorReturnTab=0,colorReturnRow=0,homeClockFocus=-1,clockScroll=0,cityEdit=-1,calendarAction=-1,cityFocus=-1,cityScroll=0,cityTarget=-1;
static int homeBlock=-1,homeFocus=200,wheelMonth=1,wheelYear=2026,wheelColumn=0;static bool monthPicker=false;
static bool calendarReader=false;static int readerScroll=0;static std::string readerText;
static int weatherRegion=-1,weatherPlaceFocus=0,weatherPlaceScroll=0,zoneFocus=0,zoneScroll=0;static bool homeCalendar=false,eventEditor=false;static HSV picker;
static Json cityResults=Json::array();static std::string citySearch,pendingCityQuery;
static std::string countrySearch;static bool circleEnter=false;static Json timeZones=Json::object(),localizedCountries=Json::object(),englishCountries=Json::object(),cityNames=Json::object();
static std::vector<std::string> backgroundFiles;static vita2d_texture* backgroundTexture=nullptr;static std::string loadedBackground;
static long long utcNow(){SceDateTime d{};sceRtcGetCurrentClock(&d,0);time_t epoch=0;sceRtcGetTime_t(&d,&epoch);return epoch;}
static const Json& zoneRules(){static const Json empty=Json::object();auto it=timeZones.find("zones");return it==timeZones.end()?empty:*it;}
static int localOffset(){return state.location.valid?zoneOffset(zoneRules(),state.location.zone,utcNow(),state.location.offset):0;}
static SceDateTime clockAt(int offset){SceDateTime d{};sceRtcGetCurrentClock(&d,offset/60);return d;}
static Appearance& editingLook(){return lookEdit?state.nightLook:state.dayLook;}
static const Appearance& currentLook(){static Appearance look;bool preview=page==2&&(settingsTab==6||((settingsTab==7||settingsTab==9)&&(colorTarget==0||colorTarget==4)));SceDateTime d{};if(state.location.valid)d=clockAt(localOffset());else sceRtcGetCurrentClockLocalTime(&d);bool night=preview?lookEdit==1:nightTheme(state,d.hour);look=night?state.nightLook:state.dayLook;look.style=state.style;look.opacity=state.dayLook.opacity;look.cardRgb=-1;look.cardColor=0;look.theme=night?1:0;return look;}
static void scanBackgrounds(){loadedBackground.clear();backgroundFiles.clear();SceUID fd=sceIoDopen((dir+"/backgrounds").c_str());if(fd<0)return;SceIoDirent entry{};while(sceIoDread(fd,&entry)>0&&backgroundFiles.size()<100){std::string name=entry.d_name;auto dot=name.rfind('.');std::string ext=dot==std::string::npos?"":upper(name.substr(dot));if(ext==".PNG"||ext==".JPG"||ext==".JPEG")backgroundFiles.push_back(dir+"/backgrounds/"+name);memset(&entry,0,sizeof(entry));}sceIoDclose(fd);std::sort(backgroundFiles.begin(),backgroundFiles.end());}
static bool imageSizeAllowed(const std::string& path){std::ifstream f(path,std::ios::binary);if(!f)return false;unsigned char b[24]{};f.read((char*)b,24);if(b[0]==137&&b[1]==80){unsigned w=(unsigned(b[16])<<24)|(b[17]<<16)|(b[18]<<8)|b[19],h=(unsigned(b[20])<<24)|(b[21]<<16)|(b[22]<<8)|b[23];return w>0&&h>0&&w<=2048&&h<=2048;}f.clear();f.seekg(2);if(b[0]!=255||b[1]!=216)return false;for(int n=0;n<1000&&f;++n){unsigned char c=0,code=0;f.read((char*)&c,1);if(c!=255)continue;do{f.read((char*)&code,1);}while(code==255&&f);if(code==0xda||code==0xd9)break;unsigned char len[2];f.read((char*)len,2);int length=(len[0]<<8)|len[1];if(length<2)break;if((code>=0xc0&&code<=0xc3)||(code>=0xc5&&code<=0xc7)||(code>=0xc9&&code<=0xcb)||(code>=0xcd&&code<=0xcf)){unsigned char info[5];f.read((char*)info,5);int h=(info[1]<<8)|info[2],w=(info[3]<<8)|info[4];return h>0&&w>0&&w<=2048&&h<=2048;}f.seekg(length-2,std::ios::cur);}return false;}
static vita2d_texture* loadPngTexture(const char* path){std::vector<unsigned char> rgba;unsigned w=0,h=0;if(!readPngPixels(path,rgba,w,h))return nullptr;auto* tex=vita2d_create_empty_texture(w,h);if(!tex)return nullptr;auto* pixels=static_cast<unsigned char*>(vita2d_texture_get_datap(tex));unsigned stride=vita2d_texture_get_stride(tex);for(unsigned y=0;y<h;++y)memcpy(pixels+y*stride,rgba.data()+size_t(y)*w*4,size_t(w)*4);return tex;}
static void prepareBackground(){const auto& a=currentLook();std::string path=a.background>=1&&a.background<=11&&a.background!=5?"app0:assets/background"+std::to_string(a.background)+".jpg":a.background==5?a.image:"";if(path==loadedBackground)return;vita2d_wait_rendering_done();if(backgroundTexture)vita2d_free_texture(backgroundTexture);backgroundTexture=nullptr;loadedBackground=path;if(path.empty())return;if(!imageSizeAllowed(path)){notice="PNG/JPEG: максимум 2048 × 2048";return;}auto dot=path.rfind('.');std::string ext=dot==std::string::npos?"":upper(path.substr(dot));backgroundTexture=ext==".PNG"?loadPngTexture(path.c_str()):vita2d_load_JPEG_file(path.c_str());if(!backgroundTexture)notice="Не удалось открыть изображение";}
static void drawBackground(){if(!backgroundTexture)return;float w=vita2d_texture_get_width(backgroundTexture),h=vita2d_texture_get_height(backgroundTexture),scale=std::max(960.f/w,544.f/h);vita2d_draw_texture_scale(backgroundTexture,(960-w*scale)/2,(544-h*scale)/2,scale,scale);}

static const char* iconNames[]={"thermometer","droplets","droplet","wind","gauge","cloud","clock","palette","image","sun","cloud-sun","cloud-rain","cloud-snow","cloud-fog","cloud-lightning","calendar-days"};
static std::map<std::string,vita2d_texture*> uiTextures,thumbnails;
static void loadUiIcons(){for(const auto* name:iconNames)for(int size:{24,32}){std::string key=std::string(name)+"-"+std::to_string(size);auto tex=loadPngTexture(("app0:assets/icons/"+key+".png").c_str());if(tex){vita2d_texture_set_filters(tex,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);uiTextures[key]=tex;}}}
static void drawAssetIcon(int x,int y,int kind,int size,unsigned color){std::string key=std::string(iconNames[std::clamp(kind,0,15)])+"-"+std::to_string(size<=24?24:32);auto it=uiTextures.find(key);if(it!=uiTextures.end())vita2d_draw_texture_tint_scale(it->second,x-size/2,y-size/2,(float)size/vita2d_texture_get_width(it->second),(float)size/vita2d_texture_get_height(it->second),color);}
static std::string galleryPath(int index){if(index<=0)return "";if(index<=10)return "app0:assets/background"+std::to_string(galleryBackground(index))+".jpg";return index-11<(int)backgroundFiles.size()?backgroundFiles[index-11]:"";}
static vita2d_texture* makeThumbnail(const std::string& path){if(!imageSizeAllowed(path))return nullptr;auto dot=path.rfind('.');std::string ext=dot==std::string::npos?"":upper(path.substr(dot));auto source=ext==".PNG"?loadPngTexture(path.c_str()):vita2d_load_JPEG_file(path.c_str());if(!source)return nullptr;auto format=vita2d_texture_get_format(source);int bpp=format==SCE_GXM_TEXTURE_FORMAT_A8B8G8R8?4:format==SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR?3:0;if(!bpp){vita2d_free_texture(source);return nullptr;}auto target=vita2d_create_empty_texture(256,144);if(!target){vita2d_free_texture(source);return nullptr;}int sw=vita2d_texture_get_width(source),sh=vita2d_texture_get_height(source),ss=vita2d_texture_get_stride(source),ts=vita2d_texture_get_stride(target);auto* from=(unsigned char*)vita2d_texture_get_datap(source);auto* to=(unsigned char*)vita2d_texture_get_datap(target);float scale=std::max(256.f/sw,144.f/sh);for(int y=0;y<144;++y)for(int x=0;x<256;++x){int sx=std::clamp((int)((x-128)/scale+sw/2.f),0,sw-1),sy=std::clamp((int)((y-72)/scale+sh/2.f),0,sh-1);auto* src=from+sy*ss+sx*bpp;auto* dst=to+y*ts+x*4;dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=bpp==4?src[3]:255;}vita2d_free_texture(source);vita2d_texture_set_filters(target,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);return target;}
static void prepareGallery(){if(page!=2||settingsTab!=6){if(!thumbnails.empty()){vita2d_wait_rendering_done();for(auto& p:thumbnails)if(p.second)vita2d_free_texture(p.second);thumbnails.clear();}return;}fileFocus=std::clamp(fileFocus,0,std::max(0,10+(int)backgroundFiles.size()));if(fileFocus<fileScroll)fileScroll=fileFocus/3*3;if(fileFocus>=fileScroll+6)fileScroll=(fileFocus/3-1)*3;std::vector<std::string> needed;for(int i=fileScroll;i<fileScroll+6&&i<11+(int)backgroundFiles.size();++i){std::string path=galleryPath(i);if(!path.empty())needed.push_back(path);}vita2d_wait_rendering_done();for(auto it=thumbnails.begin();it!=thumbnails.end();){if(std::find(needed.begin(),needed.end(),it->first)==needed.end()){if(it->second)vita2d_free_texture(it->second);it=thumbnails.erase(it);}else ++it;}for(const auto& path:needed)if(!thumbnails.count(path))thumbnails[path]=makeThumbnail(path);}
static void drawGalleryImage(int index,int x,int y,int w,int h){auto it=thumbnails.find(galleryPath(index));if(it!=thumbnails.end()&&it->second)vita2d_draw_texture_scale(it->second,x,y,w/256.f,h/144.f);}
static vita2d_texture* pickerTexture=nullptr;static double pickerTextureHue=-1;
static void preparePickerField(){if(page!=2||settingsTab!=9)return;if(!pickerTexture){pickerTexture=vita2d_create_empty_texture(64,64);if(pickerTexture)vita2d_texture_set_filters(pickerTexture,SCE_GXM_TEXTURE_FILTER_LINEAR,SCE_GXM_TEXTURE_FILTER_LINEAR);}if(!pickerTexture||std::abs(pickerTextureHue-picker.h)<.1)return;vita2d_wait_rendering_done();auto* pixels=(unsigned char*)vita2d_texture_get_datap(pickerTexture);int stride=vita2d_texture_get_stride(pickerTexture);for(int y=0;y<64;++y)for(int x=0;x<64;++x){unsigned rgb=0xff000000u|hsvRgb({picker.h,x/63.,1-y/63.});memcpy(pixels+y*stride+x*4,&rgb,4);}pickerTextureHue=picker.h;}
static void drawPickerField(double){if(pickerTexture)vita2d_draw_texture_scale(pickerTexture,44,142,300.f/64,300.f/64);}
static Date selected;
static bool deleteConfirm=false,closing=false;
static SceUID worker=-1;
static std::atomic<bool> done{false};
static Refresh job;
static void* netMemory=nullptr;
static bool netReady=false;
static uint16_t imeInput[256]{},imeInitial[256]{},imeTitle[64]{};
static int imeKind=0;
struct Palette {unsigned bg,card,ink,muted,accent,line;};
static Palette p;
static unsigned colors[]={0,RGBA8(77,163,245,255),RGBA8(75,189,143,255),RGBA8(236,179,69,255),RGBA8(236,119,143,255),RGBA8(161,131,236,255),RGBA8(234,130,78,255),RGBA8(64,191,202,255),RGBA8(203,161,111,255),RGBA8(213,77,82,255),RGBA8(156,193,67,255)};
static const char* months[]={"Январь","Февраль","Март","Апрель","Май","Июнь","Июль","Август","Сентябрь","Октябрь","Ноябрь","Декабрь"};
static const char* weekdays[]={"Пн","Вт","Ср","Чт","Пт","Сб","Вс"};
static const char* styles[]={"Минимализм","Цифровые","Аналоговые"};
static const char* t(const char* ru,const char* en){return state.language?en:ru;}
static const char* monthName(int m){static const char* en[]={"January","February","March","April","May","June","July","August","September","October","November","December"};return state.language?en[m-1]:months[m-1];}
static const char* dateMonth(int m){static const char* names[]={"января","февраля","марта","апреля","мая","июня","июля","августа","сентября","октября","ноября","декабря"};return state.language?monthName(m):names[m-1];}
static std::string dateWords(Date d){return std::to_string(d.d)+" "+dateMonth(d.m)+" "+std::to_string(d.y);}
static const char* weekName(int w){static const char* en[]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};return state.language?en[w]:weekdays[w];}
static const char* styleName(int v){static const char* en[]={"Minimal","Digital","Analog"};return state.language?en[v]:styles[v];}
static vita2d_font* fontFor(int size){auto it=fonts.find(size);if(it!=fonts.end())return it->second;auto f=vita2d_load_font_file("app0:assets/font.ttf");if(!f)return font;fonts[size]=f;return f;}
static int textWidth(int size,const std::string& value){return vita2d_font_text_width(fontFor(size),size,value.c_str());}
static void palette(){
    if(currentLook().theme==0)p={RGBA8(240,239,232,255),RGBA8(251,250,245,255),RGBA8(28,44,45,255),RGBA8(92,110,111,255),RGBA8(25,125,112,255),RGBA8(213,219,211,255)};
    else if(currentLook().theme==1)p={RGBA8(13,22,30,255),RGBA8(23,35,46,255),RGBA8(235,242,247,255),RGBA8(151,175,189,255),RGBA8(90,222,193,255),RGBA8(47,65,80,255)};
    else p={RGBA8(26,24,20,255),RGBA8(40,36,27,255),RGBA8(246,222,165,255),RGBA8(169,152,115,255),RGBA8(255,183,67,255),RGBA8(76,66,45,255)};
    const auto& a=currentLook();if(a.background==0&&a.bgColor)p.bg=colors[a.bgColor];if(a.bgRgb>=0&&a.background==0)p.bg=0xff000000u|a.bgRgb;if(state.textRgb>=0){p.ink=0xff000000u|state.textRgb;p.muted=(p.ink&0xffffffu)|0xff000000u;}if(state.accentRgb>=0)p.accent=0xff000000u|state.accentRgb;p.card=(p.card&0xffffffu)|((unsigned)(a.opacity*255/100)<<24);
}
static void rect(float x,float y,float w,float h,unsigned color){vita2d_draw_rectangle(x,y,w,h,color);}
static void text(int x,int y,int size,const std::string& s,unsigned color=0){vita2d_font_draw_text(fontFor(size),x,y,color?color:p.ink,size,s.c_str());}
static void fit(int x,int y,int size,std::string s,int width,unsigned color=0){
    for(char& c:s)if(c=='\n'||c=='\r'||c=='\t')c=' ';if(textWidth(size,s)>width){while(!s.empty()&&textWidth(size,s+"…")>width){s=clipUtf8(s,s.size()-1);}s+="…";}text(x,y,size,s,color);
}
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
static void updateNetwork(bool forceIP=false,const std::string& query=""){
    if(worker>=0||imeKind){if(!query.empty())pendingCityQuery=query;else refreshPending=true;return;}refreshPending=false;if(!netReady){notice="Сеть недоступна. Календарь работает офлайн";return;}
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
    if(job.query.empty()){state.networkReport=job.report;{std::ofstream f(std::string(dir)+"/network-last.txt");f<<state.networkReport;}if(job.any){mergeRefresh(state,job);notice=job.message=="Обновлено"||job.message=="Updated"?"":job.message;persist();if(followToday)selected=today();}else {notice=job.message;persist();}}
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
    SceImeDialogParam param;sceImeDialogParamInit(&param);param.supportedLanguages=SCE_IME_LANGUAGE_RUSSIAN|SCE_IME_LANGUAGE_ENGLISH;param.languagesForced=SCE_FALSE;param.type=SCE_IME_TYPE_DEFAULT;param.title=imeTitle;param.maxTextLength=120;param.initialText=imeInitial;param.inputTextBuffer=imeInput;
    if(sceImeDialogInit(&param)>=0)imeKind=kind;else notice="Не удалось открыть клавиатуру";
}
static void edit(){pendingKey=editKey(state,selected);keyboard(1,state.marks.count(pendingKey)?state.marks[pendingKey].title:"",t("Название события","Event title"));}
static void color(int value=-1){std::string k=editKey(state,selected);Mark& m=state.marks[k];m.color=value>=0?value:(m.color+1)%7;if(m.title.empty()&&m.color==0)state.marks.erase(k);persist();}
static void repeat(){auto k=editKey(state,selected);if(!state.marks.count(k)){notice="Сначала добавь событие или цвет";return;}state.marks[k].yearly=!state.marks[k].yearly;persist();}
static void deleteMark(){auto k=editKey(state,selected);if(!state.marks.count(k))return;if(!deleteConfirm){deleteConfirm=true;notice=state.language?"Confirm deletion again":"Подтверди удаление ещё раз";return;}state.marks.erase(k);deleteConfirm=false;notice="Отметка удалена";persist();}
static void pollKeyboard(){
    if(!imeKind||sceImeDialogGetStatus()!=SCE_COMMON_DIALOG_STATUS_FINISHED)return;SceImeDialogResult r{};sceImeDialogGetResult(&r);int kind=imeKind;imeKind=0;sceImeDialogTerm();if(r.button!=SCE_IME_DIALOG_BUTTON_ENTER)return;std::string value=trim(clipUtf8(utf8(imeInput),240));
    if(kind==1){Mark& m=state.marks[pendingKey];m.title=value;if(!m.color&&!m.title.size())state.marks.erase(pendingKey);persist();}
    else if(kind==4){countrySearch=value;countryFocus=countryScroll=0;}
    else if(kind==2&&!value.empty()){citySearch=value;settingsTab=8;page=2;cityResults=Json::array();cityFocus=-1;cityScroll=0;updateNetwork(false,value+(cityTarget>=0?", "+state.countries[cityTarget].code:usesPostal(state.location.country)&&validPostal(state.location.country,normalizePostal(state.location.country,value))?", "+state.location.country:""));}
    else if(kind==3){auto code=resolveRegion(state.location.country,value);if(!value.empty()&&code.empty()){notice=t("Регион не найден. Введи название или код","Region not found. Enter a name or code");return;}state.location.region=code;state.location.automatic=false;state.holidayYear=0;persist();updateNetwork();}
    else if(kind==5){value=normalizePostal(state.location.country,value);if(!validPostal(state.location.country,value)){notice=std::string(t("Неверный формат. Например: ","Invalid format. Example: "))+postalExample(state.location.country);return;}if(value.empty()){state.location.postal.clear();persist();return;}cityTarget=-1;citySearch=value;settingsTab=8;page=2;cityResults=Json::array();cityFocus=-1;cityScroll=0;updateNetwork(false,value+", "+state.location.country);}

}
static void button(int x,int y,int w,const std::string& title,bool active=false){rect(x,y,w,42,active?p.accent:p.line);int width=textWidth(18,title);fit(width<=w-24?x+(w-width)/2:x+12,y+28,18,title,w-24,active?p.bg:p.ink);}
static void header(){rect(0,0,960,65,p.card);text(20,39,24,"VitaDay",p.accent);const char* labels[]={t("Часы","Clock"),t("Календарь","Calendar"),t("Погода","Weather"),t("Настройки","Settings")};int pages[]={0,1,3,2};for(int i=0;i<4;++i){int x=350+i*148;if(page==pages[i])rect(x,13,138,38,p.line);int width=textWidth(18,labels[i]);text(x+(138-width)/2,39,18,labels[i]);}rect(20,62,920,1,p.line);}
static void digitalDigit(int x,int y,int number,float scale){
    static const unsigned masks[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};unsigned bits=masks[number];
    const int seg[7][4]={{7,0,36,6},{43,7,6,36},{43,50,6,36},{7,87,36,6},{0,50,6,36},{0,7,6,36},{7,43,36,6}};
    for(int i=0;i<7;++i)rect(x+seg[i][0]*scale,y+seg[i][1]*scale,seg[i][2]*scale,seg[i][3]*scale,bits&(1<<i)?p.accent:p.line);
}
#include "ui.hpp"
int main(){
    sceIoMkdir("ux0:data",0777);sceIoMkdir(dir.c_str(),0777);sceIoMkdir((dir+"/backgrounds").c_str(),0777);state=load(dir);try{std::ifstream f("app0:assets/zones.json");f>>timeZones;}catch(...){}try{std::ifstream f("app0:assets/country-names-ru.json");f>>localizedCountries;}catch(...){}try{std::ifstream f("app0:assets/country-names-en.json");f>>englishCountries;}catch(...){}try{std::ifstream f("app0:assets/city-names.json");f>>cityNames;}catch(...){}try{if(state.catalog.empty()){std::ifstream f("app0:assets/countries.json");Json j;f>>j;for(const auto& c:j)state.catalog.push_back({c.at("countryCode"),c.at("name"),1});}}catch(...){}
    if(timeZones.contains("countries")&&pruneLegacyDefaults(state,timeZones["countries"]))persist();
    SceAppUtilInitParam init{};SceAppUtilBootParam boot{};sceAppUtilInit(&init,&boot);SceCommonDialogConfigParam config{};int enterButton=SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON,&enterButton);circleEnter=enterButton==SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE;config.enterButtonAssign=(SceSystemParamEnterButtonAssign)enterButton;int systemLanguage=SCE_SYSTEM_PARAM_LANG_ENGLISH_US;sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG,&systemLanguage);config.language=(SceSystemParamLang)systemLanguage;sceCommonDialogSetConfigParam(&config);
    sceSysmoduleLoadModule(SCE_SYSMODULE_IME);sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,SCE_TOUCH_SAMPLING_STATE_START);
    vita2d_init();font=vita2d_load_font_file("app0:assets/font.ttf");if(!font){vita2d_fini();sceKernelExitProcess(1);return 1;}
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);netMemory=malloc(4*1024*1024);SceNetInitParam np{};np.memory=netMemory;np.size=4*1024*1024;
    if(netMemory&&sceNetInit(&np)>=0){if(sceNetCtlInit()>=0&&curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK)netReady=true;}
    loadUiIcons();selected=today();updateNetwork();NavigationRepeat repeatInput,leftRepeat,rightRepeat;bool heldTouch=false,dragging=false,continuousTouch=false;int touchX=0,touchY=0,lastTouchY=0;uint64_t nextRefresh=sceKernelGetProcessTimeWide()+900000000ULL,nextPowerTick=0,lastTouchStamp=0,lastFrame=sceKernelGetProcessTimeWide();
    while(true){
        if(state.keepScreen&&sceKernelGetProcessTimeWide()>=nextPowerTick){sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);nextPowerTick=sceKernelGetProcessTimeWide()+1000000ULL;}
        collectNetwork();pollKeyboard();if(closing&&worker<0)break;
        if(!closing&&worker<0&&!imeKind&&sceKernelGetProcessTimeWide()>=nextRefresh){updateNetwork();nextRefresh=sceKernelGetProcessTimeWide()+900000000ULL;}
        SceCtrlData ctrl{};sceCtrlPeekBufferPositive(0,&ctrl,1);unsigned buttons=ctrl.buttons;
        unsigned leftStick=stickDirection(ctrl.lx,ctrl.ly,128,128,SCE_CTRL_LEFT,SCE_CTRL_RIGHT,SCE_CTRL_UP,SCE_CTRL_DOWN),rightStick=stickDirection(128,128,ctrl.rx,ctrl.ry,SCE_CTRL_LEFT,SCE_CTRL_RIGHT,SCE_CTRL_UP,SCE_CTRL_DOWN);
        unsigned pressed=repeatInput.poll(mappedButtons(buttons,circleEnter,SCE_CTRL_CROSS,SCE_CTRL_CIRCLE),SCE_CTRL_UP|SCE_CTRL_DOWN|SCE_CTRL_LEFT|SCE_CTRL_RIGHT,sceKernelGetProcessTimeWide());
        if((ctrl.buttons&(SCE_CTRL_START|SCE_CTRL_SELECT))==(SCE_CTRL_START|SCE_CTRL_SELECT)&&!imeKind){closing=true;notice="Закрытие…";}
        else{uint64_t frame=sceKernelGetProcessTimeWide();double dt=(frame-lastFrame)/1000000.;lastFrame=frame;controls(pressed);auto left=leftRepeat.poll(leftStick,SCE_CTRL_UP|SCE_CTRL_DOWN|SCE_CTRL_LEFT|SCE_CTRL_RIGHT,frame),right=rightRepeat.poll(rightStick,SCE_CTRL_UP|SCE_CTRL_DOWN|SCE_CTRL_LEFT|SCE_CTRL_RIGHT,frame);if(page==2&&settingsTab==9&&!imeKind)pickerAnalog(ctrl.lx,ctrl.ly,ctrl.rx,ctrl.ry,dt);else{stickControls(left,false);stickControls(right,true);}}
        SceTouchData samples[64]{};int count=sceTouchPeek(SCE_TOUCH_PORT_FRONT,samples,64);if(count>0)std::sort(samples,samples+count,[](const SceTouchData& a,const SceTouchData& b){return a.timeStamp<b.timeStamp;});
        for(int sample=0;sample<count;++sample){const auto& td=samples[sample];if(td.timeStamp<=lastTouchStamp)continue;lastTouchStamp=td.timeStamp;
            if(td.reportNum){int x=td.report[0].x/2,y=td.report[0].y/2;bool field=page==2&&settingsTab==9&&((x>=44&&x<=392&&y>=142&&y<=442)||(x>=458&&y>=151&&y<400));bool chart=(page==0||page==3)&&weatherDay>=0&&x>=136&&x<=607&&y>=174&&y<=395;
                if(!heldTouch){if(monthPicker)wheelColumn=x<520?0:1;touchX=x;touchY=lastTouchY=y;dragging=false;continuousTouch=field||chart;if(continuousTouch)touch(x,y);}
                else{if(field||chart)touch(x,y);if(std::abs(y-touchY)>18||std::abs(x-touchX)>24)dragging=true;if(dragging&&(monthPicker||!(page==2&&settingsTab==9))&&std::abs(y-lastTouchY)>=18){scrollGesture((lastTouchY-y)/18);lastTouchY=y;}}
            }else if(heldTouch&&!dragging&&!continuousTouch)touch(touchX,touchY);heldTouch=td.reportNum>0;
        }
        prepareBackground();prepareGallery();preparePickerField();palette();vita2d_set_clear_color(p.bg);vita2d_start_drawing();vita2d_clear_screen();drawBackground();header();if(page==0){if(weatherDay>=0)weatherDetails();else home();}else if(page==1){if(calendarReader)drawCalendarText();else if(eventEditor)eventSettings();else calendar();}else if(page==3){if(weatherDay>=0)weatherDetails();else weatherPlaces();}else settings();
        if(monthPicker)drawMonthPicker();
        if(!notice.empty())fit(20,536,14,displayNotice(),920,p.muted);else if(state.hints)fit(20,536,14,footerHint(),920,p.muted);
        vita2d_end_drawing();if(imeKind)vita2d_common_dialog_update();vita2d_swap_buffers();
    }
    persist();if(netReady){curl_global_cleanup();sceNetCtlTerm();sceNetTerm();}free(netMemory);vita2d_wait_rendering_done();if(pickerTexture)vita2d_free_texture(pickerTexture);for(const auto& entry:fonts)vita2d_free_font(entry.second);vita2d_free_font(font);if(backgroundTexture)vita2d_free_texture(backgroundTexture);for(auto& v:uiTextures)vita2d_free_texture(v.second);for(auto& v:thumbnails)if(v.second)vita2d_free_texture(v.second);vita2d_fini();sceAppUtilShutdown();sceKernelExitProcess(0);return 0;
}
