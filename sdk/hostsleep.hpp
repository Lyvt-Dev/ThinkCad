#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOUSER
#define NOUSER
#define THINKCAD_SLEEP_RESTORE_NOUSER
#endif
#include <windows.h>
#ifdef THINKCAD_SLEEP_RESTORE_NOUSER
#undef NOUSER
#undef THINKCAD_SLEEP_RESTORE_NOUSER
#endif
#endif
#include <chrono>
#include <limits>
#include <thread>

namespace thinker {
inline void hostSleep(std::chrono::microseconds duration) {
    if(duration.count()<=0)return;
#ifdef _WIN32
    // Ordinary short sleeps can round up to the Windows scheduler tick. A
    // per-thread high-resolution timer avoids multiplying that rounding across
    // UART polling and microsecond peripheral delays, without busy waiting or
    // changing the machine-wide timer resolution.
    struct Timer {
        HANDLE handle=CreateWaitableTimerExW(nullptr,nullptr,0x00000002,
                                             TIMER_MODIFY_STATE|SYNCHRONIZE);
        ~Timer(){if(handle)CloseHandle(handle);}
    };
    static thread_local Timer timer;
    if(timer.handle&&duration.count()<=std::numeric_limits<LONGLONG>::max()/10){
        LARGE_INTEGER due;due.QuadPart=-duration.count()*10;
        if(SetWaitableTimer(timer.handle,&due,0,nullptr,nullptr,FALSE)&&
           WaitForSingleObject(timer.handle,INFINITE)==WAIT_OBJECT_0)return;
    }
#endif
    // Also supports older Windows systems that lack high-resolution timers.
    std::this_thread::sleep_for(duration);
}
}
