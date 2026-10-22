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

#pragma once

#include <cstdint>
#include <string>

/**
 * Format one Unicode code point as the hex digits of its UTF-16BE representation.
 *
 * @param aCodepoint a Unicode code point
 * @return the code point as four uppercase hex digits, or eight for a surrogate pair;
 *         "003F" ('?') when the value is above U+10FFFF
 */
std::string FormatPdfUtf16Hex( uint32_t aCodepoint );
