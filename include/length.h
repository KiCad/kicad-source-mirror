/**
 * @file length.h
 * @brief The physical length library. Made for nanometer scale.
 */

/* sorry if it is not styled correctly, i'll work on it further */

#ifndef LENGTH_H_INCLUDED
#define LENGTH_H_INCLUDED 1

#include <math.h>

/*!
 * Length template class.
 * @param T actual type holding a value (be aware of precision and range!)
 * @param P power of length unit: 1 - length, 2 - area, 3 - volume, -1 - lin. density etc...
 * This class check length dimension in compile time. In runtime it behaves
 * exactly like contained type t (which should be numeric type, like int or double)
 * This class can be replaced with its contained type or simple stub.
 * Check rules:
 * - comparisons (< = etc.), addition, subtraction require values of same dimension
 *   e. g. length with length, area with area etc.
 * - multiplication and division result have appropriate dimension (powers
 *   added and subtracted respectively)
 * - sqrt and cbrt have appropriate dimensions (P/2 and P/3).
 * Limitations:
 * - functions which should not be applied to dimensioned values are not implemeted:
 *   they include algebraic (exp, log...), trigo (sin, cos...), hyperbolic (sinh, cosh..)
 * - pow function is not implemented as it is require dimension check in runtime
 *   you should use multiplication, division, sqrt and cbrt functions instead.
 * - sqrt and cbrt result type should be instantiated before they used
 *   Be aware when using them in complex formulae, e. g.
 *   LENGTH< double, 1 > len = cbrt(vol) - is ok, but
 *   LENGTH< double, 2 > vol = sqrt(area*area*area*area)/length - will fail
 *   if LENGTH<..., 4> is not instantiated
 * - non-integer power values do not supported
 *   they should be implemented carefully using natural fractions, not floats, to be exact
 *   but they are very rare so you should not worry about.
 *   e. g. linear electric noise density should be in mV/sqrt(m)
 * - automatic numeric type casts are not performed. You even have to manually
 *   cast LENGTH< short > to LENGTH< int > or LENGTH< float >
 *   to LENGTH< double >. Anyway it is not such trouble as progremmer should be
 *   very careful when mixing numeric types and avoid automatic casts.
 *
 */

template < typename T = double, int P = 1 > class LENGTH;

/*!
 * Length units.
 */

template <typename T> class LENGTH_UNITS;

/*!
 * The template that "inflate" LENGTH< T, 0 > class to T. Used with (*) and (/).
 */
template < typename T, int P > struct LENGTH_TRAITS
{
    typedef LENGTH<T, P> flat;
};

template < typename T > struct LENGTH_TRAITS< T, 0 >
{
    /* length dimension to power 0 is just a number, so LENGTH<T, 0> should be automatically converted to T */
    typedef T flat;
};

/*!
 * The template for value type conversions
 */
template < typename T > struct LENGTH_CASTS
{
     /*! This function to convert length value to given type T. */
     template< typename X > static T cast( const X x )
     {
         return T( x );
     }
};

template <> struct LENGTH_CASTS < int >
{
     static int cast( const double x )
     {
         return floor( x + 0.5 );
     }
};

template <> struct LENGTH_CASTS < long >
{
     static long cast( const double x )
     {
         return floor( x + 0.5 );
     }
};

/** Forward declaration for LIMITED_INT to use with casts. */
template < typename T > class LIMITED_INT;

template < typename T > struct LENGTH_CASTS < LIMITED_INT< T > >
{
     static LIMITED_INT< T > cast( const double x )
     {
         return LIMITED_INT< T > ( floor( x + 0.5 ) );
     }
};

template< typename T, int P > class LENGTH
{
    friend class LENGTH_UNITS< T >;
    friend class LENGTH_TRAITS< T, P >;
    template < typename Y, int R > friend class LENGTH;
protected:

    T u;
    
    LENGTH( T units ) : u( units )
    {
    }
    
    static T RawValue( const LENGTH<T, P> &x )
    {
        return x.u;
    }
    
    static T RawValue( const T& x )
    {
        return x;
    }
    
public:
    typedef T value_type;
    
    enum
    {
        dimension = P
    };
    
    template< typename U > LENGTH( const LENGTH< U, P > &orig )
    : u( LENGTH_CASTS < T >::cast( orig.u ) )
    {
    }
    
    LENGTH( void ) : u()
    {
    }
    
    static LENGTH<T, P> zero ( void )
    {
        return T(0);
    }

    /* Do not use this, please */
    static LENGTH<T, P> quantum ( void )
    {
        return T(1);
    }

    LENGTH<T, P> & operator = ( const LENGTH<T, P> & y )
    {
        this->u = y.u;
        return *this;
    }
    
    template< typename Y > operator LENGTH< Y, P > ( void )
    {
        return LENGTH< Y, P >( this->u );
    }
    
    /*************************/
    /* comparisons and tests */
    /*************************/
    bool operator ==( const LENGTH < T, P > y ) const
    {
        return u == y.u;
    }
    
    bool operator !=( const LENGTH < T, P > y ) const
    {
        return u != y.u;
    }
    
    bool operator <( const LENGTH < T, P > y ) const
    {
        return u < y.u;
    }
    
    bool operator >=( const LENGTH < T, P > y ) const
    {
        return u >= y.u;
    }
    
    bool operator >( const LENGTH < T, P > y ) const
    {
        return u > y.u;
    }
    
    bool operator <=( const LENGTH < T, P > y ) const
    {
        return u <= y.u;
    }
    
    bool operator !( void ) const
    {
        return !u;
    }
    
    /*************************/
    /* basic arithmetic      */
    /*************************/
    LENGTH< T, P > operator - ( void ) const
    {
        LENGTH< T, P > z;
        z.u = -u;
        return z;
    }
    
    LENGTH< T, P >& operator -= ( const LENGTH< T, P > y )
    {
        u -= y.u;
        return *this;
    }

    friend LENGTH< T, P > operator - ( const LENGTH< T, P > x, const LENGTH< T, P > y )
    {
        LENGTH< T, P > z = x;
        z -= y;
        return z;
    }
    
    LENGTH< T, P >& operator += ( const LENGTH< T, P > y )
    {
        u += y.u;
        return *this;
    }

    friend LENGTH< T, P > operator + ( const LENGTH< T, P > x, const LENGTH< T, P > y )
    {
        LENGTH< T, P > z = x;
	z += y;
        return z;
    }
    
    LENGTH< T, P >& operator *= ( const T y )
    {
        u *= y;
        return *this;
    }

    LENGTH< T, P > operator * ( const T & y) const
    {
        LENGTH< T, P > z = *this;
        z *= y;
        return z;
    }

    template < int R >
    typename LENGTH_TRAITS< T, P + R >::flat operator * ( const LENGTH<T, R> &y ) const
    {
        LENGTH< T, P > z;
        z.u = u * y.u;
        return z;
    }
    
    LENGTH< T, P > friend operator * ( const T &y, const LENGTH<T, P> &x )
    {
        return x.u * y;
    }
    
    LENGTH< T, P >& operator /= ( const T y )
    {
        u /= y;
        return *this;
    }
    
    LENGTH< T, P > operator / ( const T &y ) const
    {
        return u / y;
    }

    template < int R >
    typename LENGTH_TRAITS< T, P - R >::flat operator / ( const LENGTH<T, R> &y ) const
    {
        return u / y.u;
    }
    
    LENGTH< T, -P > friend operator / ( const T &y, const LENGTH< T, P > &x )
    {
        return y / x.u;
    }

    /******************************/
    /* algebraic functions        */
    /******************************/
    friend LENGTH< T, P > abs( LENGTH< T, P > y )
    {
        return 0 < y.u? y : -y;
    }
    
    friend LENGTH< T, P > max( LENGTH< T, P > x, LENGTH< T, P > y )
    {
        LENGTH< T, P > z;
        z.u = hypot( x.u, y.u );
        return z;
    }

    friend LENGTH< T, P > sqrt( LENGTH< T, P*2 > y )
    {
        LENGTH< T, P > z;
        z.u = sqrt( y.u );
        return z;
    }
    
    friend LENGTH< T, P > cbrt( LENGTH< T, P*3 > y )
    {
        LENGTH< T, P > z;
        z.u = cbrt( y.u );
        return z;
    }
    
    friend LENGTH< T, P > hypot( LENGTH< T, P > x, LENGTH< T, P > y )
    {
        LENGTH< T, P > z;
        z.u = hypot( x.u, y.u );
        return z;
    }
    
    friend double atan2( LENGTH< T, P > x, LENGTH< T, P > y )
    {
        return atan2( double ( x.u ), double( y.u ) );
    }

};

/*!
 * Units of length
 *
 * How to use them:
 * there are several functions, named LENGTH_UNITS< T >::METRE, which return
 * named unit (1 meter in example) which have type LENGTH< T, P >.
 * to get specific length you should use a multiplication:
 * 3*LENGTH_UNITS::metre() gives 3 metres
 * 0.01*LENGTH_UNITS::metre() gives 0.01 inch
 * to get numeric value of length in specific units you should use a division
 * length/LENGTH_UNITS::metre() gives number of metres in length
 * legnth/LENGTH_UNITS::foot() gives number of feet in length
 *
 * Really these units are used in NEWPCB and printing routines, as EESCHEMA
 * is going to use relative units.
 */

template < typename T > class LENGTH_UNITS {
protected:
    enum
    {
        METRE = 1000000000, /*!< The ONLY constant connecting length to the real world */
        
        INCH = METRE / 10000 * 254
    };
    
public:
    /*! One metre. */
    static LENGTH< T, 1 > metre( void ) 
    {
        return T( METRE );
    }
    
    /*! One decimetre. */
    static LENGTH< T, 1 > decimetre( void ) 
    {
        return T( METRE / 10 );
    }
    
    static LENGTH< T, 1 > centimetre( void ) 
    {
        return T( METRE / 100 );
    }
    
    static LENGTH< T, 1 > millimetre( void ) 
    {
        return T( METRE / 1000 );
    }
    
    static LENGTH< T, 1 > micrometre( void ) 
    {
        return T( METRE / 1000000 );
    }
    
    static LENGTH< T, 1 > foot( void ) 
    {
        return T( INCH * 12 );
    }
    
    static LENGTH< T, 1 > inch( void ) 
    {
        return T( INCH );
    }
    
    static LENGTH< T, 1 > mil( void ) 
    {
        return T( INCH / 1000 );
    }
};

/**
  * Shortcut to get units of given length type
  */
template < typename T, int D > class LENGTH_UNITS< LENGTH< T, D > >: public LENGTH_UNITS< T >
{
};

#endif /* def LENGTH_H_INCLUDED */
