/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <pcb_dimension.h>
#include <eda_draw_frame.h>
#include <footprint_preview_panel.h>
#include <fp_lib_table.h>
#include <kiway.h>
#include <math/box2.h>
#include <pcb_painter.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_edit_frame.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <view/view.h>
#include <wx/stattext.h>
#include <zoom_defines.h>
#include <dialog_shim.h>

FOOTPRINT_PREVIEW_PANEL::FOOTPRINT_PREVIEW_PANEL( KIWAY* aKiway, wxWindow* aParent,
                                                  std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> aOpts,
                                                  GAL_TYPE aGalType ) :
        PCB_DRAW_PANEL_GAL( aParent, -1, wxPoint( 0, 0 ), wxSize( 200, 200 ), *aOpts, aGalType ),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::PANEL ),
        m_displayOptions( std::move( aOpts ) ),
        m_currentFootprint( nullptr )
{
    SetStealsFocus( false );
    ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    EnableScrolling( false, false );    // otherwise Zoom Auto disables GAL canvas

    wxWindow* topLevelParent = aParent;

    while( topLevelParent && !topLevelParent->IsTopLevel() )
        topLevelParent = topLevelParent->GetParent();

    if( topLevelParent )
    {
        if( DIALOG_SHIM* parentDlg = dynamic_cast<DIALOG_SHIM*>( topLevelParent ) )
            m_userUnits = parentDlg->GetUserUnits();
        else if( UNITS_PROVIDER* parentFrame = dynamic_cast<UNITS_PROVIDER*>( topLevelParent ) )
            m_userUnits = parentFrame->GetUserUnits();
    }

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
    if( m_currentFootprint )
    {
        GetView()->Remove( m_currentFootprint.get() );
        GetView()->Clear();
        m_currentFootprint->SetParent( nullptr );
    }
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
    if( m_currentFootprint )
    {
        GetView()->Remove( m_currentFootprint.get() );
        GetView()->Clear();
        m_currentFootprint->SetParent( nullptr );
    }

    m_currentFootprint = aFootprint;

    if( !m_currentFootprint )
        return;

    m_currentFootprint->SetParent( m_dummyBoard.get() );

    INSPECTOR_FUNC inspector =
            [&]( EDA_ITEM* descendant, void* aTestData )
            {
                static_cast<PCB_DIMENSION_BASE*>( descendant )->UpdateUnits();
                return INSPECT_RESULT::CONTINUE;
            };

    m_currentFootprint->Visit( inspector, nullptr, { PCB_FP_DIM_LEADER_T,
                                                     PCB_FP_DIM_ORTHOGONAL_T,
                                                     PCB_FP_DIM_CENTER_T,
                                                     PCB_FP_DIM_RADIAL_T } );

    // Ensure we are not using the high contrast mode to display the selected footprint
    KIGFX::PAINTER* painter = GetView()->GetPainter();
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;

    GetView()->Add( m_currentFootprint.get() );
    GetView()->SetVisible( m_currentFootprint.get(), true );
    GetView()->Update( m_currentFootprint.get(), KIGFX::ALL );

    BOX2I bbox = m_currentFootprint->ViewBBox();
    bbox.Merge( m_currentFootprint->Value().ViewBBox() );
    bbox.Merge( m_currentFootprint->Reference().ViewBBox() );

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
    FP_LIB_TABLE* fptbl = Prj().PcbFootprintLibs();

    try
    {
        const FOOTPRINT* fp = fptbl->GetEnumeratedFootprint( aFPID.GetLibNickname(),
                                                             aFPID.GetLibItemName() );

        if( fp )
            m_currentFootprint.reset( static_cast<FOOTPRINT*>( fp->Duplicate() ) );
        else
            m_currentFootprint.reset();
    }
    catch( ... )
    {
        m_currentFootprint.reset();
    }

    renderFootprint( m_currentFootprint );
    Refresh();

    return m_currentFootprint != nullptr;
}


wxWindow* FOOTPRINT_PREVIEW_PANEL::GetWindow()
{
    return static_cast<wxWindow*>( this );
}


FOOTPRINT_PREVIEW_PANEL* FOOTPRINT_PREVIEW_PANEL::New( KIWAY* aKiway, wxWindow* aParent )
{
    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    if( cfg->m_Window.grid.sizes.empty() )
        cfg->m_Window.grid.sizes = cfg->DefaultGridSizeList();

    // Currently values read from config file are not used because the user cannot
    // change this config
    //if( cfg->m_Window.zoom_factors.empty() )
    {
        cfg->m_Window.zoom_factors = { ZOOM_LIST_PCBNEW };
    }

    std::unique_ptr<KIGFX::GAL_DISPLAY_OPTIONS> gal_opts;

    gal_opts = std::make_unique<KIGFX::GAL_DISPLAY_OPTIONS>();
    gal_opts->ReadConfig( *Pgm().GetCommonSettings(), cfg->m_Window, aParent );

    auto canvasType = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( cfg->m_Graphics.canvas_type );
    auto panel = new FOOTPRINT_PREVIEW_PANEL( aKiway, aParent, std::move( gal_opts ), canvasType );

    panel->UpdateColors();

    const GRID_SETTINGS& gridCfg = cfg->m_Window.grid;

    panel->GetGAL()->SetGridVisibility( gridCfg.show );

    //Bounds checking cannot include number of elements as an index!
    int    gridIdx = std::max( 0, std::min( gridCfg.last_size_idx, (int) gridCfg.sizes.size() - 1 ) );
    double gridSize = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MILS,
                                                                 gridCfg.sizes[ gridIdx ] );
    panel->GetGAL()->SetGridSize( VECTOR2D( gridSize, gridSize ) );

    return panel;
}
