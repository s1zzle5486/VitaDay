#include "../src/weather_chart.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){
 std::vector<std::string> times;for(int h=0;h<=24;++h){char s[40];snprintf(s,sizeof(s),"2026-10-%02dT%02d:00",h==24?8:7,h%24);times.push_back(s);}
 assert(chartPast(times,"2026-10-07",18,30)==18.5);
 assert(chartPast(times,"2026-10-06",23,59)==-1);
 assert(chartPast(times,"2026-10-08",0,0)==25);
 assert(chartPast(times,"2026-10-07",0,0)==0);
 assert(chartPast(times,"2026-10-07",23,30)==23.5);
 WeatherChart c;c.values={-5,10,10,-8};chartRange(c);assert(c.low==-8&&c.high==10);
 for(int i=0;i<3;++i)for(int j=0;j<=100;++j){double v=chartValue(c,i+j/100.);assert(v>=std::min(c.values[i],c.values[i+1])&&v<=std::max(c.values[i],c.values[i+1]));}
 c.values={10,10};chartRange(c);assert(c.low<10&&c.high>10);assert(chartValue(c,.5)==10);
 auto solid=chartPixels(c);c.past=2;auto dashed=chartPixels(c);
 int on=0,off=0;int cy=WeatherChart::padding+WeatherChart::plotHeight/2;
 for(int x=0;x<WeatherChart::width;++x){assert(solid[(cy*WeatherChart::width+x)*4+3]==255);if(dashed[(cy*WeatherChart::width+x)*4+3]>150)++on;else ++off;}
 assert(on>200&&off>100);
 c.past=.5;auto half=chartPixels(c);assert(half[(cy*WeatherChart::width+400)*4+3]==255);
 // Fill is stronger near the curve, transparent at the bottom.
 assert(half[((cy+4)*WeatherChart::width+400)*4+3]>half[((cy+50)*WeatherChart::width+400)*4+3]);
 assert(half[((WeatherChart::padding+WeatherChart::plotHeight)*WeatherChart::width+400)*4+3]==0);
 c.values={10,NAN,20};chartRange(c);auto gaps=chartPixels(c);assert(std::isnan(chartValue(c,.5)));for(int y=0;y<WeatherChart::height;++y)assert(gaps[(y*WeatherChart::width+235)*4+3]==0);
 WeatherChart copy=c;assert(copy==c);copy.values[0]=11;assert(!(copy==c));
 c.values.clear();chartRange(c);auto empty=chartPixels(c);for(auto byte:empty)assert(byte==0);
 c.metric=1;c.values={0,0,0};chartRange(c);auto dry=chartPixels(c);for(auto byte:dry)assert(byte==0);
 c.values={0,2,0};chartRange(c);auto rain=chartPixels(c);assert(rain[(3*WeatherChart::width+235)*4+3]>0);assert(rain[(50*WeatherChart::width+100)*4+3]==0);
 c.metric=0;c.values={10};chartRange(c);auto single=chartPixels(c);for(int y=0;y<WeatherChart::height;++y)assert(single[(y*WeatherChart::width+200)*4+3]==0);
 c.metric=2;c.values={38,80};chartRange(c);assert(c.low==0&&c.high==100);
 assert(chartColor(0,10)!=chartColor(0,30));
 // Fixed physical scale: the same temperature keeps its colour in any day's range.
 WeatherChart cold,warm;cold.values={-10,10};warm.values={10,40};chartRange(cold);chartRange(warm);assert(chartColor(0,chartValue(cold,1))==chartColor(0,chartValue(warm,0)));
 for(int m=0;m<6;++m){c.metric=m;c.values={-5,5,30,10};chartRange(c);auto pixels=chartPixels(c);assert(pixels.size()==WeatherChart::width*WeatherChart::height*4);}
 // A sharp drop/rise must remain connected in the future, including after
 // the time cutoff. Use many samples so the slope spans > 4 pixels/column.
 for(int metric : {0,2,3,4,5})for(bool rising : {false,true}){
  WeatherChart steep;steep.metric=metric;steep.low=0;steep.high=100;
  steep.values.assign(60,rising?0.:100.);
  for(size_t i=31;i<steep.values.size();++i)steep.values[i]=rising?100.:0.;
  for(double cutoff : {-1.,29.5}){
   steep.past=cutoff;auto image=chartPixels(steep);
   int start=cutoff<0?0:int(std::ceil(cutoff*(WeatherChart::width-1)/59))+3;
   auto yAt=[&](int x){return int(std::lround(WeatherChart::padding+WeatherChart::plotHeight*(1-chartValue(steep,double(x)*59/(WeatherChart::width-1))/100)));};
   auto opaque=[&](int x,int y){return image[(y*WeatherChart::width+x)*4+3]>=150;};
   std::vector<unsigned char> seen(WeatherChart::width*WeatherChart::height,0);
   std::vector<int> queue={yAt(start)*WeatherChart::width+start};seen[queue[0]]=1;
   for(size_t head=0;head<queue.size();++head){int at=queue[head],x=at%WeatherChart::width,y=at/WeatherChart::width;
    for(auto d : {std::pair<int,int>{-1,0},{1,0},{0,-1},{0,1}}){int nx=x+d.first,ny=y+d.second;
     if(nx<start||nx>=WeatherChart::width||ny<0||ny>=WeatherChart::height)continue;
     int next=ny*WeatherChart::width+nx;if(!seen[next]&&opaque(nx,ny)){seen[next]=1;queue.push_back(next);}
    }
   }
   for(int x=start;x<WeatherChart::width;++x)assert(seen[yAt(x)*WeatherChart::width+x]);
  }
  steep.past=60;auto past=chartPixels(steep);int gaps=0;
  for(int x=0;x<200;++x){int y=rising?143:3;if(past[(y*WeatherChart::width+x)*4+3]<100)++gaps;}
  assert(gaps>20);
 }
 // Every pixel within the opaque core of the full polyline must be filled.
 // Connectivity alone cannot detect a tiny wedge missing at an angled join.
 WeatherChart joins;joins.values={0,100,0,100};joins.low=0;joins.high=100;
 for(bool reverse:{false,true}){
  if(reverse)std::reverse(joins.values.begin(),joins.values.end());
  auto pixels=chartPixels(joins);
  std::vector<double> ys(WeatherChart::width);
  for(int x=0;x<WeatherChart::width;++x)ys[x]=WeatherChart::padding+WeatherChart::plotHeight*(1-chartValue(joins,double(x)*3/(WeatherChart::width-1))/100);
  int checked=0;
  for(int x=0;x<WeatherChart::width;++x)for(int y=0;y<WeatherChart::height;++y){
   double closest=1e9;
   for(int end=std::max(1,x-3);end<=std::min(WeatherChart::width-1,x+4);++end){
    double dy=ys[end]-ys[end-1],t=std::clamp(((x-(end-1))+(y-ys[end-1])*dy)/(1+dy*dy),0.,1.);
    double dx=x-(end-1+t),distanceY=y-(ys[end-1]+t*dy);closest=std::min(closest,dx*dx+distanceY*distanceY);
   }
   if(closest<=1.19*1.19){assert(pixels[(y*WeatherChart::width+x)*4+3]==255);++checked;}
  }
  assert(checked>1000);
 }
 std::cout<<"Weather charts: local time cutoff, midnight, monotone interpolation, dashed past, fill fade, gaps, rain and fixed physical palettes passed\n";
}
