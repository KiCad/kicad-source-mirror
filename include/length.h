/**
 * The physical length library. Made for nanometer scale.
 * @file length.h
 */

/* sorry it is not styles correctly, i'll work on it further */

#ifndef LENGTH_H_INCLUDED
#define LENGTH_H_INCLUDED 1

typedef int DEF_LENGTH_VALUE;

template <typename T = DEF_LENGTH_VALUE, int P = 1> class LENGTH;

template <typename T> class LENGTH_UNITS;

template <typename T, int P> struct LENGTH_TRAITS {
	typedef LENGTH<T, P> flat;
};

template <typename T> struct LENGTH_TRAITS<T, 0> {
	typedef T flat;
};

template<typename T, int P> class LENGTH {
	friend class LENGTH_UNITS<T>;
	friend class LENGTH_TRAITS<T, P>;
	template <typename Y, int R> friend class LENGTH;
protected:

	T m_U;
	LENGTH(T units) : m_U(units) {
	}
	static T RawValue(const LENGTH<T, P> &x) {
		return x.m_U;
	}
	static T RawValue(const T& x) {
		return x;
	}
public:
	typedef LENGTH<T, P> flat;
	typedef T value_type;
	enum {
		dimension = P
	};
	LENGTH(const LENGTH <T, P> &orig) : m_U(orig.m_U) {
	}
	LENGTH( void ) : m_U() {
	}
	
	static LENGTH<T, P> zero (void) {
		return T(0);
	}

	LENGTH<T, P> & operator = (const LENGTH<T, P> & y) {
		this->m_U = y.m_U;
		return *this;
	}
	template<typename Y>
	operator LENGTH<Y, P> (void) {
		return this->m_U;
	}
	/*************************/
	/* comparisons and tests */
	/*************************/
	bool operator ==(const LENGTH <T, P> y) const {
		return m_U == y.m_U;
	}
	bool operator !=(const LENGTH <T, P> y) const {
		return m_U != y.m_U;
	}
	bool operator <(const LENGTH <T, P> y) const {
		return m_U < y.m_U;
	}
	bool operator >=(const LENGTH <T, P> y) const {
		return m_U >= y.m_U;
	}
	bool operator >(const LENGTH <T, P> y) const {
		return m_U > y.m_U;
	}
	bool operator <=(const LENGTH <T, P> y) const {
		return m_U <= y.m_U;
	}
	bool operator !( void ) const {
		return !m_U;
	}
	/*************************/
	/* basic arithmetic      */
	/*************************/
	LENGTH<T, P> operator - (void) const {
		return LENGTH<T, P>(-this->m_U);
	}
	LENGTH<T, P> operator - (const LENGTH<T, P> y) const {
		return m_U - y.m_U;
	}
	LENGTH<T, P> operator + (const LENGTH<T, P> y) const {
		return m_U + y.m_U;
	}
	template <int R>
	typename LENGTH_TRAITS<T, P + R>::flat operator * (const LENGTH<T, R> &y) const {
		return m_U * y.m_U;
	}
	LENGTH<T, P> operator * (const T &y) const {
		return m_U * y;
	}
	LENGTH<T, P> friend operator * (const T &y, const LENGTH<T, P> &x) {
		return x.m_U * y;
	}
	
	template <int R>
	typename LENGTH_TRAITS<T, P - R>::flat operator / (const LENGTH<T, R> &y) const {
		return m_U / y.m_U;
	}
	LENGTH<T, P> operator / (const T &y) const {
		return m_U / y;
	}
	LENGTH<T, -P> friend operator / (const T &y, const LENGTH<T, P> &x) {
		return y / x.m_U;
	}

	friend LENGTH<T, P> sqrt(LENGTH<T, P*2> y) {
		return sqrt(y.m_U);
	}
	friend LENGTH<T, P> cbrt(LENGTH<T, P*3> y) {
		return cbrt(y.m_U);
	}
	/*************************/
	/* assignment arithmetic */
	/*************************/
	LENGTH<T, P>& operator -= (const LENGTH<T, P> y) {
		return m_U -= y.m_U;
	}
	LENGTH<T, P>& operator += (const LENGTH<T, P> y) {
		return m_U += y.m_U;
	}
	LENGTH<T, P>& operator *= (const T y) {
		return m_U *= y;
	}
	LENGTH<T, P>& operator /= (const T y) {
		return m_U /= y;
	}
	/*************************/
	/* more arithmetic       */
	/*************************/
};

template <typename T = DEF_LENGTH_VALUE> class LENGTH_UNITS {
protected:
    enum
    {
        METRE = 1000000000, /* The ONLY constant connecting length to the real world */
        
        INCH = METRE / 10000 * 254
    };
	public:
	static LENGTH<T, 1> metre( void ) {
		return T(METRE);
	}
	static LENGTH<T, 1> decimetre( void ) {
		return T(METRE / 10);
	}
	static LENGTH<T, 1> centimetre( void ) {
		return T(METRE / 100);
	}
	static LENGTH<T, 1> millimetre( void ) {
		return T(METRE / 1000);
	}
	static LENGTH<T, 1> micrometre( void ) {
		return T(METRE / 1000000);
	}
	static LENGTH<T, 1> foot( void ) { /* do not think this will ever need */
		return T(INCH * 12);
	}
	static LENGTH<T, 1> inch( void ) {
		return T(INCH);
	}
	static LENGTH<T, 1> mil( void ) {
		return T(INCH / 1000);
	}
};

/* shortcut */
template <typename T, int D> class LENGTH_UNITS<LENGTH<T, D> >: public LENGTH_UNITS<T> {
};

/* TODO: argument promotion (but is this need? explicit casts would be enough) */

#endif
