#ifndef HASHTABLES_H_
#define HASHTABLES_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <base_struct.h>
#include <wx/string.h>

// Three strategies for providing a portable hashtable are given.
// C++, boost, and wx, in that order.  C++ solution is no good for mingw.
// So boost seems best for all platforms.


#if 0   // C++ std::unordered_map, maybe in the future

#include <unordered_map>


/// Map a C string to an integer.  Used in DSNLEXER.
typedef std::unordered_map< std::string, int >          KEYWORD_MAP;

/// Map a C string to an EDA_RECT.
/// The key is the classname of the derived wxformbuilder dialog.
typedef std::unordered_map< std::string, EDA_RECT >     RECT_MAP;


#elif 1     // boost::unordered_map

// fix a compile bug at line 97 of boost/detail/container_fwd.hpp
#define BOOST_DETAIL_TEST_FORCE_CONTAINER_FWD

#include <boost/unordered_map.hpp>

// see http://www.boost.org/doc/libs/1_49_0/doc/html/boost/unordered_map.html

/// Equality test for "const char*" type used in very specialized KEYWORD_MAP below
struct iequal_to : std::binary_function< const char*, const char*, bool >
{
    bool operator()( const char* x, const char* y ) const
    {
        return !strcmp( x, y );
    }
};


/// Very fast and efficient hash function for "const char*" type, used in specialized
/// KEYWORD_MAP below.
/// taken from: http://www.boost.org/doc/libs/1_53_0/libs/unordered/examples/fnv1.hpp
struct fnv_1a
{
    /* not used, std::string is too slow:
    std::size_t operator()( std::string const& text ) const
    {
        std::size_t hash = 2166136261u;

        for( std::string::const_iterator it = text.begin(), end = text.end();
                it != end;  ++it )
        {
            hash ^= *it;
            hash *= 16777619;
        }
        return hash;
    }
    */

    std::size_t operator()( const char* it ) const
    {
        std::size_t hash = 2166136261u;

        for( ; *it;  ++it )
        {
            hash ^= (unsigned char) *it;
            hash *= 16777619;
        }
        return hash;
    }
};


/// Hash function for wxString, counterpart of std::string hash
struct WXSTRING_HASH : std::unary_function<wxString, std::size_t>
{
    std::size_t operator()( const wxString& aString ) const
    {
        std::size_t hash = 2166136261u;

        for( wxString::const_iterator it = aString.begin(); it != aString.end(); ++it )
        {
            hash ^= (unsigned char) *it;
            hash *= 16777619;
        }

        return hash;
    }
};


/**
 * Type KEYWORD_MAP
 * is a hashtable made of a const char* and an int.  Note that use of this
 * type outside very specific circumstances is foolish since there is no storage
 * provided for the actual C string itself.  This type assumes use with type KEYWORD
 * that is created by CMake and that table creates *constant* storage for C strings
 * (and pointers to those C strings).  Here we are only interested in the C strings
 * themselves and only the pointers are duplicated within the hashtable.
 * If the strings were not constant and fixed, this type would not work.
 * Also note that normally a hashtable (i.e. unordered_map) using a const char* key
 * would simply compare the 32 bit or 64 bit pointers themselves, rather than
 * the C strings which they are known to point to in this context.
 * I force the latter behavior by supplying both "hash" and "equality" overloads
 * to the hashtable (unordered_map) template.
 * @author Dick Hollenbeck
 */
typedef boost::unordered_map< const char*, int, fnv_1a, iequal_to >     KEYWORD_MAP;


/// Map a std::string to an EDA_RECT.
/// The key is the classname of the derived wxformbuilder dialog.
typedef boost::unordered_map< std::string, EDA_RECT >  RECT_MAP;

#endif

#endif // HASHTABLES_H_
