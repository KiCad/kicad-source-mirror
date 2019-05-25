/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pl_draw_panel_gal.h"
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/pl_actions.h>
#include <view/wx_view_controls.h>
#include <ws_view_item.h>
#include <ws_data_model.h>
#include <ws_painter.h>
#include <colors_design_settings.h>
#include <pl_editor_frame.h>
#include <gal/graphics_abstraction_layer.h>

#include <functional>
#include <tools/pl_selection_tool.h>

using namespace std::placeholders;


PL_DRAW_PANEL_GAL::PL_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                      const wxPoint& aPosition, const wxSize& aSize,
                                      KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::VIEW( true );
    m_view->SetGAL( m_gal );

    GetGAL()->SetWorldUnitLength( 1.0/IU_PER_MM /* 10 nm */ / 25.4 /* 1 inch in mm */ );

    m_painter.reset( new KIGFX::WS_PAINTER( m_gal ) );
    m_view->SetPainter( m_painter.get() );

    setDefaultLayerDeps();

    m_view->SetLayerVisible( LAYER_WORKSHEET, true );
    m_view->SetLayerVisible( LAYER_WORKSHEET_PAGE1, true );
    m_view->SetLayerVisible( LAYER_WORKSHEET_PAGEn, false );

    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );
}


PL_DRAW_PANEL_GAL::~PL_DRAW_PANEL_GAL()
{
}


void PL_DRAW_PANEL_GAL::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector<MSG_PANEL_ITEM>& aList )
{
}


void PL_DRAW_PANEL_GAL::DisplayWorksheet()
{
    PL_SELECTION_TOOL* selTool = m_edaFrame->GetToolManager()->GetTool<PL_SELECTION_TOOL>();
    WS_DATA_MODEL&     model = WS_DATA_MODEL::GetTheInstance();

    selTool->GetSelection().Clear();
    m_view->Clear();

    model.SetupDrawEnvironment( m_edaFrame->GetPageSettings(), Mils2iu( 1 ) );

    for( WS_DATA_ITEM* dataItem : model.GetItems() )
        dataItem->SyncDrawItems( nullptr, m_view );

    selTool->RebuildSelection();
}


bool PL_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );

    // The next onPaint event will call m_view->UpdateItems() that is very time consumming
    // after switching to opengl. Clearing m_view and rebuild it is much faster
    if( aGalType == GAL_TYPE_OPENGL )
        m_view->Clear();

    setDefaultLayerDeps();

    GetGAL()->SetWorldUnitLength( 1.0/IU_PER_MM /* 10 nm */ / 25.4 /* 1 inch in mm */ );

    return rv;
}


void PL_DRAW_PANEL_GAL::setDefaultLayerDeps()
{
    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, KIGFX::TARGET_NONCACHED );

    m_view->SetLayerDisplayOnly( LAYER_WORKSHEET );

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY );

    m_view->SetLayerTarget( LAYER_GP_OVERLAY, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY );
}


void PL_DRAW_PANEL_GAL::SetTopLayer( int aLayer )
{
    m_view->ClearTopLayers();
    m_view->SetTopLayer( aLayer );

    m_view->SetTopLayer( LAYER_SELECT_OVERLAY );

    m_view->SetTopLayer( LAYER_GP_OVERLAY );

    m_view->UpdateAllLayersOrder();
}

