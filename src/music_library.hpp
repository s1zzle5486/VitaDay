#pragma once
#include "json.hpp"
#include "music.hpp"
#include <fstream>
namespace day {
struct MusicPlaylist{std::string id,title;std::vector<std::string> paths;};
struct MusicLibrary{
 std::vector<MusicPlaylist> lists={{"favourites","",{}}};unsigned serial=1,revision=0;
 MusicPlaylist* find(const std::string& folder){if(folder.rfind("list:",0)!=0)return nullptr;for(auto& list:lists)if(folder=="list:"+list.id)return &list;return nullptr;}
 std::string create(const std::string& name){if(name.empty()||name.size()>240||lists.size()>=64)return "";std::string id=std::to_string(serial++);lists.push_back({id,name,{}});return "list:"+id;}
 bool add(const std::string& folder,const std::string& path){auto* list=find(folder);if(!list||list->paths.size()>=256||std::find(list->paths.begin(),list->paths.end(),path)!=list->paths.end())return false;list->paths.push_back(path);return true;}
 void remove(const std::string& folder,const std::string& path){if(auto* list=find(folder))list->paths.erase(std::remove(list->paths.begin(),list->paths.end(),path),list->paths.end());}
 void forget(const std::string& path){for(auto& list:lists)remove("list:"+list.id,path);}
 bool has(const std::string& folder,const std::string& path){auto* list=find(folder);return list&&std::find(list->paths.begin(),list->paths.end(),path)!=list->paths.end();}
 void move(const std::string& folder,const std::string& path,int delta){if(auto* list=find(folder)){auto it=std::find(list->paths.begin(),list->paths.end(),path);if(it==list->paths.end())return;int index=it-list->paths.begin(),next=index+delta;if(next>=0&&next<int(list->paths.size()))std::swap(list->paths[index],list->paths[next]);}}
 void erase(const std::string& folder){if(folder=="list:favourites")return;lists.erase(std::remove_if(lists.begin(),lists.end(),[&](const MusicPlaylist& list){return folder=="list:"+list.id;}),lists.end());}
 std::vector<int> rows(const std::vector<std::string>& files,const std::string& folder){auto* list=find(folder);if(!list)return musicQueue(files,folder);std::vector<int> result;for(const auto& path:list->paths){auto it=std::find(files.begin(),files.end(),path);if(it!=files.end())result.push_back(it-files.begin());}return result;}
 bool save(const std::string& path){nlohmann::json data={{"version",1},{"revision",revision+1},{"serial",serial},{"playlists",nlohmann::json::array()}};for(auto& list:lists)data["playlists"].push_back({{"id",list.id},{"title",list.title},{"paths",list.paths}});std::string target=path+"."+std::to_string((revision+1)%2);std::ofstream file(target);file<<data.dump();file.close();if(!file)return false;try{std::ifstream check(target);nlohmann::json read;check>>read;if(read!=data)return false;}catch(...){return false;}++revision;return true;}
 void load(const std::string& path){try{nlohmann::json data;unsigned latest=0;for(auto suffix:{std::string(""),std::string(".0"),std::string(".1")}){try{std::ifstream file(path+suffix,std::ios::binary|std::ios::ate);if(!file||file.tellg()>8*1024*1024)continue;file.seekg(0);nlohmann::json candidate;file>>candidate;unsigned rev=candidate.value("revision",0u);if(candidate.contains("playlists")&&candidate["playlists"].is_array()&&(data.is_null()||rev>=latest)){latest=rev;data=std::move(candidate);}}catch(...){}}if(!data.contains("playlists")||!data["playlists"].is_array())return;MusicLibrary next;next.revision=latest;next.serial=data.value("serial",1u);for(const auto& entry:data["playlists"]){if(next.lists.size()>=64)break;auto id=entry.value("id","");auto title=entry.value("title","");if(id.empty()||id.size()>32||title.size()>240)continue;if(id!="favourites"&&!std::all_of(id.begin(),id.end(),[](char c){return c>='0'&&c<='9';}))continue;if(id!="favourites"){if(next.find("list:"+id))continue;next.lists.push_back({id,title,{}});next.serial=std::max(next.serial,unsigned(std::strtoul(id.c_str(),nullptr,10))+1);}if(entry.contains("paths")&&entry["paths"].is_array())for(const auto& value:entry["paths"]){if(!value.is_string())continue;auto track=value.get<std::string>();if(track.size()<=1024&&mp3Path(track))next.add("list:"+id,track);}}*this=std::move(next);}catch(...){} }
};
}
