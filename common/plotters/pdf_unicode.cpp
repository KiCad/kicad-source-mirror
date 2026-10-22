/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <plotters/pdf_unicode.h>

#include <fmt/format.h>


std::string FormatPdfUtf16Hex( uint32_t aCodepoint )
{
    // Code points in the BMP are a single UTF-16 code unit.
    if( aCodepoint <= 0xFFFF )
        return fmt::format( "{:04X}", aCodepoint );

    // Above the BMP: a surrogate pair, per the Unicode Standard, Table 3-5.
    if( aCodepoint <= 0x10FFFF )
    {
        const uint32_t value = aCodepoint - 0x10000;
        const uint16_t high = 0xD800 + ( value >> 10 );
        const uint16_t low = 0xDC00 + ( value & 0x3FF );
        return fmt::format( "{:04X}{:04X}", high, low );
    }

    // Not a Unicode, give a "?" replacement.
    return std::string( "003F" );
}
