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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef HASHTABLES_H_
#define HASHTABLES_H_

#include <cstddef>
#include <unordered_map>

#include <wx/string.h>

// First some utility classes and functions


/**
 * Fold @p aValue into the running hash @p aSeed using the well-known Boost
 * hash_combine mixing step. The magic constant is the 32-bit fractional part of
 * the golden ratio; the shifts spread bits so combining values is order-
 * sensitive and collisions stay rare.
 *
 * Used to build content hashes that must stay stable across runs (e.g. the
 * diff-engine summary strings), so the mixing formula must not change once
 * persisted output depends on it.
 */
inline std::size_t KiHashCombine( std::size_t aSeed, std::size_t aValue )
{
    return aSeed ^ ( aValue + 0x9e3779b9 + ( aSeed << 6 ) + ( aSeed >> 2 ) );
}

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
