/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef HASHTABLES_H_
#define HASHTABLES_H_

#include <unordered_map>

#include <wx/string.h>

// First some utility classes and functions

/// Equality test for "const char*" type used in very specialized KEYWORD_MAP below
struct iequal_to
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
    std::size_t operator()( const char* it ) const
    {
        std::size_t hash = 2166136261u;

        for( ; *it; ++it )
        {
            hash ^= (unsigned char) *it;
            hash *= 16777619;
        }
        return hash;
    }
};


#ifdef SWIG
/// Declare a std::unordered_map and also the swig %template in unison
#define DECL_HASH_FOR_SWIG( TypeName, KeyType, ValueType )          \
    namespace std                                                   \
    {                                                               \
        % template( TypeName ) unordered_map<KeyType, ValueType>;   \
    }                                                               \
    typedef std::unordered_map<KeyType, ValueType> TypeName;
#else
/// Declare a std::unordered_map but no swig %template
#define DECL_HASH_FOR_SWIG( TypeName, KeyType, ValueType )          \
    typedef std::unordered_map<KeyType, ValueType> TypeName;
#endif


/**
 * A hashtable made of a const char* and an int.
 *
 * @note The use of this type outside very specific circumstances is foolish since there is
 *       no storage provided for the actual C string itself.
 *
 * This type assumes use with type #KEYWORD that is created by CMake and that table creates
 * *constant* storage for C strings (and pointers to those C strings).  Here we are only
 * interested in the C strings themselves and only the pointers are duplicated within the
 * hashtable.  If the strings were not constant and fixed, this type would not work.  Also
 * note that normally a hashtable (i.e. unordered_map) using a const char* key would simply
 * compare the 32 bit or 64 bit pointers themselves, rather than the C strings which they
 * are known to point to in this context.  I force the latter behavior by supplying both
 * "hash" and "equality" overloads to the hashtable (unordered_map) template.
 *
 * @author Dick Hollenbeck
 */
typedef std::unordered_map< const char*, int, fnv_1a, iequal_to > KEYWORD_MAP;


#endif // HASHTABLES_H_
