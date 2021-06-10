/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <tools/ee_selection.h>
#include <sch_item.h>
#include <sch_reference_list.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_sheet.h>


EE_SELECTION::EE_SELECTION( SCH_SCREEN* aScreen ) :
    SELECTION()
{
    m_screen = aScreen;
}


EDA_ITEM* EE_SELECTION::GetTopLeftItem( bool onlyModules ) const
{
    EDA_ITEM* topLeftItem = nullptr;
    wxPoint   topLeftPos;

    // find the leftmost (smallest x coord) and highest (smallest y with the smallest x) item
    // in the selection
    for( EDA_ITEM* item : m_items )
    {
        wxPoint pos = item->GetPosition();

        if( ( topLeftItem == nullptr )
            || ( pos.x < topLeftPos.x )
            || ( topLeftPos.x == pos.x && pos.y < topLeftPos.y ) )
        {
            topLeftItem = item;
            topLeftPos = pos;
        }
    }

    return topLeftItem;
}


EDA_RECT EE_SELECTION::GetBoundingBox() const
{
    EDA_RECT bbox;

    for( EDA_ITEM* item : m_items )
    {
        if( item->Type() == SCH_SYMBOL_T )
            bbox.Merge( static_cast<SCH_SYMBOL*>( item )->GetBoundingBox( false ) );
        else if( item->Type() == SCH_SHEET_T )
            bbox.Merge( static_cast<SCH_SHEET*>( item )->GetBodyBoundingBox() );
        else
            bbox.Merge( item->GetBoundingBox() );
    }

    return bbox;
}


void EE_SELECTION::GetSymbols( SCH_REFERENCE_LIST&   aReferences,
                               const SCH_SHEET_PATH& aSelectionPath,
                               bool aIncludePowerSymbols,
                               bool aForceIncludeOrphanSymbols )
{
    for( EDA_ITEM* item : Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        aSelectionPath.AppendSymbol( aReferences, symbol, aIncludePowerSymbols,
                                    aForceIncludeOrphanSymbols );
    }
}


void EE_SELECTION::GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP& aRefList,
                                        const SCH_SHEET_PATH& aSelectionPath,
                                        bool aIncludePowerSymbols )
{
    for( EDA_ITEM* item : Items() )
    {
        if( item->Type() != SCH_SYMBOL_T )
            continue;

        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        aSelectionPath.AppendMultiUnitSymbol( aRefList, symbol, aIncludePowerSymbols );
    }
}


bool EE_SELECTION::AllItemsHaveLineStroke() const
{
    for( const EDA_ITEM* item : m_items )
    {
        const SCH_ITEM* schItem = dynamic_cast<const SCH_ITEM*>( item );

        if( !schItem || !schItem->HasLineStroke() )
            return false;
    }

    return true;
}
