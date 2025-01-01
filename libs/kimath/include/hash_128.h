/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef HASH_128_H_
#define HASH_128_H_

#include <cstdint>
#include <iomanip>
#include <sstream>

/**
 * A storage class for 128-bit hash value
 */
struct HASH_128
{
    void Clear()
    {
        memset( Value64, 0, sizeof( Value64 ) ); //
    }

    bool operator==( const HASH_128& aOther ) const
    {
        return memcmp( Value64, aOther.Value64, sizeof( Value64 ) ) == 0;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill( '0' )
           << std::setw( 16 ) << Value64[0]
           << std::setw( 16 ) << Value64[1];
        return ss.str();
    }

public:
    union
    {
        uint8_t  Value8[16]{}; // Zero-initialized
        uint32_t Value32[4];
        uint64_t Value64[2];
    };
};

#endif // HASH_128_H_
