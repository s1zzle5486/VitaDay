#pragma once
#include <psp2/appmgr.h>
namespace day {
// Bounded drain: unrelated/resume events must not close the application.
inline bool systemRequestedQuit(){
    SceAppMgrSystemEvent event{};
    bool quit=false;
    for(int n=0;n<32&&sceAppMgrReceiveSystemEvent(&event)>=0;++n)
        quit|=event.systemEvent==SCE_APPMGR_SYSTEMEVENT_REQUEST_QUIT;
    return quit;
}
}
