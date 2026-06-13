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

#ifndef FP_LIB_DIFFER_H
#define FP_LIB_DIFFER_H

#include <diff_merge/kicad_differ.h>

#include <wx/string.h>

#include <map>
#include <memory>
#include <vector>


class FOOTPRINT;


namespace KICAD_DIFF
{

/**
 * Diff two .pretty footprint library directories.
 *
 * Identity is footprint name (the LIB_ID item name). Renamed footprints
 * surface as (REMOVED old, ADDED new); per-footprint property delta is
 * available via the single-footprint differ (FOOTPRINT children walk in
 * PCB_DIFFER::diffFootprintChildren).
 */
class FP_LIB_DIFFER : public KICAD_DIFFER
{
public:
    using FOOTPRINT_MAP = std::map<wxString, const FOOTPRINT*>;

    FP_LIB_DIFFER( const FOOTPRINT_MAP& aBefore, const FOOTPRINT_MAP& aAfter,
                   const wxString& aPath = wxEmptyString );
    ~FP_LIB_DIFFER() override;

    DOCUMENT_DIFF Diff() override;

    /// Load a .pretty directory into a FOOTPRINT_MAP. The vector owns the
    /// FOOTPRINT instances; the map borrows raw pointers into it.
    static std::pair<std::vector<std::unique_ptr<FOOTPRINT>>, FOOTPRINT_MAP>
    LoadLibrary( const wxString& aPrettyPath );

private:
    const FOOTPRINT_MAP& m_before;
    const FOOTPRINT_MAP& m_after;
    wxString             m_path;
};

} // namespace KICAD_DIFF

#endif // FP_LIB_DIFFER_H
