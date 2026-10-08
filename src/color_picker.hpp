#pragma once
#include <algorithm>
#include <cmath>
namespace day {
struct HSV {double h=0,s=0,v=0;};
inline int hsvRgb(HSV c){double h=fmod(c.h+360.,360.)/60.,s=std::clamp(c.s,0.,1.),v=std::clamp(c.v,0.,1.);double a=v*(1-s),b=v*(1-s*(h-floor(h))),d=v*(1-s*(1-(h-floor(h))));double r,g,blue;switch((int)h){case 0:r=v;g=d;blue=a;break;case 1:r=b;g=v;blue=a;break;case 2:r=a;g=v;blue=d;break;case 3:r=a;g=b;blue=v;break;case 4:r=d;g=a;blue=v;break;default:r=v;g=a;blue=b;}return (int)round(r*255)|((int)round(g*255)<<8)|((int)round(blue*255)<<16);}
inline HSV rgbHsv(int rgb){double r=(rgb&255)/255.,g=((rgb>>8)&255)/255.,b=((rgb>>16)&255)/255.,max=std::max({r,g,b}),min=std::min({r,g,b}),d=max-min;HSV c;c.v=max;c.s=max?d/max:0;if(d){c.h=max==r?60*fmod((g-b)/d,6.):max==g?60*((b-r)/d+2):60*((r-g)/d+4);if(c.h<0)c.h+=360;}return c;}
inline int presetRgb(int i){if(i<12){int v=(int)round(i*255./11);return v|(v<<8)|(v<<16);}i-=12;return hsvRgb({(i%12)*30.,i/12==0?.25:i/12==1?.5:1.,i/12<3?1.:i/12==3?.7:.4});}
}
