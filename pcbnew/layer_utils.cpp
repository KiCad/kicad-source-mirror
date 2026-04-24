/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include "layer_utils.h"


wxString LAYER_UTILS::AccumulateNames( const LSEQ& aLayers, const BOARD* aBoard )
{
    wxString result;

    for( const PCB_LAYER_ID layer : aLayers )
    {
        if( !result.IsEmpty() )
            result += ", ";

        result += aBoard ? aBoard->GetLayerName( layer ) : LayerName( layer );
    }

    return result;
}


LSET LAYER_UTILS::GetAllFootprintLayers( const FOOTPRINT& aFootprint )
{
    LSET usedLayers{};

    aFootprint.RunOnChildren(
            [&]( BOARD_ITEM* aSubItem )
            {
                wxCHECK2( aSubItem, /*void*/ );
                usedLayers |= aSubItem->GetLayerSet();
            },
            RECURSE_MODE::RECURSE );

    return usedLayers;
}


LSET LAYER_UTILS::GetOrphanedFootprintLayers( const FOOTPRINT& aFootprint,
                                              const LSET&      aCustomUserLayers )
{
    LSET usedLayers = GetAllFootprintLayers( aFootprint );

    usedLayers &= ~aCustomUserLayers;
    usedLayers &= ~LSET::AllTechMask();
    usedLayers &= ~LSET::UserMask();

    // Rescue is a pseudo-layer used as a fallback for items referencing unknown layer
    // names at load time. It is not exposed in any layer-selection UI, so the user has no
    // way to "keep" it. Items on Rescue are an orphan state that predates any Footprint
    // Properties edit and are surfaced through library-parity DRC instead.
    usedLayers.reset( Rescue );

    return usedLayers;
}
