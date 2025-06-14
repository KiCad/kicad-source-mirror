/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gerbview_draw_panel_gal.h"
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <gerbview_painter.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <zoom_defines.h>

#include <gerbview_frame.h>
#include <gal/graphics_abstraction_layer.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <gerber_file_image.h>
#include <gerber_file_image_list.h>

#include <functional>
#include <memory>

using namespace std::placeholders;


GERBVIEW_DRAW_PANEL_GAL::GERBVIEW_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                                  const wxPoint& aPosition, const wxSize& aSize,
                                                  KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                                                  GAL_TYPE aGalType ) :
EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::VIEW();
    m_view->SetGAL( m_gal );
    GetGAL()->SetWorldUnitLength( 1.0/gerbIUScale.IU_PER_MM /* 10 nm */ / 25.4 /* 1 inch in mm */ );

    m_painter = std::make_unique<KIGFX::GERBVIEW_PAINTER>( m_gal );
    m_view->SetPainter( m_painter.get() );

    // This fixes the zoom in and zoom out limits:
    m_view->SetScaleLimits( ZOOM_MAX_LIMIT_GERBVIEW, ZOOM_MIN_LIMIT_GERBVIEW );

    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    setDefaultLayerDeps();

    COLOR_SETTINGS* cs = ::GetColorSettings( DEFAULT_THEME );

    if( GERBVIEW_FRAME* frame = dynamic_cast<GERBVIEW_FRAME*>( GetParentEDAFrame() ) )
        cs = frame->GetColorSettings();

    auto renderSettings = static_cast<KIGFX::GERBVIEW_RENDER_SETTINGS*>( m_painter->GetSettings() );
    renderSettings->LoadColors( cs );
}


GERBVIEW_DRAW_PANEL_GAL::~GERBVIEW_DRAW_PANEL_GAL()
{
}


void GERBVIEW_DRAW_PANEL_GAL::SetHighContrastLayer( int aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );

    rSettings->ClearHighContrastLayers();
    rSettings->SetLayerIsHighContrast( aLayer );
    rSettings->SetLayerIsHighContrast( GERBER_DCODE_LAYER( aLayer ) );

    m_view->UpdateAllLayersColor();
}


void GERBVIEW_DRAW_PANEL_GAL::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                               std::vector<MSG_PANEL_ITEM>& aList )
{
}


void GERBVIEW_DRAW_PANEL_GAL::OnShow()
{
    GERBVIEW_FRAME* frame = dynamic_cast<GERBVIEW_FRAME*>( GetParentEDAFrame() );

    if( frame )
        SetTopLayer( frame->GetActiveLayer() );

    m_view->RecacheAllItems();
}


bool GERBVIEW_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );

    // The next onPaint event will call m_view->UpdateItems() that is very time consuming
    // after switching to opengl. Clearing m_view and rebuild it is much faster
    if( aGalType == GAL_TYPE_OPENGL )
    {
        GERBVIEW_FRAME* frame = dynamic_cast<GERBVIEW_FRAME*>( GetParentEDAFrame() );

        if( frame )
        {
            m_view->Clear();

            for( int layer = GERBER_DRAWLAYERS_COUNT-1; layer>= 0; --layer )
            {
                GERBER_FILE_IMAGE* gerber = frame->GetImagesList()->GetGbrImage( layer );

                if( gerber == nullptr )    // Graphic layer not yet used
                    continue;

                for( GERBER_DRAW_ITEM* item : gerber->GetItems() )
                {
                    m_view->Add (item );
                }
            }
        }
    }

    setDefaultLayerDeps();

    GetGAL()->SetWorldUnitLength( 1.0/gerbIUScale.IU_PER_MM /* 10 nm */ / 25.4 /* 1 inch in mm */ );

    return rv;
}


void GERBVIEW_DRAW_PANEL_GAL::setDefaultLayerDeps()
{
    // caching makes no sense for Cairo and other software renderers
    auto target = m_backend == GAL_TYPE_OPENGL ? KIGFX::TARGET_CACHED : KIGFX::TARGET_NONCACHED;

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, target );

    m_view->SetLayerDisplayOnly( LAYER_DCODES );
    m_view->SetLayerDisplayOnly( LAYER_NEGATIVE_OBJECTS );
    m_view->SetLayerDisplayOnly( LAYER_GERBVIEW_GRID );
    m_view->SetLayerDisplayOnly( LAYER_GERBVIEW_AXES );
    m_view->SetLayerDisplayOnly( LAYER_GERBVIEW_BACKGROUND );
    m_view->SetLayerDisplayOnly( LAYER_DRAWINGSHEET );

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY );

    m_view->SetLayerTarget( LAYER_GP_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY );
}


void GERBVIEW_DRAW_PANEL_GAL::SetDrawingSheet( DS_PROXY_VIEW_ITEM* aDrawingSheet )
{
    m_drawingSheet.reset( aDrawingSheet );
    m_view->Add( m_drawingSheet.get() );
}


void GERBVIEW_DRAW_PANEL_GAL::SetTopLayer( int aLayer )
{
    m_view->ClearTopLayers();

    for( int i = 0; i < GERBER_DRAWLAYERS_COUNT; ++i )
    {
        m_view->SetLayerOrder( GERBER_DCODE_LAYER( GERBER_DRAW_LAYER( i ) ),
                               GERBER_DRAW_LAYER( 2 * i ) );
        m_view->SetLayerOrder( GERBER_DRAW_LAYER( i ), GERBER_DRAW_LAYER( ( 2 * i ) + 1 ) );
    }

    m_view->SetTopLayer( aLayer );

    // Move DCODE layer to the top
    m_view->SetTopLayer( GERBER_DCODE_LAYER( aLayer ) );

    m_view->SetTopLayer( LAYER_SELECT_OVERLAY );

    m_view->SetTopLayer( LAYER_GP_OVERLAY );

    m_view->UpdateAllLayersOrder();
}


BOX2I GERBVIEW_DRAW_PANEL_GAL::GetDefaultViewBBox() const
{
    // Even in Gervbview, LAYER_DRAWINGSHEET controls the visibility of the drawingsheet
    if( m_drawingSheet && m_view->IsLayerVisible( LAYER_DRAWINGSHEET ) )
        return m_drawingSheet->ViewBBox();

    return BOX2I();
}
