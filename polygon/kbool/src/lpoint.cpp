/*! \file src/lpoint.cpp
    \brief Definition of GDSII LPoint type structure
    \author Klaas Holwerda
 
    Copyright: 2001-2004 (C) Klaas Holwerda 
 
    Licence: see kboollicense.txt 
 
    RCS-ID: $Id: lpoint.cpp,v 1.3 2008/06/04 21:23:22 titato Exp $
*/

#include "kbool/lpoint.h"
#include <math.h>

// Constructors
LPoint::LPoint()
{
    _x = 0;
    _y = 0;
}


LPoint::LPoint( B_INT const X, B_INT const Y )
{
    _x = X;
    _y = Y;
}


LPoint::LPoint( LPoint* const a_point )
{
    if ( !a_point )
        throw Bool_Engine_Error( "Cannot copy a NULL Point Object.\n\nCould not create a LPoint Object.",
                                 "Fatal Creation Error", 0, 1 );
    _x = a_point->_x;
    _y = a_point->_y;
}


B_INT LPoint::GetX()
{
    return _x;
}

B_INT LPoint::GetY()
{
    return _y;
}


void LPoint::SetX( B_INT a_point_x )
{
    _x = a_point_x;
}


void LPoint::SetY( B_INT a_point_y )
{
    _y = a_point_y;
}


LPoint LPoint::GetPoint()
{
    return * this;
}


void LPoint::Set( const B_INT X, const B_INT Y )
{
    _x = X;
    _y = Y;
}


void LPoint::Set( const LPoint &a_point )
{
    _x = a_point._x;
    _y  = a_point._y;
}

bool LPoint::Equal( const LPoint a_point, B_INT Marge )
{
    B_INT delta_x, delta_y;

    delta_x = babs( ( _x - a_point._x ) );
    delta_y = babs( ( _y - a_point._y ) );

    if ( ( delta_x <= Marge ) && ( delta_y <= Marge ) )
        return true;
    else
        return false;
}


bool LPoint::Equal( const B_INT X, const B_INT Y, B_INT Marge )
{
    return ( bool )( ( babs( _x - X ) <= Marge ) && ( babs( _y - Y ) <= Marge ) );
}

bool LPoint::ShorterThan( const LPoint a_point, B_INT Marge )
{
    double a, b;
    a = ( double ) ( a_point._x - _x );
    a *= a;
    b = ( double ) ( a_point._y - _y );
    b *= b;

    return ( bool ) ( ( a + b ) <= Marge * Marge ? true : false ) ;
}


bool LPoint::ShorterThan( const B_INT X, const B_INT Y, B_INT Marge )
{
    double a, b;
    a = ( double ) ( X - _x );
    a *= a;
    b = ( double ) ( Y - _y );
    b *= b;

    return ( bool ) ( a + b <= Marge * Marge ? true : false ) ;
}


// overload the assign (=) operator
// usage : a_point = another_point;

LPoint &LPoint::operator=( const LPoint &other_point )
{
    _x = other_point._x;
    _y = other_point._y;
    return *this;
}


// overload the + operator
// usage : a_point = point1 + point2;

LPoint &LPoint::operator+( const LPoint &other_point )
{
    _x += other_point._x;
    _y += other_point._y;
    return *this;
}



// overload the - operator
// usage : a_point = point1 - point2;

LPoint &LPoint::operator-( const LPoint &other_point )
{
    _x -= other_point._x;
    _y -= other_point._y;
    return *this;
}


// overload the * operator
// usage: a_point = point1 * 100;

LPoint &LPoint::operator*( int factor )
{
    _x *= factor;
    _y *= factor;
    return *this;
}


// overload the / operator
// usage: a_point = point1 / 100;

LPoint &LPoint::operator/( int factor )
{
    _x /= factor;
    _y /= factor;
    return *this;
}


// overload the compare (==) operator
// usage: if (point1 == point2) { };

int LPoint::operator==( const LPoint &other_point ) const
{
    return ( ( other_point._x == _x ) && ( other_point._y == _y ) );
}


// overload the diffrent (!=) operator
// usage: if (point1 != point2) { };

int LPoint::operator!=( const LPoint &other_point ) const
{
    return ( ( other_point._x != _x ) || ( other_point._y != _y ) );
}



