/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_zone_gal.h"
#include <pcb_track.h>
#include <pcb_marker.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

#include <base_screen.h>
#include <base_units.h>
#include <bitmaps.h>
#include <class_draw_panel_gal.h>
#include <dialogs/dialog_configure_paths.h>
#include <eda_draw_frame.h>
#include <gal/graphics_abstraction_layer.h>
#include <id.h>
#include <kiface_base.h>
#include <settings/app_settings.h>
#include <tool/actions.h>
#include <tool/common_tools.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <board.h>
#include "zone_manager_preference.h"
#include "zone_painter.h"
#include "zone_manager/board_edges_bounding_item.h"


enum DRAW_ORDER
{
    DRAW_ORDER_BOARD_BOUNDING,
    DRAW_ORDER_ZONE

};


PANEL_ZONE_GAL::PANEL_ZONE_GAL( BOARD* aPcb, wxWindow* aParentWindow,
                                KIGFX::GAL_DISPLAY_OPTIONS& aOptions, wxWindowID aWindowId,
                                const wxPoint& aPosition, const wxSize& aSize, GAL_TYPE aGalType ) :
        PCB_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, wxDefaultSize, aOptions,
                            aGalType ),
        m_pcb( aPcb ), m_layer( UNDEFINED_LAYER ),
        m_pcb_bounding_box(
                std::make_unique<BOARD_EDGES_BOUNDING_ITEM>( aPcb->GetBoardEdgesBoundingBox() ) ),
        m_zone( nullptr )
{
    m_view->UseDrawPriority( true );
    m_painter = std::make_unique<ZONE_PAINTER>( m_gal, FRAME_FOOTPRINT_PREVIEW );
    m_view->SetPainter( m_painter.get() );
    m_view->Add( m_pcb_bounding_box.get(), DRAW_ORDER_BOARD_BOUNDING );
    UpdateColors();
    ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    StartDrawing();
    m_painter->GetSettings()->SetBackgroundColor(
            ZONE_MANAGER_PREFERENCE::GetCanvasBackgroundColor() );
}


PANEL_ZONE_GAL::~PANEL_ZONE_GAL()
{
}

BOX2I PANEL_ZONE_GAL::GetBoardBoundingBox( bool aBoardEdgesOnly ) const
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

const BOX2I PANEL_ZONE_GAL::GetDocumentExtents( bool aIncludeAllVisible ) const
{
    if( aIncludeAllVisible || !m_pcb->IsLayerVisible( Edge_Cuts ) )
        return GetBoardBoundingBox( false );
    else
        return GetBoardBoundingBox( true );
}

bool PANEL_ZONE_GAL::OnLayerSelected( int aLayer )
{
    if( m_layer == aLayer )
        return false;

    m_layer = aLayer;
    // Load layer & elements visibility settings

    for( int i = 0; i < PCB_LAYER_ID_COUNT; ++i )
        m_view->SetLayerVisible( i, m_layer == i || Edge_Cuts == i );

    Refresh();
    return true;
}


void PANEL_ZONE_GAL::ActivateSelectedZone( ZONE* aZone )
{
    if( m_zone )
        m_view->Remove( m_zone );

    if( aZone )
    {
        m_view->Add( aZone, DRAW_ORDER_ZONE );

        if( !OnLayerSelected( aZone->GetFirstLayer() ) )
            Refresh();
    }
    else
    {
        Refresh();
    }

    m_zone = aZone;
}

const wxSize PANEL_ZONE_GAL::GetPageSizeIU() const
{
    const VECTOR2D sizeIU = m_pcb->GetPageSettings().GetSizeIU( pcbIUScale.IU_PER_MILS );
    return wxSize( sizeIU.x, sizeIU.y );
}

void PANEL_ZONE_GAL::ZoomFitScreen()
{
    BOX2I bBox = GetDocumentExtents();
    BOX2I defaultBox = GetDefaultViewBBox();

    m_view->SetScale( 1.0 );
    const wxSize clientSize = GetSize();
    VECTOR2D     screenSize = m_view->ToWorld(
            VECTOR2D( static_cast<double>( clientSize.x ), static_cast<double>( clientSize.y ) ),
            false );

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
        bBox = defaultBox;

    VECTOR2D vsize = bBox.GetSize();
    double   scale = m_view->GetScale()
                   / std::max( fabs( vsize.x / screenSize.x ), fabs( vsize.y / screenSize.y ) );

    if( !std::isfinite( scale ) )
    {
        m_view->SetCenter( VECTOR2D( 0, 0 ) );
        Refresh();
        return;
    }

    m_view->SetScale( scale );
    m_view->SetCenter( bBox.Centre() );
    Refresh();
}
