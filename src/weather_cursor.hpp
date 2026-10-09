#pragma once
#include "weather_chart.hpp"
namespace day {
struct WeatherCursor {int lower=-1,upper=-1;double fraction=0,position=0;};
inline int graphStampMinute(const std::string& stamp,const std::string& day){
    if(stamp.size()<16)return -1;
    int hour=atoi(stamp.substr(11,2).c_str()),minute=atoi(stamp.substr(14,2).c_str());
    if(hour<0||hour>23||minute<0||minute>59)return -1;
    return hour*60+minute+(stamp.substr(0,10)>day?1440:stamp.substr(0,10)<day?-1440:0);
}
inline int graphLastMinute(const std::vector<std::string>& stamps,const std::string& day){return stamps.empty()?0:std::clamp(graphStampMinute(stamps.back(),day),0,1440);}
inline WeatherCursor weatherCursor(const std::vector<std::string>& stamps,const std::string& day,double minute){
    WeatherCursor point;if(stamps.empty())return point;
    if(!std::isfinite(minute))return point;
    minute=std::clamp(minute,0.,double(graphLastMinute(stamps,day)));
    for(size_t i=0;i<stamps.size();++i){int at=graphStampMinute(stamps[i],day);if(at<0)continue;
        if(at==minute){point.lower=point.upper=i;point.position=i;return point;}
        if(at>minute){if(point.lower<0)return point;point.upper=i;int before=graphStampMinute(stamps[point.lower],day);point.fraction=double(minute-before)/(at-before);point.position=point.lower+(point.upper-point.lower)*point.fraction;return point;}
        point.lower=i;
    }
    point.upper=point.lower;point.position=std::max(0,point.lower);return point;
}
inline double cursorValue(const std::vector<double>& values,const WeatherCursor& point,bool hourlyTotal=false){
    if(point.lower<0||point.upper<0||point.upper>=int(values.size()))return std::numeric_limits<double>::quiet_NaN();
    if(hourlyTotal)return values[point.fraction>0?point.upper:point.lower];
    WeatherChart chart;chart.values=values;return chartValue(chart,point.position);
}
inline bool cursorEstimated(const WeatherCursor& point){return point.lower>=0&&point.fraction>0&&point.fraction<1;}
}
