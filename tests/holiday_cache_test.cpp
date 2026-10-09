#include "../src/core.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace day;
int main(){
 State s;s.location.valid=true;s.location.country="US";s.international=false;
 HolidayCache first{2026,"",1,{{"2026-12-25","Christmas","US","Christmas Day"}}};
 s.caches["US"]=first;archiveHolidayCache(s,"US",first);
 s.caches["US"]={2027,"",2,{{"2027-12-25","Christmas","US","Christmas Day"}}};
 archiveHolidayCache(s,"US",s.caches["US"]);
 assert(holidayItems(s,{2026,12,25}).size()==1&&holidayItems(s,{2027,12,25}).size()==1);
 s.location.region="NY";assert(holidayItems(s,{2026,12,25}).empty());
 archiveHolidayCache(s,"US",{2026,"NY",3,{{"2026-12-25","Christmas","US","Christmas Day"}}});
 assert(holidayItems(s,{2026,12,25}).size()==1);
 auto folder=std::filesystem::temp_directory_path()/"vitaday-holiday-cache-test";std::filesystem::create_directories(folder);
 assert(save(folder.string(),s));auto restored=load(folder.string());
 assert(findHolidayCache(restored,"US",2026,"")&&findHolidayCache(restored,"US",2026,"NY")&&findHolidayCache(restored,"US",2027,""));
 restored.followLocal=false;assert(holidayItems(restored,{2026,12,25}).empty());assert(save(folder.string(),restored));restored=load(folder.string());restored.followLocal=true;assert(holidayItems(restored,{2026,12,25}).size()==1);
 for(int i=0;i<110;++i)archiveHolidayCache(restored,"US",{1900+i,"",i,{}});
 assert(restored.holidayArchive.size()==96);auto roundtrip=decode(encode(restored));assert(roundtrip.holidayArchive.size()==96);
 // Legacy snapshots with no archive still supply their latest country cache.
 auto legacy=encode(s);legacy.erase("holidayArchive");auto migrated=decode(legacy);assert(findHolidayCache(migrated,"US",2027,""));
 std::filesystem::remove_all(folder);
 std::cout<<"Holiday cache disk persistence, multiple years/regions, disabled tracking, legacy save and bounded eviction: PASS\n";
}
