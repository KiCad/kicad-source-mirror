/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <io/pads/pads_binary_utils.h>

#include <cstring>

namespace PADS_IO
{

void ValidateSdbFooter( const std::vector<uint8_t>& aData, size_t aFooterStart, const char* aGuid,
                        size_t aGuidLen )
{
    // Subtract rather than add so a near-SIZE_MAX offset cannot wrap past the check
    if( aData.size() < 4 || aGuidLen > aData.size() - 4
        || aFooterStart > aData.size() - aGuidLen - 4 )
    {
        THROW_IO_ERROR( "PADS binary file too small for the footer" );
    }

    if( std::memcmp( &aData[aFooterStart], aGuid, aGuidLen ) != 0 )
        THROW_IO_ERROR( "Invalid PADS binary footer GUID" );

    uint32_t containerItemsOffset = getU32LE( aData, aFooterStart + aGuidLen );

    if( containerItemsOffset > aFooterStart || aFooterStart - containerItemsOffset < 4 )
        THROW_IO_ERROR( "Invalid PADS binary container-item back-pointer" );
}

} // namespace PADS_IO
