/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2016 Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <memory>
#include <utility>

#include "pcbnew_settings.h"
#include <advanced_config.h>
#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_dimension.h>
#include <dpi_scaling_common.h>
#include <eda_draw_frame.h>
#include <footprint_preview_panel.h>
#include <footprint_library_adapter.h>
#include <gal/graphics_abstraction_layer.h>
#include <kiway.h>
#include <math/box2.h>
#include <pcb_painter.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <wx/stattext.h>
#include <dialog_shim.h>
#include <project_pcb.h>


FOOTPRINT_PREVIEW_PANEL::FOOTPRINT_PREVIEW_PANEL( KIWAY* aKiway, wxWindow* aParent,
                                                  UNITS_PROVIDER* aUnitsProvider,
                                                  std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> aOpts,
                                                  GAL_TYPE aGalType ) :
        PCB_DRAW_PANEL_GAL( aParent, -1, wxPoint( 0, 0 ), wxSize( 200, 200 ), *aOpts, aGalType ),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::PANEL ),
        m_displayOptions( std::move( aOpts ) )
{
    SetStealsFocus( false );
    ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    EnableScrolling( false, false );    // otherwise Zoom Auto disables GAL canvas

    m_userUnits = aUnitsProvider->GetUserUnits();

    m_dummyBoard = std::make_unique<BOARD>();
    m_dummyBoard->SetUserUnits( m_userUnits );
    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );
    UpdateColors();
    SyncLayersVisibility( m_dummyBoard.get() );

    Raise();
    Show( true );
    StartDrawing();
}


FOOTPRINT_PREVIEW_PANEL::~FOOTPRINT_PREVIEW_PANEL( )
{
    ClearViewAndData();
}


void FOOTPRINT_PREVIEW_PANEL::ClearViewAndData()
{
    m_dummyBoard->DetachAllFootprints();

    if( m_currentFootprint )
        GetView()->Remove( m_currentFootprint.get() );

    if( m_otherFootprint )
        GetView()->Remove( m_otherFootprint.get() );

    GetView()->Clear();

    m_currentFootprint = nullptr;
    m_otherFootprint = nullptr;
}


const COLOR4D& FOOTPRINT_PREVIEW_PANEL::GetBackgroundColor() const
{
    KIGFX::PAINTER* painter = GetView()->GetPainter();
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );

    return settings->GetBackgroundColor();
}


const COLOR4D& FOOTPRINT_PREVIEW_PANEL::GetForegroundColor() const
{
    KIGFX::PAINTER* painter = GetView()->GetPainter();
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );

    return settings->GetLayerColor( F_Fab );
}


void FOOTPRINT_PREVIEW_PANEL::renderFootprint( std::shared_ptr<FOOTPRINT> aFootprint )
{
    m_dummyBoard->Add( aFootprint.get() );

    INSPECTOR_FUNC inspector =
            [&]( EDA_ITEM* descendant, void* aTestData )
            {
                static_cast<PCB_DIMENSION_BASE*>( descendant )->UpdateUnits();
                return INSPECT_RESULT::CONTINUE;
            };

    aFootprint->Visit( inspector, nullptr, { PCB_DIM_LEADER_T,
                                             PCB_DIM_ALIGNED_T,
                                             PCB_DIM_ORTHOGONAL_T,
                                             PCB_DIM_CENTER_T,
                                             PCB_DIM_RADIAL_T } );

    for( PAD* pad : aFootprint->Pads() )
        pad->SetPinFunction( m_pinFunctions[ pad->GetNumber() ] );

    // Ensure we are not using the high contrast mode to display the selected footprint
    KIGFX::PAINTER* painter = GetView()->GetPainter();
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;

    GetView()->Add( aFootprint.get() );
    GetView()->SetVisible( aFootprint.get(), true );
    GetView()->Update( aFootprint.get(), KIGFX::ALL );
}


void FOOTPRINT_PREVIEW_PANEL::fitToCurrentFootprint()
{
    bool  includeText = m_currentFootprint->TextOnly();
    BOX2I bbox = m_currentFootprint->GetBoundingBox( includeText );

    if( bbox.GetSize().x > 0 && bbox.GetSize().y > 0 )
    {
        // Autozoom
        GetView()->SetViewport( BOX2D( bbox.GetOrigin(), bbox.GetSize() ) );

        // Add a margin
        GetView()->SetScale( GetView()->GetScale() * 0.7 );

        Refresh();
    }
}


bool FOOTPRINT_PREVIEW_PANEL::DisplayFootprint( const LIB_ID& aFPID )
{
    m_dummyBoard->DetachAllFootprints();

    if( m_currentFootprint )
        GetView()->Remove( m_currentFootprint.get() );

    GetView()->Clear();

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    try
    {
        m_currentFootprint.reset( adapter->LoadFootprint( aFPID.GetLibNickname(), aFPID.GetLibItemName(), false ) );
    }
    catch( ... )
    {
        m_currentFootprint.reset();
    }

    if( m_currentFootprint )
    {
        renderFootprint( m_currentFootprint );
        fitToCurrentFootprint();
    }

    ForceRefresh();

    return m_currentFootprint != nullptr;
}


void FOOTPRINT_PREVIEW_PANEL::DisplayFootprints( std::shared_ptr<FOOTPRINT> aFootprintA,
                                                 std::shared_ptr<FOOTPRINT> aFootprintB )
{
    m_dummyBoard->DetachAllFootprints();

    if( m_currentFootprint )
        GetView()->Remove( m_currentFootprint.get() );

    if( m_otherFootprint )
        GetView()->Remove( m_otherFootprint.get() );

    GetView()->Clear();

    m_currentFootprint = aFootprintA;
    m_otherFootprint = aFootprintB;

    if( m_currentFootprint )
    {
        wxASSERT( m_otherFootprint );

        renderFootprint( m_currentFootprint );
        renderFootprint( m_otherFootprint );
        fitToCurrentFootprint();
    }

    Layout();
    Show();
}


void FOOTPRINT_PREVIEW_PANEL::RefreshAll()
{
    GetView()->UpdateAllItems( KIGFX::REPAINT );
    ForceRefresh();
}


FOOTPRINT_PREVIEW_PANEL* FOOTPRINT_PREVIEW_PANEL::New( KIWAY* aKiway, wxWindow* aParent,
                                                       UNITS_PROVIDER* aUnitsProvider )
{
    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();
    std::unique_ptr<GAL_DISPLAY_OPTIONS_IMPL> gal_opts;

    gal_opts = std::make_unique<GAL_DISPLAY_OPTIONS_IMPL>();
    gal_opts->ReadConfig( *commonSettings, cfg->m_Window, aParent );

    auto galType = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( commonSettings->m_Graphics.canvas_type );
    FOOTPRINT_PREVIEW_PANEL* panel = new FOOTPRINT_PREVIEW_PANEL( aKiway, aParent, aUnitsProvider,
                                                                  std::move( gal_opts ), galType );

    panel->UpdateColors();

    const GRID_SETTINGS& gridCfg = cfg->m_Window.grid;

    panel->GetGAL()->SetGridVisibility( gridCfg.show );

    //Bounds checking cannot include number of elements as an index!
    int    gridIdx = std::clamp( gridCfg.last_size_idx, 0, (int) gridCfg.grids.size() - 1 );
    double gridSizeX = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS,
                                                                  gridCfg.grids[gridIdx].x );
    double gridSizeY = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS,
                                                                  gridCfg.grids[gridIdx].y );
    panel->GetGAL()->SetGridSize( VECTOR2D( gridSizeX, gridSizeY ) );

    auto painter = static_cast<KIGFX::PCB_PAINTER*>( panel->GetView()->GetPainter() );
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->SetHighlight( false );
    settings->SetNetColorMode( NET_COLOR_MODE::OFF );

    return panel;
}
