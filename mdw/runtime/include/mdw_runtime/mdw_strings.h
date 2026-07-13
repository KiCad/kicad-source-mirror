/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file mdw_strings.h
 * @brief Hashing for the runtime and backends, which cannot reach the kicommon string helpers.
 */

#ifndef MDW_STRINGS_H
#define MDW_STRINGS_H

#include <cstddef>
#include <cstdint>
#include <string>

/**
 * 64-bit FNV-1a of aText as 16 lowercase hex digits. The default basis is not FNV's own, but journal file names
 * and check-in trailers persist these digests, so it must never change.
 */
inline std::string MdwHashHex( const std::string& aText, std::uint64_t aBasis = 1469598103934665603ULL )
{
    static const char HEX[] = "0123456789abcdef";

    std::uint64_t hash = aBasis;

    for( unsigned char c : aText )
    {
        hash ^= c;
        hash *= 1099511628211ULL;
    }

    std::string text( 16, '0' );

    for( std::size_t i = text.size(); i-- > 0; hash >>= 4 )
        text[i] = HEX[hash & 0xf];

    return text;
}

#endif // MDW_STRINGS_H
