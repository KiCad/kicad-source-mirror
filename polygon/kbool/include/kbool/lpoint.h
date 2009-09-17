/*! \file include/lpoint.h
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: lpoint.h,v 1.4 2009/09/10 17:04:09 titato Exp $
*/

/* @@(#) $Source: /cvsroot/wxart2d/wxArt2D/thirdparty/kbool/include/kbool/lpoint.h,v $ $Revision: 1.4 $ $Date: 2009/09/10 17:04:09 $ */

/*
Program LPOINT.H
Purpose Definition of GDSII pointtype structure
Last Update 12-12-1995
*/

#ifndef LPOINT_H
#define LPOINT_H

#include "kbool/booleng.h"

class A2DKBOOLDLLEXP kbLPoint
{
public:
    kbLPoint();
    kbLPoint( B_INT const , B_INT const );
    kbLPoint( kbLPoint* const );

    void  Set( const B_INT, const B_INT );
    void  Set( const kbLPoint & );

    kbLPoint  GetPoint();
    B_INT  GetX();
    B_INT  GetY();
    void   SetX( B_INT );
    void  SetY( B_INT );
    bool  Equal( const kbLPoint a_point, B_INT Marge );
    bool  Equal( const B_INT, const B_INT , B_INT Marge );
    bool  ShorterThan( const kbLPoint a_point, B_INT marge );
    bool  ShorterThan( const B_INT X, const B_INT Y, B_INT );

    kbLPoint &operator=( const kbLPoint & );
    kbLPoint &operator+( const kbLPoint & );
    kbLPoint &operator-( const kbLPoint & );

    kbLPoint &operator*( int );
    kbLPoint &operator/( int );

    int  operator==( const kbLPoint & ) const;
    int  operator!=( const kbLPoint & ) const;

protected:
    B_INT _x;
    B_INT _y;

};

#endif
