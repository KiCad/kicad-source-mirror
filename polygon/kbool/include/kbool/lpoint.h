/*! \file include/lpoint.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: lpoint.h,v 1.3 2008/06/04 21:23:22 titato Exp $
*/

/* @@(#) $Source: /cvsroot/wxart2d/wxArt2D/thirdparty/kbool/include/kbool/lpoint.h,v $ $Revision: 1.3 $ $Date: 2008/06/04 21:23:22 $ */

/*
Program LPOINT.H
Purpose Definition of GDSII pointtype structure
Last Update 12-12-1995
*/

#ifndef LPOINT_H
#define LPOINT_H

#include "kbool/booleng.h"

class A2DKBOOLDLLEXP LPoint
{
public:
    LPoint();
    LPoint( B_INT const , B_INT const );
    LPoint( LPoint* const );

    void  Set( const B_INT, const B_INT );
    void  Set( const LPoint & );

    LPoint  GetPoint();
    B_INT  GetX();
    B_INT  GetY();
    void   SetX( B_INT );
    void  SetY( B_INT );
    bool  Equal( const LPoint a_point, B_INT Marge );
    bool  Equal( const B_INT, const B_INT , B_INT Marge );
    bool  ShorterThan( const LPoint a_point, B_INT marge );
    bool  ShorterThan( const B_INT X, const B_INT Y, B_INT );

    LPoint &operator=( const LPoint & );
    LPoint &operator+( const LPoint & );
    LPoint &operator-( const LPoint & );

    LPoint &operator*( int );
    LPoint &operator/( int );

    int  operator==( const LPoint & ) const;
    int  operator!=( const LPoint & ) const;

protected:
    B_INT _x;
    B_INT _y;

};

#endif
