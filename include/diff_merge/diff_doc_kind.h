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

#ifndef KICAD_DIFF_DOC_KIND_H
#define KICAD_DIFF_DOC_KIND_H

#include <kicommon.h>

#include <wx/string.h>


namespace KICAD_DIFF
{

/// Document type a diff/merge entry point should route to, derived from a
/// file path's extension. Used by the mergetool, the MR-review dialog, and
/// the kiface diff/merge function exports to pick the right kiface + differ.
enum class DOC_KIND
{
    UNKNOWN,
    PCB,
    SCH,
    SYM_LIB,
    FP_LIB,
    FOOTPRINT
};


/// Map a path's extension to a DOC_KIND (.kicad_pcb -> PCB, .kicad_sch -> SCH,
/// .kicad_sym -> SYM_LIB, .kicad_mod -> FOOTPRINT; anything else -> UNKNOWN).
KICOMMON_API DOC_KIND DocKindFromExtension( const wxString& aPath );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_DOC_KIND_H
