/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Dick Hollenbeck
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

#ifndef UTF8_H_
#define UTF8_H_

#include <iostream>
#include <string>
#include <wx/string.h>

#if defined(DEBUG)
 #define UTF8_VERIFY    // Might someday be a hidden cmake config option
#endif


/**
 * Test a C string to see if it is UTF8 encoded.
 *
 * An ASCII string is a valid UTF8 string.
 */
bool IsUTF8( const char* aString );


#if defined(UTF8_VERIFY)
 #define MAYBE_VERIFY_UTF8(x)       wxASSERT( IsUTF8(x) )
#else
 #define MAYBE_VERIFY_UTF8(x)       // nothing
#endif


/**
 * An 8 bit string that is assuredly encoded in UTF8, and supplies special conversion
 * support to and from wxString, to and from std::string, and has non-mutating iteration
 * over Unicode characters.
 *
 * I've been careful to supply only conversion facilities and not try and duplicate
 * wxString() with many member functions. There are multiple ways to create text into
 * a std::string without the need of too many member functions:
 *
 *  - std::ostringstream.
 *
 * Because this class uses no virtuals, it should be possible to cast any std::string
 * into a UTF8 using this kind of cast: (UTF8 &) without construction or copying being
 * the effect of the cast.  Be sure the source std::string holds UTF8 encoded text before
 * you do that.
 */
class UTF8
{
public:
    UTF8( const wxString& o );

    /// This is a constructor for which you could end up with
    /// non-UTF8 encoding, but that would be your fault.
    UTF8( const char* txt ) :
        m_s( txt )
    {
        MAYBE_VERIFY_UTF8( c_str() );
    }

    /// For use with _() function on wx 2.8.
    /// BTW _() on wx >= 2.9 returns wxString, not wchar_t* like on 2.8.
    UTF8( const wchar_t* txt );

    UTF8( const std::string& o ) :
        m_s( o )
    {
        MAYBE_VERIFY_UTF8( c_str() );
    }

    UTF8( const UTF8& o ) :
        m_s( o.m_s )
    {
        MAYBE_VERIFY_UTF8( c_str() );
    }

    UTF8()
    {
    }

    ~UTF8()     // Needed mainly to build python wrapper
    {
    }

    // expose some std::string functions publicly, since base class must be private.
    const char* c_str()                         const   { return m_s.c_str(); }
    bool empty()                                const   { return m_s.empty(); }

    std::string::size_type find( char c )       const   { return m_s.find( c ); }
    std::string::size_type find( char c, size_t s )     const   { return m_s.find( c, s ); }

    void clear()                                        { m_s.clear(); }
    std::string::size_type length()             const   { return m_s.length(); }
    std::string::size_type size()               const   { return m_s.size(); }
    int compare( const std::string& s )         const   { return m_s.compare( s ); }

    bool operator==( const UTF8& rhs )          const   { return m_s == rhs.m_s; }
    bool operator==( const std::string& rhs )   const   { return m_s == rhs; }
    bool operator==( const char* s )            const   { return m_s == s; }

    bool operator!=( const UTF8& rhs ) const { return !( operator==( rhs ) ); }
    bool operator<( const UTF8& rhs ) const { return m_s < rhs.m_s;  }
    bool operator>( const UTF8& rhs ) const { return m_s > rhs.m_s;  }

    std::string::size_type find_first_of( const std::string& str,
                                          std::string::size_type pos = 0 ) const
    {
        return m_s.find_first_of( str, pos );
    }

    UTF8& operator+=( const UTF8& str )
    {
        m_s += str.m_s;
        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    UTF8& operator+=( char ch )
    {
        m_s.operator+=( ch );
        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    UTF8& operator+=( const char* s )
    {
        m_s.operator+=( s );
        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    /// Append a wide (unicode) char to the UTF8 string.
    /// if this wide char is not a ASCII7 char, it will be added as a UTF8 multibyte sequence
    /// @param w_ch is a UTF-16 value (can be a UTF-32 on Linux)
    UTF8& operator+=( unsigned w_ch );

    // std::string::npos is not constexpr, so we can't use it in an
    // initializer.
    static constexpr std::string::size_type npos = -1;

    UTF8& operator=( const wxString& o );

    UTF8& operator=( const std::string& o )
    {
        m_s = o;
        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    UTF8& operator=( const char* s )
    {
        m_s = s;
        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    UTF8& operator=( char c )
    {
        m_s = c;
        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    UTF8& operator=( const UTF8& aOther )
    {
        m_s = aOther.m_s;
        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    // Move assignment operator
    UTF8& operator=( UTF8&& aOther ) noexcept
    {
        if (this != &aOther)
            m_s = std::move( aOther.m_s );

        MAYBE_VERIFY_UTF8( c_str() );
        return *this;
    }

    // a substring of a UTF8 is not necessarily a UTF8 if a multibyte character
    // was split, so return std::string not UTF8
    std::string substr( size_t pos = 0, size_t len = npos ) const
    {
        return m_s.substr( pos, len );
    }

    operator const std::string& () const    { return m_s; }
    //operator std::string& ()                { return m_s; }
    //operator std::string () const           { return m_s; }

    wxString wx_str() const;
    operator wxString () const;

    // "Read only" iterating over bytes is done with these, use the uni_iter to iterate
    // over UTF8 (multi-byte) characters
    std::string::const_iterator begin()         const   { return m_s.begin(); }
    std::string::const_iterator end()           const   { return m_s.end(); }

#ifndef SWIG
    /**
     * uni_iter
     * is a non-mutating iterator that walks through unicode code points in the UTF8 encoded
     * string.  The normal ++(), ++(int), ->(), and *() operators are all supported
     * for read only access and some return an unsigned holding the unicode character
     * appropriate for the respective operator.
     */
    class uni_iter
    {
    public:
        uni_iter()  // Needed only to build python wrapper, not used outside the wrapper
        {
            it = nullptr;
        }

        uni_iter( const uni_iter& o )
        {
            it = o.it;
        }

        /// pre-increment and return uni_iter at new position
        const uni_iter& operator++()
        {
            it += uni_forward( it );
            return *this;
        }

        /// post-increment and return uni_iter at initial position
        uni_iter operator++( int )
        {
            uni_iter ret = *this;

            it += uni_forward( it );
            return ret;
        }

        /// return unicode at current position
        unsigned operator->() const
        {
            unsigned    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }

        /// return unicode at current position
        unsigned operator*() const
        {
            unsigned    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }

        uni_iter operator-( int aVal ) const { return uni_iter( (char*) it - aVal ); }

        bool operator==( const uni_iter& other ) const  { return it == other.it; }
        bool operator!=( const uni_iter& other ) const  { return it != other.it; }

        /// Since the ++ operators advance more than one byte, this is your best
        /// loop termination test, < end(), not == end().
        bool operator< ( const uni_iter& other ) const  { return it <  other.it; }
        bool operator<=( const uni_iter& other ) const  { return it <= other.it; }
        bool operator> ( const uni_iter& other ) const  { return it >  other.it; }
        bool operator>=( const uni_iter& other ) const  { return it >= other.it; }

    private:
        friend class UTF8;

        const unsigned char* it;

        // private constructor
        uni_iter( const char* start ) :
            it( (const unsigned char*) start )
        {
        }
    };

    /**
     * Returns a @a uni_iter initialized to the start of "this" UTF8 byte sequence.
     */
    uni_iter ubegin() const
    {
        return uni_iter( m_s.data() );
    }

    /**
     * Return a @a uni_iter initialized to the end of "this" UTF8 byte sequence.
     */
    uni_iter uend() const
    {
        return uni_iter( m_s.data() + m_s.size() );
    }

    /**
     * Advance over a single UTF8 encoded multibyte character, capturing the Unicode character
     * as it goes, and returning the number of bytes consumed.
     *
     * @param aSequence is the UTF8 byte sequence, must be aligned on start of character.
     * @param aResult is where to put the unicode character, and may be NULL if no interest.
     * @return the count of bytes consumed.
     */
    static int uni_forward( const unsigned char* aSequence, unsigned* aResult = nullptr );
#endif  // SWIG

protected:
    std::string m_s;
};


std::ostream& operator<<( std::ostream& aStream, const UTF8& aRhs );

#endif // UTF8_H_
