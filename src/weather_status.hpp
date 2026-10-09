#pragma once
#include "offline_weather.hpp"
namespace day {
enum class ForecastFreshness {Missing,Expired,Cached,Fresh};
struct ForecastAge {ForecastFreshness status=ForecastFreshness::Missing;bool known=false;long long seconds=0;};
inline ForecastAge forecastAge(const Weather& w,long long updated,long long utc,bool online,Date date,int hour){
 ForecastAge out;out.known=updated>0&&updated<=utc;out.seconds=out.known?utc-updated:0;
 if(!w.valid)return out;
 if(!weatherNow(w,date,hour,false).valid){out.status=ForecastFreshness::Expired;return out;}
 out.status=online&&out.known&&out.seconds<=1800?ForecastFreshness::Fresh:ForecastFreshness::Cached;
 return out;
}
}
