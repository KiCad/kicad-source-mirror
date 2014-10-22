/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file macros.h
 * @brief This file contains miscellaneous helper definitions and functions.
 */

#ifndef MACROS_H
#define MACROS_H

#include <wx/wx.h>

/**
 * Macro TO_UTF8
 * converts a wxString to a UTF8 encoded C string for all wxWidgets build modes.
 * wxstring is a wxString, not a wxT() or _().  The scope of the return value
 * is very limited and volatile, but can be used with printf() style functions well.
 * NOTE: Trying to convert it to a function is tricky because of the
 * type of the parameter!
 */
#define TO_UTF8( wxstring )  ( (const char*) (wxstring).utf8_str() )

/**
 * function FROM_UTF8
 * converts a UTF8 encoded C string to a wxString for all wxWidgets build modes.
 */
static inline wxString FROM_UTF8( const char* cstring )
{
    wxString line = wxString::FromUTF8( cstring );

    if( line.IsEmpty() )  // happens when cstring is not a valid UTF8 sequence
        line = wxConvCurrent->cMB2WC( cstring );    // try to use locale conversion

    return line;
}

/**
 * Function GetChars
 * returns a wxChar* to the actual wxChar* data within a wxString, and is
 * helpful for passing strings to wxString::Printf() and wxString::Format().
 * It can also be passed a UTF8 parameter which will be converted to wxString
 * by the compiler.
 * <p>
 * Example:  wxString::Format( wxT( "%s" ), GetChars( UTF( "some text" ) ) );
 * <p>
 * When wxWidgets is properly built for KiCad, a const wxChar* points to either:
 * <ul>
 * <li> 32 bit unicode characters on linux/OSX or </li>
 * <li> 16 bit UTF16 characters on windows. </li>
 * </ul>
 * Note that you cannot pass 8 bit strings to wxString::Format() or Printf() so this
 * is a useful conversion function to wxChar*, which is needed by wxString::Format().
 *
 * @return const wxChar* - a pointer to the UNICODE or UTF16 (on windows) text.
 */
static inline const wxChar* GetChars( const wxString& s )
{
#if wxCHECK_VERSION( 2, 9, 0 )
    return (const wxChar*) s.c_str();
#else
    return s.GetData();
#endif
}

// This really needs a function? well, it is used *a lot* of times
template <class T> inline void NEGATE( T &x ) { x = -x; }

/// # of elements in an array
#define DIM( x )    unsigned( sizeof(x) / sizeof( (x)[0] ) )    // not size_t

/// Exchange two values
// std::swap works only with arguments of the same type (which is saner);
// here the compiler will figure out what to do (I hope to get rid of
// this soon or late)
template <class T, class T2> inline void EXCHG( T& a, T2& b )
{
    T temp = a;
    a = b;
    b = temp;
}

/**
 * Function Clamp
 * limits @a value within the range @a lower <= @a value <= @a upper.  It will work
 * on temporary expressions, since they are evaluated only once, and it should work
 * on most if not all numeric types, string types, or any type for which "operator < ()"
 * is present. The arguments are accepted in this order so you can remember the
 * expression as a memory aid:
 * <p>
 * result is:  lower <= value <= upper
 */
template <typename T> inline const T& Clamp( const T& lower, const T& value, const T& upper )
{
    wxASSERT( lower <= upper );
    if( value < lower )
        return lower;
    else if( upper < value )
        return upper;
    return value;
}


#endif /* ifdef MACRO_H */
