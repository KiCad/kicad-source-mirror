/**
 * The physical length library. Made for nanometer scale.
 * @file length.h
 */

/* sorry it is not styled correctly, i'll work on it further */

#ifndef LENGTH_H_INCLUDED
#define LENGTH_H_INCLUDED 1

/* type to be used by length units by default */
typedef int DEF_LENGTH_VALUE;

/**
 * Length template class
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

template < typename T = DEF_LENGTH_VALUE, int P = 1 > class LENGTH;

/**
 * Length units contained in this class
 */

template <typename T> class LENGTH_UNITS;

/**
 * For internal needs
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

template< typename T, int P > class LENGTH
{
    friend class LENGTH_UNITS< T >;
    friend class LENGTH_TRAITS< T, P >;
    template < typename Y, int R > friend class LENGTH;
protected:

    T m_U;
    LENGTH( T units ) : m_U( units )
    {
    }
    static T RawValue( const LENGTH<T, P> &x )
    {
        return x.m_U;
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
    LENGTH( const LENGTH <T, P> &orig ) : m_U( orig.m_U )
    {
    }
    LENGTH( void ) : m_U()
    {
    }
    
    static LENGTH<T, P> zero ( void )
    {
        return T(0);
    }

    LENGTH<T, P> & operator = ( const LENGTH<T, P> & y )
    {
        this->m_U = y.m_U;
        return *this;
    }
    template<typename Y> operator LENGTH< Y, P > ( void )
    {
        return this->m_U;
    }
    /*************************/
    /* comparisons and tests */
    /*************************/
    bool operator ==( const LENGTH < T, P > y ) const
    {
        return m_U == y.m_U;
    }
    bool operator !=( const LENGTH < T, P > y ) const
    {
        return m_U != y.m_U;
    }
    bool operator <( const LENGTH < T, P > y ) const
    {
        return m_U < y.m_U;
    }
    bool operator >=( const LENGTH < T, P > y ) const
    {
        return m_U >= y.m_U;
    }
    bool operator >( const LENGTH < T, P > y ) const
    {
        return m_U > y.m_U;
    }
    bool operator <=( const LENGTH < T, P > y ) const
    {
        return m_U <= y.m_U;
    }
    bool operator !( void ) const
    {
        return !m_U;
    }
    /*************************/
    /* basic arithmetic      */
    /*************************/
    LENGTH< T, P > operator - ( void ) const
    {
        return LENGTH<T, P>(-this->m_U);
    }
    LENGTH< T, P > operator - ( const LENGTH< T, P > y ) const
    {
        return m_U - y.m_U;
    }
    LENGTH< T, P > operator + ( const LENGTH< T, P > y ) const
    {
        return m_U + y.m_U;
    }
    template < int R >
    typename LENGTH_TRAITS< T, P + R >::flat operator * ( const LENGTH<T, R> &y ) const
    {
        return m_U * y.m_U;
    }
    LENGTH< T, P > operator * ( const T & y) const
    {
        return m_U * y;
    }
    LENGTH< T, P > friend operator * ( const T &y, const LENGTH<T, P> &x )
    {
        return x.m_U * y;
    }
    
    template < int R >
    typename LENGTH_TRAITS< T, P - R >::flat operator / ( const LENGTH<T, R> &y ) const
    {
        return m_U / y.m_U;
    }
    LENGTH< T, P > operator / ( const T &y ) const
    {
        return m_U / y;
    }
    LENGTH< T, -P > friend operator / ( const T &y, const LENGTH< T, P > &x )
    {
        return y / x.m_U;
    }

    friend LENGTH< T, P > sqrt( LENGTH< T, P*2 > y )
    {
        return sqrt( y.m_U );
    }
    friend LENGTH< T, P > cbrt( LENGTH< T, P*3 > y )
    {
        return cbrt( y.m_U );
    }
    /*************************/
    /* assignment arithmetic */
    /*************************/
    LENGTH< T, P >& operator -= ( const LENGTH< T, P > y )
    {
        return m_U -= y.m_U;
    }
    LENGTH< T, P >& operator += ( const LENGTH< T, P > y )
    {
        return m_U += y.m_U;
    }
    LENGTH< T, P >& operator *= ( const T y )
    {
        return m_U *= y;
    }
    LENGTH< T, P >& operator /= ( const T y )
    {
        return m_U /= y;
    }
    /*************************/
    /* more arithmetic       */
    /*************************/
};

/**
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
 */

template < typename T = DEF_LENGTH_VALUE > class LENGTH_UNITS {
protected:
    enum
    {
        METRE = 1000000000, /* The ONLY constant connecting length to the real world */
        
        INCH = METRE / 10000 * 254
    };
    public:
    static LENGTH< T, 1 > metre( void ) {
        return T( METRE );
    }
    static LENGTH< T, 1 > decimetre( void ) {
        return T( METRE / 10 );
    }
    static LENGTH< T, 1 > centimetre( void ) {
        return T( METRE / 100 );
    }
    static LENGTH< T, 1 > millimetre( void ) {
        return T( METRE / 1000 );
    }
    static LENGTH< T, 1 > micrometre( void ) {
        return T( METRE / 1000000 );
    }
    static LENGTH< T, 1 > foot( void ) { /* do not think this will ever need */
        return T( INCH * 12 );
    }
    static LENGTH< T, 1 > inch( void ) {
        return T( INCH );
    }
    static LENGTH< T, 1 > mil( void ) {
        return T( INCH / 1000 );
    }
};

/**
  * shortcut to get units of given length type
  */
template < typename T, int D > class LENGTH_UNITS< LENGTH< T, D > >: public LENGTH_UNITS< T >
{
};

#endif
