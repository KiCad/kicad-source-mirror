/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 Virtenio GmbH, Torsten Hueter, torsten.hueter <at> virtenio.de
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <limits>
#include <iostream>
#include <sstream>
#include <type_traits>

#include <math/util.h>

#ifdef WX_COMPATIBILITY
#include <wx/gdicmn.h>
#endif

/**
 * Traits class for VECTOR2.
 */
template <class T>
struct VECTOR2_TRAITS
{
    ///< extended range/precision types used by operations involving multiple
    ///< multiplications to prevent overflow.
    typedef T extended_type;
};

template <>
struct VECTOR2_TRAITS<int>
{
    typedef int64_t extended_type;
};

// Forward declarations for template friends
template <class T>
class VECTOR2;
template <class T>
std::ostream& operator<<( std::ostream& aStream, const VECTOR2<T>& aVector );

/**
 * Define a general 2D-vector/point.
 *
 * This class uses templates to be universal. Several operators are provided to help
 * easy implementing of linear algebra equations.
 *
 */
template <class T = int>
class VECTOR2
{
public:
    typedef typename VECTOR2_TRAITS<T>::extended_type extended_type;
    typedef T coord_type;

    static constexpr extended_type ECOORD_MAX = std::numeric_limits<extended_type>::max();
    static constexpr extended_type ECOORD_MIN = std::numeric_limits<extended_type>::min();

    T x, y;

    /// Construct a 2D-vector with x, y = 0
    VECTOR2();

#ifdef WX_COMPATIBILITY
    /// Constructor with a wxPoint as argument
    VECTOR2( const wxPoint& aPoint );

    /// Constructor with a wxSize as argument
    VECTOR2( const wxSize& aSize );
#endif

    /// Construct a vector with given components x, y
    VECTOR2( T x, T y );

    /// Initializes a vector from another specialization. Beware of rounding issues.
    template <typename CastingType>
    VECTOR2( const VECTOR2<CastingType>& aVec )
    {
        x = (T) aVec.x;
        y = (T) aVec.y;
    }

    /// Copy a vector
    VECTOR2( const VECTOR2<T>& aVec )
    {
        x = aVec.x;
        y = aVec.y;
    }

    /// Cast a vector to another specialized subclass. Beware of rounding issues.
    template <typename CastedType>
    VECTOR2<CastedType> operator()() const
    {
        return VECTOR2<CastedType>( (CastedType) x, (CastedType) y );
    }

    /**
     * Implement the cast to wxPoint.
     *
     * @return the vector cast to wxPoint.
     */
    explicit operator wxPoint() const
    {
        return wxPoint( x, y );
    }

    // virtual ~VECTOR2();

    /**
     * Compute the Euclidean norm of the vector, which is defined as sqrt(x ** 2 + y ** 2).
     *
     * It is used to calculate the length of the vector.
     *
     * @return Scalar, the euclidean norm
     */
    T EuclideanNorm() const;

    /**
     * Compute the squared euclidean norm of the vector, which is defined as (x ** 2 + y ** 2).
     *
     * It is used to calculate the length of the vector.
     *
     * @return Scalar, the euclidean norm
     */
    extended_type SquaredEuclideanNorm() const;


    /**
     * Compute the perpendicular vector.
     *
     * @return Perpendicular vector
     */
    VECTOR2<T> Perpendicular() const;

    /**
     * Return a vector of the same direction, but length specified in \a aNewLength.
     *
     * @param aNewLength is the length of the rescaled vector.
     * @return the rescaled vector.
     */
    VECTOR2<T> Resize( T aNewLength ) const;

    /**
     * Compute the angle of the vector.
     *
     * @return the vector angle in radians.
     */
    double Angle() const;

    /**
     * Rotate the vector by a given angle.
     *
     * @param aAngle rotation angle in radians
     * @return rotated vector
     */
    VECTOR2<T> Rotate( double aAngle ) const;

    /**
     * Return the vector formatted as a string.
     *
     * @return the formatted string
     */
    const std::string Format() const;

    /**
     * Compute cross product of self with \a aVector.
     */
    extended_type Cross( const VECTOR2<T>& aVector ) const;

    /**
     * Compute dot product of self with \a aVector.
     */
    extended_type Dot( const VECTOR2<T>& aVector ) const;


    // Operators

    /// Assignment operator
    VECTOR2<T>& operator=( const VECTOR2<T>& aVector );

    /// Vector addition operator
    VECTOR2<T> operator+( const VECTOR2<T>& aVector ) const;

    /// Scalar addition operator
    VECTOR2<T> operator+( const T& aScalar ) const;

    /// Compound assignment operator
    VECTOR2<T>& operator+=( const VECTOR2<T>& aVector );

    /// Compound assignment operator
    VECTOR2<T>& operator+=( const T& aScalar );

    /// Vector subtraction operator
    VECTOR2<T> operator-( const VECTOR2<T>& aVector ) const;

    /// Scalar subtraction operator
    VECTOR2<T> operator-( const T& aScalar ) const;

    /// Compound assignment operator
    VECTOR2<T>& operator-=( const VECTOR2<T>& aVector );

    /// Compound assignment operator
    VECTOR2<T>& operator-=( const T& aScalar );

    /// Negate Vector operator
    VECTOR2<T> operator-();

    /// Scalar product operator
    extended_type operator*( const VECTOR2<T>& aVector ) const;

    /// Multiplication with a factor
    VECTOR2<T> operator*( const T& aFactor ) const;

    /// Division with a factor
    VECTOR2<T> operator/( const T& aFactor ) const;

    /// Equality operator
    bool operator==( const VECTOR2<T>& aVector ) const;

    /// Not equality operator
    bool operator!=( const VECTOR2<T>& aVector ) const;

    /// Smaller than operator
    bool operator<( const VECTOR2<T>& aVector ) const;
    bool operator<=( const VECTOR2<T>& aVector ) const;

    /// Greater than operator
    bool operator>( const VECTOR2<T>& aVector ) const;
    bool operator>=( const VECTOR2<T>& aVector ) const;
};


// ----------------------
// --- Implementation ---
// ----------------------

template <class T>
VECTOR2<T>::VECTOR2()
{
    x = y = 0.0;
}


#ifdef WX_COMPATIBILITY
template <class T>
VECTOR2<T>::VECTOR2( const wxPoint& aPoint )
{
    x = T( aPoint.x );
    y = T( aPoint.y );
}


template <class T>
VECTOR2<T>::VECTOR2( const wxSize& aSize )
{
    x = T( aSize.x );
    y = T( aSize.y );
}
#endif

template <class T>
VECTOR2<T>::VECTOR2( T aX, T aY )
{
    x = aX;
    y = aY;
}


template <class T>
T VECTOR2<T>::EuclideanNorm() const
{
    return sqrt( (extended_type) x * x + (extended_type) y * y );
}


template <class T>
typename VECTOR2<T>::extended_type VECTOR2<T>::SquaredEuclideanNorm() const
{
    return (extended_type) x * x + (extended_type) y * y;
}


template <class T>
double VECTOR2<T>::Angle() const
{
    return atan2( (double) y, (double) x );
}


template <class T>
VECTOR2<T> VECTOR2<T>::Perpendicular() const
{
    VECTOR2<T> perpendicular( -y, x );
    return perpendicular;
}


template <class T>
VECTOR2<T>& VECTOR2<T>::operator=( const VECTOR2<T>& aVector )
{
    x = aVector.x;
    y = aVector.y;
    return *this;
}


template <class T>
VECTOR2<T>& VECTOR2<T>::operator+=( const VECTOR2<T>& aVector )
{
    x += aVector.x;
    y += aVector.y;
    return *this;
}


template <class T>
VECTOR2<T>& VECTOR2<T>::operator+=( const T& aScalar )
{
    x += aScalar;
    y += aScalar;
    return *this;
}


template <class T>
VECTOR2<T>& VECTOR2<T>::operator-=( const VECTOR2<T>& aVector )
{
    x -= aVector.x;
    y -= aVector.y;
    return *this;
}


template <class T>
VECTOR2<T>& VECTOR2<T>::operator-=( const T& aScalar )
{
    x -= aScalar;
    y -= aScalar;
    return *this;
}


/**
 * Rotate a VECTOR2 by aAngle.
 * @param aAngle = rotation angle in radians
 */
template <class T>
VECTOR2<T> VECTOR2<T>::Rotate( double aAngle ) const
{
    // Avoid common radian rotations that may allow for angular error
    if( aAngle == 0.0 || aAngle == 2 * M_PI )
        return VECTOR2<T> ( T( x ), T( y ) );

    if( aAngle == M_PI_2 )
        return VECTOR2<T>( -T( y ), T( x ) );

    if( aAngle == M_PI )
        return VECTOR2<T>( -T(x), -T( y ) );

    if( aAngle == 3 * M_PI_2 )
        return VECTOR2<T>( T( y ), -T( x ) );

    double  sa  = sin( aAngle );
    double  ca  = cos( aAngle );

    if( std::is_integral<T>::value )
    {
        return VECTOR2<T> ( KiROUND( (double) x * ca - (double) y * sa ),
                            KiROUND( (double) x * sa + (double) y * ca ) );

    }
    else
    {
        return VECTOR2<T> ( T( (double) x * ca - (double) y * sa ),
                            T( (double) x * sa + (double) y * ca ) );
    }
}


template <class T>
VECTOR2<T> VECTOR2<T>::Resize( T aNewLength ) const
{
    if( x == 0 && y == 0 )
        return VECTOR2<T> ( 0, 0 );

    extended_type l_sq_current = (extended_type) x * x + (extended_type) y * y;
    extended_type l_sq_new = (extended_type) aNewLength * aNewLength;

    if( std::is_integral<T>::value )
    {
        return VECTOR2<T> (
            ( x < 0 ? -1 : 1 ) *
                KiROUND( std::sqrt( rescale( l_sq_new, (extended_type) x * x, l_sq_current ) ) ),
            ( y < 0 ? -1 : 1 ) *
                KiROUND( std::sqrt( rescale( l_sq_new, (extended_type) y * y, l_sq_current ) ) ) )
                    * sign( aNewLength );
    }
    else
    {
        return VECTOR2<T> (
            ( x < 0 ? -1 : 1 ) *
                std::sqrt( rescale( l_sq_new, (extended_type) x * x, l_sq_current ) ),
            ( y < 0 ? -1 : 1 ) *
                std::sqrt( rescale( l_sq_new, (extended_type) y * y, l_sq_current ) ) )
                    * sign( aNewLength );
    }
}


template <class T>
const std::string VECTOR2<T>::Format() const
{
    std::stringstream ss;

    ss << "( xy " << x << " " << y << " )";

    return ss.str();
}


template <class T>
VECTOR2<T> VECTOR2<T>::operator+( const VECTOR2<T>& aVector ) const
{
    return VECTOR2<T> ( x + aVector.x, y + aVector.y );
}


template <class T>
VECTOR2<T> VECTOR2<T>::operator+( const T& aScalar ) const
{
    return VECTOR2<T> ( x + aScalar, y + aScalar );
}


template <class T>
VECTOR2<T> VECTOR2<T>::operator-( const VECTOR2<T>& aVector ) const
{
    return VECTOR2<T> ( x - aVector.x, y - aVector.y );
}


template <class T>
VECTOR2<T> VECTOR2<T>::operator-( const T& aScalar ) const
{
    return VECTOR2<T> ( x - aScalar, y - aScalar );
}


template <class T>
VECTOR2<T> VECTOR2<T>::operator-()
{
    return VECTOR2<T> ( -x, -y );
}


template <class T>
typename VECTOR2<T>::extended_type VECTOR2<T>::operator*( const VECTOR2<T>& aVector ) const
{
    return (extended_type)aVector.x * x + (extended_type)aVector.y * y;
}


template <class T>
VECTOR2<T> VECTOR2<T>::operator*( const T& aFactor ) const
{
    VECTOR2<T> vector( x * aFactor, y * aFactor );
    return vector;
}


template <class T>
VECTOR2<T> VECTOR2<T>::operator/( const T& aFactor ) const
{
    if( std::is_integral<T>::value )
        return VECTOR2<T>( KiROUND( x / aFactor ), KiROUND( y / aFactor ) );
    else
        return VECTOR2<T>( x / aFactor, y / aFactor );
}


template <class T>
VECTOR2<T> operator*( const T& aFactor, const VECTOR2<T>& aVector )
{
    VECTOR2<T> vector( aVector.x * aFactor, aVector.y * aFactor );
    return vector;
}


template <class T>
typename VECTOR2<T>::extended_type VECTOR2<T>::Cross( const VECTOR2<T>& aVector ) const
{
    return (extended_type) x * (extended_type) aVector.y -
           (extended_type) y * (extended_type) aVector.x;
}


template <class T>
typename VECTOR2<T>::extended_type VECTOR2<T>::Dot( const VECTOR2<T>& aVector ) const
{
    return (extended_type) x * (extended_type) aVector.x +
           (extended_type) y * (extended_type) aVector.y;
}


template <class T>
bool VECTOR2<T>::operator<( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) < ( aVector * aVector );
}


template <class T>
bool VECTOR2<T>::operator<=( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) <= ( aVector * aVector );
}


template <class T>
bool VECTOR2<T>::operator>( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) > ( aVector * aVector );
}


template <class T>
bool VECTOR2<T>::operator>=( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) >= ( aVector * aVector );
}


template <class T>
bool VECTOR2<T>::operator==( VECTOR2<T> const& aVector ) const
{
    return ( aVector.x == x ) && ( aVector.y == y );
}


template <class T>
bool VECTOR2<T>::operator!=( VECTOR2<T> const& aVector ) const
{
    return ( aVector.x != x ) || ( aVector.y != y );
}


template <class T>
const VECTOR2<T> LexicographicalMax( const VECTOR2<T>& aA, const VECTOR2<T>& aB )
{
    if( aA.x > aB.x )
        return aA;
    else if( aA.x == aB.x && aA.y > aB.y )
        return aA;

    return aB;
}


template <class T>
const VECTOR2<T> LexicographicalMin( const VECTOR2<T>& aA, const VECTOR2<T>& aB )
{
    if( aA.x < aB.x )
        return aA;
    else if( aA.x == aB.x && aA.y < aB.y )
        return aA;

    return aB;
}


template <class T>
const int LexicographicalCompare( const VECTOR2<T>& aA, const VECTOR2<T>& aB )
{
    if( aA.x < aB.x )
        return -1;
    else if( aA.x > aB.x )
        return 1;
    else    // aA.x == aB.x
    {
        if( aA.y < aB.y )
            return -1;
        else if( aA.y > aB.y )
            return 1;
        else
            return 0;
    }
}


template <class T>
std::ostream& operator<<( std::ostream& aStream, const VECTOR2<T>& aVector )
{
    aStream << "[ " << aVector.x << " | " << aVector.y << " ]";
    return aStream;
}


/* Default specializations */
typedef VECTOR2<double>       VECTOR2D;
typedef VECTOR2<int>          VECTOR2I;
typedef VECTOR2<unsigned int> VECTOR2U;

/* Compatibility typedefs */
// FIXME should be removed to avoid multiple typedefs for the same type
typedef VECTOR2<double> DPOINT;
typedef DPOINT          DSIZE;

#endif    // VECTOR2D_H_
