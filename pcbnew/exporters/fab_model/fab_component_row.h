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

#include "fab_pin.h"
#include <wx/string.h>

class FOOTPRINT;


/// Footprint facts resolved for one assembly variant
struct FAB_COMPONENT_ROW
{
    const FOOTPRINT* m_footprint;
    wxString         m_variant;
    bool             m_dnp;
    bool             m_excludedFromBom;
    bool             m_pressFit;
    FAB_MOUNT        m_mount;

    /// Variant-resolved shown text, empty when the field is missing
    wxString Field( const wxString& aName ) const;
    wxString Value() const;
};


FAB_COMPONENT_ROW MakeFabComponentRow( const FOOTPRINT& aFootprint, const wxString& aVariant );
