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

    /** @defgroup vector-elements Access individual attributes
     * @{
     */
    LENGTH_PCB &operator[]( int i )
    {
        return data[i];
    }

    const LENGTH_PCB &operator[]( int i ) const
    {
        return data[i];
    }

    /** @} */

    /** @defgroup vector-cartesian Access to cartesian coordinates
     * Definitions follow the agreement:
     * - methods are named exactly as attributes, thus setter and getter have same name;
     * - all methods (setters and getterrs) return actual attribute value;
     * - method without argument gets the attribute;
     * - method with argument sets it to argument value.
     * These methods different than operator[]
     * because vector actually may have any storage layout,
     * so cartesian coords may be calculated rahter than actually stored.
     * E. g. homogeneous coordinates is likely to be used in perspective.
     * @{
     */

    const LENGTH_PCB x() const
    {
        return data[0];
    }

    const LENGTH_PCB x( LENGTH_PCB nx )
    {
        return data[0] = nx;
    }

    const LENGTH_PCB y() const
    {
        return data[1];
    }

    const LENGTH_PCB y( LENGTH_PCB ny )
    {
        return data[1] = ny;
    }

    /** @} */

    /** @defgroup vector-comparisons Compare vectors
     * @{
     */

    bool operator == ( const VECTOR_PCB &b ) const
    {
        return data[0] == b.data[0] && data[1] == b.data[1];
    }

    bool operator != ( const VECTOR_PCB &b ) const
    {
        return !(*this == b);
    }

    /** @} */

    /** @defgroup vector-arithmetic Arithmetic operations on vectors
     * @{
     */

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

    /** @} */
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
