#include "../src/core.hpp"
#include "../src/input.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
using namespace day;
Json fixture(const char* p){std::ifstream f(std::filesystem::path(__FILE__).parent_path()/"fixtures"/std::filesystem::path(p).filename());Json j;f>>j;return j;}
int main(){
 assert(!stickDirection(128,128,128,128,1,2,4,8));assert(stickDirection(128,128,0,128,1,2,4,8)==1);assert(stickDirection(255,0,128,128,1,2,4,8)==4);
 NavigationRepeat repeat;assert(repeat.poll(1,1,0)==1);assert(!repeat.poll(1,1,300000));assert(repeat.poll(1,1,350000)==1);assert(!repeat.poll(1,1,400000));assert(repeat.poll(1,1,435000)==1);assert(!repeat.poll(0,1,450000));assert(repeat.poll(1,1,460000)==1);
 assert(mappedButtons(1,true,1,2)==2&&mappedButtons(2,true,1,2)==1&&mappedButtons(3,true,1,2)==3&&mappedButtons(5,false,1,2)==5);
 assert(isDayHour(8,7,20)&&!isDayHour(22,7,20)&&isDayHour(23,20,7)&&!isDayHour(12,20,7));
 assert(matchesCountry("сШа","US","United States","США"));assert(matchesCountry("GER","DE","Germany","Германия"));assert(!matchesCountry("France","DE","Germany","Германия"));
 State s;s.location.valid=true;s.location.country="US";s.countries={{"US","United States",2,false,true,"America/New_York"},{"JP","Japan",3,true,false,"Asia/Tokyo"}};assert(activeCountries(s).size()==1&&activeCountries(s)[0].code=="JP");
 s.hints=true;s.dualUnits=true;s.autoAppearance=true;s.dayStart=9;s.nightStart=22;s.dayLook={0,2,5,4,3,40,"ux0:data/VitaDay/backgrounds/a.png"};s.nightLook={1,1,2,5,2,60,""};s.dayLook.bgRgb=0x123456;s.nightLook.cardRgb=0xabcdef;auto copied=decode(encode(s));assert(copied.dayLook.bgRgb==0x123456&&copied.nightLook.cardRgb==0xabcdef);assert(copied.countries[0].clock&&!copied.countries[0].holidays&&copied.countries[0].zone=="America/New_York");assert(copied.hints&&copied.dualUnits&&copied.autoAppearance&&copied.dayStart==9&&copied.nightStart==22&&copied.dayLook.opacity==40&&copied.dayLook.image==s.dayLook.image);
 Json legacy=encode(s);legacy.erase("display");legacy["theme"]=2;legacy["style"]=1;legacy["countries"][0].erase("clock");legacy["countries"][0].erase("holidays");auto old=decode(legacy);assert(!old.hints&&!old.dualUnits&&old.dayLook.theme==2&&old.dayLook.style==1&&old.countries[0].holidays&&!old.countries[0].clock);
 Json zones;{std::ifstream z(std::filesystem::path(__FILE__).parent_path().parent_path()/"assets/zones.json");z>>zones;}assert(zoneOffset(zones["zones"],"America/New_York",1767225600)==-18000);assert(zoneOffset(zones["zones"],"America/New_York",1782864000)==-14400);assert(zoneOffset(zones["zones"],"Asia/Kolkata",1782864000)==19800);assert(zoneOffset(zones["zones"],"Asia/Tokyo",1782864000)==32400);assert(zoneOffset(zones["zones"],"unknown",1782864000,123)==123);
 auto weather=parseWeather(fixture("work/weather-ten.json"));assert(weather.dates.size()==10&&weather.details["hourly"]["time"].size()==240);assert(std::isfinite(number(weather.details["hourly"],"temperature_2m",239)));weather.details["hourly"]["rain"][230]=1;auto ranges=rainIntervals(weather,weather.dates[9]);assert(!ranges.empty());
 std::cout<<"Repeat, system X/O mapping, country search and tracking, day/night profiles, migration, DST and 10-day weather: PASS\n";
}
