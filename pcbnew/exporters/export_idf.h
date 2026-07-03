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

#ifndef EXPORT_IDF_H
#define EXPORT_IDF_H

#include <wx/string.h>

class BOARD;
class FILENAME_RESOLVER;

/**
 * Generate IDFv3 compliant board (*.emn) and library (*.emp) files for @p aPcb.
 *
 * Headless-friendly core of the IDF exporter. @p aResolver locates 3D model files and must be
 * non-null; the GUI passes the project resolver while non-GUI callers may pass a bare resolver.
 * On failure the routine returns false and, when @p aErrorMsg is non-null, stores a
 * human-readable message rather than showing a dialog.
 */
bool ExportBoardToIDF3( BOARD* aPcb, const wxString& aFullFileName, bool aUseThou, double aXRef,
                        double aYRef, bool aIncludeUnspecified, bool aIncludeDNP,
                        FILENAME_RESOLVER* aResolver, wxString* aErrorMsg = nullptr );

#endif // EXPORT_IDF_H
