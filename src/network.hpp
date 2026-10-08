#pragma once
#include "core.hpp"
#include "address.hpp"
#include <curl/curl.h>
#ifdef __vita__
#include <psp2/kernel/threadmgr.h>
#else
#include <thread>
#include <chrono>
#endif

namespace day {
inline size_t receive(char* p,size_t a,size_t b,void* data) {
    auto& s=*static_cast<std::string*>(data);size_t n=a*b;if(n>1024*1024||s.size()+n>1024*1024)return 0;s.append(p,n);return n;
}
inline Json request(const std::string& url,const char* ca="app0:assets/cacert.pem") {
    CURL* c=curl_easy_init();if(!c)throw std::runtime_error("Нет памяти для сети");std::string body;char detail[CURL_ERROR_SIZE]{};curl_easy_setopt(c,CURLOPT_ERRORBUFFER,detail);
    curl_easy_setopt(c,CURLOPT_URL,url.c_str());curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(c,CURLOPT_WRITEDATA,&body);
    curl_easy_setopt(c,CURLOPT_CAINFO,ca);curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);
    curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,8L);curl_easy_setopt(c,CURLOPT_TIMEOUT,18L);curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(c,CURLOPT_USERAGENT,"VitaDay/0.9.6");
    CURLcode r=curl_easy_perform(c);long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);std::string error=detail[0]?detail:curl_easy_strerror(r);curl_easy_cleanup(c);
    if(r!=CURLE_OK)throw std::runtime_error(error);if(status!=200)throw std::runtime_error("HTTP "+std::to_string(status));return Json::parse(body);
}
inline std::string escape(const std::string& s){CURL* c=curl_easy_init();if(!c)throw std::runtime_error("Нет памяти");char* p=curl_easy_escape(c,s.c_str(),s.size());if(!p){curl_easy_cleanup(c);throw std::runtime_error("Нет памяти");}std::string out=p;curl_free(p);curl_easy_cleanup(c);return out;}
inline void translateLocation(Location& l){
 if(!l.cityRu.empty()&&!l.cityEn.empty())return;
 try{Json names;std::ifstream f("app0:assets/city-names.json");f>>names;auto it=names.find(l.zone);std::string canonical=l.zone.substr(l.zone.rfind('/')+1);std::replace(canonical.begin(),canonical.end(),'_',' ');if(it!=names.end()&&(l.city==canonical||l.city==it->value("ru",std::string{})||l.city==it->value("en",std::string{}))){l.cityRu=it->value("ru",l.city);l.cityEn=it->value("en",canonical);return;}}catch(...){}
 try{
  if(!l.geoname){Json j=request("https://geocoding-api.open-meteo.com/v1/search?name="+escape(l.city)+"&count=10&language=ru&countryCode="+l.country);if(j.contains("results"))for(const auto& g:j["results"])if(std::abs(g.value("latitude",90.)-l.lat)<.2&&std::abs(g.value("longitude",180.)-l.lon)<.2){l.geoname=g.value("id",0);l.cityRu=g.value("name",l.city);break;}}
  if(l.geoname)for(const auto* lang:{"ru","en"}){auto& name=std::string(lang)=="ru"?l.cityRu:l.cityEn;if(!name.empty())continue;Json g=request("https://geocoding-api.open-meteo.com/v1/get?id="+std::to_string(l.geoname)+"&language="+lang);name=g.value("name",l.city);}
 }catch(...){} // Previously cached names remain usable offline.
}
inline Json localizedSearch(const std::string& query,const std::string& cc,int language){
 static Json cache=Json::object();static bool loaded=false;const std::string file="ux0:data/VitaDay/search-cache.json";if(!loaded){loaded=true;try{std::ifstream f(file);f>>cache;if(!cache.is_object())cache=Json::object();}catch(...){}}
 std::string key=query+"|"+cc+"|"+std::to_string(language);if(cache.contains(key))return cache[key];
 std::string endpoint="https://nominatim.openstreetmap.org/search";try{Json config;std::ifstream f("ux0:data/VitaDay/services.json");f>>config;auto value=config.value("citySearch",endpoint);if(value.rfind("https://",0)==0)endpoint=value;}catch(...){}
 // Only explicit submitted searches use this service; no autocomplete or background queries.
 static time_t lastRequest=0;while(std::time(nullptr)<=lastRequest){
 #ifdef __vita__
 sceKernelDelayThread(100000);
 #else
 std::this_thread::sleep_for(std::chrono::milliseconds(100));
 #endif
 }lastRequest=std::time(nullptr)+1;
 Json found=request(endpoint+( !cc.empty()&&usesPostal(cc)&&validPostal(cc,normalizePostal(cc,query))?"?postalcode=":"?q=")+escape(query)+"&format=jsonv2&addressdetails=1&namedetails=1&accept-language="+(language?"en":"ru")+"&limit=10"+(cc.empty()?"":"&countrycodes="+cc));Json out=Json::array();
 for(const auto& g:found){Json address=g.value("address",Json::object()),names=g.value("namedetails",Json::object());std::string country=upper(address.value("country_code",std::string{}));if(!countryValid(country)||(!cc.empty()&&country!=cc))continue;Location l;l.valid=true;l.automatic=false;l.country=country;l.lat=std::stod(g.at("lat").get<std::string>());l.lon=std::stod(g.at("lon").get<std::string>());l.city=address.value("city",address.value("town",address.value("village",g.value("name",std::string{}))));l.postal=address.value("postcode",std::string{});l.region=address.value("ISO3166-2-lvl4",std::string{});if(l.region.rfind(country+"-",0)==0)l.region=l.region.substr(3);if(l.region.empty())l.region=resolveRegion(country,address.value("state",std::string{}));if(l.city.empty())continue;l.cityRu=names.value("name:ru",l.city);l.cityEn=names.value("name:en",names.value("int_name",l.city));if(g.value("type",std::string{})=="postcode"){l.cityRu=language?"":l.city;l.cityEn=language?l.city:"";}bool duplicate=false;for(const auto& item:out){auto old=decodeLocation(item.at("location"));if(old.country==l.country&&old.cityEn==l.cityEn&&std::abs(old.lat-l.lat)<.2&&std::abs(old.lon-l.lon)<.2)duplicate=true;}if(!duplicate)out.push_back({{"location",encodeLocation(l)},{"region",address.value("state",std::string{})}});if(out.size()>=5)break;}
 if(!out.empty()){
  std::string lat,lon;for(const auto& item:out){auto l=decodeLocation(item.at("location"));if(!lat.empty()){lat+=",";lon+=",";}lat+=std::to_string(l.lat);lon+=std::to_string(l.lon);}
  Json zones=request("https://api.open-meteo.com/v1/forecast?latitude="+lat+"&longitude="+lon+"&current=temperature_2m&forecast_days=1&timezone=auto");if(!zones.is_array())zones=Json::array({zones});if(zones.size()!=out.size())throw std::runtime_error("Timezone lookup failed");for(size_t i=0;i<out.size();++i){auto l=decodeLocation(out[i]["location"]);l.zone=zones[i].value("timezone",std::string{});l.offset=zones[i].value("utc_offset_seconds",0);out[i]["location"]=encodeLocation(l);}
  if(cache.size()>=24)cache.erase(cache.begin());cache[key]=out;try{std::ofstream f(file);f<<cache.dump();}catch(...){}
 }return out;
}
inline WeatherPlace fetchForecast(Location location){
        translateLocation(location);
        char coords[120];snprintf(coords,sizeof(coords),"latitude=%.6f&longitude=%.6f",location.lat,location.lon);
        Json j=request(std::string("https://api.open-meteo.com/v1/forecast?")+coords+"&current=temperature_2m,apparent_temperature,weather_code,wind_speed_10m,relative_humidity_2m&daily=temperature_2m_max,temperature_2m_min,weather_code,precipitation_sum,precipitation_probability_max,wind_speed_10m_max,sunrise,sunset,uv_index_max&hourly=temperature_2m,apparent_temperature,relative_humidity_2m,precipitation_probability,precipitation,rain,showers,weather_code,wind_speed_10m,wind_gusts_10m,pressure_msl,cloud_cover&forecast_days=11&timezone=auto");
        for(auto& values:j["daily"])if(values.is_array()&&values.size()>10)values.erase(values.begin()+10,values.end());WeatherPlace p;p.location=location;p.weather=parseWeather(j);p.location.offset=j.value("utc_offset_seconds",location.offset);p.location.zone=j.value("timezone",location.zone);p.updated=std::time(nullptr);return p;
}
struct Refresh { State state; Location origin;int year=2026;std::string query,message,report;bool any=false;Json cities=Json::array(); };
inline void refresh(Refresh& job) {
    State& s=job.state;Location old=s.location;
    auto t=[&](const char* ru,const char* en){return s.language?en:ru;};
    auto log=[&](const std::string& line){job.report+=line+"\n\n";};
    log(std::string(t("Год праздников: ","Holiday year: "))+std::to_string(job.year));
    try {
        if(!job.query.empty()) {
            std::string name=job.query,cc;size_t comma=name.rfind(',');if(comma!=std::string::npos){cc=upper(trim(name.substr(comma+1)));if(countryValid(cc))name=trim(name.substr(0,comma));else cc.clear();}
            if(!cc.empty()&&usesPostal(cc)&&validPostal(cc,normalizePostal(cc,name))){job.cities=localizedSearch(normalizePostal(cc,name),cc,s.language);for(auto& result:job.cities){auto l=decodeLocation(result["location"]);l.postal=normalizePostal(cc,name);result["location"]=encodeLocation(l);}if(job.cities.empty())throw std::runtime_error(t("Индекс не найден","Postal code not found"));return;}
            Json j=request("https://geocoding-api.open-meteo.com/v1/search?name="+escape(name)+"&count=10&language="+(s.language?"en":"ru")+"&format=json");
            if(!j.contains("results")||j["results"].empty()){job.cities=localizedSearch(name,cc,s.language);if(job.cities.empty())job.cities=localizedSearch(name,cc,s.language);if(job.cities.empty())throw std::runtime_error(t("Город не найден","City not found"));return;}
            for(const auto& g:j["results"]){if(!cc.empty()&&g.value("country_code",std::string{})!=cc)continue;Location l;l.valid=true;l.automatic=false;l.city=g.at("name");l.geoname=g.value("id",0);if(s.language)l.cityEn=l.city;else l.cityRu=l.city;l.country=g.value("country_code",std::string{});l.region=resolveRegion(l.country,g.value("admin1",std::string{}));l.lat=g.at("latitude");l.lon=g.at("longitude");l.zone=g.value("timezone",std::string{});l=decodeLocation(encodeLocation(l));if(l.valid)job.cities.push_back({{"location",encodeLocation(l)},{"region",g.value("admin1",std::string{})}});}
            if(job.cities.empty())job.cities=localizedSearch(name,cc,s.language);if(job.cities.empty())throw std::runtime_error(t("Город не найден","City not found"));return;
        } else if(s.location.automatic) s.location=parseIP(request("https://ipwho.is/"));
        if(!s.location.valid)throw std::runtime_error("Место ещё не определено");
        if(old.lat!=s.location.lat||old.lon!=s.location.lon)s.weather=Weather{};
        job.any=true;
    }catch(const std::exception& e){job.message=std::string(t("Место: ","Location: "))+e.what();log(job.message);if(!job.query.empty()||(!s.location.valid&&activeCountries(s).empty()&&s.regions.empty()))return;}
    if(s.location.valid)try {auto place=fetchForecast(s.location);s.weather=place.weather;s.location=place.location;s.updated=place.updated;job.any=true;log(s.location.city+t(": основная погода загружена",": main forecast loaded"));}catch(const std::exception& e){job.message+=std::string(t("Погода: ","Weather: "))+e.what();log(std::string(t("Основная погода: ","Main forecast: "))+e.what());}
    // Resolve selected countries to an explicit city before requesting forecasts.
    for(const auto& c:s.countries){
        if(!c.cityEnabled||c.zone.empty()||s.regions.size()>=16)continue;
        bool exists=std::any_of(s.regions.begin(),s.regions.end(),[&](const WeatherPlace& p){return countryPlace(c,p.location);});if(exists)continue;
        try{
            auto slash=c.zone.rfind('/');std::string canonical=c.zone.substr(slash==std::string::npos?0:slash+1);std::replace(canonical.begin(),canonical.end(),'_',' ');
            std::vector<std::string> names;if(!c.clockCity.empty())names.push_back(c.clockCity);if(names.empty()||names.front()!=canonical)names.push_back(canonical);
            bool found=false;
            for(const auto& name:names){
                Json cities=request("https://geocoding-api.open-meteo.com/v1/search?name="+escape(name)+"&count=100&language="+(s.language?"en":"ru")+"&format=json");
                if(!cities.contains("results"))continue;
                for(const auto& city:cities["results"]){
                    if(city.value("country_code",std::string{})!=c.code||city.value("timezone",std::string{})!=c.zone)continue;
                    Location l;l.valid=true;l.automatic=false;l.city=c.clockCity.empty()?city.at("name").get<std::string>():c.clockCity;l.country=c.code;l.zone=c.zone;l.geoname=city.value("id",0);if(s.language)l.cityEn=city.value("name",l.city);else l.cityRu=city.value("name",l.city);l.lat=city.at("latitude");l.lon=city.at("longitude");
                    s.regions.push_back({l,{},0,1});job.any=true;found=true;break;
                }if(found)break;
            }
            if(!found)throw std::runtime_error(t("Город не найден: ","City not found: ")+names.front());
        }catch(const std::exception& e){job.message+=(job.message.empty()?"":" / ");job.message+=c.code+": "+e.what();log(c.code+": "+e.what());}
    }
    for(auto& place:s.regions){if(place.weather.valid&&std::time(nullptr)-place.updated<900){log(place.location.city+t(": сохранённая погода актуальна",": cached forecast is current"));continue;}try{auto origin=place.origin;bool clock=place.clock;place=fetchForecast(place.location);place.origin=origin;place.clock=clock;job.any=true;log(place.location.city+t(": погода загружена",": forecast loaded"));}catch(const std::exception& e){job.message+=(job.message.empty()?"":" / ");job.message+=place.location.city+": "+e.what();log(place.location.city+": "+e.what());}}

    if(s.catalog.empty())try{Json countries=request("https://date.nager.at/api/v3/AvailableCountries");for(const auto& c:countries){std::string code=c.at("countryCode");if(countryValid(code)&&s.catalog.size()<300)s.catalog.push_back({code,c.at("name"),1});}job.any=true;}catch(...){/* Offline country list remains available in the UI. */}
    auto active=activeCountries(s);for(auto it=s.caches.begin();it!=s.caches.end();){bool keep=std::any_of(active.begin(),active.end(),[&](const Country& c){return c.code==it->first;});if(!keep)it=s.caches.erase(it);else ++it;}
    for(const auto& c:active)try{
        Location location=s.location;location.country=c.code;location.region=c.code==s.location.country?s.location.region:"";
        auto cached=s.caches.find(c.code);if(cached!=s.caches.end()&&cached->second.year==job.year&&cached->second.region==location.region&&std::time(nullptr)-cached->second.fetched<86400){log(c.code+t(": праздники из кэша · ",": cached holidays · ")+std::to_string(cached->second.items.size()));continue;}
        auto h=request("https://date.nager.at/api/v3/PublicHolidays/"+std::to_string(job.year)+"/"+c.code);
        s.caches[c.code]={job.year,location.region,std::time(nullptr),parseHolidays(h,location,job.year)};job.any=true;log(c.code+t(": праздники загружены · ",": holidays loaded · ")+std::to_string(s.caches[c.code].items.size()));
    }catch(const std::exception& e){job.message+=(job.message.empty()?"":" / ");job.message+=std::string(t("Праздники ","Holidays "))+c.code+": "+e.what();log(std::string(t("Праздники ","Holidays "))+c.code+": "+e.what());}
    for(const auto& c:s.countries)if(!c.holidays)log(c.code+t(": отслеживание праздников выключено",": holiday tracking is off"));
    if(job.message.empty())job.message=t("Обновлено","Updated");
}
}

namespace day {
inline bool sameForecastPlace(const Location& a,const Location& b){if(a.country!=b.country)return false;if(a.geoname&&b.geoname)return a.geoname==b.geoname;if(locationKey(a)==locationKey(b))return true;return a.valid&&b.valid&&a.city==b.city&&std::abs(a.lat-b.lat)<.00001&&std::abs(a.lon-b.lon)<.00001;}
inline bool sameLocation(const Location& a,const Location& b){return locationKey(a)==locationKey(b)&&a.automatic==b.automatic&&a.lat==b.lat&&a.lon==b.lon&&a.region==b.region&&a.postal==b.postal;}
inline void mergeRefresh(State& state,const Refresh& job){
 if(sameLocation(state.location,job.origin)){state.location=job.state.location;if(job.state.weather.valid){state.weather=job.state.weather;state.updated=job.state.updated;}}
 for(const auto& place:job.state.regions){auto it=std::find_if(state.regions.begin(),state.regions.end(),[&](const WeatherPlace& p){return sameForecastPlace(p.location,place.location);});if(it!=state.regions.end()){if(place.weather.valid&&place.updated>=it->updated){for(auto& c:state.countries)if(countryPlace(c,it->location)){c.zone=place.location.zone;c.clockCity=place.location.city;c.cityPinned=true;c.cityLat=place.location.lat;c.cityLon=place.location.lon;}int origin=it->origin;bool clock=it->clock;*it=place;it->clock=clock;if(origin>=2)it->origin=origin;}}else if(state.regions.size()<16&&std::any_of(state.countries.begin(),state.countries.end(),[&](const Country& c){return countryPlace(c,place.location);}))state.regions.push_back(place);}
 for(const auto& entry:job.state.caches){auto it=state.caches.find(entry.first);if(it==state.caches.end()||entry.second.fetched>=it->second.fetched)state.caches[entry.first]=entry.second;}
 if(!job.state.catalog.empty())state.catalog=job.state.catalog;
}
}
