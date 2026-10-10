#include <thread>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <map>
#include <atomic>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
using SceUID=int;using SceSize=unsigned;
static std::mutex gate;static std::thread thread;static int(*workerEntry)(SceSize,void*)=nullptr;
int sceKernelCreateMutex(const char*,int,int,void*){return 1;}int sceKernelLockMutex(int,int,void*){gate.lock();return 0;}int sceKernelUnlockMutex(int,int){gate.unlock();return 0;}int sceKernelDeleteMutex(int){return 0;}
int sceKernelCreateThread(const char*,int(*f)(SceSize,void*),int,int,int,int,void*){workerEntry=f;return 2;}int sceKernelStartThread(int,unsigned,void* a){auto object=*static_cast<void**>(a);thread=std::thread([object]() mutable {workerEntry(sizeof(object),&object);});return 0;}int sceKernelWaitThreadEnd(int,void*,void*){thread.join();return 0;}int sceKernelDeleteThread(int){return 0;}int sceKernelDelayThread(int us){std::this_thread::sleep_for(std::chrono::microseconds(us));return 0;}
struct FileStat{int st_mode;long long st_size;};struct SceIoDirent{char d_name[256];FileStat d_stat;};
#define SCE_S_ISDIR(m) ((m)==1)
static std::map<int,std::pair<std::vector<std::filesystem::directory_entry>,size_t>> dirs;static int nextDir=10;
int sceIoDopen(const char* p){if(!std::filesystem::is_directory(p))return -1;std::vector<std::filesystem::directory_entry> entries;for(auto& e:std::filesystem::directory_iterator(p))entries.push_back(e);int id=nextDir++;dirs[id]={entries,0};return id;}
int sceIoDread(int fd,SceIoDirent* out){auto& d=dirs.at(fd);if(d.second==d.first.size())return 0;auto e=d.first[d.second++];std::string n=e.path().filename();strncpy(out->d_name,n.c_str(),255);out->d_stat={e.is_directory()?1:0,e.is_directory()?0:static_cast<long long>(e.file_size())};return 1;}int sceIoDclose(int fd){dirs.erase(fd);return 0;}
enum SceAudioOutMode{SCE_AUDIO_OUT_MODE_MONO,SCE_AUDIO_OUT_MODE_STEREO};enum SceAudioOutChannelFlag{Left=1,Right=2};constexpr int SCE_AUDIO_OUT_PORT_TYPE_BGM=1,SCE_AUDIO_VOLUME_0DB=32768,SCE_AUDIO_VOLUME_FLAG_L_CH=1,SCE_AUDIO_VOLUME_FLAG_R_CH=2;
static std::atomic<int> ports{0},owners{0},buffers{0},volume{0},rate{44100},failOutput{0};
int sceAppMgrAcquireBgmPort(){++owners;return 0;}int sceAppMgrReleaseBgmPort(){--owners;return 0;}int sceAudioOutOpenPort(int,int,int hz,SceAudioOutMode){rate=hz;++ports;return 4;}int sceAudioOutReleasePort(int){--ports;return 0;}int sceAudioOutSetVolume(int,SceAudioOutChannelFlag,int* values){volume=values[0];return 0;}int sceAudioOutOutput(int,const void*){if(failOutput.exchange(0))return -1;++buffers;std::this_thread::sleep_for(std::chrono::microseconds(1024*1000000LL/rate));return 0;}
static unsigned long long sceKernelGetProcessTimeWide(){return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
#define DR_MP3_IMPLEMENTATION

static std::atomic<int> systemVolume{24},powerTicks{0};
int sceAVConfigGetSystemVol(int* value){*value=systemVolume;return 0;}
int sceAVConfigSetSystemVol(int value){assert(value>=0&&value<=30);systemVolume=value;return 0;}
constexpr int SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND=1;
int sceKernelPowerTick(int){++powerTicks;return 0;}
