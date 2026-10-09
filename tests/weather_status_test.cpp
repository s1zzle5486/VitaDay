#include "../src/weather_status.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){Weather w;w.valid=true;w.dates={"2026-10-07"};w.lows={3};w.highs={13};
 auto check=[&](long long updated,long long now,bool online){return forecastAge(w,updated,now,online,{2026,10,7},12);};
 assert(check(1000,1000,true).status==ForecastFreshness::Fresh);assert(check(1000,2800,true).status==ForecastFreshness::Fresh);
 assert(check(1000,2801,true).status==ForecastFreshness::Cached);assert(check(1000,1200,false).status==ForecastFreshness::Cached);
 assert(!check(0,1200,true).known);assert(check(0,1200,true).status==ForecastFreshness::Cached);
 assert(!check(2000,1200,true).known);assert(check(2000,1200,true).status==ForecastFreshness::Cached);
 assert(forecastAge(w,1000,1200,true,{2026,10,8},0).status==ForecastFreshness::Expired);
 w.valid=false;assert(check(1000,1200,true).status==ForecastFreshness::Missing);
 Weather partial;partial.valid=true;partial.details={{"hourly",{{"time",{"2026-10-08T00:00"}},{"temperature_2m",{8}}}}};
 assert(forecastAge(partial,1000,1200,true,{2026,10,8},0).status==ForecastFreshness::Fresh);
 assert(forecastAge(partial,1000,1200,true,{2026,10,8},1).status==ForecastFreshness::Expired);
 std::cout<<"Forecast age: individual fetch times, 30-minute boundary, offline cache, unknown/future timestamps, expired days and hourly-only coverage passed\n";
}
