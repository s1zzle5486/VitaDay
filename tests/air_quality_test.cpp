#include "../src/air_quality.hpp"
#include "../src/localization.hpp"
#include "../src/weather_chart.hpp"
#include <cassert>
#include <filesystem>
using namespace day;
int main(){
 Json forecast={{"utc_offset_seconds",7200},{"hourly",{{"time",{"2026-10-09T00:00","2026-10-09T01:00","2026-10-09T02:00","2026-10-10T00:00"}}}}};
 Json air={{"utc_offset_seconds",7200},{"hourly",{{"time",{"2026-10-09T02:00","2026-10-09T00:00","2026-10-09T01:00"}},{"ozone",{42.,0.,nullptr}},{"pm2_5",{8.,-1.,"bad"}}}}};
 assert(mergeAirQuality(forecast,air));assert(forecast["hourly"]["ozone"][0]==0.);assert(forecast["hourly"]["ozone"][1].is_null());assert(forecast["hourly"]["ozone"][2]==42.);assert(forecast["hourly"]["ozone"][3].is_null());assert(forecast["hourly"]["pm2_5"][0].is_null());assert(forecast["hourly"]["pm2_5"][2]==8.);
 Json before=forecast;air["utc_offset_seconds"]=0;assert(!mergeAirQuality(forecast,air)&&forecast==before);assert(!mergeAirQuality(forecast,Json::object()));
 Weather weather;weather.details=forecast;auto restored=weatherFromJson(weatherJson(weather));assert(restored.details==forecast);
 for(int metric=6;metric<10;++metric){WeatherChart c;c.metric=metric;c.values={NAN,0.,100.,NAN};chartRange(c);assert(c.low==0&&c.high==100);assert(std::isnan(chartValue(c,.5)));assert(chartValue(c,1.5)>=0&&chartValue(c,1.5)<=100);assert(chartColor(metric,0)!=chartColor(metric,800));assert(chartPixels(c).size()==size_t(WeatherChart::width*WeatherChart::height*4));}
 const auto fixtures=std::filesystem::path(__FILE__).parent_path()/"fixtures";Json liveSolar,liveAir;std::ifstream(fixtures/"solar-risuty.json")>>liveSolar;std::ifstream(fixtures/"air-risuty.json")>>liveAir;assert(mergeAirQuality(liveSolar,liveAir));assert(liveSolar["hourly"]["ozone"].size()==264);assert(liveSolar["hourly"]["ozone"][240].is_null());assert(liveSolar["hourly_units"]["direct_radiation"]=="W/m²");
 assert(loadLanguages("assets/locales"));assert(std::string(localized(11,"Осадки","Precipitation"))=="降水");assert(std::string(localized(11,"Давление","Pressure"))=="气压");
}
