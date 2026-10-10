#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <cctype>
#include <cstdio>
#include <random>
#include "music_tags.hpp"
namespace day {
inline bool mp3Path(const std::string& path){if(path.size()<4)return false;auto ext=path.substr(path.size()-4);for(auto& c:ext)c=char(std::tolower(static_cast<unsigned char>(c)));return ext==".mp3";}
inline std::string trackName(const std::string& path){auto n=path.substr(path.find_last_of('/')+1);return mp3Path(n)?n.substr(0,n.size()-4):n;}
inline int adjacentTrack(int index,int delta,int count){return count<=0?-1:(std::clamp(index,0,count-1)+delta%count+count)%count;}
inline std::string musicTime(double seconds){int s=int(std::max(0.,seconds));char text[32];snprintf(text,sizeof(text),"%d:%02d",s/60,s%60);return text;}
inline std::string trackFolder(const std::string& path){auto slash=path.find_last_of('/');return slash==std::string::npos?"":path.substr(0,slash);}
inline std::vector<int> musicQueue(const std::vector<std::string>& files,const std::string& folder){std::vector<int> out;for(int i=0;i<int(files.size());++i)if(folder.empty()||trackFolder(files[i])==folder)out.push_back(i);return out;}
class MusicOrder {
    std::vector<std::string> paths;std::mt19937 random;
public:
    explicit MusicOrder(unsigned seed=0x56444159):random(seed){}
    void reset(const std::vector<std::string>& files,const std::string& folder,int mode,const std::string& current){paths.clear();for(int i:musicQueue(files,folder))paths.push_back(files[i]);if(mode==1){paths.erase(std::remove(paths.begin(),paths.end(),current),paths.end());std::shuffle(paths.begin(),paths.end(),random);if(std::find(files.begin(),files.end(),current)!=files.end()&&(folder.empty()||trackFolder(current)==folder))paths.insert(paths.begin(),current);}}
    std::string next(const std::string& current,int delta,bool automatic,int mode)const{if(paths.empty())return "";auto it=std::find(paths.begin(),paths.end(),current);if(it==paths.end())return delta<0?paths.back():paths.front();if(automatic&&mode==3)return current;int position=int(it-paths.begin())+delta;if(automatic&&mode!=2&&(position<0||position>=int(paths.size())))return "";return paths[(position%int(paths.size())+int(paths.size()))%int(paths.size())];}
};
struct MusicStatus {std::vector<std::string> files;std::vector<MusicTags> tags;std::string path,playlist,title,artist,deletedPath;int deleteRevision=0,index=-1,error=0,mode=0,repeat=1;double position=0,duration=0;bool playing=false,loading=true;};
}
