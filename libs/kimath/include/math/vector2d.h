/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 Virtenio GmbH, Torsten Hueter, torsten.hueter <at> virtenio.de
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <limits>
#include <iostream>
#include <sstream>
#include <type_traits>

#include <math/util.h>

/**
 * Traits class for VECTOR2.
 */
template <class T>
struct VECTOR2_TRAITS
{
    /// extended range/precision types used by operations involving multiple
    /// multiplications to prevent overflow.
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
    constexpr VECTOR2();

    /// Construct a vector with given components x, y
    constexpr VECTOR2( T x, T y );

    /// Initializes a vector from another specialization. Beware of rounding issues.
    template <typename CastingType>
    constexpr VECTOR2( const VECTOR2<CastingType>& aVec )
    {
        if( std::is_floating_point<T>() )
        {
            x = static_cast<T>( aVec.x );
            y = static_cast<T>( aVec.y );
        }
        else if( std::is_floating_point<CastingType>() )
        {
            CastingType minI = static_cast<CastingType>( std::numeric_limits<T>::min() );
            CastingType maxI = static_cast<CastingType>( std::numeric_limits<T>::max() );

            x = static_cast<T>( std::clamp( aVec.x, minI, maxI ) );
            y = static_cast<T>( std::clamp( aVec.y, minI, maxI ) );
        }
        else if( std::is_integral<T>() && std::is_integral<CastingType>() )
        {
            int64_t minI = static_cast<int64_t>( std::numeric_limits<T>::min() );
            int64_t maxI = static_cast<int64_t>( std::numeric_limits<T>::max() );

            x = static_cast<T>( std::clamp( static_cast<int64_t>( aVec.x ), minI, maxI ) );
            y = static_cast<T>( std::clamp( static_cast<int64_t>( aVec.y ), minI, maxI ) );
        }
        else
        {
            x = static_cast<T>( aVec.x );
            y = static_cast<T>( aVec.y );
        }
    }

    /// Copy a vector
    constexpr VECTOR2( const VECTOR2<T>& aVec )
    {
        x = aVec.x;
        y = aVec.y;
    }

    /// Cast a vector to another specialized subclass. Beware of rounding issues.
    template <typename U>
    constexpr VECTOR2<U> operator()() const
    {
        if( std::is_floating_point<U>::value )
        {
            return VECTOR2<U>( static_cast<U>( x ), static_cast<U>( y ) );
        }
        else if( std::is_floating_point<T>() )
        {
            T minI = static_cast<T>( std::numeric_limits<U>::min() );
            T maxI = static_cast<T>( std::numeric_limits<U>::max() );
            return VECTOR2<U>( static_cast<U>( std::clamp( x, minI, maxI ) ),
                               static_cast<U>( std::clamp( y, minI, maxI ) ) );
        }
        else if( std::is_integral<T>() && std::is_integral<U>() )
        {
            int64_t minI = static_cast<int64_t>( std::numeric_limits<U>::min() );
            int64_t maxI = static_cast<int64_t>( std::numeric_limits<U>::max() );
            int64_t x64 = static_cast<int64_t>( x );
            int64_t y64 = static_cast<int64_t>( y );

            return VECTOR2<U>(
                    static_cast<U>( std::clamp( x64, minI, maxI ) ),
                    static_cast<U>( std::clamp( y64, minI, maxI ) ) );
        }
        else
        {
            return VECTOR2<U>( static_cast<U>( x ), static_cast<U>( y ) );
        }
    }

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
    constexpr extended_type SquaredEuclideanNorm() const;


    /**
     * Compute the perpendicular vector.
     *
     * @return Perpendicular vector
     */
    constexpr VECTOR2<T> Perpendicular() const;

    /**
     * Return a vector of the same direction, but length specified in \a aNewLength.
     *
     * @param aNewLength is the length of the rescaled vector.
     * @return the rescaled vector.
     */
    VECTOR2<T> Resize( T aNewLength ) const;

    /**
     * Return the vector formatted as a string.
     *
     * @return the formatted string
     */
    const std::string Format() const;

    /**
     * Compute cross product of self with \a aVector.
     */
    constexpr extended_type Cross( const VECTOR2<T>& aVector ) const;

    /**
     * Compute dot product of self with \a aVector.
     */
    constexpr extended_type Dot( const VECTOR2<T>& aVector ) const;

    /**
     * Compute the distance between two vectors.  This is a double precision
     * value because the distance is frequently non-integer.
     */
    double Distance( const VECTOR2<extended_type>& aVector ) const;

    /**
     * Compute the squared distance between two vectors.
     */
    constexpr extended_type SquaredDistance( const VECTOR2<T>& aVector ) const;

    // Operators

    /// Assignment operator
    constexpr VECTOR2<T>& operator=( const VECTOR2<T>& aVector );

    /// Compound assignment operator
    constexpr VECTOR2<T>& operator+=( const VECTOR2<T>& aVector );

    /// Compound assignment operator
    constexpr VECTOR2<T>& operator*=( const VECTOR2<T>& aVector );

    constexpr VECTOR2<T>& operator*=( const T& aScalar );

    /// Compound assignment operator
    constexpr VECTOR2<T>& operator+=( const T& aScalar );

    /// Compound assignment operator
    constexpr VECTOR2<T>& operator-=( const VECTOR2<T>& aVector );

    /// Compound assignment operator
    constexpr VECTOR2<T>& operator-=( const T& aScalar );

    /// Negate Vector operator
    constexpr VECTOR2<T> operator-();

    /// Division with a factor
    constexpr VECTOR2<T> operator/( double aFactor ) const;

    /// Equality operator
    constexpr bool operator==( const VECTOR2<T>& aVector ) const;

    /// Not equality operator
    constexpr bool operator!=( const VECTOR2<T>& aVector ) const;

    /// Smaller than operator
    constexpr bool operator<( const VECTOR2<T>& aVector ) const;
    constexpr bool operator<=( const VECTOR2<T>& aVector ) const;

    /// Greater than operator
    constexpr bool operator>( const VECTOR2<T>& aVector ) const;
    constexpr bool operator>=( const VECTOR2<T>& aVector ) const;
};


// ----------------------
// --- Implementation ---
// ----------------------

template <class T>
constexpr VECTOR2<T>::VECTOR2() : x{}, y{}
{
}


template <class T>
constexpr VECTOR2<T>::VECTOR2( T aX, T aY )
{
    x = aX;
    y = aY;
}


template <class T>
T VECTOR2<T>::EuclideanNorm() const
{
    // 45° are common in KiCad, so we can optimize the calculation
    if( std::abs( x ) == std::abs( y ) )
    {
        if( std::is_integral<T>::value )
            return KiROUND<double, T>( std::abs( x ) * M_SQRT2 );

        return static_cast<T>( std::abs( x ) * M_SQRT2 );
    }

    if( x == 0 )
        return static_cast<T>( std::abs( y ) );
    if( y == 0 )
        return static_cast<T>( std::abs( x ) );

    if( std::is_integral<T>::value )
        return KiROUND<double, T>( std::hypot( x, y ) );

    return static_cast<T>( std::hypot( x, y ) );
}


template <class T>
constexpr typename VECTOR2<T>::extended_type VECTOR2<T>::SquaredEuclideanNorm() const
{
    return (extended_type) x * x + (extended_type) y * y;
}


template <class T>
constexpr VECTOR2<T> VECTOR2<T>::Perpendicular() const
{
    VECTOR2<T> perpendicular( -y, x );
    return perpendicular;
}


template <class T>
constexpr VECTOR2<T>& VECTOR2<T>::operator=( const VECTOR2<T>& aVector )
{
    x = aVector.x;
    y = aVector.y;
    return *this;
}


template <class T>
constexpr VECTOR2<T>& VECTOR2<T>::operator+=( const VECTOR2<T>& aVector )
{
    x += aVector.x;
    y += aVector.y;
    return *this;
}


template <class T>
constexpr VECTOR2<T>& VECTOR2<T>::operator*=( const VECTOR2<T>& aVector )
{
    x *= aVector.x;
    y *= aVector.y;
    return *this;
}


template <class T>
constexpr VECTOR2<T>& VECTOR2<T>::operator*=( const T& aScalar )
{
    x *= aScalar;
    y *= aScalar;
    return *this;
}


template <class T>
constexpr VECTOR2<T>& VECTOR2<T>::operator+=( const T& aScalar )
{
    x += aScalar;
    y += aScalar;
    return *this;
}


template <class T>
constexpr VECTOR2<T>& VECTOR2<T>::operator-=( const VECTOR2<T>& aVector )
{
    x -= aVector.x;
    y -= aVector.y;
    return *this;
}


template <class T>
constexpr VECTOR2<T>& VECTOR2<T>::operator-=( const T& aScalar )
{
    x -= aScalar;
    y -= aScalar;
    return *this;
}


template <class T>
VECTOR2<T> VECTOR2<T>::Resize( T aNewLength ) const
{
    if( x == 0 && y == 0 )
        return VECTOR2<T> ( 0, 0 );

    double newX;
    double newY;

    if( std::abs( x ) == std::abs( y ) )
    {
        newX = newY = std::abs( aNewLength ) * M_SQRT1_2;
    }
    else
    {
        extended_type x_sq = (extended_type) x * x;
        extended_type y_sq = (extended_type) y * y;
        extended_type l_sq = x_sq + y_sq;
        extended_type newLength_sq = (extended_type) aNewLength * aNewLength;
        newX = std::sqrt( rescale( newLength_sq, x_sq, l_sq ) );
        newY = std::sqrt( rescale( newLength_sq, y_sq, l_sq ) );
    }

    if( std::is_integral<T>::value )
    {
        return VECTOR2<T>( static_cast<T>( x < 0 ? -KiROUND( newX ) : KiROUND( newX ) ),
                           static_cast<T>( y < 0 ? -KiROUND( newY ) : KiROUND( newY ) ) )
               * sign( aNewLength );
    }
    else
    {
        return VECTOR2<T>( static_cast<T>( x < 0 ? -newX : newX ),
                           static_cast<T>( y < 0 ? -newY : newY ) )
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
concept FloatingPoint = std::is_floating_point<T>::value;

template <class T>
concept Integral = std::is_integral<T>::value;


template <class T, class U>
constexpr VECTOR2<std::common_type_t<T, U>> operator+( const VECTOR2<T>& aLHS,
                                                       const VECTOR2<U>& aRHS )
{
    return VECTOR2<std::common_type_t<T, U>>( aLHS.x + aRHS.x, aLHS.y + aRHS.y );
}


template <FloatingPoint T, class U>
constexpr VECTOR2<T> operator+( const VECTOR2<T>& aLHS, const U& aScalar )
{
    return VECTOR2<T>( aLHS.x + aScalar, aLHS.y + aScalar );
}


#ifndef SWIG
template <Integral T, Integral U>
constexpr VECTOR2<T> operator+( const VECTOR2<T>& aLHS, const U& aScalar )
{
    return VECTOR2<T>( aLHS.x + aScalar, aLHS.y + aScalar );
}


template <Integral T, FloatingPoint U>
constexpr VECTOR2<T> operator+( const VECTOR2<T>& aLHS, const U& aScalar )
{
    return VECTOR2<T>( KiROUND( aLHS.x + aScalar ), KiROUND( aLHS.y + aScalar ) );
}
#endif


template <class T, class U>
constexpr VECTOR2<std::common_type_t<T, U>> operator-( const VECTOR2<T>& aLHS,
                                                       const VECTOR2<U>& aRHS )
{
    return VECTOR2<std::common_type_t<T, U>>( aLHS.x - aRHS.x, aLHS.y - aRHS.y );
}


template <FloatingPoint T, class U>
constexpr VECTOR2<T> operator-( const VECTOR2<T>& aLHS, U aScalar )
{
    return VECTOR2<T>( aLHS.x - aScalar, aLHS.y - aScalar );
}


#ifndef SWIG
template <Integral T, Integral U>
constexpr VECTOR2<T> operator-( const VECTOR2<T>& aLHS, U aScalar )
{
    return VECTOR2<T>( aLHS.x - aScalar, aLHS.y - aScalar );
}


template <Integral T, FloatingPoint U>
constexpr VECTOR2<T> operator-( const VECTOR2<T>& aLHS, const U& aScalar )
{
    return VECTOR2<T>( KiROUND( aLHS.x - aScalar ), KiROUND( aLHS.y - aScalar ) );
}
#endif


template <class T>
constexpr VECTOR2<T> VECTOR2<T>::operator-()
{
    return VECTOR2<T> ( -x, -y );
}


template <class T, class U>
#ifdef SWIG
constexpr double operator*( const VECTOR2<T>& aLHS, const VECTOR2<U>& aRHS )
#else
constexpr auto operator*( const VECTOR2<T>& aLHS, const VECTOR2<U>& aRHS )
#endif
{
    using extended_type = typename VECTOR2<std::common_type_t<T, U>>::extended_type;
    return (extended_type)aLHS.x * aRHS.x + (extended_type)aLHS.y * aRHS.y;
}


template <class T, class U>
constexpr VECTOR2<std::common_type_t<T, U>> operator*( const VECTOR2<T>& aLHS, const U& aScalar )
{
    return VECTOR2<std::common_type_t<T, U>>( aLHS.x * aScalar, aLHS.y * aScalar );
}


template <class T, class U>
constexpr VECTOR2<std::common_type_t<T, U>> operator*( const T& aScalar, const VECTOR2<U>& aVector )
{
    return VECTOR2<std::common_type_t<T, U>>( aScalar * aVector.x, aScalar * aVector.y );
}


template <class T>
constexpr VECTOR2<T> VECTOR2<T>::operator/( double aFactor ) const
{
    if( std::is_integral<T>::value )
        return VECTOR2<T>( KiROUND( x / aFactor ), KiROUND( y / aFactor ) );
    else
        return VECTOR2<T>( static_cast<T>( x / aFactor ), static_cast<T>( y / aFactor ) );
}


template <class T>
constexpr typename VECTOR2<T>::extended_type VECTOR2<T>::Cross( const VECTOR2<T>& aVector ) const
{
    return (extended_type) x * (extended_type) aVector.y -
           (extended_type) y * (extended_type) aVector.x;
}


template <class T>
constexpr typename VECTOR2<T>::extended_type VECTOR2<T>::Dot( const VECTOR2<T>& aVector ) const
{
    return (extended_type) x * (extended_type) aVector.x +
           (extended_type) y * (extended_type) aVector.y;
}

template <class T>
double VECTOR2<T>::Distance( const VECTOR2<extended_type>& aVector ) const
{
    VECTOR2<double> diff( static_cast<double>( aVector.x - x ), static_cast<double>( aVector.y - y ) );
    return diff.EuclideanNorm();
}

template <class T>
constexpr typename VECTOR2<T>::extended_type
VECTOR2<T>::SquaredDistance( const VECTOR2<T>& aVector ) const
{
    const extended_type dx = (extended_type) x - aVector.x;
    const extended_type dy = (extended_type) y - aVector.y;
    return dx * dx + dy * dy;
}


template <class T>
constexpr bool VECTOR2<T>::operator<( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) < ( aVector * aVector );
}


template <class T>
constexpr bool VECTOR2<T>::operator<=( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) <= ( aVector * aVector );
}


template <class T>
constexpr bool VECTOR2<T>::operator>( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) > ( aVector * aVector );
}


template <class T>
constexpr bool VECTOR2<T>::operator>=( const VECTOR2<T>& aVector ) const
{
    return ( *this * *this ) >= ( aVector * aVector );
}


template <class T>
constexpr bool VECTOR2<T>::operator==( VECTOR2<T> const& aVector ) const
{
    return ( aVector.x == x ) && ( aVector.y == y );
}


template <class T>
constexpr bool VECTOR2<T>::operator!=( VECTOR2<T> const& aVector ) const
{
    return ( aVector.x != x ) || ( aVector.y != y );
}


template <class T>
constexpr const VECTOR2<T>& LexicographicalMax( const VECTOR2<T>& aA, const VECTOR2<T>& aB )
{
    if( aA.x > aB.x )
        return aA;
    else if( aA.x == aB.x && aA.y > aB.y )
        return aA;

    return aB;
}


template <class T>
constexpr const VECTOR2<T>& LexicographicalMin( const VECTOR2<T>& aA, const VECTOR2<T>& aB )
{
    if( aA.x < aB.x )
        return aA;
    else if( aA.x == aB.x && aA.y < aB.y )
        return aA;

    return aB;
}


template <class T>
constexpr int LexicographicalCompare( const VECTOR2<T>& aA, const VECTOR2<T>& aB )
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


/**
 * Template to compare two VECTOR2<T> values for equality within a required epsilon.
 *
 * Not constexpr until C++23 (without forgoing std::abs in equals)
 *
 * @param aFirst value to compare.
 * @param aSecond value to compare.
 * @param aEpsilon allowed error.
 * @return true if the values considered equal within the specified epsilon, otherwise false.
 */
template <class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
equals( VECTOR2<T> const& aFirst, VECTOR2<T> const& aSecond,
        T aEpsilon = std::numeric_limits<T>::epsilon() )
{
    if( !equals( aFirst.x, aSecond.x, aEpsilon ) )
    {
        return false;
    }

    return equals( aFirst.y, aSecond.y, aEpsilon );
}


template <class T>
std::ostream& operator<<( std::ostream& aStream, const VECTOR2<T>& aVector )
{
    aStream << "[ " << aVector.x << " | " << aVector.y << " ]";
    return aStream;
}

/* Default specializations */
typedef VECTOR2<double>  VECTOR2D;
typedef VECTOR2<int32_t> VECTOR2I;
typedef VECTOR2<int64_t> VECTOR2L;

/* KiROUND specialization for vectors */
inline constexpr VECTOR2I KiROUND( const VECTOR2D& vec )
{
    return VECTOR2I( KiROUND( vec.x ), KiROUND( vec.y ) );
}

inline constexpr VECTOR2I KiROUND( double x, double y )
{
    return VECTOR2I( KiROUND( x ), KiROUND( y ) );
}

/* STL specializations */
namespace std
{
    // Required to enable correct use in std::map/unordered_map
    // DO NOT USE hash tables with VECTOR2 elements.  It is inefficient
    // and degenerates to a linear search.  Use the std::map/std::set
    // trees instead that utilize the less operator below
    // This function is purposely deleted after substantial testing
    template <>
    struct hash<VECTOR2I>
    {
        size_t operator()( const VECTOR2I& k ) const = delete;
    };

    // Required to enable use of std::hash with maps.
    template <>
    struct less<VECTOR2I>
    {
        bool operator()( const VECTOR2I& aA, const VECTOR2I& aB ) const;
    };
}

#endif    // VECTOR2D_H_
