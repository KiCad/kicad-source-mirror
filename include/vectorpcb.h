/**
 * @file vectorpcb.h
 * @brief Planar vector definitions for PCBNEW.
 */

#ifndef VECTORPCB_H_INCLUDED
#define VECTORPCB_H_INCLUDED 1

/// @TODO: nice template and refiling for it
class VECTOR_PCB
{
public:
    LENGTH_PCB data[2];

    /** A vector from pair of coords. Constructor is avoided in favor to POD-like object.
     */
    static VECTOR_PCB fromXY( LENGTH_PCB x, LENGTH_PCB y )
    {
        VECTOR_PCB z = { { x, y } };
        return z;
    }

    LENGTH_PCB &operator[]( int i )
    {
        return data[i];
    }

    const LENGTH_PCB &operator[]( int i ) const
    {
        return data[i];
    }

    LENGTH_PCB & x()
    {
        return data[0];
    }

    const LENGTH_PCB & x() const
    {
        return data[0];
    }

    LENGTH_PCB & y()
    {
        return data[1];
    }

    const LENGTH_PCB & y() const
    {
        return data[1];
    }

    //LENGTH_PCB x, y;
    bool operator == ( const VECTOR_PCB &b ) const
    {
        return data[0] == b.data[0] && data[1] == b.data[1];
    }

    bool operator != ( const VECTOR_PCB &b ) const
    {
        return !(*this == b);
    }

    VECTOR_PCB & operator -= ( const VECTOR_PCB &b )
    {
        data[0] -= b.data[0];
        data[1] -= b.data[1];
        return *this;
    }

    VECTOR_PCB operator - ( const VECTOR_PCB &b ) const
    {
        VECTOR_PCB z = *this;
        z -= b;
        return z;
    }

    VECTOR_PCB & operator += ( const VECTOR_PCB &b )
    {
        data[0] += b.data[0];
        data[1] += b.data[1];
        return *this;
    }

    VECTOR_PCB operator + ( VECTOR_PCB b ) const
    {
        VECTOR_PCB z = *this;
        z += b;
        return z;
    }

    VECTOR_PCB & operator *= ( int b )
    {
        data[0] *= b;
        data[1] *= b;
        return *this;
    }

    VECTOR_PCB operator * ( int b ) const
    {
        VECTOR_PCB z = *this;
        z *= b;
        return z;
    }

    VECTOR_PCB & operator /= ( int b )
    {
        data[0] /= b;
        data[1] /= b;
        return *this;
    }

    VECTOR_PCB operator / ( int b ) const
    {
        VECTOR_PCB z = *this;
        z /= b;
        return z;
    }
};

#define TO_LEGACY_LU_WXP( p ) ( wxPoint( \
    TO_LEGACY_LU( ( p )[0] ), \
    TO_LEGACY_LU( ( p )[1] ) ) )
#define TO_LEGACY_LU_WXS( p ) ( wxSize( \
    TO_LEGACY_LU( ( p )[0] ), \
    TO_LEGACY_LU( ( p )[1] ) ) )
#define FROM_LEGACY_LU_VEC( p ) ( VECTOR_PCB::fromXY( \
    FROM_LEGACY_LU( ( p ).x ), \
    FROM_LEGACY_LU( ( p ).y ) ) )

#endif /* def VECTORPCB_H_INCLUDED */
