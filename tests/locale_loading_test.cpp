#include "../src/localization.hpp"
#include <cassert>
#include <filesystem>
#include <thread>
#include <vector>
#include <iostream>
using namespace day;
int main(){
 const auto root=(std::filesystem::path(__FILE__).parent_path().parent_path()/"assets/locales").string();
 assert(configureLanguages(root,11));
 for(int i=0;i<20;++i)assert(localeReady()[i].load()==(i==11));
 const char* chinese=localized(11,"Осадки","Precipitation");
 assert(std::string(chinese)=="降水");
 assert(std::string(localized(11,"Давление","Pressure"))=="气压");
 std::vector<std::string> results(8);std::vector<std::thread> workers;
 for(int i=0;i<8;++i)workers.emplace_back([&,i]{results[i]=localized(8,"Язык","Language");});
 for(auto& worker:workers)worker.join();
 for(const auto& result:results)assert(result==locale(8)["ui"]["Language"].get<std::string>());
 assert(localeReady()[8].load()&&!localeReady()[1].load());
 assert(chinese==localized(11,"Осадки","Precipitation"));
 assert(localizedDate({2026,10,7},0)=="7 октября 2026");
 assert(std::string(localized(1,"Язык","Language"))=="Language");
 assert(configureLanguages(root+"/does-not-exist",1)==false);
 assert(localeReady()[1].load());
 assert(std::string(localized(1,"Язык","Language"))=="Language");
 assert(std::string(localized(0,"Язык","Language"))=="Язык");
 assert(loadLanguages(root));
 for(int i=0;i<20;++i)assert(localeReady()[i].load());
 std::cout<<"Lazy active language, concurrent first access, stable translations, dates and missing-file fallback: PASS\n";
}
