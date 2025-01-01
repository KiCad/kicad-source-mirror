/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Seth Hillbrand <seth@kipro-pcb.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_BASE64_H_
#define INCLUDE_BASE64_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace base64
{
    size_t encode_length( size_t aInputLen );

    size_t decode_length( size_t aInputLen );

    void encode( const std::vector<uint8_t>& aInput, std::vector<uint8_t>& aOutput );

    void decode( const std::vector<uint8_t>& aInput, std::vector<uint8_t>& aOutput );
}


#endif /* INCLUDE_BASE64_H_ */
