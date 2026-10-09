#include "../src/control_guide.hpp"
#include <cassert>
#include <fstream>
#include <set>
#include <iostream>
using namespace day;
int main(){std::set<std::string> paths;assert(loadLanguages("assets/locales"));
 for(int i=0;i<20;++i){auto path=controlGuidePath(i);assert(paths.insert(path).second);std::ifstream image(path.substr(5),std::ios::binary);assert(image);std::vector<unsigned char> pixels;unsigned w=0,h=0;std::string error;assert(readControlGuide(path.substr(5),pixels,w,h,error)&&w==960&&h==540&&pixels.size()==960*540*2);assert(locale(i)["ui"].contains("Control layout"));}
 assert(controlGuidePath(-1)==controlGuidePath(0));assert(controlGuidePath(30)==controlGuidePath(19));
 ControlGuideView v;assert(v.width==960&&v.height==540&&v.scale()==1&&v.x()==0&&v.y()==2);
 std::cout<<"20 language resources and labels; screen-sized fixed image passed\n";}
