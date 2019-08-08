/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 CERN
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
#include <view/view_rtree.h>
#include <view/wx_view_controls.h>
#include <ws_proxy_view_item.h>
#include <layers_id_colors_and_visibility.h>
#include <class_libentry.h>
#include <sch_sheet.h>
#include <sch_screen.h>
#include <sch_component.h>
#include <lib_pin.h>
#include <preview_items/selection_area.h>
#include <sch_edit_frame.h>

#include "sch_view.h"


namespace KIGFX {


SCH_VIEW::SCH_VIEW( bool aIsDynamic, SCH_BASE_FRAME* aFrame ) :
    VIEW( aIsDynamic )
{
    m_frame = aFrame;
    // Set m_boundary to define the max working area size. The default value
    // is acceptable for Pcbnew and Gerbview, but too large for Eeschema due to
    // very different internal units.
    // So we have to use a smaller value.
    // A full size = 3 * MAX_PAGE_SIZE_MILS size allows a wide margin
    // around the worksheet.
    double max_size = MAX_PAGE_SIZE_MILS * IU_PER_MILS * 3.0;
    m_boundary.SetOrigin( -max_size/4, -max_size/4 );
    m_boundary.SetSize( max_size, max_size );

    m_selectionArea.reset( new KIGFX::PREVIEW::SELECTION_AREA() );
    m_preview.reset( new KIGFX::VIEW_GROUP() );
}


SCH_VIEW::~SCH_VIEW()
{
}


void SCH_VIEW::SetScale( double aScale, VECTOR2D aAnchor )
{
    VIEW::SetScale( aScale, aAnchor );

    //Redraw selection halos since their width is dependant on zoom
    if( m_frame )
        m_frame->RefreshSelection();
}


void SCH_VIEW::ResizeSheetWorkingArea( SCH_SCREEN* aScreen )
{
    const PAGE_INFO& page_info = aScreen->GetPageSettings();
    // A full size = 3 * page size allows a wide margin around the worksheet.
    // This is useful to have a large working area.
    double max_size_x = page_info.GetWidthMils() * IU_PER_MILS * 2.0;
    double max_size_y = page_info.GetHeightMils() * IU_PER_MILS * 2.0;
    m_boundary.SetOrigin( -max_size_x /4, -max_size_y/4 );
    m_boundary.SetSize( max_size_x, max_size_y );
}


void SCH_VIEW::DisplaySheet( SCH_SCREEN *aScreen )
{
    for( auto item = aScreen->GetDrawItems(); item; item = item->Next() )
        Add( item );

    m_worksheet.reset( new KIGFX::WS_PROXY_VIEW_ITEM( 1, &aScreen->GetPageSettings(),
                                                      &aScreen->GetTitleBlock() ) );
    m_worksheet->SetSheetNumber( aScreen->m_ScreenNumber );
    m_worksheet->SetSheetCount( aScreen->m_NumberOfScreens );
    m_worksheet->SetFileName( TO_UTF8( aScreen->GetFileName() ) );

    if( m_frame && m_frame->IsType( FRAME_SCH ) )
        m_worksheet->SetSheetName( TO_UTF8( m_frame->GetScreenDesc() ) );
    else
        m_worksheet->SetSheetName( "" );

    ResizeSheetWorkingArea( aScreen );

    m_selectionArea.reset( new KIGFX::PREVIEW::SELECTION_AREA() );
    m_preview.reset( new KIGFX::VIEW_GROUP() );

    Add( m_worksheet.get() );
    Add( m_selectionArea.get() );
    Add( m_preview.get() );
}


void SCH_VIEW::DisplaySheet( SCH_SHEET* aSheet )
{
    DisplaySheet( aSheet->GetScreen() );
}


void SCH_VIEW::DisplayComponent( LIB_PART* aPart )
{
    Clear();

    if( !aPart )
        return;

    for( auto& item : aPart->GetDrawItems() )
        Add( &item );

    m_selectionArea.reset( new KIGFX::PREVIEW::SELECTION_AREA() );
    m_preview.reset( new KIGFX::VIEW_GROUP() );
    Add( m_selectionArea.get() );
    Add( m_preview.get() );
}


void SCH_VIEW::ClearPreview()
{
    m_preview->Clear();

    for( auto item : m_ownedItems )
        delete item;

    m_ownedItems.clear();
    Update( m_preview.get() );
}


void SCH_VIEW::AddToPreview( EDA_ITEM* aItem, bool aTakeOwnership )
{
    Hide( aItem, false );
    m_preview->Add( aItem );

    if( aTakeOwnership )
        m_ownedItems.push_back( aItem );

    SetVisible( m_preview.get(), true );
    Hide( m_preview.get(), false );
    Update( m_preview.get() );
}


void SCH_VIEW::ShowPreview( bool aShow )
{
    SetVisible( m_preview.get(), aShow );
}


void SCH_VIEW::ClearHiddenFlags()
{
    for( auto item : *m_allItems )
        Hide( item, false );
}


void SCH_VIEW::HideWorksheet()
{
    //    SetVisible( m_worksheet.get(), false );
}

void SCH_VIEW::HighlightItem( EDA_ITEM *aItem, LIB_PIN* aPin )
{
    if( aItem && aItem->Type() == SCH_COMPONENT_T && aPin )
    {
        static_cast<SCH_COMPONENT*>( aItem )->HighlightPin( aPin );
    }
    else if( aItem )
    {
        aItem->SetFlags( HIGHLIGHTED );
    }
    else
    {
        for( auto item : *m_allItems )
        {
            // Not all view items can be highlighted, only EDA_ITEMs
            // So clear flag of only EDA_ITEMs.
            EDA_ITEM* eitem = dynamic_cast<EDA_ITEM*>( item );

            if( eitem )
            {
                eitem->ClearFlags( HIGHLIGHTED );

                if( eitem->Type() == SCH_COMPONENT_T )
                {
                    // Items inside a component (pins, fields can be highlighted.
                    static_cast<SCH_COMPONENT*>( eitem )->ClearAllHighlightFlags();
                }
            }
        }
    }

    // ugly but I guess OK for the moment...
    UpdateAllItems( ALL );
}

}; // namespace KIGFX

