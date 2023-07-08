/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <core/typeinfo.h>
#include <memory>
#include <view/view.h>
#include <view/view_rtree.h>
#include <view/wx_view_controls.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <tool/tool_manager.h>
#include <layer_ids.h>
#include <sch_screen.h>
#include <schematic.h>
#include <sch_base_frame.h>
#include <sch_edit_frame.h>

#include "sch_view.h"


namespace KIGFX {


SCH_VIEW::SCH_VIEW( bool aIsDynamic, SCH_BASE_FRAME* aFrame ) :
    VIEW( aIsDynamic )
{
    m_frame = aFrame;

    // Set m_boundary to define the max working area size. The default value is acceptable for
    // Pcbnew and Gerbview, but too large for Eeschema due to very different internal units.
    // A full size = 3 * MAX_PAGE_SIZE_MILS size allows a wide margin around the drawing-sheet.
    double max_size = schIUScale.MilsToIU( MAX_PAGE_SIZE_EESCHEMA_MILS ) * 3.0;
    m_boundary.SetOrigin( -max_size/4, -max_size/4 );
    m_boundary.SetSize( max_size, max_size );
}


SCH_VIEW::~SCH_VIEW()
{
}


void SCH_VIEW::Cleanup()
{
    Clear();
    m_drawingSheet.reset();
    m_preview.reset();
}


void SCH_VIEW::SetScale( double aScale, VECTOR2D aAnchor )
{
    VIEW::SetScale( aScale, aAnchor );

    // Redraw items whose rendering is dependent on zoom
    if( m_frame )
        m_frame->RefreshZoomDependentItems();
}


void SCH_VIEW::ResizeSheetWorkingArea( const SCH_SCREEN* aScreen )
{
    const PAGE_INFO& page_info = aScreen->GetPageSettings();
    double           max_size_x = page_info.GetWidthIU( schIUScale.IU_PER_MILS ) * 3.0;
    double           max_size_y = page_info.GetHeightIU( schIUScale.IU_PER_MILS ) * 3.0;
    m_boundary.SetOrigin( -max_size_x / 4, -max_size_y / 4 );
    m_boundary.SetSize( max_size_x, max_size_y );
}


void SCH_VIEW::DisplaySheet( const SCH_SCREEN *aScreen )
{
    for( SCH_ITEM* item : aScreen->Items() )
        Add( item );

    m_drawingSheet.reset( new DS_PROXY_VIEW_ITEM( schIUScale, &aScreen->GetPageSettings(),
                                                  &aScreen->Schematic()->Prj(),
                                                  &aScreen->GetTitleBlock(),
                                                  aScreen->Schematic()->GetProperties() ) );
    m_drawingSheet->SetPageNumber( TO_UTF8( aScreen->GetPageNumber() ) );
    m_drawingSheet->SetSheetCount( aScreen->GetPageCount() );
    m_drawingSheet->SetFileName( TO_UTF8( aScreen->GetFileName() ) );
    m_drawingSheet->SetColorLayer( LAYER_SCHEMATIC_DRAWINGSHEET );
    m_drawingSheet->SetPageBorderColorLayer( LAYER_SCHEMATIC_PAGE_LIMITS );
    m_drawingSheet->SetIsFirstPage( aScreen->GetVirtualPageNumber() == 1 );

    if( m_frame && m_frame->IsType( FRAME_SCH ) )
    {
        SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

        wxCHECK( editFrame, /* void */ );

        m_drawingSheet->SetSheetName( TO_UTF8( editFrame->GetScreenDesc() ) );
        m_drawingSheet->SetSheetPath( TO_UTF8( editFrame->GetFullScreenDesc() ) );
    }
    else
    {
        m_drawingSheet->SetSheetName( "" );
        m_drawingSheet->SetSheetPath( "" );
    }

    ResizeSheetWorkingArea( aScreen );

    Add( m_drawingSheet.get() );

    InitPreview();

    // Allow tools to add anything they require to the view (such as the selection VIEW_GROUP)
    if( m_frame && m_frame->GetToolManager() )
        m_frame->GetToolManager()->ResetTools( TOOL_BASE::REDRAW );
}


void SCH_VIEW::DisplaySymbol( LIB_SYMBOL* aSymbol )
{
    Clear();

    if( !aSymbol )
        return;

    // Draw the fields.
    for( LIB_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( item.Type() == LIB_FIELD_T )
        {
            LIB_FIELD* field = static_cast< LIB_FIELD* >( &item );

            wxCHECK2( field, continue );

            Add( &item );
        }
    }

    LIB_SYMBOL* drawnSymbol = aSymbol;

    // Draw the parent items if the symbol is inherited from another symbol.
    if( aSymbol->IsAlias() )
    {
        std::shared_ptr< LIB_SYMBOL > parent = aSymbol->GetParent().lock();

        wxCHECK( parent, /* void */ );

        drawnSymbol = parent.get();
    }

    for( LIB_ITEM& item : drawnSymbol->GetDrawItems() )
    {
        // Fields already drawn above.  (Besides, we don't want to show parent symbol fields as
        // users may be confused by shown fields that can not be edited.)
        if( item.Type() == LIB_FIELD_T )
            continue;

        Add( &item );
    }

    InitPreview();
}


void SCH_VIEW::ClearHiddenFlags()
{
    for( VIEW_ITEM* item : *m_allItems )
        Hide( item, false );
}


void SCH_VIEW::HideDrawingSheet()
{
    //    SetVisible( m_drawingSheet.get(), false );
}


}; // namespace KIGFX
