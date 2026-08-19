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

std::optional<SDB_FOOTER_ERROR> CheckSdbFooter( const std::vector<uint8_t>& aData, size_t aFooterStart,
                                                const char* aGuid, size_t aGuidLen )
{
    // Subtract rather than add so a near-SIZE_MAX offset cannot wrap past the check
    if( aData.size() < 4 || aGuidLen > aData.size() - 4
        || aFooterStart > aData.size() - aGuidLen - 4 )
    {
        return SDB_FOOTER_ERROR{ aData.size(), "PADS binary file too small for the footer" };
    }

    if( std::memcmp( &aData[aFooterStart], aGuid, aGuidLen ) != 0 )
        return SDB_FOOTER_ERROR{ aFooterStart, "Invalid PADS binary footer GUID" };

    size_t   backPointerOffset = aFooterStart + aGuidLen;
    uint32_t containerItemsOffset = getU32LE( aData, backPointerOffset );

    if( containerItemsOffset > aFooterStart || aFooterStart - containerItemsOffset < 4 )
        return SDB_FOOTER_ERROR{ backPointerOffset, "Invalid PADS binary container-item back-pointer" };

    return std::nullopt;
}


void ValidateSdbFooter( const std::vector<uint8_t>& aData, size_t aFooterStart, const char* aGuid,
                        size_t aGuidLen )
{
    if( std::optional<SDB_FOOTER_ERROR> error = CheckSdbFooter( aData, aFooterStart, aGuid, aGuidLen ) )
        THROW_IO_ERROR( error->detail );
}

} // namespace PADS_IO
