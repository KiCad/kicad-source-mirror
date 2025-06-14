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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/length_delay_calculation_item.h>

#include <board.h>


void LENGTH_DELAY_CALCULATION_ITEM::CalculateViaLayers( const BOARD* aBoard )
{
    static std::initializer_list<KICAD_T> traceAndPadTypes = { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T };

    PCB_LAYER_ID top_layer = UNDEFINED_LAYER;
    PCB_LAYER_ID bottom_layer = UNDEFINED_LAYER;

    const LSET layers = aBoard->GetDesignSettings().GetEnabledLayers();

    for( auto layer_it = layers.copper_layers_begin(); layer_it != layers.copper_layers_end(); ++layer_it )
    {
        if( aBoard->GetConnectivity()->IsConnectedOnLayer( m_via, *layer_it, traceAndPadTypes ) )
        {
            if( top_layer == UNDEFINED_LAYER )
                top_layer = *layer_it;
            else
                bottom_layer = *layer_it;
        }
    }

    if( top_layer == UNDEFINED_LAYER )
        top_layer = m_via->TopLayer();
    if( bottom_layer == UNDEFINED_LAYER )
        bottom_layer = m_via->BottomLayer();

    SetLayers( top_layer, bottom_layer );
}
