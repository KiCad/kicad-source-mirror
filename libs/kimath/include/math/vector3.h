/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VECTOR3_H_
#define VECTOR3_H_

#include <iostream>
#include <stdint.h>
#include <limits>
#include <wx/debug.h>

/**
 * Traits class for VECTOR2.
 */
template <class T>
struct VECTOR3_TRAITS
{
    /// extended range/precision types used by operations involving multiple
    /// multiplications to prevent overflow.
    typedef T extended_type;
};

template <>
struct VECTOR3_TRAITS<int>
{
    typedef int64_t extended_type;
};


/**
 * Define a general 3D-vector.
 *
 * This class uses templates to be universal. Several operators are provided to help
 * easy implementing of linear algebra equations.
 *
 */
template <class T = int>
class VECTOR3
{
public:
    typedef typename VECTOR3_TRAITS<T>::extended_type extended_type;
    typedef T                                         coord_type;

    static constexpr extended_type ECOORD_MAX = std::numeric_limits<extended_type>::max();
    static constexpr extended_type ECOORD_MIN = std::numeric_limits<extended_type>::min();

    T x{};
    T y{};
    T z{};

    /// Construct a 3D-vector with x, y, z = 0
    VECTOR3() = default;

    /// Construct a vector with given components x, y, z
    VECTOR3( T x, T y, T z );

    /// Initializes a vector from another specialization. Beware of rounding
    /// issues.
    template <typename CastingType>
    VECTOR3( const VECTOR3<CastingType>& aVec );

    /**
     * Compute cross product of self with \a aVector
     */
    VECTOR3<T> Cross( const VECTOR3<T>& aVector ) const;

    /**
     * Compute the dot product of self with \a aVector
     */
    VECTOR3<T>::extended_type Dot( const VECTOR3<T>& aVector ) const;

    /**
     * Compute the Euclidean norm of the vector, which is defined as sqrt(x ** 2 + y ** 2).
     *
     * It is used to calculate the length of the vector.
     *
     * @return Scalar, the euclidean norm
     */
    T EuclideanNorm() const;

    /**
     * Compute the normalized vector.
     */
    VECTOR3<T> Normalize();

    /**
     * Set all elements to \a val
     */
    VECTOR3<T> SetAll( T val );

    /// Equality operator
    bool operator==( const VECTOR3<T>& aVector ) const;

    /// Not equality operator
    bool operator!=( const VECTOR3<T>& aVector ) const;

    VECTOR3<T>& operator*=( T val );
    VECTOR3<T>& operator/=( T val );
};


template <class T>
VECTOR3<T>::VECTOR3( T aX, T aY, T aZ ) :
        x( aX ), y( aY ), z( aZ )
{
}


template <class T>
template <typename CastingType>
VECTOR3<T>::VECTOR3( const VECTOR3<CastingType>& aVec ) :
        x( aVec.x ), y( aVec.y ), z( aVec.z )
{
}


template <class T>
VECTOR3<T> VECTOR3<T>::Cross( const VECTOR3<T>& aVector ) const
{
    return VECTOR3<T>( ( y * aVector.z ) - ( z * aVector.y ),
                       ( z * aVector.x ) - ( x * aVector.z ),
                       ( x * aVector.y ) - ( y * aVector.x )
                     );
}


template <class T>
typename VECTOR3<T>::extended_type VECTOR3<T>::Dot( const VECTOR3<T>& aVector ) const
{
    return extended_type{x} * extended_type{aVector.x}
            + extended_type{y} * extended_type{aVector.y}
            + extended_type{z} * extended_type{aVector.z};
}


template <class T>
T VECTOR3<T>::EuclideanNorm() const
{
    return sqrt( (extended_type) x * x + (extended_type) y * y + (extended_type) z * z );
}


template <class T>
VECTOR3<T> VECTOR3<T>::Normalize()
{
    T norm = EuclideanNorm();

    wxCHECK_MSG( norm > T( 0 ), *this, wxT( "Invalid element length 0" ) );

    x /= norm;
    y /= norm;
    z /= norm;

    return *this;
}


template <class T>
VECTOR3<T> VECTOR3<T>::SetAll( T val )
{
    x = val;
    y = val;
    z = val;

    return *this;
}


template <class T>
bool VECTOR3<T>::operator==( VECTOR3<T> const& aVector ) const
{
    return ( aVector.x == x ) && ( aVector.y == y ) && ( aVector.z == z );
}


template <class T>
bool VECTOR3<T>::operator!=( VECTOR3<T> const& aVector ) const
{
    return ( aVector.x != x ) || ( aVector.y != y ) || ( aVector.z != z );
}


template <class T>
VECTOR3<T>& VECTOR3<T>::operator*=( T aScalar )
{
    x = x * aScalar;
    y = y * aScalar;
    z = z * aScalar;

    return *this;
}


template <class T>
VECTOR3<T>& VECTOR3<T>::operator/=( T aScalar )
{
    x = x / aScalar;
    y = y / aScalar;
    z = z / aScalar;

    return *this;
}


template <class T>
std::ostream& operator<<( std::ostream& aStream, const VECTOR3<T>& aVector )
{
    aStream << "[ " << aVector.x << " | " << aVector.y << " | " << aVector.z << " ]";
    return aStream;
}


/* Default specializations */
typedef VECTOR3<double>       VECTOR3D;
typedef VECTOR3<int>          VECTOR3I;
typedef VECTOR3<unsigned int> VECTOR3U;

#endif // VECTOR3_H_
