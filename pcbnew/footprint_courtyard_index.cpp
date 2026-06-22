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

#include <footprint_courtyard_index.h>

#include <board.h>
#include <footprint.h>

FOOTPRINT_COURTYARD_INDEX::FOOTPRINT_COURTYARD_INDEX( const BOARD* aBoard )
{
    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        BOX2I bbox;
        bool  hasCourtyard = false;

        // Index by the union of the front and back courtyard bounds so a single query serves
        // intersectsCourtyard/Front/Back; the downstream per-side test discards any extra hits.
        for( PCB_LAYER_ID side : { F_Cu, B_Cu } )
        {
            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( side );

            if( courtyard.OutlineCount() == 0 )
                continue;

            if( hasCourtyard )
                bbox.Merge( courtyard.BBox() );
            else
                bbox = courtyard.BBox();

            hasCourtyard = true;
        }

        if( !hasCourtyard )
            continue;

        const int min[2] = { bbox.GetLeft(), bbox.GetTop() };
        const int max[2] = { bbox.GetRight(), bbox.GetBottom() };

        m_tree.Insert( min, max, footprint );
    }
}


void FOOTPRINT_COURTYARD_INDEX::QueryOverlapping( const BOX2I& aBox,
        const std::function<bool( FOOTPRINT* )>& aVisitor ) const
{
    const int min[2] = { aBox.GetLeft(), aBox.GetTop() };
    const int max[2] = { aBox.GetRight(), aBox.GetBottom() };

    m_tree.Search( min, max, aVisitor );
}
