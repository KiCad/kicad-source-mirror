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

#include <wx/string.h>

#include <optional>
#include <vector>


class EDA_BASE_FRAME;


/**
 * Read a CSV/TSV table from a file selected via a dialog (aFromFile) or from the system
 * clipboard.
 *
 * @return the parsed table, or std::nullopt if the user cancelled the file dialog or the
 *         data could not be decoded.
 */
std::optional<std::vector<std::vector<wxString>>> ReadTableFromFileOrClipboard( EDA_BASE_FRAME& aFrame,
                                                                                bool            aFromFile );


/**
 * Write aTable as CSV to aToFile (if non-empty) or to the system clipboard.
 */
void WriteTableToFileOrClipboard( const wxString& aToFile, const std::vector<std::vector<wxString>>& aTable );
