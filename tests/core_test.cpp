#include "../src/core.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace day;
int main(){
    assert(days(2024,2)==29&&days(2100,2)==28&&days(2000,2)==29);
    assert(!valid(parseDate("2025-02-29"))&&!valid(parseDate("2026-10-07garbage")));
    assert(weekday({2026,10,6})==1&&weekday({2024,2,29})==3);
    assert(key(shift({2024,2,28},1))=="2024-02-29");
    assert(key(shift({2024,3,1},-1))=="2024-02-29");
    assert(key(month({2024,1,31},1))=="2024-02-29");
    assert(key(month({2026,12,31},1))=="2027-01-31");
    State s;s.marks["2024-02-29"]={"День рождения",4,true};s.marks["2026-10-06"]={"Путешествие",2,false};
    assert(marksFor(s,{2028,2,29}).size()==1&&marksFor(s,{2027,2,28}).empty());
    assert(editKey(s,{2028,2,29})=="2024-02-29");
    Location l;l.valid=true;l.country="US";l.region="CA";
    Json h=Json::parse(R"([{"date":"2026-01-01","global":true,"localName":"New Year"},{"date":"2026-03-31","global":false,"counties":["US-CA"],"localName":"CA holiday"},{"date":"2026-03-31","global":false,"counties":["US-NY"],"localName":"NY holiday"},{"date":"2025-01-01","global":true,"name":"Wrong year"}])");
    assert(parseHolidays(h,l,2026).size()==2);l.region="";assert(parseHolidays(h,l,2026).size()==1);
    s.location=l;s.holidays=parseHolidays(h,l,2026);s.holidayYear=2026;s.holidayCountry="US";
    assert(holidaysFor(s,{2026,1,1}).size()==1);s.location.country="DE";assert(holidaysFor(s,{2026,1,1}).empty());
    const std::string dir="work/test-save";std::filesystem::create_directories(dir);std::filesystem::remove(dir+"/store0.json");std::filesystem::remove(dir+"/store1.json");
    assert(save(dir,s));s.marks["2026-10-07"]={"Новая запись",6,true};assert(save(dir,s));assert(load(dir).marks.size()==3);
    std::ofstream(dir+"/store0.json")<<"{broken";State recovered=load(dir);assert(recovered.revision==1&&recovered.marks.size()==2&&recovered.marks.at("2026-10-06").title=="Путешествие");
    assert(save(dir,recovered)&&load(dir).revision==2);
    assert(clipUtf8("Привет",5)=="Пр");
    std::cout<<"Calendar, regional holidays, Unicode and interrupted-save recovery: PASS\n";
}
