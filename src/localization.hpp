#pragma once
#include "core.hpp"
#include <array>
namespace day {
struct Language {const char* code;const char* native;const char* api;};
inline const std::array<Language,20>& languages(){static const std::array<Language,20> list={{{"ru","Русский","ru"},{"en-US","English (United States)","en"},{"fr","Français","fr"},{"es","Español","es"},{"de","Deutsch","de"},{"it","Italiano","it"},{"nl","Nederlands","nl"},{"pt-PT","Português (Portugal)","pt"},{"ja","日本語","ja"},{"ko","한국어","ko"},{"zh-TW","繁體中文","zh"},{"zh-CN","简体中文","zh"},{"fi","Suomi","fi"},{"sv","Svenska","sv"},{"da","Dansk","da"},{"nb","Norsk","no"},{"pl","Polski","pl"},{"pt-BR","Português (Brasil)","pt"},{"en-GB","English (United Kingdom)","en"},{"tr","Türkçe","tr"}}};return list;}
inline int systemLanguageId(int system){return system==0?8:system==8?0:system>=0&&system<20?system:1;}
inline const Language& languageInfo(int id){return languages()[std::clamp(id,0,19)];}
// Immutable after startup: the network worker can read its language snapshot safely.
inline std::array<Json,20>& localePacks(){static std::array<Json,20> packs;return packs;}
inline bool loadLanguages(const std::string& root){bool complete=true;for(int i=0;i<20;++i){try{std::ifstream f(root+"/"+languageInfo(i).code+".json");Json j;f>>j;localePacks()[i]=std::move(j);}catch(...){localePacks()[i]=Json::object();complete=false;}}return complete;}
inline const Json& locale(int id){return localePacks()[std::clamp(id,0,19)];}
inline const char* localized(int id,const char* ru,const char* en){if(id==0)return ru;const auto& pack=locale(id);auto ui=pack.find("ui");if(ui!=pack.end()&&ui->is_object()){auto it=ui->find(en);if(it!=ui->end()&&it->is_string())return it->get_ref<const std::string&>().c_str();}return en;}
inline std::string localeName(int id,const char* section,const std::string& key,const std::string& fallback){const auto& pack=locale(id);auto list=pack.find(section);if(list!=pack.end()&&list->is_object()){auto it=list->find(key);if(it!=list->end()&&it->is_string())return it->get<std::string>();}return fallback;}
inline const char* localeDate(int id,const char* section,int index,const char* fallback){const auto& pack=locale(id);auto list=pack.find(section);if(list!=pack.end()&&list->is_array()&&index>=0&&index<(int)list->size()&&(*list)[index].is_string())return (*list)[index].get_ref<const std::string&>().c_str();return fallback;}
inline std::string localizedDate(Date d,int id,bool year=true){auto m=localeDate(id,"dateMonths",d.m-1,"");if(!*m)return key(d);if(id==8||id==9||id==10||id==11)return (year?std::to_string(d.y)+(id==9?"년 ":"年"):"")+m+(id==9?" ":"")+std::to_string(d.d)+(id==9?"일":"日");if(id==1)return std::string(m)+" "+std::to_string(d.d)+(year?", "+std::to_string(d.y):"");return std::to_string(d.d)+" "+m+(year?" "+std::to_string(d.y):"");}
inline void applySystemLanguage(State& s,int system){if(s.followSystemLanguage)s.language=systemLanguageId(system);}
}
