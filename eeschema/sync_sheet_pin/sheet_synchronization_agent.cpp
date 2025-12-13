/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#include "sheet_synchronization_agent.h"
#include "sheet_synchronization_item.h"

#include <sch_base_frame.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_sheet_pin.h>

SHEET_SYNCHRONIZATION_AGENT::SHEET_SYNCHRONIZATION_AGENT( DO_MODIFY_ITEM  aDoModify,
                                                          DO_DELETE_ITEM  aNotifyItemChange,
                                                          DO_PLACE_ITEM   aPlaceItem,
                                                          TOOL_MANAGER*   aToolManager,
                                                          SCH_EDIT_FRAME* a_frame ) :
        m_doModify( std::move( aDoModify ) ),
        m_doDelete( std::move( aNotifyItemChange ) ),
        m_doPlaceItem( std::move( aPlaceItem ) )
{
}


SHEET_SYNCHRONIZATION_AGENT::~SHEET_SYNCHRONIZATION_AGENT() = default;


void SHEET_SYNCHRONIZATION_AGENT::ModifyItem( SHEET_SYNCHRONIZATION_ITEM&  aItem,
                                              std::function<void()> const& aDoModify,
                                              SCH_SHEET_PATH const&        aPath )
{
    return ModifyItem( aItem.GetItem(), aDoModify, aPath, aItem.GetKind() );
}


void SHEET_SYNCHRONIZATION_AGENT::ModifyItem( SCH_ITEM*                       sch_item,
                                              std::function<void()> const&    aDoModify,
                                              const SCH_SHEET_PATH&           aPath,
                                              SHEET_SYNCHRONIZATION_ITEM_KIND aKind )
{
    if( !aDoModify )
        return;

    switch( aKind )
    {
    case SHEET_SYNCHRONIZATION_ITEM_KIND::HIERLABEL:
        m_doModify( sch_item, aPath, aDoModify );
        break;

    case SHEET_SYNCHRONIZATION_ITEM_KIND::SHEET_PIN:
    {
        SCH_SHEET_PATH path_cp = aPath;
        path_cp.pop_back();
        m_doModify( sch_item, path_cp, aDoModify );
        break;
    }

    case SHEET_SYNCHRONIZATION_ITEM_KIND::HIERLABEL_AND_SHEET_PIN:
        break;
    }
}


void SHEET_SYNCHRONIZATION_AGENT::RemoveItem( SHEET_SYNCHRONIZATION_ITEM& aItem, SCH_SHEET* aSheet,
                                              const SCH_SHEET_PATH& aPath )
{
    if( !aSheet )
        return;

    switch( aItem.GetKind() )
    {
    case SHEET_SYNCHRONIZATION_ITEM_KIND::HIERLABEL:
        m_doDelete( aItem.GetItem(), aPath );
        break;

    case SHEET_SYNCHRONIZATION_ITEM_KIND::SHEET_PIN:
    {
        SCH_SHEET_PATH path_cp = aPath;
        path_cp.pop_back();
        m_doDelete( aItem.GetItem(), std::move( path_cp ) );
        break;
    }

    case SHEET_SYNCHRONIZATION_ITEM_KIND::HIERLABEL_AND_SHEET_PIN:
        break;
    }
}


void SHEET_SYNCHRONIZATION_AGENT::PlaceSheetPin( SCH_SHEET* aSheet, SCH_SHEET_PATH const& aPath,
                                                 std::set<EDA_ITEM*> const& aLabels )
{
    SCH_SHEET_PATH cp = aPath;
    cp.pop_back();
    m_doPlaceItem( aSheet, cp, SHEET_SYNCHRONIZATION_PLACEMENT::PLACE_SHEET_PIN, aLabels );
}


void SHEET_SYNCHRONIZATION_AGENT::PlaceHieraLable( SCH_SHEET* aSheet, SCH_SHEET_PATH const& aPath,
                                                   std::set<EDA_ITEM*> const& aPins )
{
    m_doPlaceItem( aSheet, aPath, SHEET_SYNCHRONIZATION_PLACEMENT::PLACE_HIERLABEL, aPins );
}
