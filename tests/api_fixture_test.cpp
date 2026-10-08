#include "../src/core.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
using namespace day;
Json file(const char* name){std::ifstream f(std::filesystem::path(__FILE__).parent_path()/"fixtures"/std::filesystem::path(name).filename());Json j;f>>j;return j;}
int main(){
    Location l=parseIP(file("work/ip-fixture.json"));assert(l.valid&&countryValid(l.country)&&!l.zone.empty());
    Weather w=parseWeather(file("work/weather-fixture.json"));assert(w.valid&&w.dates.size()==3);
    l.country="US";l.region="CA";auto h=parseHolidays(file("work/holidays-fixture.json"),l,2026);assert(!h.empty());
    for(const auto& a:h)assert(parseDate(a.date).y==2026);
    std::cout<<"Live API response schemas: IP, weather, regional holidays: PASS\n";
}
