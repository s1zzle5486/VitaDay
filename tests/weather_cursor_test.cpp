#include "../src/weather_cursor.hpp"
#include <cassert>
#include <iostream>
using namespace day;
int main(){
 std::vector<std::string> stamps;for(int h=0;h<=24;++h){char s[40];snprintf(s,sizeof(s),"2026-10-%02dT%02d:00",h==24?8:7,h%24);stamps.push_back(s);}
 assert(graphLastMinute(stamps,"2026-10-07")==1440);
 std::vector<double> values;for(int h=0;h<=24;++h)values.push_back(h*2.);
 auto p=weatherCursor(stamps,"2026-10-07",13*60+37);assert(p.lower==13&&p.upper==14&&cursorEstimated(p));assert(std::abs(cursorValue(values,p)-(26+74/60.))<1e-9);assert(cursorValue(values,p,true)==28);
 for(int minute=0;minute<=1440;++minute){auto at=weatherCursor(stamps,"2026-10-07",minute);assert(std::abs(cursorValue(values,at)-minute/30.)<1e-9);assert(cursorEstimated(at)==bool(minute%60));}
 for(double minute=816.;minute<818.;minute+=.03125){auto at=weatherCursor(stamps,"2026-10-07",minute);assert(std::abs(cursorValue(values,at)-minute/30.)<1e-9);}
 assert(weatherCursor(stamps,"2026-10-07",NAN).lower<0);
 p=weatherCursor(stamps,"2026-10-07",1440);assert(p.lower==24&&p.upper==24&&!cursorEstimated(p));assert(cursorValue(values,p)==48);
 stamps.pop_back();assert(graphLastMinute(stamps,"2026-10-07")==1380);assert(weatherCursor(stamps,"2026-10-07",1440).position==23);
 assert(weatherCursor({},"2026-10-07",720).lower<0);assert(std::isnan(cursorValue({},{})));
 values[14]=NAN;p=weatherCursor(stamps,"2026-10-07",817);assert(std::isnan(cursorValue(values,p)));
 std::vector<std::string> gap={"2026-10-07T01:00","2026-10-07T03:00"};p=weatherCursor(gap,"2026-10-07",120);assert(p.fraction==.5&&p.position==.5);assert(weatherCursor(gap,"2026-10-07",0).lower<0);
 std::cout<<"Minute cursor: 1441 minutes, matching curve values, hourly totals, midnight, truncated days, missing values and timestamp gaps passed\n";
}
