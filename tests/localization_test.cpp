#include "../src/localization.hpp"
#include "../src/holiday_names.hpp"
#include "../src/address.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace day;
int main(){
 const auto assets=std::filesystem::path(__FILE__).parent_path().parent_path()/"assets";
 assert(loadLanguages((assets/"locales").string()));
 State fresh;assert(fresh.followSystemLanguage&&fresh.language==1);
 const int mapping[]={8,1,2,3,4,5,6,7,0,9,10,11,12,13,14,15,16,17,18,19};
 for(int system=0;system<20;++system){applySystemLanguage(fresh,system);assert(fresh.language==mapping[system]);}
 applySystemLanguage(fresh,999);assert(fresh.language==1);
 for(int i=0;i<20;++i){fresh.language=i;fresh.followSystemLanguage=false;auto saved=decode(encode(fresh));assert(saved.language==i&&!saved.followSystemLanguage);applySystemLanguage(saved,8);assert(saved.language==i);const auto& pack=locale(i);assert(pack["months"].size()==12&&pack["weekdays"].size()==7&&pack["fullWeekdays"].size()==7);assert(pack["ui"].size()>=230&&pack["holidays"].size()>=200);for(auto it=locale(1)["ui"].begin();it!=locale(1)["ui"].end();++it)assert(pack["ui"].contains(it.key())&&!pack["ui"][it.key()].get<std::string>().empty());assert(!localizedDate({2026,10,7},i).empty());assert(std::string(localized(i,"Язык","Language"))==pack["ui"]["Language"].get<std::string>());}
 std::ifstream(assets/"address-rules.json")>>addressData();std::ifstream(assets/"subdivision-names.json")>>subdivisionData();auto ny=regionName("US","NY",11);assert(resolveRegion("US",ny,11)=="NY");
 assert(localizedDate({2026,10,7},0)=="7 октября 2026");assert(localizedDate({2026,10,7},1)=="October 7, 2026");assert(localizedDate({2026,10,7},18)=="7 October 2026");
 auto old=encode(fresh);old["preferences"].erase("followSystemLanguage");old["preferences"]["language"]=0;auto legacy=decode(old);assert(legacy.language==0&&!legacy.followSystemLanguage);old["preferences"]["language"]=1;assert(decode(old).language==1);
 fresh.followSystemLanguage=true;assert(decode(encode(fresh)).followSystemLanguage);
 assert(std::string(localized(8,"Несуществующий","Missing key"))=="Missing key");
 Location l;l.names={{"ja","東京"},{"zh-TW","東京"},{"fr","Tokyo"}};auto restored=decodeLocation(encodeLocation(l));assert(restored.names==l.names);
 Holiday h{"2026-12-25","Weihnachten","DE","Christmas Day"};for(int i=2;i<20;++i)assert(localizedHolidayTitle(h,i)!=h.english||i==18);
 std::cout<<"20 system languages, translation coverage, dates, manual/system preference, legacy saves, multilingual location persistence and holidays: PASS\n";
}
