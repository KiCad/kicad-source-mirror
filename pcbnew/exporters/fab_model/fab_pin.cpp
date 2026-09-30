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

#include "fab_pin.h"

#include <footprint.h>
#include <pad.h>


FAB_PAD_ROLE GetFabPadRole( const PAD& aPad )
{
    switch( aPad.GetProperty() )
    {
    case PAD_PROP::BGA:            return FAB_PAD_ROLE::BGA;
    case PAD_PROP::FIDUCIAL_GLBL:  return FAB_PAD_ROLE::FIDUCIAL_GLOBAL;
    case PAD_PROP::FIDUCIAL_LOCAL: return FAB_PAD_ROLE::FIDUCIAL_LOCAL;
    case PAD_PROP::TESTPOINT:      return FAB_PAD_ROLE::TESTPOINT;
    case PAD_PROP::HEATSINK:       return FAB_PAD_ROLE::HEATSINK;
    case PAD_PROP::CASTELLATED:    return FAB_PAD_ROLE::CASTELLATED;
    case PAD_PROP::MECHANICAL:     return FAB_PAD_ROLE::MECHANICAL;
    case PAD_PROP::PRESSFIT:       return FAB_PAD_ROLE::PRESSFIT;
    case PAD_PROP::NONE:           break;
    }

    if( aPad.GetAttribute() == PAD_ATTRIB::NPTH && aPad.GetNumber().IsEmpty() )
    {
        return FAB_PAD_ROLE::TOOLING_HOLE;
    }

    return FAB_PAD_ROLE::NONE;
}


FAB_PIN MakeFabPin( const PAD& aPad, size_t aIndexInFootprint )
{
    FAB_PIN pin;
    pin.m_name = aPad.GetNumber();
    pin.m_role = GetFabPadRole( aPad );

    // Pins need names, so NPTH and unnumbered pads use their footprint ordinal
    if( aPad.GetAttribute() == PAD_ATTRIB::NPTH || pin.m_name.IsEmpty() )
    {
        const FOOTPRINT* footprint = aPad.GetParentFootprint();
        wxASSERT( footprint && aIndexInFootprint < footprint->Pads().size()
                  && footprint->Pads()[aIndexInFootprint] == &aPad );

        if( aPad.GetAttribute() == PAD_ATTRIB::NPTH )
        {
            pin.m_name = wxString::Format( "NPTH%zu", aIndexInFootprint );
        }
        else
        {
            pin.m_name = wxString::Format( "PAD%zu", aIndexInFootprint );
        }
    }

    if( aPad.GetAttribute() == PAD_ATTRIB::NPTH )
    {
        pin.m_electrical = FAB_ELECTRICAL::MECHANICAL;
    }
    else if( aPad.IsOnCopperLayer() )
    {
        pin.m_electrical = FAB_ELECTRICAL::ELECTRICAL;
    }

    if( ( aPad.HasHole() && aPad.IsOnCopperLayer() ) || aPad.GetAttribute() == PAD_ATTRIB::PTH )
    {
        pin.m_mount = FAB_MOUNT::THT;
    }
    else if( aPad.HasHole() && aPad.GetAttribute() == PAD_ATTRIB::NPTH )
    {
        pin.m_mount = FAB_MOUNT::HOLE;
    }
    else if( aPad.GetAttribute() == PAD_ATTRIB::SMD )
    {
        pin.m_mount = FAB_MOUNT::SMT;
    }

    return pin;
}
