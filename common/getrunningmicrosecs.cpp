
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
    FILETIME    now;

    GetSystemTimeAsFileTime( &now );

    typedef unsigned long long UINT64;

    UINT64 t = (UINT64(now.dwHighDateTime) << 32) + now.dwLowDateTime;

    t /= 10;

    return unsigned( t );
}


#if 0
// test program
#include <stdio.h>
int main( int argc, char** argv )
{
    unsigned then = GetRunningMicroSecs();

    Sleep( 2000 );      // Windows Sleep( msecs )

    printf( "delta: %u\n", GetRunningMicroSecs() - then );

    return 0;
}
#endif

#endif

