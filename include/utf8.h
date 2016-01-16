/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <string>
#include <wx/string.h>

/**
 * Class UTF8
 * is an 8 bit std::string that is assuredly encoded in UTF8, and supplies special
 * conversion support to and from wxString, and has iteration over unicode characters.
 *
 * <p>I've been careful to supply only conversion facilities and not try
 * and duplicate wxString() with many member functions.  In the end it is
 * to be a std::string.  There are multiple ways to create text into a std::string
 * without the need of too many member functions:
 *
 * <ul>
 *  <li>richio.h's StrPrintf()</li>
 *  <li>std::ostringstream.</li>
 * </ul>
 *
 * <p>Because this class used no virtuals, it should be possible to cast any
 * std::string into a UTF8 using this kind of cast: (UTF8 &) without construction
 * or copying being the effect of the cast.  Be sure the source std::string holds
 * UTF8 encoded text before you do that.
 *
 * @author Dick Hollenbeck
 */
class UTF8 : public std::string
{
public:

    UTF8( const wxString& o );

    /// This is a constructor for which you could end up with
    /// non-UTF8 encoding, but that would be your fault.
    UTF8( const char* txt ) :
        std::string( txt )
    {
    }

    /// For use with _() function on wx 2.8.
    /// BTW _() on wx >= 2.9 returns wxString, not wchar_t* like on 2.8.
    UTF8( const wchar_t* txt );

    UTF8( const std::string& o ) :
        std::string( o )
    {
    }

    UTF8() :
        std::string()
    {
    }

    ~UTF8()     // Needed mainly to build python wrapper
    {
    }

    UTF8& operator=( const wxString& o );

    UTF8& operator=( const std::string& o )
    {
        std::string::operator=( o );
        return *this;
    }

    UTF8& operator=( const char* s )
    {
        std::string::operator=( s );
        return *this;
    }

    UTF8& operator=( char c )
    {
        std::string::operator=( c );
        return *this;
    }

    UTF8 substr( size_t pos = 0, size_t len = npos ) const
    {
        return std::string::substr( pos, len );
    }

    operator wxString () const;

    /// This one is not in std::string, and one wonders why... might be a solid
    /// enough reason to remove it still.
    operator char* () const
    {
        return (char*) c_str();
    }

    /**
     * Function uni_forward
     * advances over a single UTF8 encoded multibyte character, capturing the
     * unicode character as it goes, and returning the number of bytes consumed.
     *
     * @param aSequence is the UTF8 byte sequence, must be aligned on start of character.
     * @param aResult is where to put the unicode character, and may be NULL if no interest.
     * @return int - the count of bytes consumed.
     */
    static int uni_forward( const unsigned char* aSequence, unsigned* aResult = NULL );

    /**
     * class uni_iter
     * is a non-muting iterator that walks through unicode code points in the UTF8 encoded
     * string.  The normal ++(), ++(int), ->(), and *() operators are all supported
     * for read only access and some return an unsigned holding the unicode character
     * appropriate for the respective operator.
     */
    class uni_iter
    {
        friend class UTF8;

        const unsigned char* it;

        // private constructor.
        uni_iter( const char* start ) :
            it( (const unsigned char*) start )
        {
            // for the human: assert( sizeof(unsigned) >= 4 );
        }


    public:

        uni_iter()  // Needed only to build python wrapper, not used outside the wrapper
        {
            it = NULL;
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

        /*
        /// return unicode at current position
        unsigned operator->() const
        {
            unsigned    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }
        */

        /// return unicode at current position
        unsigned operator*() const
        {
            unsigned    result;

            // grab the result, do not advance
            uni_forward( it, &result );
            return result;
        }

        bool operator==( const uni_iter& other ) const  { return it == other.it; }
        bool operator!=( const uni_iter& other ) const  { return it != other.it; }

        /// Since the ++ operators advance more than one byte, this is your best
        /// loop termination test, < end(), not == end().
        bool operator< ( const uni_iter& other ) const  { return it <  other.it; }
        bool operator<=( const uni_iter& other ) const  { return it <= other.it; }
        bool operator> ( const uni_iter& other ) const  { return it >  other.it; }
        bool operator>=( const uni_iter& other ) const  { return it >= other.it; }
    };

    /**
     * Function ubegin
     * returns a @a uni_iter initialized to the start of "this" UTF8 byte sequence.
     */
    uni_iter ubegin() const
    {
        return uni_iter( data() );
    }

    /**
     * Function uend
     * returns a @a uni_iter initialized to the end of "this" UTF8 byte sequence.
     */
    uni_iter uend() const
    {
        return uni_iter( data() + size() );
    }
};

#endif // UTF8_H_
