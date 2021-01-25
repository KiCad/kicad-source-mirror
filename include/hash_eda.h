/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef HASH_EDA_H_
#define HASH_EDA_H_

/**
 * @brief Hashing functions for EDA_ITEMs.
 */

#include <cstdlib>
#include <functional>

class EDA_ITEM;

///< Enables/disables properties that will be used for calculating the hash.
///< The properties might be combined using the bitwise 'or' operator.
enum HASH_FLAGS
{
    HASH_POS    = 0x01,

    ///< use coordinates relative to the parent object
    REL_COORD   = 0x02,
    HASH_ROT    = 0x04,
    HASH_LAYER  = 0x08,
    HASH_NET    = 0x10,
    HASH_REF    = 0x20,
    HASH_VALUE  = 0x40,
    HASH_ALL    = 0xff
};

/**
 * Calculate hash of an EDA_ITEM.
 *
 * @param aItem is the item for which the hash will be computed.
 * @return Hash value.
 */
std::size_t hash_fp_item( const EDA_ITEM* aItem, int aFlags = HASH_FLAGS::HASH_ALL );

/**
 * This is a dummy function to take the final case of hash_combine below
 * @param seed
 */
static inline void hash_combine( std::size_t &seed ) {}

/**
 * Combine multiple hashes utilizing previous hash result.
 *
 * @tparam T      A hashable type
 * @param seed    A seed value input and output for the result.
 * @param val     A hashable object of type T
 */
template< typename T, typename ... Types >
static inline void hash_combine( std::size_t &seed, const T &val, const Types &... args )
{
    seed ^= std::hash<T>()( val ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    hash_combine( seed, args... );
}

template <typename... Types>
static inline std::size_t hash_val( const Types &... args )
{
    std::size_t seed = 0xa82de1c0;
    hash_combine( seed, args... );
    return seed;
}

#endif
