/* Copyright (C) 2001-2007 Peter Selinger.
 *  This file is part of Potrace. It is free software and it is covered
 *  by the GNU General Public License. See the file COPYING for details. */

/* This header file collects some general-purpose macros (and static
 *  inline functions) that are used in various places. */

#ifndef AUXILIARY_H
#define AUXILIARY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* ---------------------------------------------------------------------- */
/* point arithmetic */

#include "potracelib.h"

struct point_s
{
    long x;
    long y;
};
typedef struct point_s   point_t;

typedef potrace_dpoint_t dpoint_t;

/* convert point_t to dpoint_t */
static inline dpoint_t dpoint( point_t p )
{
    dpoint_t res;

    res.x = p.x;
    res.y = p.y;
    return res;
}


/* range over the straight line segment [a,b] when lambda ranges over [0,1] */
static inline dpoint_t interval( double lambda, dpoint_t a, dpoint_t b )
{
    dpoint_t res;

    res.x = a.x + lambda * (b.x - a.x);
    res.y = a.y + lambda * (b.y - a.y);
    return res;
}


/* ---------------------------------------------------------------------- */

/* some useful macros. Note: the "mod" macro works correctly for
 *  negative a. Also note that the test for a>=n, while redundant,
 *  speeds up the mod function by 70% in the average case (significant
 *  since the program spends about 16% of its time here - or 40%
 *  without the test). The "floordiv" macro returns the largest integer
 *  <= a/n, and again this works correctly for negative a, as long as
 *  a,n are integers and n>0. */

/* integer arithmetic */

static inline int mod( int a, int n )
{
    return a>=n ? a % n : a>=0 ? a : n - 1 - (-1 - a) % n;
}


static inline int floordiv( int a, int n )
{
    return a>=0 ? a / n : -1 - (-1 - a) / n;
}


/* Note: the following work for integers and other numeric types. */
/* those are bad programming practice because of several issues with
overloading and integer limits
#undef sign
#undef abs
#undef min
#undef max
#undef sq
#undef cu
#define sign( x )   ( (x)>0 ? 1 : (x)<0 ? -1 : 0 )
#define abs( a )    ( (a)>0 ? (a) : -(a) )
#define min( a, b ) ( (a)<(b) ? (a) : (b) )
#define max( a, b ) ( (a)>(b) ? (a) : (b) )
#define sq( a )     ( (a) * (a) )
#define cu( a )     ( (a) * (a) * (a) )*/

// these functions better than macros because they do not cause double evaluation.
static inline int sign( int x )
{
    return (0 < x) - (x < 0);
}

static inline int sign( long x )
{
    return (0 < x) - (x < 0);
}

static inline int sign( double x )
{
    return (0.0 < x) - (x < 0.0);
}

/*static inline int abs( int x ) is not required because exist in stdlib.h */

static inline long abs( long x )
{
    return x < 0? -x : x;
}

static inline int max( int x, int y )
{
    return x < y? y : x;
}

static inline long max( long x, long y )
{
    return x < y? y : x;
}

static inline int min( int x, int y )
{
    return x < y? x : y;
}

static inline long min( long x, long y )
{
    return x < y? x : y;
}

static inline int sq( int x )
{
    return x * x;
}

static inline long sq( long x )
{
    return x * x;
}

static inline double sq( double x )
{
    return x * x;
}

static inline int cu( int x )
{
    return x * x * x;
}

static inline long cu( long x )
{
    return x * x * x;
}

static inline double cu( double x )
{
    return x * x * x;
}

#endif /* AUXILIARY_H */
