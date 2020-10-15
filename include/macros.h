/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <deque>
#include <vector>
#include <map>
#include <set>
#include <memory>       // std::shared_ptr

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
    // GNU-specific C++ attribute to sliencing the warning
    #define KI_FALLTHROUGH [[gnu::fallthrough]]

#elif defined( __GNUC__ ) && __GNUC__ >= 7
    // GCC 7+ includes the "-Wimplicit-fallthrough" warning, and this attribute to silence it
    #define KI_FALLTHROUGH __attribute__ ((fallthrough))

#else
    // In every other case, don't do anything
    #define KI_FALLTHROUGH ( ( void ) 0 )

#endif

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
 * Stringifies the given parameter by placing in quotes
 * @param cstring STRING (no spaces)
 * @return "STRING"
 */
#define TO_STR2(x) #x
#define TO_STR(x) TO_STR2(x)

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
    return (const wxChar*) s.c_str();
}

/// # of elements in an array.  This implements type-safe compile time checking
template <typename T, std::size_t N>
constexpr std::size_t arrayDim(T const (&)[N]) noexcept
{
    return N;
}

/**
 * Function MIRROR
 * Mirror @a aPoint in @a aMirrorRef.
 */
template<typename T>
T Mirror( T aPoint, T aMirrorRef )
{
    return -( aPoint - aMirrorRef ) + aMirrorRef;
}
template<typename T>
void MIRROR( T& aPoint, const T& aMirrorRef )
{
    aPoint = Mirror( aPoint, aMirrorRef );
}


#ifdef SWIG
/// Declare a std::vector and also the swig %template in unison
#define DECL_VEC_FOR_SWIG(TypeName, MemberType) namespace std { %template(TypeName) vector<MemberType>; } typedef std::vector<MemberType> TypeName;
#define DECL_DEQ_FOR_SWIG(TypeName, MemberType) namespace std { %template(TypeName) deque<MemberType>; } typedef std::deque<MemberType> TypeName;
#define DECL_MAP_FOR_SWIG(TypeName, KeyType, ValueType) namespace std { %template(TypeName) map<KeyType, ValueType>; } typedef std::map<KeyType, ValueType> TypeName;
#define DECL_SPTR_FOR_SWIG(TypeName, MemberType) %shared_ptr(MemberType) namespace std { %template(TypeName) shared_ptr<MemberType>; } typedef std::shared_ptr<MemberType> TypeName;
#define DECL_SET_FOR_SWIG(TypeName, MemberType) namespace std { %template(TypeName) set<MemberType>; } typedef std::set<MemberType> TypeName;
#else
/// Declare a std::vector but no swig %template
#define DECL_VEC_FOR_SWIG(TypeName, MemberType) typedef std::vector<MemberType> TypeName;
#define DECL_DEQ_FOR_SWIG(TypeName, MemberType) typedef std::deque<MemberType> TypeName;
#define DECL_MAP_FOR_SWIG(TypeName, KeyType, ValueType) typedef std::map<KeyType, ValueType> TypeName;
#define DECL_SPTR_FOR_SWIG(TypeName, MemberType) typedef std::shared_ptr<MemberType> TypeName;
#define DECL_SET_FOR_SWIG(TypeName, MemberType) typedef std::set<MemberType> TypeName;
#endif

#endif // MACROS_H
