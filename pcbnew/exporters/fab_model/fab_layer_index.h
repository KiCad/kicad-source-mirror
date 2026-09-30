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

#include <layer_ids.h>
#include <map>
#include <vector>

class BOARD;
class BOARD_ITEM;
class PCB_SHAPE;
class PCB_TRACK;


/// Board items grouped by authored layer and net, sorted by FabItemLess
class FAB_LAYER_INDEX
{
public:
    using NET_ITEMS = std::map<int, std::vector<BOARD_ITEM*>>;

    explicit FAB_LAYER_INDEX( BOARD& aBoard );

    const NET_ITEMS& Items( PCB_LAYER_ID aLayer ) const;

private:
    std::map<PCB_LAYER_ID, NET_ITEMS> m_items;
};


/// Use only for traces and arcs; vias require a layer-specific width
int  FabTrackWidth( const PCB_TRACK& aTrack, PCB_LAYER_ID aLayer );
/// Build an expanded mask copy of an exposed copper shape when needed
bool FabMaskShape( const PCB_SHAPE& aShape, PCB_LAYER_ID aLayer, PCB_SHAPE& aResult );
