#include "../src/relative_time.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace day;
int main(){auto assets=std::filesystem::path(__FILE__).parent_path().parent_path()/"assets/locales";assert(loadLanguages(assets.string()));
 assert(updatedRelative(0,0)=="Обновлено только что");assert(relativeAge(59,0)=="только что");assert(relativeAge(60,0)=="1 минуту назад");assert(relativeAge(120,0)=="2 минуты назад");assert(relativeAge(300,0)=="5 минут назад");assert(relativeAge(660,0)=="11 минут назад");assert(relativeAge(1260,0)=="21 минуту назад");assert(relativeAge(1320,0)=="22 минуты назад");assert(relativeAge(3599,0)=="59 минут назад");assert(relativeAge(3600,0)=="1 час назад");assert(relativeAge(7200,0)=="2 часа назад");assert(relativeAge(5*3600,0)=="5 часов назад");assert(relativeAge(86400,0,true)=="вчера");assert(relativeAge(86400,0,false)=="1 день назад");assert(relativeAge(60,0,true)=="1 минуту назад");assert(relativeAge(3*86400,0,true)=="3 дня назад");assert(relativeAge(11*86400,0)=="11 дней назад");assert(relativeAge(21*86400,0)=="21 день назад");
 assert(relativeAge(60,16)=="1 minutę temu");assert(relativeAge(120,16)=="2 minuty temu");assert(relativeAge(1260,16)=="21 minut temu");assert(relativeAge(1320,16)=="22 minuty temu");assert(updatedRelative(300,1)=="Updated 5 minutes ago");assert(updatedRelative(86400,1,true)=="Updated yesterday");assert(relativeAge(7200,8)=="2時間前");assert(relativeAge(7200,9)=="2시간 전");
 for(int lang=0;lang<20;++lang){assert(locale(lang).contains("relativeTime"));for(long long seconds:{0LL,59LL,60LL,120LL,300LL,660LL,1260LL,3600LL,7200LL,86400LL,259200LL,1814400LL}){auto text=updatedRelative(seconds,lang,true);assert(!text.empty()&&text.find('{')==std::string::npos);if(lang!=1&&lang!=18)assert(text.size()<4||text.substr(text.size()-4)!=" ago");}}
 assert(relativeAge(-10,1)=="just now");
 std::cout<<"Relative age: minute/hour/day boundaries, Russian and Polish plural rules, yesterday flag, CJK, 20-language coverage and interpolation passed\n";
}
