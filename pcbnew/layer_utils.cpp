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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "layer_utils.h"

#include <algorithm>

#include <pad.h>


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


LSET LAYER_UTILS::GetUsedFootprintLayers( const FOOTPRINT& aFootprint )
{
    LSET usedLayers{};

    aFootprint.RunOnChildren(
            [&]( BOARD_ITEM* aSubItem )
            {
                wxCHECK2( aSubItem, /*void*/ );

                LSET itemLayers = aSubItem->GetLayerSet();

                // *.Cu wildcard pins no copper layer, an explicit padstack layer still does
                if( aSubItem->Type() == PCB_PAD_T )
                {
                    const PAD* pad = static_cast<const PAD*>( aSubItem );
                    LSET       copper = itemLayers & LSET::AllCuMask();

                    // A blind span such as F.Cu..In2.Cu is a real layer choice, not the wildcard
                    if( copper.count() > 1 && copper == LSET::AllCuMask( copper.count() ) )
                    {
                        for( PCB_LAYER_ID layer : copper )
                        {
                            if( !pad->HasExplicitDefinitionForLayer( layer ) )
                                itemLayers.reset( layer );
                        }
                    }
                }

                usedLayers |= itemLayers;
            },
            RECURSE_MODE::RECURSE );

    return usedLayers;
}


int LAYER_UTILS::MinimalCopperLayerCount( const LSET& aLayers )
{
    const LSET innerCopper = aLayers & LSET::InternalCuMask();
    int        copperCount = 2;

    for( PCB_LAYER_ID layer : innerCopper )
        copperCount = std::max( copperCount, static_cast<int>( CopperLayerToOrdinal( layer ) ) + 2 );

    // Stackups only come in even copper counts
    return copperCount + copperCount % 2;
}


LSET LAYER_UTILS::GetOrphanedFootprintLayers( const FOOTPRINT& aFootprint,
                                              const LSET&      aCustomUserLayers )
{
    LSET usedLayers = GetUsedFootprintLayers( aFootprint );

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
