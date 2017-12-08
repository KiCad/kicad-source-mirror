/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <utf8.h>
#include <ki_exception.h>

/* THROW_IO_ERROR needs this, but it includes this file, so until some
    factoring of THROW_IO_ERROR into a separate header, defer and use the asserts.
#include <richio.h>
*/

#include <assert.h>


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
        THROW_IO_ERROR( "invalid start byte" );
        break;

    case 2:
        if( ( s[1] & 0xc0 ) != 0x80 )
        {
            THROW_IO_ERROR( "invalid continuation byte" );
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
            THROW_IO_ERROR( "invalid continuation byte" );
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
            THROW_IO_ERROR( "invalid continuation byte" );
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

        try
        {
            while( next < end )
            {
                next += UTF8::uni_forward( next, NULL );
            }

            // uni_forward() should find the exact end if it is truly UTF8
            if( next > end )
                return false;
        }
        catch( const IO_ERROR& ioe )
        {
            return false;
        }
    }

    return true;
}


UTF8::UTF8( const wchar_t* txt ) :
    // size initial string safely large enough, then shrink to known size later.
    m_s( wcslen( txt ) * 4, 0 )
{
    /*

        "this" string was sized to hold the worst case UTF8 encoded byte
        sequence, and was initialized with all nul bytes. Overwrite some of
        those nuls, then resize, shrinking down to actual size.

        Use the wx 2.8 function, not new FromWChar(). It knows about wchar_t
        possibly being 16 bits wide on Windows and holding UTF16 input.

    */

    int sz = wxConvUTF8.WC2MB( (char*) m_s.data(), txt, m_s.size() );

    m_s.resize( sz );
}


UTF8& UTF8::operator+=( wchar_t ch )
{
    if( ch <= 0x7F )
        m_s.operator+=( char( ch ) );
    else
    {
        wchar_t wide_chr[2];    // buffer to store wide chars (unicode) read from aText
        wide_chr[1] = 0;
        wide_chr[0] = ch;
        UTF8 substr( wide_chr );
        m_s += substr.m_s;
    }

    return (UTF8&) *this;
}


#if 0   // some unit tests:

#include <stdio.h>

wxString wxFunctionTaking_wxString( const wxString& wx )
{
    printf( "%s:'%s'\n", __func__, (char*) UTF8( wx ) );
    printf( "%s:'%s'\n", __func__, (const char*) UTF8( wx ) );
    printf( "%s:'%s'\n", __func__, UTF8( wx ).c_str() );

    return wx;
}

int main()
{
    std::string str = "input";

    UTF8        u0 = L"wide string";
    UTF8        u1 = "initial";
    wxString    wx = wxT( "input2" );

    printf( "u0:'%s'\n", u0.c_str() );
    printf( "u1:'%s'\n", u1.c_str() );

    u1 = str;

    wxString    wx2 = u1;

    // force a std::string into a UTF8, then into a wxString, then copy construct:
    wxString    wx3 = (UTF8&) u1;

    UTF8        u2 = wx2;

    u2 += 'X';

    printf( "u2:'%s'\n", u2.c_str() );

    // key accomplishments here:
    // 1) passing a UTF8 to a function which normally takes a wxString.
    // 2) return a wxString back into a UTF8.
    UTF8    result = wxFunctionTaking_wxString( u2 );

    printf( "result:'%s'\n", result.c_str() );

    // test the unicode iterator:
    for( UTF8::uni_iter it = u2.ubegin();  it < u2.uend();  )
    {
        // test post-increment:
        printf( " _%02x_", *it++ );
    }

    printf( "\n" );

    UTF8::uni_iter it = u2.ubegin();

    UTF8::uni_iter it2 = it++;

    printf( "post_inc:'%c' should be 'i'\n", *it2 );

    it2 = ++it;

    printf( "pre_inc:'%c' should be 'p'\n", *it2 );

    printf( "u[1]:'%c' should be 'n'\n", u2[1] );

    return 0;
}

#endif
