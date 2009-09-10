/*! \file kbool/include/kbool/statusb.h
    \author Probably Klaas Holwerda
 
    Copyright: 2001-2004 (C) Probably Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: statusb.h,v 1.3 2009/02/06 21:33:03 titato Exp $
*/

/* @@(#) $Source: /cvsroot/wxart2d/wxArt2D/thirdparty/kbool/include/kbool/statusb.h,v $ $Revision: 1.3 $ $Date: 2009/02/06 21:33:03 $ */

/*
Program STATUSB.H
Purpose Controls the statusbar of the application (header)
  This statusbar is a typical Windows statusbar
  For porting to another platform there must be a StatusBar class
  derived from this.
  User interface element (See documentation for more details
  about the functions needed in this class)
*/

#ifndef STATUSB_H
#define STATUSB_H
#include <time.h>

// abstract base class for own statusbar inherite from it
class A2DKBOOLDLLEXP StatusBar
{
public:
    // constructor & destructor
    StatusBar(){};
    ~StatusBar(){};

    virtual void SetXY( double = 0.0, double = 0.0 ) = 0;
    virtual void ResetCoord() = 0;
    virtual void  SetFile( char* = 0 ) = 0;
    virtual void SetProcess( char* = 0 ) = 0;
    virtual void  SetTime( time_t seconds = 0 ) = 0;
    virtual void SetRecording( int status = 0 ) = 0;
    virtual void SetZoom( float factor = 1 ) = 0;
    virtual void Reset() = 0;
    void     StartDTimer();
    void     EndDTimer();
    int    GetDTimerOn();
    time_t   GetDTimer();

protected:
    int    timer;
    time_t   oldtime;
    time_t   curtime;
};

#endif
