/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/radiobox.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/statline.h>

#include <core/ignore.h>
#include <widgets/gal_options_panel.h>
#include <settings/app_settings.h>
#include <eda_draw_frame.h>

#include <config_map.h>

/*
 * Spin control parameters
 */
static const double gridThicknessMin = 1.0;
static const double gridThicknessMax = 10.0;
static const double gridThicknessStep = 0.5;

static const double gridMinSpacingMin = 5;
static const double gridMinSpacingMax = 200;
static const double gridMinSpacingStep = 5;


///TODO: These are duplicated in gal_display_options - Unify!
static const UTIL::CFG_MAP<KIGFX::GRID_STYLE> gridStyleSelectMap =
{
    { KIGFX::GRID_STYLE::DOTS,        0 },  // Default
    { KIGFX::GRID_STYLE::LINES,       1 },
    { KIGFX::GRID_STYLE::SMALL_CROSS, 2 },
};


static const UTIL::CFG_MAP<KIGFX::GRID_SNAPPING> gridSnapConfigVals =
{
    { KIGFX::GRID_SNAPPING::ALWAYS,     0 },
    { KIGFX::GRID_SNAPPING::WITH_GRID,  1 },
    { KIGFX::GRID_SNAPPING::NEVER,      2 }
};


GAL_OPTIONS_PANEL::GAL_OPTIONS_PANEL( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
    wxPanel( aParent, wxID_ANY ),
    m_cfg( aAppSettings )
{
    // the main sizer that holds "columns" of settings
    m_mainSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( m_mainSizer );

    // second-level sizers that are one "column" of settings each
    wxBoxSizer* sLeftSizer = new wxBoxSizer( wxVERTICAL );
    m_mainSizer->Add( sLeftSizer, 1, wxRIGHT | wxBOTTOM | wxEXPAND, 5 );

    /*
     * Rendering engine
     */
#ifndef __WXMAC__
    {
        wxString engineChoices[] = { _( "Accelerated graphics" ), _( "Fallback graphics" ) };
       	m_renderingEngine = new wxRadioBox( this, wxID_ANY, _( "Rendering Engine" ),
                                            wxDefaultPosition, wxDefaultSize,
                                            sizeof( engineChoices ) / sizeof( wxString ),
                                            engineChoices, 1, wxRA_SPECIFY_COLS );
        m_renderingEngine->SetItemToolTip( 0, _( "Hardware-accelerated graphics (recommended)" ) );
        m_renderingEngine->SetItemToolTip( 1, _( "Software graphics (for computers which do not "
                                                 "support KiCad's hardware acceleration "
                                                 "requirements)" ) );

        sLeftSizer->Add( m_renderingEngine, 0, wxTOP | wxBOTTOM | wxRIGHT | wxEXPAND, 5 );
    }
#endif

    /*
     * Grid settings subpanel
     */
    {
        wxStaticText* gridLabel = new wxStaticText( this, wxID_ANY, _( "Grid Options" ) );
        sLeftSizer->Add( gridLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

        wxStaticLine* staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition,
                                                      wxDefaultSize, wxLI_HORIZONTAL );
        sLeftSizer->Add( staticline1, 0, wxEXPAND|wxBOTTOM, 5 );

        wxBoxSizer* sGridSettings = new wxBoxSizer( wxVERTICAL );

        wxString m_gridStyleChoices[] = {
            _( "Dots" ),
            _( "Lines" ),
            _( "Small crosses" )
        };

        int m_gridStyleNChoices = sizeof( m_gridStyleChoices ) / sizeof( wxString );
        m_gridStyle = new wxRadioBox( this, wxID_ANY, _( "Grid Style" ), wxDefaultPosition,
                                      wxDefaultSize, m_gridStyleNChoices, m_gridStyleChoices, 1,
                                      wxRA_SPECIFY_COLS );
        sGridSettings->Add( m_gridStyle, 0, wxALL | wxEXPAND, 5 );

        wxFlexGridSizer* sGridSettingsGrid;
        sGridSettingsGrid = new wxFlexGridSizer( 0, 3, 0, 0 );
        sGridSettingsGrid->AddGrowableCol( 1 );
        sGridSettingsGrid->SetFlexibleDirection( wxBOTH );
        sGridSettingsGrid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

        l_gridLineWidth = new wxStaticText( this, wxID_ANY, _( "Grid thickness:" ) );
        l_gridLineWidth->Wrap( -1 );
        sGridSettingsGrid->Add( l_gridLineWidth, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP, 5 );

        m_gridLineWidth = new wxSpinCtrlDouble( this, wxID_ANY );
        m_gridLineWidth->SetRange( gridThicknessMin, gridThicknessMax );
        m_gridLineWidth->SetIncrement( gridThicknessStep );
        m_gridLineWidth->SetDigits( 1 );
        sGridSettingsGrid->Add( m_gridLineWidth, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxTOP, 5 );

        l_gridLineWidthUnits = new wxStaticText( this, wxID_ANY, _( "px" ) );
        l_gridLineWidthUnits->Wrap( -1 );
        sGridSettingsGrid->Add( l_gridLineWidthUnits, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT | wxTOP, 5 );

        l_gridMinSpacing = new wxStaticText( this, wxID_ANY, _( "Min grid spacing:" ) );
        l_gridMinSpacing->Wrap( -1 );
        sGridSettingsGrid->Add( l_gridMinSpacing, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP, 5 );

        m_gridMinSpacing = new wxSpinCtrlDouble( this, wxID_ANY);
        m_gridMinSpacing->SetRange( gridMinSpacingMin, gridMinSpacingMax );
        m_gridMinSpacing->SetIncrement( gridMinSpacingStep );
        m_gridMinSpacing->SetDigits( 0 );
        sGridSettingsGrid->Add( m_gridMinSpacing, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxTOP, 5 );

        l_gridMinSpacingUnits = new wxStaticText( this, wxID_ANY, _( "px" ) );
        l_gridMinSpacingUnits->Wrap( -1 );
        sGridSettingsGrid->Add( l_gridMinSpacingUnits, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT | wxTOP, 5 );

        l_gridSnapOptions = new wxStaticText( this, wxID_ANY, _( "Snap to Grid:" ) );
        l_gridSnapOptions->Wrap( -1 );
        sGridSettingsGrid->Add( l_gridSnapOptions, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxTOP, 5 );

        wxString gridSnapChoices[] = { _( "Always" ), _( "When grid shown" ), _( "Never" ) };
        int gridSnapNChoices = sizeof( gridSnapChoices ) / sizeof( wxString );
        m_gridSnapOptions = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          gridSnapNChoices, gridSnapChoices );
        m_gridSnapOptions->Select( 0 );
        sGridSettingsGrid->Add( m_gridSnapOptions, 0,
                                wxALIGN_CENTER_VERTICAL | wxEXPAND | wxTOP | wxBOTTOM, 5 );

        l_gridSnapSpace = new wxStaticText( this, wxID_ANY, _( "px" ) );
        l_gridSnapSpace->Wrap( -1 );
        l_gridSnapSpace->Hide();
        sGridSettingsGrid->Add( l_gridSnapSpace, 0,
                                wxALIGN_CENTER_VERTICAL | wxALL | wxRESERVE_SPACE_EVEN_IF_HIDDEN,
                                5 );


        sGridSettings->Add( sGridSettingsGrid, 1, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5 );

        sLeftSizer->Add( sGridSettings, 0, wxEXPAND|wxLEFT, 5 );
    }

    /*
     * Cursor settings subpanel
     */
    {
        sLeftSizer->Add( 0, 15, 0, wxEXPAND, 5 );

        wxStaticText* gridLabel = new wxStaticText( this, wxID_ANY, _( "Cursor Options" ) );
        sLeftSizer->Add( gridLabel, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 13 );

        wxStaticLine* staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition,
                                                      wxDefaultSize, wxLI_HORIZONTAL );
        sLeftSizer->Add( staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

        wxBoxSizer* sCursorSettings = new wxBoxSizer( wxVERTICAL );
        sLeftSizer->Add( sCursorSettings, 0, wxEXPAND|wxLEFT, 5 );

        wxString m_CursorShapeChoices[] = {
            _( "Small crosshair" ),
            _( "Full window crosshair" )
        };

        int m_CursorShapeNChoices = sizeof( m_CursorShapeChoices ) / sizeof( wxString );
        m_cursorShape = new wxRadioBox( this, wxID_ANY, _( "Cursor Shape" ), wxDefaultPosition,
                                        wxDefaultSize, m_CursorShapeNChoices, m_CursorShapeChoices,
                                        1, wxRA_SPECIFY_COLS );

        m_cursorShape->SetSelection( 0 );
        m_cursorShape->SetToolTip( _( "Cursor shape for drawing, placement and movement tools" ) );
        sCursorSettings->Add( m_cursorShape, 0, wxALL | wxEXPAND, 5 );

        m_forceCursorDisplay = new wxCheckBox( this, wxID_ANY, _( "Always show crosshairs" ) );
        sCursorSettings->Add( m_forceCursorDisplay, 0, wxALL | wxEXPAND, 5 );
    }
}


bool GAL_OPTIONS_PANEL::TransferDataToWindow()
{
#ifndef __WXMAC__
    auto canvasType = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( m_cfg->m_Graphics.canvas_type );

    if( canvasType == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL )
        m_renderingEngine->SetSelection( 0 );
    else
        m_renderingEngine->SetSelection( 1 );
#endif

    m_gridSnapOptions->SetSelection( m_cfg->m_Window.grid.snap );
    m_gridStyle->SetSelection( m_cfg->m_Window.grid.style );
    m_gridLineWidth->SetValue( m_cfg->m_Window.grid.line_width );
    m_gridMinSpacing->SetValue( m_cfg->m_Window.grid.min_spacing );

    m_cursorShape->SetSelection( m_cfg->m_Window.cursor.fullscreen_cursor );
    m_forceCursorDisplay->SetValue( m_cfg->m_Window.cursor.always_show_cursor );

    return true;
}


bool GAL_OPTIONS_PANEL::TransferDataFromWindow()
{
    m_cfg->m_Window.grid.snap = m_gridSnapOptions->GetSelection();
    m_cfg->m_Window.grid.style = m_gridStyle->GetSelection();
    m_cfg->m_Window.grid.line_width = m_gridLineWidth->GetValue();
    m_cfg->m_Window.grid.min_spacing = m_gridMinSpacing->GetValue();

    m_cfg->m_Window.cursor.fullscreen_cursor = m_cursorShape->GetSelection();
    m_cfg->m_Window.cursor.always_show_cursor = m_forceCursorDisplay->GetValue();

#ifndef __WXMAC__
    m_cfg->m_Graphics.canvas_type = m_renderingEngine->GetSelection() == 0 ?
                                                    EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL :
                                                    EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif

    return true;
}


bool GAL_OPTIONS_PANEL::ResetPanel( APP_SETTINGS_BASE* aAppSettings )
{
    APP_SETTINGS_BASE* saved = m_cfg;

    m_cfg = aAppSettings;
    TransferDataToWindow();
    m_cfg = saved;

    return true;
}


