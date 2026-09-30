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

#include "fab_component_row.h"

#include "fab_pin.h"
#include <algorithm>
#include <footprint.h>
#include <pad.h>
#include <pcb_field.h>


wxString FAB_COMPONENT_ROW::Field( const wxString& aName ) const
{
    const PCB_FIELD* field = m_footprint->GetField( aName );
    return field ? field->GetShownText( m_variant, RESOLVED ) : wxString();
}


wxString FAB_COMPONENT_ROW::Value() const
{
    const PCB_FIELD* field = m_footprint->GetField( FIELD_T::VALUE );
    return field ? field->GetShownText( m_variant, RESOLVED ) : m_footprint->GetValue();
}


FAB_COMPONENT_ROW MakeFabComponentRow( const FOOTPRINT& aFootprint, const wxString& aVariant )
{
    bool      pressFit = std::any_of( aFootprint.Pads().begin(), aFootprint.Pads().end(),
                                      []( const PAD* aPad )
                                      {
                                     return GetFabPadRole( *aPad ) == FAB_PAD_ROLE::PRESSFIT;
                                 } );
    FAB_MOUNT mount = FAB_MOUNT::OTHER;

    if( aFootprint.GetAttributes() & FP_SMD )
        mount = FAB_MOUNT::SMT;
    else if( aFootprint.GetAttributes() & FP_THROUGH_HOLE )
        mount = FAB_MOUNT::THT;

    return { &aFootprint,
             aVariant,
             aFootprint.GetDNPForVariant( aVariant ),
             aFootprint.GetExcludedFromBOMForVariant( aVariant ),
             pressFit,
             mount };
}
