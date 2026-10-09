#pragma once
#include <algorithm>
#include <string>
#include <vector>
namespace day {
struct FittedText {std::string value;int size;};
// Width-based fitting keeps UTF-8 intact and never draws an ellipsis outside the box.
template<class Measure> FittedText fitLabel(std::string text,int size,int width,int minimum,Measure measure){
 for(char& c:text)if(c=='\n'||c=='\r'||c=='\t')c=' ';
 if(width<=0)return {"",size};
 minimum=std::clamp(minimum,1,size);
 while(size>minimum&&measure(size,text)>width)--size;
 if(measure(size,text)<=width)return {text,size};
 const std::string dots="…";if(measure(size,dots)>width)return {"",size};
 std::vector<size_t> boundaries{0};for(size_t i=0;i<text.size();){unsigned char c=text[i];i=std::min(text.size(),i+size_t(c<128?1:c<224?2:c<240?3:4));boundaries.push_back(i);}
 size_t lo=0,hi=boundaries.size()-1;while(lo<hi){size_t mid=(lo+hi+1)/2;if(measure(size,text.substr(0,boundaries[mid])+dots)<=width)lo=mid;else hi=mid-1;}
 return {text.substr(0,boundaries[lo])+dots,size};
}
}
