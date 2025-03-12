/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <tools/sch_selection.h>
#include <sch_item.h>
#include <sch_reference_list.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_sheet.h>


SCH_SELECTION::SCH_SELECTION( SCH_SCREEN* aScreen ) :
        SELECTION()
{
    m_screen = aScreen;
}


EDA_ITEM* SCH_SELECTION::GetTopLeftItem( bool onlyModules ) const
{
    EDA_ITEM* topLeftConnectedItem = nullptr;
    VECTOR2I  topLeftConnectedPos;

    EDA_ITEM* topLeftItem = nullptr;
    VECTOR2I  topLeftPos;

    auto processItem =
            []( EDA_ITEM* aItem, EDA_ITEM** aCurrent, VECTOR2I* aCurrentPos )
            {
                VECTOR2I pos = aItem->GetPosition();

                if( ( *aCurrent == nullptr )
                    || ( pos.x < aCurrentPos->x )
                    || ( pos.x == aCurrentPos->x && pos.y < aCurrentPos->y ) )
                {
                    *aCurrent = aItem;
                    *aCurrentPos = pos;
                }
            };

    // Find the leftmost (smallest x coord) and highest (smallest y with the smallest x) item
    // in the selection

    for( EDA_ITEM* item : m_items )
    {
        SCH_ITEM* sch_item = dynamic_cast<SCH_ITEM*>( item );
        SCH_PIN*  pin = dynamic_cast<SCH_PIN*>( item );

        // Prefer connection points (which should remain on grid)

        if( ( sch_item && sch_item->IsConnectable() ) || pin )
            processItem( item, &topLeftConnectedItem, &topLeftConnectedPos );

        processItem( item, &topLeftItem, &topLeftPos );
    }

    if( topLeftConnectedItem )
        return topLeftConnectedItem;
    else
        return topLeftItem;
}


BOX2I SCH_SELECTION::GetBoundingBox() const
{
    BOX2I bbox;

    for( EDA_ITEM* item : m_items )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            // Quiet Coverity warning.  The LIB_SYMBOL field container is a Boost ptr_vector
            // so the exception is legit.
            try
            {
                bbox.Merge( static_cast<SCH_SYMBOL*>( item )->GetBoundingBox() );
            }
            catch( const boost::bad_pointer& )
            {
                wxFAIL_MSG( "Invalid pointer." );
            }
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            bbox.Merge( static_cast<SCH_SHEET*>( item )->GetBodyBoundingBox() );
        }
        else
        {
            bbox.Merge( item->GetBoundingBox() );
        }
    }

    return bbox;
}


const std::vector<KIGFX::VIEW_ITEM*> SCH_SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

    auto addItem =
            [&]( EDA_ITEM* item )
            {
                items.push_back( item );
            };

    for( EDA_ITEM* item : m_items )
        addItem( item );

    return items;
}
