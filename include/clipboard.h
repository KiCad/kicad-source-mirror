/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <memory>
#include <string>

class wxImage;

/**
 * Store information to the system clipboard.
 *
 * @param aText is the information to be stored, expected UTF8 encoding.  The text will be
 *              stored as Unicode string (not stored as UTF8 string).
 * @return False if error occurred.
 */
bool SaveClipboard( const std::string& aTextUTF8 );

/**
 * Return the information currently stored in the system clipboard.
 *
 * If data stored in the clipboard is in non-text format, empty string is returned.
 *
 * @note The clipboard is expected containing Unicode chars, not only ASCII7 chars.
 *       The returned string is UTF8 encoded
 */
std::string GetClipboardUTF8();

/**
 * Get image data from the clipboard, if there is any.
 *
 * If there's a filename there, and it can be loaded as an image, do that.
 */
std::unique_ptr<wxImage> GetImageFromClipboard();