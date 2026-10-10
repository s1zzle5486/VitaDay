#pragma once
#include "music.hpp"
#include "dr_mp3.h"
#include <psp2/audioout.h>
#include <psp2/appmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <atomic>
#include <cstdio>
#include <cstring>
// Only the audio worker owns the decoder and BGM port. Commands and snapshots
// use short mutex sections; filesystem/decode/audio calls never hold the mutex.
class VitaMusic {
    SceUID mutex=-1,worker=-1;std::atomic<bool> quit{false};day::MusicStatus shared;
    bool rescan=false,toggle=false;int requested=-1;double seekDelta=0;int modeChanges=0,repeatChanges=0,stepChanges=0;double seekTarget=-1;std::string requestedPath,requestedFolder;bool folderPending=false;std::vector<std::string> requestedQueue;std::string requestedDelete,updateFolder;std::vector<std::string> updatePaths;
    void lock(){sceKernelLockMutex(mutex,1,nullptr);}void unlock(){sceKernelUnlockMutex(mutex,1);}
    static int entry(SceSize,void* args){auto self=*static_cast<VitaMusic**>(args);self->run();return 0;}
    void scan(const std::string& root,int depth,std::vector<std::string>& files,int& visited){
        if(depth>4||quit||files.size()>=256||visited>=4096)return;auto fd=sceIoDopen(root.c_str());if(fd<0)return;
        SceIoDirent e{};while(!quit&&files.size()<256&&visited<4096&&sceIoDread(fd,&e)>0){++visited;std::string name=e.d_name;
            if(!name.empty()&&name[0]!='.'){auto path=root+"/"+name;if(SCE_S_ISDIR(e.d_stat.st_mode))scan(path,depth+1,files,visited);else if(day::mp3Path(name)&&e.d_stat.st_size>0&&e.d_stat.st_size<=512LL*1024*1024){drmp3 probe{};if(drmp3_init_file(&probe,path.c_str(),nullptr)){short sample[2];if((probe.channels==1||probe.channels==2)&&probe.sampleRate>=8000&&probe.sampleRate<=48000&&drmp3_read_pcm_frames_s16(&probe,1,sample)==1)files.push_back(path);drmp3_uninit(&probe);}}}memset(&e,0,sizeof(e));}sceIoDclose(fd);
    }
    void run(){
        drmp3 decoder{};bool opened=false,bgm=false;int port=-1;uint64_t nextPowerTick=0;day::MusicStatus status;bool scanNow=true,custom=false;std::vector<std::string> customQueue;short pcm[2048]{};day::MusicOrder order(static_cast<unsigned>(sceKernelGetProcessTimeWide()));
        auto release=[&](){if(port>=0){sceAudioOutReleasePort(port);port=-1;}if(opened){drmp3_uninit(&decoder);opened=false;}if(bgm){sceAppMgrReleaseBgmPort();bgm=false;}};
        while(!quit.load()){
            lock();scanNow|=rescan;rescan=false;int target=requested;requested=-1;std::string targetPath=std::move(requestedPath);requestedPath.clear();std::string folder=std::move(requestedFolder);bool setFolder=folderPending;folderPending=false;double setPosition=seekTarget;seekTarget=-1;bool pause=toggle;toggle=false;double seek=seekDelta;seekDelta=0;int modes=modeChanges;modeChanges=0;int repeats=repeatChanges;repeatChanges=0;int step=stepChanges;stepChanges=0;auto queue=std::move(requestedQueue);std::string deletion=std::move(requestedDelete);requestedDelete.clear();std::string updatedFolder=std::move(updateFolder);updateFolder.clear();auto updatedPaths=std::move(updatePaths);unlock();
            if(!deletion.empty()){if(deletion==status.path){release();status.playing=false;status.path.clear();status.title.clear();status.artist.clear();status.index=-1;status.position=status.duration=0;}if(std::remove(deletion.c_str())==0){status.deletedPath=deletion;++status.deleteRevision;scanNow=true;}else status.error=3;}
            if(scanNow){status.loading=true;lock();shared=status;unlock();std::vector<std::string> files;int visited=0;scan("ux0:data/VitaDay/music",0,files,visited);scan("ux0:music",0,files,visited);std::sort(files.begin(),files.end());files.erase(std::unique(files.begin(),files.end()),files.end());status.files=std::move(files);status.tags.clear();for(const auto& path:status.files){if(quit)break;status.tags.push_back(day::readMusicTags(path));}auto it=std::find(status.files.begin(),status.files.end(),status.path);status.index=it==status.files.end()?-1:int(it-status.files.begin());status.loading=false;scanNow=false;order.reset(custom?customQueue:status.files,custom?"":status.playlist,status.mode,status.path);}
            if(setFolder){custom=folder.rfind("list:",0)==0;customQueue=std::move(queue);}
            if(!updatedFolder.empty()&&custom&&updatedFolder==status.playlist){customQueue=std::move(updatedPaths);order.reset(customQueue,"",status.mode,status.path);}
            const auto& queuePaths=custom?customQueue:status.files;
            if(!targetPath.empty()){auto it=std::find(status.files.begin(),status.files.end(),targetPath);target=it==status.files.end()?-1:int(it-status.files.begin());}
            if(modes){status.mode=(status.mode+modes)%2;order.reset(queuePaths,custom?"":status.playlist,status.mode,status.path);}status.repeat=(status.repeat+repeats)%3;if(step){auto path=order.next(status.path,step,false,status.mode);auto it=std::find(status.files.begin(),status.files.end(),path);if(it!=status.files.end()){target=int(it-status.files.begin());targetPath=path;setFolder=false;}}
            if(target>=0&&target<int(status.files.size())){
                if(setFolder){status.playlist=std::move(folder);order.reset(queuePaths,custom?"":status.playlist,status.mode,status.files[target]);}
                release();status.index=target;status.path=status.files[target];status.playing=false;status.loading=true;status.error=0;status.position=status.duration=0;lock();shared=status;unlock();
                auto tags=target<int(status.tags.size())?status.tags[target]:day::MusicTags{};status.title=tags.title.empty()?day::trackName(status.path):tags.title;status.artist=tags.artist;
                opened=drmp3_init_file(&decoder,status.path.c_str(),nullptr);
                if(opened&&(decoder.channels==1||decoder.channels==2)&&decoder.sampleRate>=8000&&decoder.sampleRate<=48000){
                    auto frames=drmp3_get_pcm_frame_count(&decoder);status.duration=double(frames)/decoder.sampleRate;
                    if(!quit){bgm=sceAppMgrAcquireBgmPort()>=0;if(bgm)port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,1024,decoder.sampleRate,decoder.channels==1?SCE_AUDIO_OUT_MODE_MONO:SCE_AUDIO_OUT_MODE_STEREO);}
                    if(port>=0)status.playing=true;else {status.error=2;release();}
                }else{status.error=1;release();}status.loading=false;
            }
            if(pause&&opened)status.playing=!status.playing;
            if((seek!=0||setPosition>=0)&&opened){double next=std::clamp(setPosition>=0?setPosition:status.position+seek,0.,std::max(0.,status.duration-.05));if(drmp3_seek_to_pcm_frame(&decoder,drmp3_uint64(next*decoder.sampleRate)))status.position=next;}
            if(status.playing&&opened&&port>=0){
                if(sceKernelGetProcessTimeWide()>=nextPowerTick){sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);nextPowerTick=sceKernelGetProcessTimeWide()+1000000ULL;}
                int levels[2]={SCE_AUDIO_VOLUME_0DB,SCE_AUDIO_VOLUME_0DB};sceAudioOutSetVolume(port,(SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH),levels);
                auto frames=drmp3_read_pcm_frames_s16(&decoder,1024,pcm);
                if(frames){if(frames<1024)memset(pcm+frames*decoder.channels,0,(1024-frames)*decoder.channels*sizeof(short));int output=sceAudioOutOutput(port,pcm);if(output<0){
                    // Firmware can invalidate the BGM port across a power transition.
                    // Reopen it and retry this buffer without discarding decoder position.
                    sceAudioOutReleasePort(port);port=-1;if(bgm){sceAppMgrReleaseBgmPort();bgm=false;}bgm=sceAppMgrAcquireBgmPort()>=0;if(bgm)port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,1024,decoder.sampleRate,decoder.channels==1?SCE_AUDIO_OUT_MODE_MONO:SCE_AUDIO_OUT_MODE_STEREO);if(port>=0){sceAudioOutSetVolume(port,(SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH),levels);output=sceAudioOutOutput(port,pcm);}}
                    if(output<0){status.error=2;status.playing=false;release();}else status.position=double(decoder.currentPCMFrame)/decoder.sampleRate;}
                else if(status.index>=0&&!status.files.empty()){auto nextPath=order.next(status.path,1,true,status.repeat==2?3:status.repeat==1?2:status.mode);auto it=std::find(status.files.begin(),status.files.end(),nextPath);if(it!=status.files.end()){lock();if(requested<0){requested=int(it-status.files.begin());requestedPath=nextPath;}unlock();}else{status.playing=false;release();}}else{status.playing=false;release();}
            }else sceKernelDelayThread(20000);
            lock();shared=status;unlock();
        }release();
    }
public:
    bool start(){mutex=sceKernelCreateMutex("VitaDayMusicLock",0,0,nullptr);if(mutex<0){shared.error=2;shared.loading=false;return false;}worker=sceKernelCreateThread("VitaDayMusic",entry,0x10000100,192*1024,0,0,nullptr);auto self=this;if(worker>=0&&sceKernelStartThread(worker,sizeof(self),&self)>=0)return true;if(worker>=0)sceKernelDeleteThread(worker);worker=-1;sceKernelDeleteMutex(mutex);mutex=-1;shared.loading=false;shared.error=2;return false;}
    void stop(){quit=true;if(worker>=0){sceKernelWaitThreadEnd(worker,nullptr,nullptr);sceKernelDeleteThread(worker);worker=-1;}if(mutex>=0){sceKernelDeleteMutex(mutex);mutex=-1;}}
    day::MusicStatus snapshot(){if(mutex<0)return shared;lock();auto value=shared;unlock();return value;}
    void play(int i,const std::string& folder="",const std::vector<std::string>& queue={}){if(mutex<0)return;lock();if(i>=0&&i<int(shared.files.size())){requested=i;requestedPath=shared.files[i];requestedFolder=folder;requestedQueue=queue;folderPending=true;}unlock();}
    void updateQueue(const std::string& folder,const std::vector<std::string>& paths){if(mutex<0)return;lock();updateFolder=folder;updatePaths=paths;unlock();}
    void removeFile(const std::string& path){if(mutex<0)return;lock();if(std::find(shared.files.begin(),shared.files.end(),path)!=shared.files.end())requestedDelete=path;unlock();}
    void pause(){if(mutex<0)return;lock();toggle=!toggle;unlock();}
    void refresh(){if(mutex<0)return;lock();rescan=true;unlock();}
    void seek(double delta){if(mutex<0)return;lock();seekDelta+=delta;unlock();}
    void seekTo(double target){if(mutex<0)return;lock();seekTarget=std::max(0.,target);unlock();}
    void mode(){if(mutex<0)return;lock();++modeChanges;unlock();}
    void repeat(){if(mutex<0)return;lock();++repeatChanges;unlock();}
    void restart(){if(mutex<0)return;lock();if(shared.index>=0){requested=shared.index;requestedPath=shared.path;}unlock();}
    void next(int delta){if(mutex<0)return;lock();stepChanges+=delta;unlock();}
};
