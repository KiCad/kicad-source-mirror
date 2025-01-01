/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <board.h>

#include "pns_item.h"
#include "pns_sizes_settings.h"

namespace PNS {


void SIZES_SETTINGS::ClearLayerPairs()
{
    m_layerPairs.clear();
}


void SIZES_SETTINGS::AddLayerPair( int aL1, int aL2 )
{
    int top = std::min( aL1, aL2 );
    int bottom = std::max( aL1, aL2 );

    m_layerPairs[bottom] = top;
    m_layerPairs[top] = bottom;
}


int SIZES_SETTINGS::GetLayerTop() const
{
    if( m_layerPairs.empty() )
        return F_Cu;
    else
        return m_layerPairs.begin()->first;
}


int SIZES_SETTINGS::GetLayerBottom() const
{
    if( m_layerPairs.empty() )
        return B_Cu;
    else
        return m_layerPairs.begin()->second;
}

}
