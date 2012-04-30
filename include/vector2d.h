/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 Virtenio GmbH, Torsten Hueter, torsten.hueter <at> virtenio.de
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef VECTOR2D_H_
#define VECTOR2D_H_

#include <cmath>
#include <wx/gdicmn.h>


/// Forward declaration for template friends
template<class T> class VECTOR2;

/*
#include <iostream>
template<class T> ostream& operator<<( ostream &stream, const VECTOR2<T>& vector );
*/

/**
 * Class VECTOR2
 * defines a general 2D-vector.
 *
 * This class uses templates to be universal. Several operators are provided to help easy implementing
 * of linear algebra equations.
 *
 */
template<class T> class VECTOR2
{
public:
    T x, y;

    // Constructors

    /// Construct a 2D-vector with x, y = 0
    VECTOR2();

    /// Copy constructor
    VECTOR2( const VECTOR2<T>& aVector );

    /// Constructor with a wxPoint as argument
    VECTOR2( const wxPoint& aPoint );

    /// Constructor with a wxSize as argument
    VECTOR2( const wxSize& aSize );

    /// Construct a vector with given components x, y
    VECTOR2( T x, T y );

    /// Destructor
    // virtual ~VECTOR2();

    /**
     * Function Euclidean Norm
     * computes the Euclidean norm of the vector, which is defined as sqrt(x ** 2 + y ** 2).
     * It is used to calculate the length of the vector.
     * @return Scalar, the euclidean norm
     */
    T EuclideanNorm();

    /**
     * Function Perpendicular
     * computes the perpendicular vector
     * @return Perpendicular vector
     */
    VECTOR2<T> Perpendicular();

    /**
     * Function Angle
     * computes the angle of the vector
     * @return vector angle
     */
    T Angle();

    // Operators

    /// Assignment operator
    VECTOR2<T>& operator=( const VECTOR2<T>& aVector );

    /// Vector addition operator
    VECTOR2<T> operator+( const VECTOR2<T>& aVector );

    /// Compound assignment operator
    VECTOR2<T>& operator+=( const VECTOR2<T>& aVector );

    /// Vector subtraction operator
    VECTOR2<T> operator-( const VECTOR2<T>& aVector );

    /// Compound assignment operator
    VECTOR2<T>& operator-=( const VECTOR2<T>& aVector );

    /// Negate Vector operator
    VECTOR2<T> operator-();

    /// Scalar product operator
    T operator*( const VECTOR2<T>& aVector );

    /// Multiplication with a factor
    VECTOR2<T> operator*( const T& aFactor );

    /// Cross product operator
    T operator^( const VECTOR2<T>& aVector );

    /// Equality operator
    const bool operator==( const VECTOR2<T>& aVector );

    /// Not equality operator
    const bool operator!=( const VECTOR2<T>& aVector );

    /// Smaller than operator
    bool operator<( const VECTOR2<T>& aVector );
    bool operator<=( const VECTOR2<T>& aVector );

    /// Greater than operator
    bool operator>( const VECTOR2<T>& aVector );
    bool operator>=( const VECTOR2<T>& aVector );

    /// Casting to int vector
    // operator VECTOR2<int>();

    /// Type casting operator for the class wxPoint
    //operator wxPoint();

    // friend ostream& operator<< <T> ( ostream &stream, const VECTOR2<T>& vector );
};


// ----------------------
// --- Implementation ---
// ----------------------

template<class T> VECTOR2<T>::VECTOR2( VECTOR2<T> const& aVector ) :
    x( aVector.x ), y( aVector.y )
{
}

template<class T> VECTOR2<T>::VECTOR2()
{
    x = y = 0.0;
}

template<class T> VECTOR2<T>::VECTOR2( wxPoint const& aPoint )
{
    x = T( aPoint.x );
    y = T( aPoint.y );
}

template<class T> VECTOR2<T>::VECTOR2( wxSize const& aSize )
{
    x = T( aSize.x );
    y = T( aSize.y );
}

template<class T> VECTOR2<T>::VECTOR2( T aX, T aY )
{
    x = aX;
    y = aY;
}

// Not required at the moment for this class
//template<class T> VECTOR2<T>::~VECTOR2()
//{
//    // TODO Auto-generated destructor stub
//}

template<class T> T VECTOR2<T>::EuclideanNorm()
{
    return sqrt( ( *this ) * ( *this ) );
}

template<class T> T VECTOR2<T>::Angle()
{
    return atan2(y, x);
}

template<class T> VECTOR2<T> VECTOR2<T>::Perpendicular(){
    VECTOR2<T> perpendicular(-y, x);
    return perpendicular;
}

/*
template<class T> ostream &operator<<( ostream &aStream, const VECTOR2<T>& aVector )
{
    aStream << "[ " << aVector.x << " | " << aVector.y << " ]";
    return aStream;
}
*/

template<class T> VECTOR2<T> &VECTOR2<T>::operator=( const VECTOR2<T>& aVector )
{
    x = aVector.x;
    y = aVector.y;
    return *this;
}

template<class T> VECTOR2<T> &VECTOR2<T>::operator+=( const VECTOR2<T>& aVector )
{
    x += aVector.x;
    y += aVector.y;
    return *this;
}

template<class T> VECTOR2<T>& VECTOR2<T>::operator-=( const VECTOR2<T>& aVector )
{
    x -= aVector.x;
    y -= aVector.y;
    return *this;
}

//template<class T> VECTOR2<T>::operator wxPoint()
//{
//    wxPoint point;
//    point.x = (int) x;
//    point.y = (int) y;
//    return point;
//}
//

//// Use correct rounding for casting to wxPoint
//template<> VECTOR2<double>::operator wxPoint()
//{
//    wxPoint point;
//    point.x = point.x >= 0 ? (int) ( x + 0.5 ) : (int) ( x - 0.5 );
//    point.y = point.y >= 0 ? (int) ( y + 0.5 ) : (int) ( y - 0.5 );
//    return point;
//}

// Use correct rounding for casting double->int
//template<> VECTOR2<double>::operator VECTOR2<int>()
//{
//    VECTOR2<int> vector;
//    vector.x = vector.x >= 0 ? (int) ( x + 0.5 ) : (int) ( x - 0.5 );
//    vector.y = vector.y >= 0 ? (int) ( y + 0.5 ) : (int) ( y - 0.5 );
//    return vector;
//}

template<class T> VECTOR2<T> VECTOR2<T>::operator+( const VECTOR2<T>& aVector )
{
    return VECTOR2<T> ( x + aVector.x, y + aVector.y );
}

template<class T> VECTOR2<T> VECTOR2<T>::operator-( const VECTOR2<T>& aVector )
{
    return VECTOR2<T> ( x - aVector.x, y - aVector.y );
}

template<class T> VECTOR2<T> VECTOR2<T>::operator-()
{
    return VECTOR2<T> ( -x, -y );
}

template<class T> T VECTOR2<T>::operator*( const VECTOR2<T>& aVector )
{
    return aVector.x * x + aVector.y * y;
}

template<class T> VECTOR2<T> VECTOR2<T>::operator*( const T& aFactor )
{
    VECTOR2<T> vector( x * aFactor, y * aFactor );
    return vector;
}

template<class T> VECTOR2<T> operator*( const T& aFactor, const VECTOR2<T>& aVector){
    VECTOR2<T> vector( aVector.x * aFactor, aVector.y * aFactor );
    return vector;
}

template<class T> T VECTOR2<T>::operator^( const VECTOR2<T>& aVector )
{
    return x * aVector.y - y * aVector.x;
}

template<class T> bool VECTOR2<T>::operator<( const VECTOR2<T>& o )
{
    // VECTOR2<T> vector( aVector );
    return (double( x ) * x + double( y ) * y) < (double( o.x ) * o.x + double( o.y ) * y);
}

template<class T> bool VECTOR2<T>::operator<=( const VECTOR2<T>& aVector )
{
    VECTOR2<T> vector( aVector );
    return ( *this * *this ) <= ( vector * vector );
}

template<class T> bool VECTOR2<T>::operator>( const VECTOR2<T>& aVector )
{
    VECTOR2<T> vector( aVector );
    return ( *this * *this ) > ( vector * vector );
}

template<class T> bool VECTOR2<T>::operator>=( const VECTOR2<T>& aVector )
{
    VECTOR2<T> vector( aVector );
    return ( *this * *this ) >= ( vector * vector );
}

template<class T> bool const VECTOR2<T>::operator==( VECTOR2<T> const& aVector )
{
    return ( aVector.x == x ) && ( aVector.y == y );
}

template<class T> bool const VECTOR2<T>::operator!=( VECTOR2<T> const& aVector )
{
    return ( aVector.x != x ) || ( aVector.y != y );
}


/**
 * Class BOX2
 * is a description of a rectangle in a cartesion coordinate system.
 */
template<class T> class BOX2
{
public:
    BOX2() : x(0), y(0), width(0), height(0) {}

    BOX2( T aX, T aY, T aWidth, T aHeight ):
        x( aX ), y( aY ), width( aWidth ), height( aHeight )
    {}


    /// Copy constructor
    BOX2( const BOX2<T>& aRect ) :
        x( aRect.x ), y( aRect.y ), width( aRect.width ), height( aRect.height )
    {}

    /// Constructor with a wxPoint as argument?

    VECTOR2<T>  GetSize() const         { return VECTOR2<T> ( width, height ); }
    VECTOR2<T>  GetPosition() const     { return VECTOR2<T> ( x, y ); }

    T GetLeft() const { return x; }
    void SetLeft( T n ) { width += x - n; x = n; }
    void MoveLeftTo( T n ) { x = n; }

    T GetTop() const { return y; }
    void SetTop( T n ) { height += y - n; y = n; }
    void MoveTopTo( T n ) { y = n; }

    T GetBottom() const { return y + height; }
    void SetBottom( T n ) { height += n - ( y + height ); }
    void MoveBottomTo( T n ) { y = n - height; }

    T GetRight() const { return x + width; }
    void SetRight( T n ) { width += n - ( x + width ); }
    void MoveRightTo( T n ) { x = n - width; }

    VECTOR2<T> GetLeftTop() const { return VECTOR2<T>( x , y ); }
    void SetLeftTop( const VECTOR2<T>& pt ) { width += x - pt.x; height += y - pt.y; x = pt.x; y = pt.y; }
    void MoveLeftTopTo( const VECTOR2<T> &pt ) { x = pt.x; y = pt.y; }

    VECTOR2<T> GetLeftBottom() const { return VECTOR2<T>( x, y + height ); }
    void SetLeftBottom( const VECTOR2<T>& pt ) { width += x - pt.x; height += pt.y - (y + height); x = pt.x; }
    void MoveLeftBottomTo( const VECTOR2<T>& pt ) { x = pt.x; y = pt.y - height; }

    VECTOR2<T> GetRightTop() const { return VECTOR2<T>( x + width, y ); }
    void SetRightTop( const VECTOR2<T>& pt ) { width += pt.x - ( x + width ); height += y - pt.y; y = pt.y; }
    void MoveRightTopTo( const VECTOR2<T>& pt ) { x = pt.x - width; y = pt.y; }

    VECTOR2<T> GetRightBottom() const { return VECTOR2<T>( x + width, y + height ); }
    void SetRightBottom( const VECTOR2<T>& pt ) { width += pt.x - ( x + width ); height += pt.y - ( y + height); }
    void MoveRightBottomTo( const VECTOR2<T>& pt ) { x = pt.x - width; y = pt.y - height; }

    VECTOR2<T> GetCentre() const { return VECTOR2<T>( x + width/2, y + height/2 ); }
    void SetCentre( const VECTOR2<T>& pt ) { MoveCentreTo( pt ); }
    void MoveCentreTo( const VECTOR2<T>& pt ) { x += pt.x - (x + width/2), y += pt.y - (y + height/2); }

    T   x, y, width, height;
};


typedef VECTOR2<double>     DPOINT;
typedef DPOINT              DSIZE;

typedef BOX2<double>        DBOX;


#endif // VECTOR2D_H_
