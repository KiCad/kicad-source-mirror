/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#include <eda_item.h>

class SCHEMATIC;


class VARIANT_PROXY_UNDO_ITEM : public EDA_ITEM
{
public:
    VARIANT_PROXY_UNDO_ITEM( const SCHEMATIC* aSchematic );

    /*
     * Restores the saved variant definitions to the current schematic.
     */
    void Restore( SCHEMATIC* aSchematic );

#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const override { }
#endif

    wxString GetClass() const override { return wxT( "VARIANT_PROXY_UNDO_ITEM" ); }

protected:
    std::map<wxString, wxString> m_variants;    // variant to variant-description map
};

