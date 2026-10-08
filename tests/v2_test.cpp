#include "../src/core.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
using namespace day;
Json fixture(const char* path){std::ifstream f(std::filesystem::path(__FILE__).parent_path()/"fixtures"/std::filesystem::path(path).filename());Json j;f>>j;return j;}
int main(){
    assert(clockTime(0,5,false)=="12:05 AM");assert(clockTime(12,30,false)=="12:30 PM");assert(clockTime(23,59,true)=="23:59");
    assert(std::abs(temperature(0,1)-32)<1e-8&&std::abs(temperature(100,1)-212)<1e-8);
    assert(std::abs(windSpeed(1.609344,1)-1)<1e-8&&std::abs(precipitation(25.4,1)-1)<1e-8);
    State s;s.location.valid=true;s.location.country="US";s.location.region="CA";s.countries={{"US","United States",2},{"DE","Germany",3}};
    assert(activeCountries(s).size()==2);s.followLocal=false;assert(activeCountries(s).size()==2);s.followLocal=true;s.location.country="CA";assert(activeCountries(s).size()==3);
    s.location.country="US";s.caches["US"]={2026,"CA",0,{{"2026-01-01","Новый год","US","New Year"}}};s.caches["DE"]={2026,"",0,{{"2026-01-01","Neujahr","DE","New Year"}}};
    auto events=holidayItems(s,{2026,1,1});assert(events.size()==2&&events[0].country!=events[1].country);assert(countryColor(s,"US")==2&&countryColor(s,"DE")==3);
    auto international=holidayItems(s,{2026,3,8});assert(international.size()==1&&international[0].country=="INT");s.international=false;assert(holidayItems(s,{2026,3,8}).empty());
    s.location.region="NY";assert(holidayItems(s,{2026,1,1}).size()==1);s.countries.erase(s.countries.begin());s.followLocal=false;assert(holidayItems(s,{2026,1,1}).size()==1);
    s.weather=parseWeather(fixture("work/weather-week.json"));assert(s.weather.dates.size()==7&&s.weather.details.at("hourly").at("time").size()==168);
    assert(std::isfinite(number(s.weather.details["hourly"],"relative_humidity_2m",0)));assert(!std::isfinite(number(s.weather.details["hourly"],"missing",0)));
    Json withNull={{"humidity",Json::array({nullptr})}};assert(!std::isfinite(number(withNull,"humidity",0)));
    s.language=1;s.units=1;s.time24=false;s.keepScreen=false;s.marks["2026-10-07"]={"Мой день",6,true};State roundtrip=decode(encode(s));
    assert(!roundtrip.keepScreen);assert(roundtrip.language==1&&roundtrip.units==1&&!roundtrip.time24&&roundtrip.countries.size()==1&&roundtrip.weather.dates.size()==7&&roundtrip.marks.at("2026-10-07").title=="Мой день");
    Json legacy=encode(s);legacy["version"]=1;legacy.erase("preferences");legacy.erase("countries");legacy.erase("catalog");legacy.erase("caches");legacy["holidayYear"]=2026;legacy["holidayCountry"]="DE";legacy["holidayRegion"]="";legacy["holidays"]={{{"date","2026-01-01"},{"name","Neujahr"}}};State migrated=decode(legacy);
    assert(migrated.marks.size()==1&&migrated.marks.begin()->second.yearly&&migrated.caches.at("DE").items.size()==1&&migrated.language==0&&migrated.time24);
    assert(migrated.keepScreen);
    Weather wet;wet.details={{"hourly",{{"time",{"2026-10-07T00:00","2026-10-07T01:00","2026-10-07T02:00","2026-10-07T04:00","2026-10-08T00:00"}},{"rain",{2,0.2,0.3,0,1}},{"showers",{0,0,0,0.2,0}}}}};
    auto ranges=rainIntervals(wet,"2026-10-07");assert((ranges==std::vector<std::pair<int,int>>{{0,2},{3,4},{23,24}}));assert(rainIntervals(Weather{},"2026-10-07").empty());
    std::cout<<"V2 migration, 7-day/hourly weather, country isolation, international colors, units and 12-hour time: PASS\n";
}
