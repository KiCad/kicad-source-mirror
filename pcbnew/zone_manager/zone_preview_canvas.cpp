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

#include "zone_preview_canvas.h"
#include <wx/gdicmn.h>
#include <wx/string.h>

#include <base_screen.h>
#include <base_units.h>
#include <class_draw_panel_gal.h>
#include <dialogs/dialog_configure_paths.h>
#include <eda_draw_frame.h>
#include <gal/graphics_abstraction_layer.h>
#include <id.h>
#include <kiface_base.h>
#include <tool/actions.h>
#include <tool/common_tools.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <board.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <pcb_painter.h>
#include <zone_manager/board_edges_bounding_item.h>
#include <kiplatform/ui.h>


enum DRAW_ORDER
{
    DRAW_ORDER_BOARD_BOUNDING,
    DRAW_ORDER_ZONE

};


wxColour getCanvasBackgroundColor()
{
    if( KIPLATFORM::UI::IsDarkTheme() )
        return wxColour( 0, 0, 0, 30 );

    return wxColour( 238, 243, 243 );
}


wxColour getBoundBoundingFillColor()
{
    if( KIPLATFORM::UI::IsDarkTheme() )
        return wxColour( 238, 243, 243, 60 );

    return wxColour( 84, 84, 84, 40 );
}


class ZONE_PAINTER : public KIGFX::PCB_PAINTER
{
public:
    using KIGFX::PCB_PAINTER::PCB_PAINTER;

    bool Draw( const KIGFX::VIEW_ITEM* aItem, int aLayer ) override
    {
        const BOARD_EDGES_BOUNDING_ITEM* item = dynamic_cast<const BOARD_EDGES_BOUNDING_ITEM*>( aItem );

        if( item )
        {
            m_gal->Save();
            m_gal->SetFillColor( getBoundBoundingFillColor() );
            m_gal->SetLineWidth( 0 );
            m_gal->SetIsFill( true );
            m_gal->SetIsStroke( false );
            m_gal->DrawRectangle( item->ViewBBox() );
            m_gal->Restore();
            return true;
        }

        return KIGFX::PCB_PAINTER::Draw( aItem, aLayer );
    }
};


ZONE_PREVIEW_CANVAS::ZONE_PREVIEW_CANVAS( BOARD* aPcb, ZONE* aZone, PCB_LAYER_ID aLayer, wxWindow* aParentWindow,
                                          KIGFX::GAL_DISPLAY_OPTIONS& aOptions, wxWindowID aWindowId,
                                          const wxPoint& aPosition, const wxSize& aSize, GAL_TYPE aGalType ) :
        PCB_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, wxDefaultSize, aOptions, aGalType ),
        m_pcb( aPcb ),
        m_pcb_bounding_box( std::make_unique<BOARD_EDGES_BOUNDING_ITEM>( aPcb->GetBoardEdgesBoundingBox() ) )
{
    m_view->UseDrawPriority( true );
    m_painter = std::make_unique<ZONE_PAINTER>( m_gal, FRAME_FOOTPRINT_PREVIEW );
    m_view->SetPainter( m_painter.get() );
    m_view->Add( m_pcb_bounding_box.get(), DRAW_ORDER_BOARD_BOUNDING );

    if( aZone )
        m_view->Add( aZone, DRAW_ORDER_ZONE );

    UpdateColors();
    m_painter->GetSettings()->SetBackgroundColor( getCanvasBackgroundColor() );

    // Load layer & elements visibility settings
    for( int i = 0; i < PCB_LAYER_ID_COUNT; ++i )
        m_view->SetLayerVisible( i, aLayer == i || Edge_Cuts == i );

    ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );

    Bind( wxEVT_SIZE,
          [this]( wxSizeEvent& aEvent )
          {
              ZoomFitScreen();
              aEvent.Skip();
          } );

    StartDrawing();
    RequestRefresh();
}


BOX2I ZONE_PREVIEW_CANVAS::GetBoardBoundingBox( bool aBoardEdgesOnly ) const
{
    BOX2I area = aBoardEdgesOnly ? m_pcb->GetBoardEdgesBoundingBox() : m_pcb->GetBoundingBox();

    if( area.GetWidth() == 0 && area.GetHeight() == 0 )
    {
        wxSize pageSize = GetPageSizeIU();
        area.SetOrigin( 0, 0 );
        area.SetEnd( pageSize.x, pageSize.y );
    }

    return area;
}


const BOX2I ZONE_PREVIEW_CANVAS::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    if( aIncludeAllVisible || !m_pcb->IsLayerVisible( Edge_Cuts ) )
        return GetBoardBoundingBox( false );
    else
        return GetBoardBoundingBox( true );
}


const wxSize ZONE_PREVIEW_CANVAS::GetPageSizeIU() const
{
    const VECTOR2D sizeIU = m_pcb->GetPageSettings().GetSizeIU( pcbIUScale.IU_PER_MILS );
    return wxSize( KiROUND( sizeIU.x ), KiROUND( sizeIU.y ) );
}


void ZONE_PREVIEW_CANVAS::ZoomFitScreen()
{
    BOX2I bBox = GetDocumentExtents( false );
    BOX2I defaultBox = GetDefaultViewBBox();

    m_view->SetScale( 1.0 );
    const wxSize clientSize = GetSize();
    VECTOR2D     screenSize = m_view->ToWorld( VECTOR2D( (double) clientSize.x, (double) clientSize.y ), false );

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
        bBox = defaultBox;

    VECTOR2D vsize = bBox.GetSize();
    double   scale = m_view->GetScale() / std::max( fabs( vsize.x / screenSize.x ), fabs( vsize.y / screenSize.y ) );

    if( !std::isfinite( scale ) )
    {
        m_view->SetCenter( VECTOR2D( 0, 0 ) );
        Refresh();
        return;
    }

    m_view->SetScale( scale );
    m_view->SetCenter( bBox.Centre() );
    RequestRefresh();
}
