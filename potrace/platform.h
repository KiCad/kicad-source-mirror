/* Copyright (C) 2001-2015 Peter Selinger.
 *  This file is part of Potrace. It is free software and it is covered
 *  by the GNU General Public License. See the file COPYING for details. */

/* this header file contains some platform dependent stuff */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* in Windows, set all file i/o to binary */
#ifdef __MINGW32__
#include <fcntl.h>
unsigned int _CRT_fmode = _O_BINARY;
static inline void platform_init( void )
{
    _setmode( _fileno( stdin ), _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
}


#else

#ifdef __CYGWIN__
#include <fcntl.h>
#include <io.h>
static inline void platform_init( void )
{
    setmode( 0, O_BINARY );
    setmode( 1, O_BINARY );
}


#else
static inline void platform_init( void )
{
    /* NOP */
}


#endif
#endif

#endif /* PLATFORM_H */
