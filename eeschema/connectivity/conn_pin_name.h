/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/string.h>

namespace SCH_CONNECTIVITY
{
struct PIN_NAME_FACT
{
    // Names come from the library; alternate names are used only to detect collisions.
    wxString name;
    wxString shownNumber;
    wxString number;
    wxString padNumber;
    bool noConnect = false;
    bool hasDuplicateName = false;
};

struct PIN_NAME_REFERENCE
{
    wxString reference;
    wxString referenceWithUnit;
    wxString symbolUuid;
};

/** Render a non-power pin name from unescaped values; an absent reference uses the symbol UUID. */
wxString RenderPinNetName( const PIN_NAME_FACT& aPin, const PIN_NAME_REFERENCE& aReference,
                          bool aForceNoConnect = false );
}
