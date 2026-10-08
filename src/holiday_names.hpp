#pragma once
#include "input.hpp"
namespace day {
inline Json& holidayNames(){static Json data=[](){try{Json j;std::ifstream f("app0:assets/holiday-names-ru.json");f>>j;return j;}catch(...){return Json::object();}}();return data;}
inline std::string holidayNameKey(std::string value){value=searchFold(trim(value));for(size_t at=0;(at=value.find("’",at))!=std::string::npos;)value.replace(at,3,"'");for(size_t at=0;(at=value.find("evey",at))!=std::string::npos;)value.replace(at,4,"eve");return value;}
inline std::string russianHolidayEnglish(const std::string& title){const auto& names=holidayNames();auto english=names.find("english");if(english!=names.end()&&english->is_object()){auto found=english->find(holidayNameKey(title));if(found!=english->end()&&found->is_string())return found->get<std::string>();}return {};}
inline std::string localizedHolidayTitle(const Holiday& h,int language){
    if(language)return h.english.empty()?h.name:h.english;
    if(h.country=="INT")return h.name;
    if(!h.english.empty()){auto translated=russianHolidayEnglish(h.english);if(!translated.empty())return translated;}
    const auto& names=holidayNames();auto local=names.find("local");
    if(local!=names.end()&&local->is_object()){auto country=local->find(h.country);if(country!=local->end()&&country->is_object()){auto found=country->find(h.name);if(found!=country->end()&&found->is_string())return found->get<std::string>();}}
    auto translated=russianHolidayEnglish(h.name);return translated.empty()?h.name:translated;
}
}
