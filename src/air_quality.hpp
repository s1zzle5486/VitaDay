#pragma once
#include "core.hpp"
namespace day {
// Join by local timestamp, never array position: the two forecasts have different horizons.
inline bool mergeAirQuality(Json& forecast,const Json& air) {
 if(!forecast.contains("hourly")||!air.contains("hourly")||!forecast["hourly"].contains("time")||!air["hourly"].contains("time")||!forecast["hourly"]["time"].is_array()||!air["hourly"]["time"].is_array()||forecast.value("utc_offset_seconds",0)!=air.value("utc_offset_seconds",0))return false;
 std::map<std::string,size_t> indices;const auto& source=air["hourly"];
 for(size_t i=0;i<source["time"].size();++i)if(source["time"][i].is_string())indices[source["time"][i].get<std::string>()]=i;
 bool any=false;
 for(const char* field:{"ozone","pm2_5"}){
  Json values=Json::array();auto column=source.find(field);
  for(const auto& stamp:forecast["hourly"]["time"]){Json value=nullptr;if(stamp.is_string()){auto found=indices.find(stamp.get<std::string>());if(found!=indices.end()&&column!=source.end()&&column->is_array()&&found->second<column->size()&&(*column)[found->second].is_number()){double n=(*column)[found->second].get<double>();if(std::isfinite(n)&&n>=0){value=n;any=true;}}}values.push_back(value);}
  forecast["hourly"][field]=std::move(values);
 }
 return any;
}
}
