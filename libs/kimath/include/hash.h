/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#pragma once

#include <functional>

/**
 * This is a dummy function to take the final case of hash_combine below
 * @param seed
 */
static inline constexpr void hash_combine( std::size_t& seed )
{
}

/**
 * Combine multiple hashes utilizing previous hash result.
 *
 * @tparam T      A hashable type
 * @param seed    A seed value input and output for the result.
 * @param val     A hashable object of type T
 */
template <typename T, typename... Types>
static inline constexpr void hash_combine( std::size_t& seed, const T& val, const Types&... args )
{
    seed ^= std::hash<T>()( val ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    hash_combine( seed, args... );
}

template <typename... Types>
static inline constexpr std::size_t hash_val( const Types&... args )
{
    std::size_t seed = 0xa82de1c0;
    hash_combine( seed, args... );
    return seed;
}
