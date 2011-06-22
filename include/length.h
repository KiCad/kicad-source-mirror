/**
 * The physical length library. Made for nanometer scale.
 * @file length.h
 */

#ifndef UNITS_H_INCLUDED
#define UNITS_H_INCLUDED 1

#include <math.h>
/**********************************************/
/*! I'm a physical length                     */
/**********************************************/

class LENGTH
{

private:
    enum
    {
        METER = 1000000000, /* The ONLY constant connecting length to the real world */
    };
    int m_Units;

    /*!
     * The only constructor allowing direct input of numeric value
     * in internal units. As this is not allowed in public, it's private.
     * Length objects elsewhere are created indirectly
     * @param unit Length in internal units.
     */
    LENGTH( int units )
    {
        m_Units = units;
    }

public:
    /*!
     * Equality comparison of physical lengths.
     * @param y length to compare
     * @return lengths are equal
     */
    bool operator == ( const LENGTH y ) const
    {
        return m_Units == y.m_Units;
    }

    /*!
     * Non-equality comparison of physical lengths.
     * @param y length to compare
     * @return lengts are different
     */
    bool operator != ( const LENGTH y ) const
    {
        return m_Units != y.m_Units;
    }

    /*!
     * Order comparison of physical lengths.
     * @param y length to compare
     * @return one less than another
     */
    bool operator < ( const LENGTH y ) const
    {
        return m_Units < y.m_Units;
    }

    /*!
     * Order comparison of physical lengths.
     * @param y length to compare
     * @return one greater than another
     */
    bool operator > ( const LENGTH y ) const
    {
        return m_Units > y.m_Units;
    }

    /*!
     * Order comparison of physical lengths.
     * @param y length to compare
     * @return one less or equal than another
     */
    bool operator <= ( const LENGTH y ) const
    {
        return m_Units <= y.m_Units;
    }

    /*!
     * Order comparison of physical lengths.
     * @param y length to compare
     * @return one greater or equal than another
     */
    bool operator >= ( const LENGTH y ) const
    {
        return m_Units >= y.m_Units;
    }

    /*!
     * Sum of two physical lengths. Only another length can be added.
     * @param y length to add
     * @return result of addition
     */
    const LENGTH operator + ( const LENGTH y ) const
    {
        return LENGTH( m_Units + y.m_Units );
    }

    /*!
     * Add a length inplace
     * @param y length to add
     * @return result of addition
     */
    LENGTH & operator += ( const LENGTH y )
    {
        m_Units += y.m_Units;
        return *this;
    }

    /*!
     * Differece of two physical lengths. Only another length can be subtracted.
     * @param y length to subtract
     * @return result of subtraction
     */
    const LENGTH operator - ( const LENGTH y ) const
    {
        return LENGTH( m_Units - y.m_Units );
    }

    /*!
     * Subtract a length inplace
     * @param y length to add
     * @return result of addition
     */
    LENGTH & operator -= ( const LENGTH y )
    {
        m_Units -= y.m_Units;
        return *this;
    }

    /*!
     * Negation of length.
     * @return length negated
     */
    const LENGTH operator - ( void ) const {
        return LENGTH( - m_Units );
    }

    /*!
     * Scale length to rational number, given numerator and denominator.
     * This is done without overflow or precision loss unlike dealing
     * with * / and floating point.
     * @param mul numerator, length is multiplied by this value
     * @param div denominator. length is divided by this value
     * @return scaled length
     */
    const LENGTH byRatio ( int mul, int div ) const
    {
        return LENGTH( ( int )( ( long long ) m_Units * mul / div ) );
    }

    /*!
     * Scale length to rational number inplace.
     * @param mul numerator, length is multiplied by this value
     * @param div denominator. length is divided by this value
     * @return scaled length
     */
    LENGTH & setByRatio ( int mul, int div )
    {
        m_Units = ( int )( ( long long ) m_Units * mul / div );
	return *this;
    }

    /*!
     * Multiplies length by integer number.
     * @param y factor
     * @return scaled length
     */
    const LENGTH operator * ( int y ) const
    {
        return LENGTH( m_Units * y );
    }

    /*!
     * Multiply a length inplace
     * @param y factor
     * @return scaled length
     */
    LENGTH & operator *= ( int y )
    {
        m_Units *= y;
        return *this;
    }

    /*!
     * Multiplies length by floating point.
     * @param y factor
     * @return scaled length
     */
    const LENGTH operator * ( double y ) const
    {
        return LENGTH( ( int )( m_Units * y ) );
    }

    /*!
     * Multiply a length inplace
     * @param y factor
     * @return scaled length
     */
    LENGTH & operator *= ( double y )
    {
        m_Units *= y;
        return *this;
    }

    /*!
     * Multiplies integer by length ( like abowe with args swapped ).
     * @param x factor
     * @param y length
     * @return scaled length
     */
    const LENGTH friend operator * ( int x, const LENGTH y )
    {
        return y * x;
    }

    /*!
     * Multiplies floating point by length ( like abowe with args swapped ).
     * @param x factor
     * @param y length
     * @return scaled length
     */
    const LENGTH friend operator * ( double x, const LENGTH y )
    {
        return y * x;
    }

    /*!
     * Divides length by integer number.
     * @param y divider
     * @return scaled length
     */
    const LENGTH operator / ( int y ) const
    {
        return LENGTH( m_Units / y );
    }

    /*!
     * Divide a length inplace
     * @param y divider
     * @return scaled length
     */
    LENGTH & operator /= ( int y )
    {
        m_Units /= y;
        return *this;
    }

    /*!
     * Divides length by floating point.
     * @param y divider
     * @return scaled length
     */
    const LENGTH operator / ( double y ) const
    {
        return LENGTH( ( long long )( m_Units / y ) );
    }

    /*!
     * Divide a length inplace
     * @param y divider
     * @return scaled length
     */
    LENGTH & operator /= ( double y )
    {
        m_Units /= y;
        return *this;
    }

    /*!
     * Gets ratio of two lengths.
     * It is usable to get number of length units in length by
     * division of length by length unit ( See length units below ).
     * @param y base length
     * @return scaled length
     */
    double operator / ( const LENGTH y ) const
    {
        return ( double ) m_Units / y.m_Units;
    }

    /*!
     * Gets integer ( unlike operator / ) ratio of two lengths.
     * It is usable to get number of length units in length by
     * division of length by length unit ( See length units below ).
     * @param y base length
     * @return scaled length
     */
    int idiv( const LENGTH y ) const
    {
        return ( int )( m_Units / y.m_Units );
    }

    /*!
     * Zero.
     * @return Zero length
     */
    static const LENGTH zero( void )
    {
        return LENGTH( 0 );
    }

    /*!
     * The metre unit.
     * @return One metre length
     */
    static const LENGTH metre( void )
    {
        return LENGTH( METER );
    }

    /*!
     * The millimetre unit.
     * @return One millimetre length
     */
    static const LENGTH millimetre( void )
    {
        return LENGTH( METER/1000 );
    }

    /*!
     * The inch unit.
     * @return One inch length
     */
    static const LENGTH inch( void )
    {
        return LENGTH( METER/10000*254 ); // ensure it's done without precision loss
    }

    /*!
     * The mil unit.
     * @return One mil length
     */
    static const LENGTH mil( void )
    {
        return inch()/1000;
    }

    /*!
     * Hypotenuse of a triangle with two given katheti.
     * @param y another kathetus
     * @return hypothenuse
     */
    const LENGTH hypotenuse( LENGTH y ) const
    {
        return LENGTH ( ( int ) sqrt (
                ( ( double ) m_Units * m_Units
                + ( double ) y.m_Units * y.m_Units ) ) );
    }

    /*!
     * Another kathetus of a triangle with given hypothenuse and kathetus.
     * @param y kathetus
     * @return another kathetus
     */
    const LENGTH kathetus( LENGTH y ) const
    {
        return LENGTH ( ( int ) sqrt ( 
               ( ( double ) m_Units * m_Units
               - ( double ) y.m_Units * y.m_Units ) ) );
    }

};

/**********************************************/
/*! I'm a point/vector in a physical 2D plane */
/**********************************************/

class LENGTH_XY {
private:
    LENGTH m_X, m_Y;

public:
    /*!
     * One given x and y coords of type LENGTH
     * @param x coordinate
     * @param y coordinate
     */
    LENGTH_XY( const LENGTH x, const LENGTH y ) : m_X( x ), m_Y( y )
    {
    }

    /*! 
     * A point ( or vector ) given x and y multiplies of some unit.
     * Given just for a convenience, you can use ( x*unit, y*unit ) instead.
     * @param x coordinate factor
     * @param y coordinate factor
     * @param unit the unit
     */
    LENGTH_XY( int x, int y, const LENGTH unit ) : m_X( unit * x ), m_Y( unit * y )
    {
    }

    /*!
     * x coordinate
     * @return x coordinate
     */
    const LENGTH x( void ) const
    {
        return m_X;
    }

    /*!
     * y coordinate
     * @return y coordinate
     */
    const LENGTH y( void ) const 
    {
        return m_Y;
    }
    
    /*!
     * Absoulte value / length
     * @return absolute value 
     */
    const LENGTH abs( void ) const
    {
        return m_X.hypotenuse(m_Y);
    }

    /*!
     * Equality comparison of vectors.
     * @param y vectors to compare
     * @return vectors are equal
     */
    bool operator == ( const LENGTH_XY y ) const
    {
        return m_X == y.m_X && m_Y == y.m_Y;
    }

    /*!
     * Non-equality comparison of vectors.
     * @param y vectors to compare
     * @return vectors are different
     */
    bool operator != ( const LENGTH_XY y ) const
    {
        return m_X != y.m_X || m_Y != y.m_Y;
    }

    /*!
     * Sum of two vectors ( or a point translated by vector )
     * @param y vector to add
     * @return result of addition
     */
    const LENGTH_XY operator + ( const LENGTH_XY y ) const
    {
        return LENGTH_XY( m_X + y.m_X, m_Y + y.m_Y );
    }

    /*!
     * Translate a vector inplace
     * @param y vector to add
     * @return result of addition
     */
    LENGTH_XY & operator += ( const LENGTH_XY y )
    {
        m_X += y.m_X;
        m_Y += y.m_Y;
        return *this;
    }

    /*!
     * Difference of two vectors ( or a point translated by vector in reverse direction ).
     * @param y vector to subtract
     * @return result of subtraction
     */
    const LENGTH_XY operator - ( const LENGTH_XY y ) const
    {
        return LENGTH_XY( m_X - y.m_X, m_Y - y.m_Y );
    }

    /*!
     * Translate a vector inplace in opposite direction
     * @param y vector to subtract
     * @return result of subtraction
     */
    LENGTH_XY & operator -= ( const LENGTH_XY y )
    {
        m_X -= y.m_X;
        m_Y -= y.m_Y;
        return *this;
    }

    /*!
     * Vector with reverse direction.
     * @return reverse direction vector
     */
    const LENGTH_XY operator - ( void ) const
    {
        return LENGTH_XY( - m_X, - m_Y );
    }

    /*!
     * Scale vector to rational number, given numerator and denominator.
     * This is done without overflow or precision loss unlike dealing
     * with * / and floating point.
     * @param mul numerator ( length is multiplied by this value )
     * @param div denominator ( length is divided by this value )
     * @return scaled vector
     */
    const LENGTH_XY byRatio ( int mul, int div )
    {
        return LENGTH_XY( m_X.byRatio( mul, div ), m_Y.byRatio( mul, div ) );
    }

    /*!
     * Scale vector to rational number, inplace (like operator *=).
     * @param mul numerator ( length is multiplied by this value )
     * @param div denominator ( length is divided by this value )
     * @return scaled vector
     */
    LENGTH_XY & setByRatio ( int mul, int div )
    {
        m_X.setByRatio( mul, div );
        m_Y.setByRatio( mul, div );
        return *this;
    }

    /*!
     * Multiplies vector length by integer number.
     * @param y factor
     * @return scaled vector
     */
    const LENGTH_XY operator * ( int y ) const
    {
        return LENGTH_XY( m_X * y, m_Y * y );
    }

    /*!
     * Multiply a vector inplace
     * @param y factor
     * @return scaled vector
     */
    LENGTH_XY & operator *= ( int y )
    {
        m_X *= y;
        m_Y *= y;
        return *this;
    }

    /*!
     * Multiplies vector length by floating point number.
     * @param y factor
     * @return scaled length
     */
    const LENGTH_XY operator * ( double y ) const
    {
        return LENGTH_XY( m_X * y, m_Y * y );
    }

    /*!
     * Multiply a vector inplace
     * @param y factor
     * @return scaled vector
     */
    LENGTH_XY & operator *= ( double y )
    {
        m_X *= y;
        m_Y *= y;
        return *this;
    }

    /*!
     * Divides vector length by integer number.
     * @param y divider
     * @return scaled vector
     */
    const LENGTH_XY operator / ( int y ) const
    {
        return LENGTH_XY( m_X / y, m_Y / y );
    }

    /*!
     * Divide a vector inplace
     * @param y divider
     * @return scaled vector
     */
    LENGTH_XY & operator /= ( int y )
    {
        m_X /= y;
        m_Y /= y;
        return *this;
    }

    /*!
     * Divides vector length by floating point number.
     * @param y divider
     * @return scaled vector
     */
    const LENGTH_XY operator / ( double y ) const
    {
        return LENGTH_XY( m_X / y, m_Y / y );
    }

    /*!
     * Divide a vector inplace
     * @param y divider
     * @return scaled vector
     */
    LENGTH_XY & operator /= ( double y )
    {
        m_X /= y;
        m_Y /= y;
        return *this;
    }

    /*!
     * Rotates vector 90 degrees ( X axis towards Y )
     * @return rotated
     */
    const LENGTH_XY rot90 ( void ) const
    {
        return LENGTH_XY( m_Y, -m_X );
    }
};
#endif