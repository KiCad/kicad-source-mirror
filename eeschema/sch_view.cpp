/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <memory>
#include <view/view.h>
#include <view/view_group.h>
#include <view/wx_view_controls.h>
#include <worksheet_viewitem.h>
#include <layers_id_colors_and_visibility.h>
#include <class_libentry.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <preview_items/selection_area.h>

#include "sch_view.h"


namespace KIGFX {


SCH_VIEW::SCH_VIEW( bool aIsDynamic ) :
    VIEW( aIsDynamic )
{
}


SCH_VIEW::~SCH_VIEW()
{
}


static const LAYER_NUM SCH_LAYER_ORDER[] =
{
    LAYER_GP_OVERLAY,
    LAYER_DRC,
    LAYER_WORKSHEET
};


void SCH_VIEW::DisplaySheet( SCH_SCREEN *aSheet )
{

    for( auto item = aSheet->GetDrawItems(); item; item = item->Next() )
        Add( item );

    m_worksheet.reset( new KIGFX::WORKSHEET_VIEWITEM( 1, &aSheet->GetPageSettings(),
                                                      &aSheet->GetTitleBlock() ) );
    m_selectionArea.reset( new KIGFX::PREVIEW::SELECTION_AREA( ) );
    m_preview.reset( new KIGFX::VIEW_GROUP () );

    Add( m_worksheet.get() );
    Add( m_selectionArea.get() );
    Add( m_preview.get() );
}


void SCH_VIEW::DisplaySheet( SCH_SHEET *aSheet )
{
    DisplaySheet( aSheet->GetScreen() );
}


void SCH_VIEW::DisplayComponent( LIB_PART *aPart )
{
    Clear();

    int fgPriority = INT_MAX / 2;
    int bgPriority = 0;

    for ( auto &item : aPart->GetDrawItems() )
    {
        //printf("-- ADD %p\n", &item );

        // Apply a z-order heuristic (because we don't yet let the user edit it):
        // push body-filled objects to the back.
        if( item.GetFillMode() == FILLED_WITH_BG_BODYCOLOR )
            Add( &item, bgPriority++ );
        else
            Add( &item, fgPriority++ );
    }

    m_selectionArea.reset( new KIGFX::PREVIEW::SELECTION_AREA( ) );
    m_preview.reset( new KIGFX::VIEW_GROUP () );
    //printf("Display-screen\n");
    Add( m_selectionArea.get() );
    Add( m_preview.get() );
}


void SCH_VIEW::ClearPreview()
{
    m_preview->Clear();

    for( auto item : m_previewItems )
        delete item;

    m_previewItems.clear();
    Update(m_preview.get());
}


void SCH_VIEW::AddToPreview( EDA_ITEM *aItem, bool owned )
{
    m_preview->Add( aItem );

    if( owned )
        m_previewItems.push_back( aItem );

    SetVisible( m_preview.get(), true );
    Hide( m_preview.get(), false );
    Update( m_preview.get() );
}


void SCH_VIEW::ShowSelectionArea( bool aShow  )
{
    if( aShow )
    {
        // Reset seleciton area so the previous one doesn't flash before the first
        // mouse move updates it
        m_selectionArea->SetOrigin( VECTOR2I() );
        m_selectionArea->SetEnd( VECTOR2I() );
    }

    SetVisible( m_selectionArea.get(), aShow );
}


void SCH_VIEW::ShowPreview( bool aShow  )
{
    SetVisible( m_preview.get(), aShow );
}


void SCH_VIEW::ClearHiddenFlags()
{
    for( auto item : m_allItems )
        Hide ( item, false );
}


void SCH_VIEW::HideWorksheet()
{
//    SetVisible( m_worksheet.get(), false );
}


};

