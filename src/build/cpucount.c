#include <stdio.h>
#include <stdlib.h>

#ifdef __CYGWIN__
#define WIN32
#endif //__CYGWIN__

#ifdef WIN32
#include <windows.h>
#endif //WIN32

#ifdef __linux__
#define __USE_GNU
#include <sched.h>
#undef __USE_GNU
#endif //__linux__

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#ifndef HW_AVAILCPU
#define HW_AVAILCPU 25
#endif //HW_AVAILCPU
#endif //BSD

int getcpucount()
{
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    return sysinfo.dwNumberOfProcessors;
#else
#ifdef __linux__
    cpu_set_t cs;
    int i, cpucount = 0;

    CPU_ZERO(&cs);
    sched_getaffinity(0, sizeof(cs), &cs);

    for(i = 0; i < 128; i++)
    {
        if(CPU_ISSET(i, &cs))
            cpucount++;
    }

    return cpucount ? cpucount : 1;
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
    int cpucount = 0;
    int mib[4];
    size_t len = sizeof(cpucount);

    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;

    sysctl(mib, 2, &cpucount, &len, NULL, 0);

    if(cpucount < 1)
    {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &cpucount, &len, NULL, 0);
    }

    return cpucount ? cpucount : 1;
#else
#warning unknown platform
    return 1;
#endif //BSD
#endif //__linux__
#endif //WIN32
}

int main()
{
    printf("%d\n", getcpucount());
    return 0;
}
