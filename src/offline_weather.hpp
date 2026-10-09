#pragma once
#include "core.hpp"
namespace day {
struct WeatherNow {bool valid=false,forecast=true,hourly=false,hasCode=false;int code=0,day=-1;double temperature=NAN,low=NAN,high=NAN;};
inline WeatherNow weatherNow(const Weather& w,Date date,int hour,bool observed){
 WeatherNow out;if(!valid(date))return out;const auto day=key(date);for(size_t i=0;i<w.dates.size();++i)if(w.dates[i]==day){out.day=i;if(i<w.lows.size())out.low=w.lows[i];if(i<w.highs.size())out.high=w.highs[i];break;}
 if(observed&&w.valid&&w.time.size()>=16&&w.time.substr(0,10)==day&&atoi(w.time.substr(11,2).c_str())==hour){out.valid=true;out.forecast=false;out.hourly=true;out.code=w.code;out.hasCode=true;out.temperature=w.temperature;return out;}
 static const Json empty=Json::object();auto di=w.details.find("daily"),hi=w.details.find("hourly");const Json& daily=di==w.details.end()?empty:*di;const Json& hourly=hi==w.details.end()?empty:*hi;
 if(out.day>=0){auto code=number(daily,"weather_code",out.day);if(std::isfinite(code)){out.code=(int)code;out.hasCode=true;}}
 auto times=hourly.find("time");if(times!=hourly.end()&&times->is_array())for(size_t i=0;i<times->size();++i){if(!(*times)[i].is_string())continue;auto stamp=(*times)[i].get<std::string>();if(stamp.size()<16||stamp.substr(0,10)!=day||atoi(stamp.substr(11,2).c_str())!=hour)continue;auto value=number(hourly,"temperature_2m",i);if(!std::isfinite(value))continue;out.valid=true;out.hourly=true;out.temperature=value;auto code=number(hourly,"weather_code",i);if(std::isfinite(code)){out.code=(int)code;out.hasCode=true;}return out;}
 // A daily forecast describes a range, not an invented current temperature.
 out.valid=out.day>=0&&std::isfinite(out.low)&&std::isfinite(out.high);return out;
}
inline bool freshObservation(long long updated,long long utc,bool connected){return connected&&updated>0&&utc>=updated&&utc-updated<=1800;}
}
