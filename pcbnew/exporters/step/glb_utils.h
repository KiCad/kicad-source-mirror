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

#ifndef GLB_UTILS_H
#define GLB_UTILS_H

#include <wx/string.h>

/**
 * Fix LINES primitives in a GLB file that have odd index counts.
 *
 * OCCT 7.9+ can emit glTF LINES primitives with odd index counts for degenerate
 * BSpline edges. The glTF spec requires LINES (mode 1) to have an even number of
 * indices since each pair defines one line segment. Blender and other strict
 * importers reject files that violate this constraint.
 *
 * This function reads the GLB, adjusts any offending accessor counts to be even,
 * and rewrites the file in place.
 *
 * @param aFilePath path to the GLB file to fix
 * @return true if the file was processed successfully (or needed no changes)
 */
bool FixGlbLinesPrimitives( const wxString& aFilePath );

#endif // GLB_UTILS_H
