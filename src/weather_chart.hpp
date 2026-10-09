#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace day {
// All palette stops use API units (C, mm/h, %, km/h, hPa, %).
// Display-unit conversion must never change the meaning of a colour.
struct ChartStop { double value; unsigned r,g,b; };
inline unsigned chartColor(int metric,double value) {
    static const std::vector<ChartStop> scales[]={
        {{-20,94,111,235},{0,66,172,240},{10,66,191,177},{20,232,193,79},{30,244,141,60},{40,232,83,94}},
        {{0,112,193,246},{2,55,157,239},{10,70,110,222},{25,115,83,203}},
        {{0,130,211,213},{50,62,186,195},{100,43,128,201}},
        {{0,83,194,177},{20,74,162,231},{50,140,118,233},{100,206,100,208}},
        {{980,157,119,215},{1013,82,166,220},{1040,76,194,175}},
        {{0,124,193,230},{50,140,170,199},{100,137,146,168}}
    };
    const auto& stops=scales[std::clamp(metric,0,5)];
    if(!std::isfinite(value))value=stops.front().value;
    size_t i=1;while(i<stops.size()-1&&value>stops[i].value)++i;
    const auto& a=stops[i-1];const auto& b=stops[i];
    double t=std::clamp((value-a.value)/(b.value-a.value),0.,1.);
    auto lerp=[&](unsigned x,unsigned y){return unsigned(std::lround(x+(double(y)-x)*t));};
    return lerp(a.r,b.r)|(lerp(a.g,b.g)<<8)|(lerp(a.b,b.b)<<16)|0xff000000u;
}
struct WeatherChart {
    static constexpr int width=472,height=147,padding=3,plotHeight=140;
    int metric=0;
    std::vector<double> values;
    double low=0,high=1,past=-1; // past is the fractional sample index at local now
    bool dark=false;
    bool operator==(const WeatherChart& other)const {
        if(metric!=other.metric||low!=other.low||high!=other.high||past!=other.past||dark!=other.dark||values.size()!=other.values.size())return false;
        for(size_t i=0;i<values.size();++i)if(values[i]!=other.values[i]&&!(std::isnan(values[i])&&std::isnan(other.values[i])))return false;
        return true;
    }
};
inline void chartRange(WeatherChart& c) {
    c.low=std::numeric_limits<double>::infinity();c.high=-c.low;
    for(double v:c.values)if(std::isfinite(v)){c.low=std::min(c.low,v);c.high=std::max(c.high,v);}
    if(!std::isfinite(c.low)){c.low=0;c.high=1;}
    if(c.metric==1||c.metric==2||c.metric==3||c.metric==5)c.low=0;
    if(c.metric==2||c.metric==5)c.high=100;
    if(c.high-c.low<1){if(c.metric==0||c.metric==4){c.low-=.5;c.high+=.5;}else c.high=c.low+1;}
}
// Locate now amongst local ISO timestamps, including the following midnight.
inline double chartPast(const std::vector<std::string>& stamps,const std::string& date,int hour,int minute) {
    if(stamps.empty())return -1;
    char clock[8];snprintf(clock,sizeof(clock),"%02d:%02d",hour,minute);
    std::string now=date+"T"+clock;
    if(now<stamps.front())return -1;
    for(size_t i=1;i<stamps.size();++i)if(now<stamps[i]){
        const auto& a=stamps[i-1];const auto& b=stamps[i];
        if(a.size()<16||b.size()<16)return double(i-1);
        int am=atoi(a.substr(11,2).c_str())*60+atoi(a.substr(14,2).c_str());
        int bm=atoi(b.substr(11,2).c_str())*60+atoi(b.substr(14,2).c_str());
        if(b.substr(0,10)!=a.substr(0,10))bm+=1440;
        int nm=hour*60+minute+(date!=a.substr(0,10)?1440:0);
        return i-1+std::clamp(double(nm-am)/std::max(1,bm-am),0.,1.);
    }
    return double(stamps.size());
}
inline double chartSlope(double before,double value,double after) {
    if(!std::isfinite(before)||!std::isfinite(after))return 0;
    double a=value-before,b=after-value;
    return a*b<=0?0:2*a*b/(a+b); // harmonic slope preserves local extrema
}
inline double chartValue(const WeatherChart& c,double position) {
    if(c.values.empty())return std::numeric_limits<double>::quiet_NaN();
    position=std::clamp(position,0.,double(c.values.size()-1));
    size_t i=std::min(size_t(position),c.values.size()-1);
    if(position==double(i)||i+1==c.values.size())return c.values[i];
    double a=c.values[i],b=c.values[i+1];if(!std::isfinite(a)||!std::isfinite(b))return std::numeric_limits<double>::quiet_NaN();
    double t=position-i,d=b-a,m0=i?chartSlope(c.values[i-1],a,b):d,m1=i+2<c.values.size()?chartSlope(a,b,c.values[i+2]):d;
    double v=(2*t*t*t-3*t*t+1)*a+(t*t*t-2*t*t+t)*m0+(-2*t*t*t+3*t*t)*b+(t*t*t-t*t)*m1;
    return std::clamp(v,std::min(a,b),std::max(a,b));
}
inline std::vector<unsigned char> chartPixels(const WeatherChart& c) {
    std::vector<unsigned char> pixels(WeatherChart::width*WeatherChart::height*4,0);
    if(c.values.empty()||!std::isfinite(c.high-c.low)||c.high<=c.low)return pixels;
    const int w=WeatherChart::width,h=WeatherChart::height;
    std::array<unsigned,WeatherChart::height> rowColors{};
    std::array<double,WeatherChart::height> rowAlpha{};
    for(int y=0;y<h;++y){
        double scaled=c.high-(y-WeatherChart::padding)*(c.high-c.low)/WeatherChart::plotHeight;
        rowColors[y]=chartColor(c.metric,scaled);
        double fade=std::clamp((WeatherChart::padding+WeatherChart::plotHeight-y)/double(WeatherChart::plotHeight),0.,1.);
        rowAlpha[y]=(c.dark?112.:100.)*std::pow(fade,.8);
    }
    double arc=0,previousY=0;bool previous=false;
    for(int x=0;x<w;++x){
        if(c.values.size()==1&&x>2)continue;
        double position=double(x)*(c.values.size()-1)/(w-1),v;
        bool edge=true;double dash;
        if(c.metric==1){
            int index=int(std::lround(position));double spacing=double(w-1)/std::max(size_t(1),c.values.size()-1);
            edge=std::abs(position-index)*spacing<=std::min(6.,spacing*.32);
            v=c.values[index];dash=x;
        }else{v=chartValue(c,position);dash=arc;}
        if(!edge||!std::isfinite(v)||(c.metric==1&&v<=0)){previous=false;continue;}
        double cy=WeatherChart::padding+WeatherChart::plotHeight*(1-std::clamp((v-c.low)/(c.high-c.low),0.,1.));
        if(previous)arc+=std::hypot(1.,cy-previousY);else arc=0;
        if(c.metric!=1)dash=arc;
        previous=true;previousY=cy;
        bool past=position<c.past,stroke=!past||std::fmod(dash,10.)<6.;
        for(int y=0;y<h;++y){
            double distance=std::abs(y-cy),alpha=0;
            unsigned color=rowColors[y];
            if(y>=cy&&y<=WeatherChart::padding+WeatherChart::plotHeight){
                alpha=rowAlpha[y]*(past?.55:1.);
            }
            if(c.metric==1&&stroke&&distance<2.2){
                color=chartColor(c.metric,v);
                alpha=std::max(alpha,255.*std::clamp(2.2-distance,0.,1.)*(past?.82:1.));
            }
            auto* p=pixels.data()+(y*w+x)*4;p[0]=color&255;p[1]=(color>>8)&255;p[2]=(color>>16)&255;p[3]=unsigned(std::lround(alpha));
        }
    }
    // Rasterize connected curve segments, rather than isolated vertical slices.
    // On steep slopes adjacent slice centres can be many pixels apart.
    if(c.metric!=1){
        struct StrokeSample {float distanceSquared=std::numeric_limits<float>::infinity(),position=0,arc=0,value=0;};
        std::vector<StrokeSample> nearest(w*h);
        // Find the closest point on the entire polyline first. Rounded joins
        // cover the wedges between adjacent segments without filling dash gaps.
        auto strokeSegment=[&](double ax,double ay,double av,double bx,double by,double bv,double startArc,double length){
            double dx=bx-ax,dy=by-ay,norm=dx*dx+dy*dy;
            int left=std::max(0,int(std::floor(ax-2.2))),right=std::min(w-1,int(std::ceil(bx+2.2)));
            int top=std::max(0,int(std::floor(std::min(ay,by)-2.2))),bottom=std::min(h-1,int(std::ceil(std::max(ay,by)+2.2)));
            for(int x=left;x<=right;++x)for(int y=top;y<=bottom;++y){
                double t=norm?std::clamp(((x-ax)*dx+(y-ay)*dy)/norm,0.,1.):0.;
                double px=ax+t*dx,py=ay+t*dy,distanceSquared=(x-px)*(x-px)+(y-py)*(y-py);
                auto& sample=nearest[y*w+x];
                if(distanceSquared>=2.2*2.2||distanceSquared>=sample.distanceSquared)continue;
                sample.distanceSquared=distanceSquared;
                sample.position=px*(c.values.size()-1)/(w-1);
                sample.arc=startArc+t*length;sample.value=av+t*(bv-av);
            }
        };
        double lastY=0,lastValue=0,arc=0;bool connected=false;
        for(int x=0;x<w;++x){
            if(c.values.size()==1&&x>2)break;
            double v=chartValue(c,double(x)*(c.values.size()-1)/(w-1));
            if(!std::isfinite(v)){connected=false;continue;}
            double y=WeatherChart::padding+WeatherChart::plotHeight*(1-std::clamp((v-c.low)/(c.high-c.low),0.,1.));
            if(connected){
                double length=std::hypot(1.,y-lastY);
                strokeSegment(x-1,lastY,lastValue,x,y,v,arc,length);arc+=length;
            }else{arc=0;strokeSegment(x,y,v,x,y,v,0,0);}
            lastY=y;lastValue=v;connected=true;
        }
        for(size_t i=0;i<nearest.size();++i){
            const auto& sample=nearest[i];if(!std::isfinite(sample.distanceSquared))continue;
            bool past=sample.position<c.past;
            if(past&&std::fmod(sample.arc,10.f)>=6.f)continue;
            unsigned alpha=unsigned(std::lround(255.*std::clamp(2.2-std::sqrt(double(sample.distanceSquared)),0.,1.)*(past?.82:1.)));
            auto* p=pixels.data()+i*4;if(alpha<=p[3])continue;
            unsigned color=chartColor(c.metric,sample.value);
            p[0]=color&255;p[1]=(color>>8)&255;p[2]=(color>>16)&255;p[3]=alpha;
        }
    }
    return pixels;
}
}
