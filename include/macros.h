/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @brief This file contains miscellaneous commonly used macros and functions.
 */

#ifndef MACROS_H
#define MACROS_H

#include <wx/string.h>

#if defined( __has_attribute )
    #define KI_HAS_ATTRIBUTE( x ) __has_attribute( x )
#else
    #define KI_HAS_ATTRIBUTE( x ) 0
#endif

// Based on the declaration inside the LLVM source code
#if defined( __cplusplus ) && defined( __has_cpp_attribute )
    #define KI_HAS_CPP_ATTRIBUTE( x ) __has_cpp_attribute( x )
#else
    #define KI_HAS_CPP_ATTRIBUTE( x ) 0
#endif

/**
 * The KI_FALLTHROUGH macro is to be used when switch statement cases should purposely
 * fallthrough from one to the next. It must be followed by a ";".
 *
 * Sample code:
 *     switch( a )
 *     {
 *     case 1:
 *         // Some code
 *         KI_FALLTHROUGH;
 *
 *     case 2:
 *         // More code
 *         break;
 *     }
 */
#if __cplusplus >= 201703L
    // C++ 17 includes this macro on all compilers
    #define KI_FALLTHROUGH [[fallthrough]]

#elif KI_HAS_CPP_ATTRIBUTE( clang::fallthrough )
    // Clang provides this attribute to silence the "-Wimplicit-fallthrough" warning
    #define KI_FALLTHROUGH [[clang::fallthrough]]

#elif KI_HAS_CPP_ATTRIBUTE( gnu::fallthrough )
    // GNU-specific C++ attribute to silencing the warning
    #define KI_FALLTHROUGH [[gnu::fallthrough]]

#elif defined( __GNUC__ ) && __GNUC__ >= 7
    // GCC 7+ includes the "-Wimplicit-fallthrough" warning, and this attribute to silence it
    #define KI_FALLTHROUGH __attribute__ ((fallthrough))

#else
    // In every other case, don't do anything
    #define KI_FALLTHROUGH ( ( void ) 0 )

#endif

/**
 * Convert a wxString to a UTF8 encoded C string for all wxWidgets build modes.
 *
 * wxstring is a wxString, not a wxT() or _().  The scope of the return value
 * is very limited and volatile, but can be used with printf() style functions well.
 *
 * @note Trying to convert it to a function is tricky because of the type of the
 *       parameter!
 */
#define TO_UTF8( wxstring )  ( (const char*) (wxstring).utf8_str() )

/**
 * Stringifies the given parameter by placing in quotes.
 *
 * @param cstring STRING (no spaces)
 * @return "STRING"
 */
#define TO_STR2(x) #x
#define TO_STR(x) TO_STR2(x)

/**
 * Convert a UTF8 encoded C string to a wxString for all wxWidgets build modes.
 */
static inline wxString FROM_UTF8( const char* cstring )
{
    wxString line = wxString::FromUTF8( cstring );

    if( line.IsEmpty() )  // happens when cstring is not a valid UTF8 sequence
        line = wxConvCurrent->cMB2WC( cstring );    // try to use locale conversion

    return line;
}

#define UNIMPLEMENTED_FOR( type ) \
        wxFAIL_MSG( wxString::Format( "%s: unimplemented for %s", __FUNCTION__, type ) )

#endif // MACROS_H
