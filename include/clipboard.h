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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <wx/buffer.h>
#include <wx/image.h>
#include <wx/string.h>

struct CLIPBOARD_MIME_DATA
{
    wxString       m_mimeType;
    wxMemoryBuffer m_data;

    /// Optional pre-decoded image for "image/png" MIME type.
    /// When set, SaveClipboard() uses this directly instead of decoding m_data,
    /// avoiding expensive double PNG encode/decode during clipboard operations.
    std::optional<wxImage> m_image;
};

/**
 * Store information to the system clipboard.
 *
 * @param aText is the information to be stored, expected UTF8 encoding.  The text will be
 *              stored as Unicode string (not stored as UTF8 string).
 * @return False if error occurred.
 */
bool SaveClipboard( const std::string& aTextUTF8 );

/**
 * Store information to the system clipboard with additional MIME types.
 *
 * Creates a composite clipboard object with text as the primary format and additional
 * custom MIME type data.  When aMimeData is empty, falls back to SaveClipboard(aTextUTF8).
 *
 * @param aTextUTF8 is the text to store, expected UTF8 encoding.  Stored as primary text format.
 * @param aMimeData is additional data to store by MIME type (e.g., image/svg+xml, image/png).
 * @return False if error occurred.
 */
bool SaveClipboard( const std::string& aTextUTF8, const std::vector<CLIPBOARD_MIME_DATA>& aMimeData );

/**
 * Store tabular data to the system clipboard.
 */
bool SaveTabularDataToClipboard( const std::vector<std::vector<wxString>>& aData );

/**
 * Attempt to get tabular data from the clipboard.
 */
bool GetTabularDataFromClipboard( std::vector<std::vector<wxString>>& aData );

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
