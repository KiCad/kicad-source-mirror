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

#ifndef SCH_FOOTPRINT_FIELD_RECONCILER_H
#define SCH_FOOTPRINT_FIELD_RECONCILER_H

#include <set>
#include <vector>
#include <wx/string.h>

class SCHEMATIC;
class REPORTER;

struct SCH_FP_FIELD_RECONCILE_RESULT
{
    int m_relinkedToCache = 0;   ///< footprint fields re-pointed at the generated cache
    int m_keptSource      = 0;   ///< footprint fields left pointing at a provenance source library
};

/**
 * Frame-independent, non-interactive service that rewrites the library nickname of every schematic
 * symbol Footprint-field FPID after a non-KiCad import so it resolves to the coordinated project
 * library.  A field already pointing at a registered provenance source library is kept; every other
 * field is re-pointed at the manager-committed generated cache nickname, preserving the item name.
 */
class SCH_FOOTPRINT_FIELD_RECONCILER
{
public:
    SCH_FOOTPRINT_FIELD_RECONCILER( const wxString&              aCacheNickname,
                                    const std::vector<wxString>& aSourceLibNicknames,
                                    REPORTER*                    aReporter = nullptr );

    SCH_FP_FIELD_RECONCILE_RESULT Reconcile( SCHEMATIC& aSchematic );

private:
    wxString           m_cacheNickname;
    std::set<wxString> m_sourceLibs;
    REPORTER*          m_reporter;
};

#endif // SCH_FOOTPRINT_FIELD_RECONCILER_H
