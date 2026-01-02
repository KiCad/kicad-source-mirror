/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>

#include "sch_view.h"


namespace KIGFX {


SCH_VIEW::SCH_VIEW( SCH_BASE_FRAME* aFrame ) :
    VIEW()
{
    m_frame = aFrame;
}


SCH_VIEW::~SCH_VIEW()
{
}


void SCH_VIEW::Update( const KIGFX::VIEW_ITEM* aItem, int aUpdateFlags ) const
{
    if( aItem->IsSCH_ITEM() )
    {
        // The equivalent function in PCB_VIEW doesn't need to do this, but
        // that's only because RunOnChildren's constness is misleading in the PCB editor;
        // it doesn't modify target item, but our uses of it modify its children.
        SCH_ITEM* schItem = const_cast<SCH_ITEM*>( static_cast<const SCH_ITEM*>( aItem ) );

        if( schItem->Type() == SCH_TABLECELL_T )
        {
            VIEW::Update( schItem->GetParent() );
        }
        else
        {
            schItem->RunOnChildren(
                    [this, aUpdateFlags]( SCH_ITEM* child )
                    {
                        VIEW::Update( child, aUpdateFlags );
                    },
                    RECURSE_MODE::RECURSE );
        }
    }

    VIEW::Update( aItem, aUpdateFlags );
}


void SCH_VIEW::Update( const KIGFX::VIEW_ITEM* aItem ) const
{
    SCH_VIEW::Update( aItem, KIGFX::ALL );
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


void SCH_VIEW::DisplaySheet( const SCH_SCREEN *aScreen )
{
    for( SCH_ITEM* item : aScreen->Items() )
        Add( item );

    m_drawingSheet.reset( new DS_PROXY_VIEW_ITEM( schIUScale, &aScreen->GetPageSettings(),
                                                  &aScreen->Schematic()->Project(),
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
    for( SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( item.Type() == SCH_FIELD_T )
            Add( &item );
    }

    LIB_SYMBOL* drawnSymbol = aSymbol;

    // Draw the parent items if the symbol is inherited from another symbol.
    if( aSymbol->IsDerived() )
    {
        if( std::shared_ptr< LIB_SYMBOL > parent = aSymbol->GetRootSymbol() )
            drawnSymbol = parent.get();
        else
        {
            wxCHECK( false, /* void */ );
        }
    }

    for( SCH_ITEM& item : drawnSymbol->GetDrawItems() )
    {
        // Fields already drawn above.  (Besides, we don't want to show parent symbol fields as
        // users may be confused by shown fields that can not be edited.)
        if( item.Type() == SCH_FIELD_T )
            continue;

        Add( &item );
    }

    InitPreview();
}


void SCH_VIEW::ClearHiddenFlags()
{
    for( VIEW_ITEM* item : *m_allItems )
    {
        if( !item )
            continue;

        Hide( item, false );
    }
}


void SCH_VIEW::HideDrawingSheet()
{
    //    SetVisible( m_drawingSheet.get(), false );
}


}; // namespace KIGFX
