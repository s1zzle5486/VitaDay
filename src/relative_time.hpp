#pragma once
#include "localization.hpp"
namespace day {
inline std::string substitute(std::string pattern,const std::string& token,const std::string& value){auto at=pattern.find(token);if(at!=std::string::npos)pattern.replace(at,token.size(),value);return pattern;}
inline int relativePlural(int language,long long count){
 if(language==0){long long last=count%10,last2=count%100;return last==1&&last2!=11?0:last>=2&&last<=4&&(last2<12||last2>14)?1:2;}
 if(language==16){long long last=count%10,last2=count%100;return count==1?0:last>=2&&last<=4&&(last2<12||last2>14)?1:2;}
 return count==1?0:2;
}
inline std::string relativeAge(long long seconds,int language,bool previousDay=false){
 const auto& pack=locale(language);auto it=pack.find("relativeTime");const Json* labels=it!=pack.end()&&it->is_object()?&*it:nullptr;
 auto label=[&](const char* key,const char* fallback){return labels?labels->value(key,std::string(fallback)):std::string(fallback);};
 seconds=std::max(0LL,seconds);if(seconds<60)return label("justNow","just now");
 const char* unit=seconds<3600?"minute":seconds<86400?"hour":"day";
 long long count=seconds/(seconds<3600?60:seconds<86400?3600:86400);
 if(count==1&&seconds>=86400&&previousDay)return label("yesterday","yesterday");
 std::string pattern=std::string("{n} ")+unit+(count==1?" ago":"s ago");
 if(labels){auto forms=labels->find(unit);int index=relativePlural(language,count);if(forms!=labels->end()&&forms->is_array()&&index<(int)forms->size()&&(*forms)[index].is_string())pattern=(*forms)[index].get<std::string>();}
 return substitute(pattern,"{n}",std::to_string(count));
}
inline std::string updatedRelative(long long seconds,int language,bool previousDay=false){return substitute(localeName(language,"relativeTime","updated","Updated {age}"),"{age}",relativeAge(seconds,language,previousDay));}
}
