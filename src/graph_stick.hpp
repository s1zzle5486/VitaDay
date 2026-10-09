#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace day {
struct GraphStickMotion {
    int direction=0;
    uint64_t began=0,last=0;
    void reset(){direction=0;began=last=0;}
    static double speed(uint64_t held){
        double u=std::clamp((double(held)/1000000.-.5)/2.5,0.,1.);
        return 8.+325.*u*u*(3.-2.*u); // minutes/second, not jumps
    }
    double poll(int rx,int ry,uint64_t now){
        int x=rx-128,y=ry-128,axis=std::abs(x)>=std::abs(y)?x:y;
        int magnitude=std::abs(axis);
        if(magnitude<=(direction?18:22)){reset();return 0.;}
        int wanted=axis<0?-1:1;
        if(wanted!=direction||now<last){direction=wanted;began=last=now;return 0.;}
        auto elapsed=now-last;last=now;
        // Don't accumulate motion while the app stalls or resumes.
        if(elapsed>250000){began=now;return 0.;}
        double dt=std::min(double(elapsed)/1000000.,.05);
        double throwAmount=std::clamp((magnitude-18.)/110.,0.,1.);
        uint64_t midpoint=now-began-elapsed/2;
        return direction*speed(midpoint)*std::pow(throwAmount,1.5)*dt;
    }
};
}
