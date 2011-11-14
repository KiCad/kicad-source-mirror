/**
 * @file limited_int.h
 * @brief Integer class catching overflows.
 */

/* sorry if it is not styled correctly, i'll work on it further */

#ifndef LIMITED_INT_H_INCLUDED
#define LIMITED_INT_H_INCLUDED 1

#include <limits>
#include <assert.h>
#include <math.h>

template < typename T = int > class LIMITED_INT {
private:
    T m_Value;
public:
    LIMITED_INT( void ) : m_Value() {
    }
    template<typename V> LIMITED_INT( const LIMITED_INT< V >& orig )
    :  m_Value( orig.m_Value )
    {
        assert(std::numeric_limits<T>::min() <= orig.m_Value);
        assert(orig.m_Value <= std::numeric_limits<T>::max());
    }
    template<typename V> LIMITED_INT( const double v )
    :  m_Value( floor(v+0.5) )
    {
        assert(std::numeric_limits<T>::min() <= v);
        assert(v <= std::numeric_limits<T>::max());
    }
    LIMITED_INT( T v ): m_Value( v )
    {
    }

    operator T( void ) const
    {
        return m_Value;
    }
    operator double( void ) const
    {
        return ( double )m_Value;
    }

    LIMITED_INT<T> & operator = ( LIMITED_INT< T > src )
    {
        m_Value = src.m_Value;
        return *this;
    }
    LIMITED_INT<T> & operator = ( T src )
    {
        m_Value = src;
        return *this;
    }
    /*************************/
    /* comparisons and tests */
    /*************************/
    bool operator ! (void) const {
	    return !m_Value;
    }
    
    bool operator == ( const LIMITED_INT< T > &y ) const
    {
        return m_Value == y.m_Value;
    }
    bool operator == ( const T y ) const
    {
        return m_Value == y;
    }
    friend bool operator == ( const T x, const LIMITED_INT< T > &y )
    {
        return x == y.m_Value;
    }

    bool operator != ( const LIMITED_INT<T> &y ) const
    {
        return m_Value != y.m_Value;
    }
    bool operator != ( const T y ) const
    {
        return m_Value != y;
    }
    friend bool operator != ( const T x, const LIMITED_INT< T > &y )
    {
        return x != y.m_Value;
    }

    bool operator < ( const LIMITED_INT< T > &y ) const
    {
        return m_Value < y.m_Value;
    }
    bool operator < ( const T y ) const
    {
        return m_Value < y;
    }
    friend bool operator < ( const T x, const LIMITED_INT< T > &y )
    {
        return x < y.m_Value;
    }

    bool operator >= ( const LIMITED_INT< T > &y ) const
    {
        return m_Value >= y.m_Value;
    }
    bool operator >= ( const T y ) const
    {
        return m_Value >= y;
    }
    friend bool operator >= ( const T x, const LIMITED_INT<T> &y )
    {
        return x >= y.m_Value;
    }

    bool operator > ( const LIMITED_INT< T > &y ) const
    {
        return m_Value > y.m_Value;
    }
    bool operator > ( const T y ) const
    {
        return m_Value > y;
    }
    friend bool operator > ( const T x, const LIMITED_INT< T > &y )
    {
        return x > y.m_Value;
    }

    bool operator <= ( const LIMITED_INT< T > &y ) const
    {
        return m_Value <= y.m_Value;
    }
    bool operator <= ( const T y ) const
    {
        return m_Value <= y;
    }
    friend bool operator <= ( const T x, const LIMITED_INT< T > &y )
    {
        return x <= y.m_Value;
    }

    /*************************/
    /* basic arithmetic      */
    /*************************/
    LIMITED_INT< T > operator + ( const LIMITED_INT< T > &y ) const
    {
        assert( !( 0 < m_Value ) || y.m_Value <= std::numeric_limits< T >::max() - m_Value );
        assert( !( m_Value < 0 ) || std::numeric_limits< T >::min() - m_Value <= y.m_Value );
        return m_Value + y.m_Value;
    }
    LIMITED_INT< T > operator + ( const T y ) const
    {
        return *this + LIMITED_INT< T >( y );
    }
    friend LIMITED_INT< T > operator + ( const T x, const LIMITED_INT< T > &y )
    {
        return LIMITED_INT< T >( x ) + y;
    }
    double operator + ( const double y ) const
    {
        return double( m_Value ) + y;
    }
    friend double operator + ( const double x, const LIMITED_INT< T > &y )
    {
        return x + double( y.m_Value );
    }

    LIMITED_INT< T > operator - ( void ) const
    {
        assert( -std::numeric_limits< T >::max() <= m_Value );
        return -m_Value;
    }
    LIMITED_INT< T > operator - ( const LIMITED_INT< T > &y ) const
    {
        assert( !( 0 < m_Value ) || m_Value - std::numeric_limits< T >::max() <= y.m_Value );
        assert( !( m_Value < 0 ) || y.m_Value <= m_Value - std::numeric_limits< T >::min() );
        return m_Value - y.m_Value;
    }
    LIMITED_INT< T > operator - ( const T y ) const
    {
        return *this - LIMITED_INT< T >( y );
    }
    friend LIMITED_INT< T > operator - ( const T x, const LIMITED_INT< T > &y )
    {
        return LIMITED_INT< T >( x ) - y;
    }
    double operator - ( const double y ) const
    {
        return double( m_Value ) - y;
    }
    friend double operator - ( const double x, const LIMITED_INT< T > &y )
    {
        return x - double( y.m_Value );
    }

    LIMITED_INT< T > operator * ( const LIMITED_INT< T > &y ) const
    {
        assert( !( 0 < m_Value && 0 < y.m_Value )
        || y.m_Value <= std::numeric_limits<T>::max() / m_Value );
        assert( !( 0 < m_Value && y.m_Value < 0 )
        || std::numeric_limits<T>::min() / m_Value <= y.m_Value );
        assert( !( m_Value < 0 && 0 < y.m_Value )
        || std::numeric_limits<T>::min() / y.m_Value <= m_Value );
        assert( !( m_Value < 0 && y.m_Value < 0 )
        || std::numeric_limits<T>::max() / m_Value <= y.m_Value );
        return m_Value * y.m_Value;
    }
    LIMITED_INT<T> operator * ( const T y) const
    {
        return *this * LIMITED_INT< T >( y );
    }
    friend LIMITED_INT< T > operator *( const T x, const LIMITED_INT< T > &y )
    {
        return LIMITED_INT< T >( x ) * y;
    }
    double operator * ( const double y ) const
    {
        return double( m_Value ) * y;
    }
    friend double operator * ( const double x, const LIMITED_INT< T > &y )
    {
        return x * double( y.m_Value );
    }

    LIMITED_INT<T> operator / ( const LIMITED_INT<T> &y ) const
    {
        assert( !( -1 == y.m_Value )
        || -std::numeric_limits< T >::max() <= m_Value );
        return m_Value / y.m_Value;
    }
    LIMITED_INT<T> operator / ( const T y ) const
    {
        return *this / LIMITED_INT<T>(y);
    }
    friend LIMITED_INT< T > operator / ( const T x, const LIMITED_INT< T > &y )
    {
        return LIMITED_INT< T >( x ) / y;
    }
    double operator / ( const double y ) const
    {
        return double( m_Value ) / y;
    }
    friend double operator / ( const double x, const LIMITED_INT< T > &y )
    {
        return x / double( y.m_Value );
    }
    
    LIMITED_INT<T> operator % ( const LIMITED_INT<T> &y ) const
    {
        return m_Value % y.m_Value;
    }
    LIMITED_INT<T> operator % ( const T y ) const
    {
        return *this % LIMITED_INT<T>(y);
    }
    friend LIMITED_INT< T > operator % ( const T x, const LIMITED_INT< T > &y )
    {
        return LIMITED_INT< T >( x ) % y;
    }
    /*************************/
    /* assignment arithmetic */
    /*************************/
    LIMITED_INT< T >& operator += ( const LIMITED_INT< T > &y )
    {
        *this = *this + y;
        return *this;
    }
    LIMITED_INT< T >& operator += ( const T y )
    {
        *this = *this + y;
        return *this;
    }
    LIMITED_INT< T >& operator ++ ( void )
    {
        *this = *this + 1;
        return *this;
    }
    LIMITED_INT< T >& operator -= ( const LIMITED_INT< T > &y )
    {
        *this = *this - y;
        return *this;
    }
    LIMITED_INT< T >& operator -= ( const T y )
    {
        *this = *this - y;
        return *this;
    }
    LIMITED_INT< T >& operator -- ( void )
    {
        *this = *this - 1;
        return *this;
    }
    LIMITED_INT< T >& operator *= ( const LIMITED_INT< T > &y )
    {
        *this = *this * y;
        return *this;
    }
    LIMITED_INT< T >& operator *= ( const T y )
    {
        *this = *this * y;
        return *this;
    }
    LIMITED_INT< T >& operator /= ( const LIMITED_INT< T > &y )
    {
        *this = *this / y;
        return *this;
    }
    LIMITED_INT< T >& operator /= ( const T y )
    {
        *this = *this / y;
        return *this;
    }
};

#endif /* def LIMITED_INT_H_INCLUDED*/
