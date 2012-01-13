
#include <config.h>


#if defined(HAVE_CLOCK_GETTIME)

#include <time.h>

unsigned GetRunningMicroSecs()
{
    struct timespec	now;

    clock_gettime( CLOCK_MONOTONIC, &now );

    unsigned usecs = ((unsigned)now.tv_nsec)/1000 + ((unsigned)now.tv_sec) * 1000000;
//    unsigned msecs = (now.tv_nsec / (1000*1000)) + now.tv_sec * 1000;

    return usecs;
}


#elif defined(HAVE_GETTIMEOFDAY_FUNC)

#include <sys/time.h>
unsigned GetRunningMicroSecs()
{
    timeval tv;

    gettimeofday( &tv, 0 );

    return (tv.tv_sec * 1000000) + tv.tv_usec;
}


#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <assert.h>

unsigned GetRunningMicroSecs()
{
    LARGE_INTEGER   curtime;

    static unsigned timerFreq;              // timer frequency

    if( !timerFreq )
    {
        QueryPerformanceFrequency( &curtime );

        timerFreq = curtime.QuadPart / 1000000;     // i.e., ticks per usec

        assert( timerFreq );
    }

    QueryPerformanceCounter( &curtime );

    return ( curtime.LowPart / timerFreq );
}

#endif

