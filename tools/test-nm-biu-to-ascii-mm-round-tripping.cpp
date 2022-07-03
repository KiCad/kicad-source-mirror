
/*
    A test program to which explores the ability to round trip a nanometer
    internal unit in the form of a 32 bit int, out to ASCII floating point
    millimeters and back in without variation.  It tests all 4 billion values
    that an int can hold, and converts to ASCII and back and verifies integrity
    of the round tripped value.

    Author: Dick Hollenbeck
*/


#include <limits.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


static inline int KiROUND( double v )
{
    return int( v < 0 ? v - 0.5 : v + 0.5 );
}


typedef int             BIU;

#define BIU_PER_MM      1e6


//double  scale = BIU_PER_MM;
//double  scale = UM_PER_BIU;
double scale = 1.0/BIU_PER_MM;


std::string biuFmt( BIU aValue )
{
    double  engUnits = aValue * scale;
    char    temp[48];
    int     len;

    if( engUnits != 0.0 && fabsl( engUnits ) <= 0.0001 )
    {
        len = snprintf( temp, sizeof( temp ), "%.10f", engUnits );

        while( --len > 0 && temp[len] == '0' )
            temp[len] = '\0';

        ++len;
    }
    else
    {
        len = snprintf( temp, sizeof( temp ), "%.10g", engUnits );
    }

    return std::string( temp, len );;
}


int parseBIU( const char* s )
{
    double d = strtod( s, NULL );
    return KiROUND( double( d * BIU_PER_MM ) );
//    return int( d * BIU_PER_MM );
}


int main( int argc, char** argv )
{
    unsigned mismatches = 0;

    if( argc > 1 )
    {
        // take a value on the command line and round trip it back to ASCII.

        int i = parseBIU( argv[1] );

        printf( "%s: i:%d\n", __func__, i );

        std::string s = biuFmt( i );

        printf( "%s: s:%s\n", __func__, s.c_str() );

        exit(0);
    }

    // printf( "sizeof(long double): %zd\n", sizeof( long double ) );

    // Empirically prove that we can round trip all 4 billion 32 bit integers representative
    // of nanometers out to textual floating point millimeters, and back without error using
    // the above two functions.
//    for( int i = INT_MIN;  int64_t( i ) <= int64_t( INT_MAX );  ++i )
    for( int64_t j = INT_MIN;  j  <= int64_t( INT_MAX );  ++j )
    {
        int i = int( j );

        std::string s = biuFmt( int( i ) );

        int r = parseBIU( s.c_str() );

        if( r != i )
        {
            printf( "i:%d  biuFmt:%s  r:%d\n", i, s.c_str(), r );
            ++mismatches;
        }

        if( !( i & 0xFFFFFF ) )
        {
            printf( " %08x", i );
            fflush( stdout );
        }
    }

    printf( "mismatches:%u\n", mismatches );

    return 0;
}

