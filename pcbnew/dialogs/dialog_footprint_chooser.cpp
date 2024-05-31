/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_footprint_chooser.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <pcb_base_frame.h>
#include <widgets/panel_footprint_chooser.h>
#include <3d_canvas/eda_3d_canvas.h>
#include <board.h>
#include <project_pcb.h>
#include <board_design_settings.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <footprint_preview_panel.h>
#include <widgets/bitmap_button.h>


DIALOG_FOOTPRINT_CHOOSER::DIALOG_FOOTPRINT_CHOOSER( PCB_BASE_FRAME* aParent,
                                                    const LIB_ID& aPreselect,
                                                    const wxArrayString& aFootprintHistoryList ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Choose Footprint" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_boardAdapter(),
        m_currentCamera( m_trackBallCamera ),
        m_trackBallCamera( 2 * RANGE_SCALE_3D )
{
    m_parent = aParent;
    m_showFpMode = true;

    wxBoxSizer* m_SizerTop = new wxBoxSizer( wxVERTICAL );
    m_chooserPanel = new PANEL_FOOTPRINT_CHOOSER( aParent, this, aFootprintHistoryList,
            // Filter
            []( LIB_TREE_NODE& aNode ) -> bool
            {
                return true;
            },
            // Accept handler
            [this]()
            {
                EndModal( wxID_OK );
            },
            // Escape handler
            [this]()
            {
                EndModal( wxID_CANCEL );
            } );

    m_SizerTop->Add( m_chooserPanel, 1, wxEXPAND, 5 );
    FOOTPRINT_PREVIEW_WIDGET* viewerFpPanel = m_chooserPanel->GetViewerPanel();

    viewerFpPanel->Show( m_showFpMode );

    build3DCanvas();

    m_preview3DCanvas->Show( !m_showFpMode );

    wxBoxSizer* bSizerBottom;
    bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

    bSizerBottom->Add( 0, 0, 1, 0, 5 );     // Add spacer to right-align buttons

    BITMAP_BUTTON* separator = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap );
    separator->SetIsSeparator();
    bSizerBottom->Add( separator, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

	m_grButton3DView = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap );
    m_grButton3DView->SetIsRadioButton();
    m_grButton3DView->SetBitmap( KiBitmapBundle( BITMAPS::shape_3d ) );
    m_grButton3DView->Check( !m_showFpMode );
	bSizerBottom->Add( m_grButton3DView, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

	m_grButtonFpView = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap );
    m_grButtonFpView->SetIsRadioButton();
    m_grButtonFpView->SetBitmap( KiBitmapBundle( BITMAPS::module ) );
    m_grButtonFpView->Check( m_showFpMode );
	bSizerBottom->Add( m_grButtonFpView, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

    separator = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap );
    separator->SetIsSeparator();
    bSizerBottom->Add( separator, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 1 );

    if( aPreselect.IsValid() )
        m_chooserPanel->SetPreselect( aPreselect );

    SetTitle( GetTitle() + wxString::Format( _( " (%d items loaded)" ),
                                             m_chooserPanel->GetItemCount() ) );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( this, wxID_OK );
    wxButton*               cancelButton = new wxButton( this, wxID_CANCEL );
    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    bSizerBottom->Add( 20, 0, 0, 0, 5 );     // Add spacer
    bSizerBottom->Add( sdbSizer, 0, wxEXPAND | wxALL, 5 );

    m_SizerTop->Add( bSizerBottom, 0, wxEXPAND, 5 );
    SetSizer( m_SizerTop );

    SetInitialFocus( m_chooserPanel->GetFocusTarget() );
    SetupStandardButtons();

    m_chooserPanel->FinishSetup();
    Layout();

    // Connect Events
    m_grButton3DView->Connect( wxEVT_COMMAND_BUTTON_CLICKED ,
                               wxCommandEventHandler( DIALOG_FOOTPRINT_CHOOSER::on3DviewReq ),
                               nullptr, this );
    m_grButtonFpView->Connect( wxEVT_COMMAND_BUTTON_CLICKED ,
                               wxCommandEventHandler( DIALOG_FOOTPRINT_CHOOSER::onFpViewReq ),
                               nullptr, this );

    Connect( FP_SELECTION_EVENT, wxCommandEventHandler( DIALOG_FOOTPRINT_CHOOSER::onFpChanged ),
             nullptr, this );
}


DIALOG_FOOTPRINT_CHOOSER::~DIALOG_FOOTPRINT_CHOOSER()
{
    if( m_boardAdapter.m_Cfg )
        m_boardAdapter.m_Cfg->m_Render = m_initialRender;

    // Disconnect Events
    m_grButton3DView->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler( DIALOG_FOOTPRINT_CHOOSER::on3DviewReq ),
                                  nullptr, this );
    m_grButtonFpView->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler( DIALOG_FOOTPRINT_CHOOSER::onFpViewReq ),
                                  nullptr, this );

    Disconnect( FP_SELECTION_EVENT, wxCommandEventHandler( DIALOG_FOOTPRINT_CHOOSER::onFpChanged ),
                nullptr, this );
}


void DIALOG_FOOTPRINT_CHOOSER::build3DCanvas()
{
    // Create the dummy board used by the 3D canvas
    m_dummyBoard = new BOARD();
    m_dummyBoard->SetProject( &m_parent->Prj(), true );

    // This board will only be used to hold a footprint for viewing
    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );

    m_boardAdapter.SetBoard( m_dummyBoard );
    m_boardAdapter.m_IsBoardView = false;
    m_boardAdapter.m_IsPreviewer = true;   // Force display 3D models, regardless the 3D viewer options

    // Build the 3D canvas

    m_preview3DCanvas = new EDA_3D_CANVAS( m_chooserPanel->m_RightPanel,
                                           OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                           m_boardAdapter, m_currentCamera,
                                           PROJECT_PCB::Get3DCacheManager( &m_parent->Prj() ) );

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    // TODO(JE) use all control options
    m_boardAdapter.m_MousewheelPanning = settings->m_Input.scroll_modifier_zoom != 0;

    EDA_3D_VIEWER_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EDA_3D_VIEWER_SETTINGS>();

    if( cfg )
    {
        // Save the 3D viewer render settings, to restore it after closing the preview
        m_initialRender = cfg->m_Render;

        m_boardAdapter.m_Cfg = cfg;

        m_preview3DCanvas->SetAnimationEnabled( cfg->m_Camera.animation_enabled );
        m_preview3DCanvas->SetMovingSpeedMultiplier( cfg->m_Camera.moving_speed_multiplier );
        m_preview3DCanvas->SetProjectionMode( cfg->m_Camera.projection_mode );

        // Ensure the board body is always shown, and do not use the settings of the 3D viewer
        cfg->m_Render.show_copper_top = true;
        cfg->m_Render.show_copper_bottom = true;
        cfg->m_Render.show_soldermask_top = true;
        cfg->m_Render.show_soldermask_bottom = true;
        cfg->m_Render.show_solderpaste = true;
        cfg->m_Render.show_zones = true;
        cfg->m_Render.show_board_body = true;
    }

    m_chooserPanel->m_RightPanelSizer->Add( m_preview3DCanvas, 1, wxEXPAND, 5 );
    m_chooserPanel->m_RightPanel->Layout();

    BOARD_DESIGN_SETTINGS parent_bds = m_parent->GetDesignSettings();
    BOARD_DESIGN_SETTINGS& dummy_bds = m_dummyBoard->GetDesignSettings();
    dummy_bds.SetBoardThickness( parent_bds.GetBoardThickness() );
    dummy_bds.SetEnabledLayers( LSET::FrontMask() | LSET::BackMask() );
    BOARD_STACKUP& dummy_board_stackup = m_dummyBoard->GetDesignSettings().GetStackupDescriptor();
    dummy_board_stackup.RemoveAll();
    dummy_board_stackup.BuildDefaultStackupList( &dummy_bds, 2 );
}



LIB_ID DIALOG_FOOTPRINT_CHOOSER::GetSelectedLibId() const
{
    return m_chooserPanel->GetSelectedLibId();
}


void DIALOG_FOOTPRINT_CHOOSER::onFpChanged( wxCommandEvent& event )
{
    if( m_showFpMode )      // the 3D viewer is not activated
        return;

    on3DviewReq( event );
}


void DIALOG_FOOTPRINT_CHOOSER::on3DviewReq( wxCommandEvent& event )
{
    m_showFpMode = false;

    m_grButtonFpView->Check( m_showFpMode );
    m_grButton3DView->Check( !m_showFpMode );

    FOOTPRINT_PREVIEW_WIDGET* viewFpPanel = m_chooserPanel->GetViewerPanel();
    viewFpPanel->Show( m_showFpMode );
    m_preview3DCanvas->Show( !m_showFpMode );
    m_dummyBoard->DeleteAllFootprints();

    if( m_chooserPanel->m_CurrFootprint )
        m_dummyBoard->Add( (FOOTPRINT*)m_chooserPanel->m_CurrFootprint->Clone() );

    m_preview3DCanvas->ReloadRequest();
    m_preview3DCanvas->Request_refresh();
    m_chooserPanel->m_RightPanel->Layout();
    m_chooserPanel->m_RightPanel->Refresh();
}


void DIALOG_FOOTPRINT_CHOOSER::onFpViewReq( wxCommandEvent& event )
{
    m_showFpMode = true;

    m_grButtonFpView->Check( m_showFpMode );
    m_grButton3DView->Check( !m_showFpMode );

    FOOTPRINT_PREVIEW_WIDGET* viewFpPanel = m_chooserPanel->GetViewerPanel();
    viewFpPanel->Show( m_showFpMode );
    m_preview3DCanvas->Show( !m_showFpMode );
    m_chooserPanel->m_RightPanel->Layout();
    m_chooserPanel->m_RightPanel->Refresh();
}
