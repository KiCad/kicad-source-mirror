/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/utf8.h>
#include <wx/strconv.h>
#include <wx/buffer.h>
#include <vector>

#include <cassert>


/*
    These are not inlined so that code space is saved by encapsulating the
    creation of intermediate objects and the referencing of wxConvUTF8.
*/


UTF8::UTF8( const wxString& o ) :
    m_s( (const char*) o.utf8_str() )
{
}


wxString UTF8::wx_str() const
{
    return wxString( c_str(), wxConvUTF8 );
}


UTF8::operator wxString () const
{
    return wxString( c_str(), wxConvUTF8 );
}


UTF8& UTF8::operator=( const wxString& o )
{
    m_s = (const char*) o.utf8_str();
    return *this;
}


// There is no wxWidgets function that does this, because wchar_t is 16 bits
// on windows and wx wants to encode the output in UTF16 for such.

int UTF8::uni_forward( const unsigned char* aSequence, unsigned* aResult )
{
    unsigned ch = *aSequence;

    if( ch < 0x80 )
    {
        if( aResult )
            *aResult = ch;
        return 1;
    }

    const unsigned char* s = aSequence;

    static const unsigned char utf8_len[] = {
        // Map encoded prefix byte to sequence length.  Zero means
        // illegal prefix.  See RFC 3629 for details
        /*
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 00-0F
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 70-7F
        */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 80-8F
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B0-BF
        0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0-C1 + C2-CF
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D0-DF
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E0-EF
        4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // F0-F4 + F5-FF
    };

    int len = utf8_len[ *s - 0x80  /* top half of table is missing */ ];

    switch( len )
    {
    default:
    case 0:
        if( aResult )
            wxFAIL_MSG( wxS( "uni_forward: invalid start byte" ) );

        return 0;
        break;

    case 2:
        if( ( s[1] & 0xc0 ) != 0x80 )
        {
            if( aResult )
                wxFAIL_MSG( wxS( "uni_forward: invalid continuation byte" ) );

            return 0;
        }

        ch =    ((s[0] & 0x1f) << 6) +
                ((s[1] & 0x3f) << 0);

        // assert( ch > 0x007F && ch <= 0x07FF );
        break;

    case 3:
        if( (s[1] & 0xc0) != 0x80 ||
            (s[2] & 0xc0) != 0x80 ||
            (s[0] == 0xE0 && s[1] < 0xA0)
            // || (s[0] == 0xED && s[1] > 0x9F)
        )
        {
            if( aResult )
                wxFAIL_MSG( wxS( "uni_forward: invalid continuation byte" ) );

            return 0;
        }

        ch =    ((s[0] & 0x0f) << 12) +
                ((s[1] & 0x3f) << 6 ) +
                ((s[2] & 0x3f) << 0 );

        // assert( ch > 0x07FF && ch <= 0xFFFF );
        break;

    case 4:
        if( (s[1] & 0xc0) != 0x80 ||
            (s[2] & 0xc0) != 0x80 ||
            (s[3] & 0xc0) != 0x80 ||
            (s[0] == 0xF0 && s[1] < 0x90) ||
            (s[0] == 0xF4 && s[1] > 0x8F) )
        {
            if( aResult )
                wxFAIL_MSG( wxS( "uni_forward: invalid continuation byte" ) );

            return 0;
        }

        ch =    ((s[0] & 0x7)  << 18) +
                ((s[1] & 0x3f) << 12) +
                ((s[2] & 0x3f) << 6 ) +
                ((s[3] & 0x3f) << 0 );

        // assert( ch > 0xFFFF && ch <= 0x10ffff );
        break;
    }

    if( aResult )
        *aResult = ch;

    return len;
}


bool IsUTF8( const char* aString )
{
    int len = strlen( aString );

    if( len )
    {
        const unsigned char* next = (unsigned char*) aString;
        const unsigned char* end  = next + len;

        while( next < end )
        {
            int charLen = UTF8::uni_forward( next, nullptr );

            if( charLen == 0 )
                return false;

            next += charLen;
        }

        // uni_forward() should find the exact end if it is truly UTF8
        if( next > end )
            return false;
    }

    return true;
}


UTF8::UTF8( const wchar_t* txt )
{
    try
    {
        std::vector< char > temp( wcslen( txt ) * 4 + 1 );
        wxConvUTF8.WC2MB( temp.data(), txt, temp.size() );
        m_s.assign( temp.data() );
    }
    catch(...)
    {
        auto string = wxSafeConvertWX2MB( txt );
        m_s.assign( string );
    }

    m_s.shrink_to_fit();
}


UTF8& UTF8::operator+=( unsigned w_ch )
{
    if( w_ch <= 0x7F )
    {
        m_s.operator+=( char( w_ch ) );
    }
    else
    {
        wchar_t wide_chr[2];    // buffer to store wide chars (UTF16) read from aText
        wide_chr[1] = 0;
        wide_chr[0] = w_ch;
        UTF8 substr( wide_chr );
        m_s += substr.m_s;
    }

    return *this;
}


std::ostream& operator<<( std::ostream& aStream, const UTF8& aRhs )
{
    aStream << static_cast<const std::string&>( aRhs );
    return aStream;
}
