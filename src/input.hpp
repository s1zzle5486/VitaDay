#pragma once
#include <cstdint>
#include "core.hpp"
namespace day {
struct NavigationRepeat {
    unsigned previous=0,held=0;uint64_t next=0;
    unsigned poll(unsigned buttons,unsigned directions,uint64_t us){
        unsigned pressed=buttons&~previous;previous=buttons;unsigned current=buttons&directions;
        if(current!=held){held=current;next=us+350000;}
        else if(held&&us>=next){pressed|=held;next=us+85000;}
        return pressed;
    }
};
inline unsigned mappedButtons(unsigned b,bool circleEnter,unsigned cross,unsigned circle){if(!circleEnter)return b;unsigned out=b&~(cross|circle);if(b&circle)out|=cross;if(b&cross)out|=circle;return out;}
inline unsigned stickDirection(int lx,int ly,int rx,int ry,unsigned left,unsigned right,unsigned up,unsigned down){int x=std::abs(lx-128)>=std::abs(rx-128)?lx-128:rx-128;int y=std::abs(ly-128)>=std::abs(ry-128)?ly-128:ry-128;if(std::max(std::abs(x),std::abs(y))<=64)return 0;if(std::abs(x)>std::abs(y))return x<0?left:right;return y<0?up:down;}
inline bool isDayHour(int hour,int dayStart,int nightStart){return dayStart==nightStart?true:dayStart<nightStart?(hour>=dayStart&&hour<nightStart):(hour>=dayStart||hour<nightStart);}
inline bool nightTheme(const State& s,int hour){return s.themeMode==1||(s.themeMode==2&&!isDayHour(hour,s.dayStart,s.nightStart));}
inline std::string shortDate(Date d){return std::to_string(d.d)+"."+(d.m<10?"0":"")+std::to_string(d.m)+"."+std::to_string(d.y);}
inline std::string searchFold(std::string s){for(size_t i=0;i<s.size();++i){unsigned char c=s[i];if(c>='A'&&c<='Z')s[i]+=32;else if(c==0xd0&&i+1<s.size()){unsigned char next=s[i+1];if(next>=0x90&&next<=0xaf){if(next<0xa0)s[i+1]=next+32;else{s[i]=char(0xd1);s[i+1]=next-32;}}else if(next==0x81){s[i]=char(0xd1);s[i+1]=char(0x91);}++i;}}return s;}
inline bool matchesCountry(const std::string& query,const std::string& code,const std::string& name,const std::string& local){auto q=searchFold(trim(query));if(q.size()==2&&q[0]>='a'&&q[0]<='z'&&q[1]>='a'&&q[1]<='z')return searchFold(code)==q;return q.empty()||searchFold(code+" "+name+" "+local).find(q)!=std::string::npos;}
inline int zoneOffset(const Json& zones,const std::string& id,long long epoch,int fallback=0){auto z=zones.find(id);if(z==zones.end())return fallback;int offset=0;for(const auto& point:*z){if(point[0].get<long long>()>epoch)break;offset=point[1];}return offset;}
}
