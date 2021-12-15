/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <panel_setup_layers.h>
#include <panel_setup_text_and_graphics.h>
#include <panel_setup_constraints.h>
#include <dialogs/panel_setup_netclasses.h>
#include <panel_setup_tracks_and_vias.h>
#include <panel_setup_mask_and_paste.h>
#include <../board_stackup_manager/panel_board_stackup.h>
#include <../board_stackup_manager/panel_board_finish.h>
#include <confirm.h>
#include <board_design_settings.h>
#include <kiface_base.h>
#include <drc/drc_item.h>
#include <dialog_import_settings.h>
#include <io_mgr.h>
#include <dialogs/panel_setup_severities.h>
#include <panel_text_variables.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <widgets/resettable_panel.h>
#include <widgets/wx_progress_reporters.h>
#include <wildcards_and_files_ext.h>

#include "dialog_board_setup.h"
#include "panel_setup_rules.h"

#include <wx/treebook.h>

using std::placeholders::_1;


DIALOG_BOARD_SETUP::DIALOG_BOARD_SETUP( PCB_EDIT_FRAME* aFrame ) :
        PAGED_DIALOG( aFrame, _( "Board Setup" ), false,
                      _( "Import Settings from Another Board..." ) ),
        m_frame( aFrame )
{
    BOARD*                 board = aFrame->GetBoard();
    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    m_layers = new PANEL_SETUP_LAYERS( this, aFrame );
    m_textAndGraphics = new PANEL_SETUP_TEXT_AND_GRAPHICS( this, aFrame );
    m_constraints = new PANEL_SETUP_CONSTRAINTS( this, aFrame );
    m_rules = new PANEL_SETUP_RULES( this, aFrame );
    m_tracksAndVias = new PANEL_SETUP_TRACKS_AND_VIAS( this, aFrame, m_constraints );
    m_maskAndPaste = new PANEL_SETUP_MASK_AND_PASTE( this, aFrame );
    m_physicalStackup = new PANEL_SETUP_BOARD_STACKUP( this, aFrame, m_layers );
    m_boardFinish = new PANEL_SETUP_BOARD_FINISH( this, board );

    m_severities = new PANEL_SETUP_SEVERITIES( this, DRC_ITEM::GetItemsWithSeverities(),
                                               bds.m_DRCSeverities );

    m_netclasses = new PANEL_SETUP_NETCLASSES( this, &bds.GetNetClasses(),
                                               board->GetNetClassAssignmentCandidates(), false );

    m_textVars = new PANEL_TEXT_VARIABLES( m_treebook, &Prj() );

    /*
     * WARNING: If you change page names you MUST update calls to ShowBoardSetupDialog().
     */

    m_treebook->AddPage( new wxPanel( this ),  _( "Board Stackup" ) );

    m_currentPage = -1;

    /*
     * WARNING: Code currently relies on the layers setup coming before the physical stackup panel,
     * and thus transferring data to the board first.  See comment in
     * PANEL_SETUP_BOARD_STACKUP::TransferDataFromWindow and rework this logic if it is determined
     * that the order of these pages should be changed.
     */
    m_treebook->AddSubPage( m_layers,  _( "Board Editor Layers" ) );
    m_layerSetupPage = 1;

    m_treebook->AddSubPage( m_physicalStackup,  _( "Physical Stackup" ) );
    // Change this value if m_physicalStackup is not the page 2 of m_treebook
    m_physicalStackupPage = 2;  // The page number (from 0) to select the m_physicalStackup panel

    m_treebook->AddSubPage( m_boardFinish, _( "Board Finish" ) );
    m_treebook->AddSubPage( m_maskAndPaste,  _( "Solder Mask/Paste" ) );

    m_treebook->AddPage( new wxPanel( this ),  _( "Text & Graphics" ) );
    m_treebook->AddSubPage( m_textAndGraphics,  _( "Defaults" ) );
    m_treebook->AddSubPage( m_textVars, _( "Text Variables" ) );

    m_treebook->AddPage( new wxPanel( this ),  _( "Design Rules" ) );
    m_treebook->AddSubPage( m_constraints,  _( "Constraints" ) );
    m_treebook->AddSubPage( m_tracksAndVias, _( "Pre-defined Sizes" ) );
    m_treebook->AddSubPage( m_netclasses,  _( "Net Classes" ) );
    m_treebook->AddSubPage( m_rules, _( "Custom Rules" ) );
    m_treebook->AddSubPage( m_severities, _( "Violation Severity" ) );

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

    m_treebook->SetMinSize( wxSize( -1, 480 ) );

	// Connect Events
	m_treebook->Connect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( DIALOG_BOARD_SETUP::OnPageChange ), nullptr,
                         this );

    finishDialogSettings();

    if( Prj().IsReadOnly() )
    {
        m_infoBar->ShowMessage( _( "Project is missing or read-only. Some settings will not "
                                   "be editable." ), wxICON_WARNING );
    }
}


DIALOG_BOARD_SETUP::~DIALOG_BOARD_SETUP()
{
	m_treebook->Disconnect( wxEVT_TREEBOOK_PAGE_CHANGED,
                            wxBookCtrlEventHandler( DIALOG_BOARD_SETUP::OnPageChange ), nullptr,
                            this );
}


void DIALOG_BOARD_SETUP::OnPageChange( wxBookCtrlEvent& event )
{
    int page = event.GetSelection();

    // Ensure layer page always gets updated even if we aren't moving towards it
    if( m_currentPage == m_physicalStackupPage )
        m_layers->SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );

    if( page == m_physicalStackupPage )
        m_physicalStackup->OnLayersOptionsChanged( m_layers->GetUILayerMask() );
    else if( Prj().IsReadOnly() )
        KIUI::Disable( m_treebook->GetPage( page ) );

    m_currentPage = page;

#ifdef __WXMAC__
    // Work around an OSX bug where the wxGrid children don't get placed correctly until
    // the first resize event
    if( m_macHack[ page ] )
    {
        wxSize pageSize = m_treebook->GetPage( page )->GetSize();
        pageSize.x -= 1;
        pageSize.y += 2;

        m_treebook->GetPage( page )->SetSize( pageSize );
        m_macHack[ page ] = false;
    }
#endif
}


void DIALOG_BOARD_SETUP::OnAuxiliaryAction( wxCommandEvent& event )
{
    DIALOG_IMPORT_SETTINGS importDlg( this, m_frame );

    if( importDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName boardFn( importDlg.GetFilePath() );
    wxFileName projectFn( boardFn );

    projectFn.SetExt( ProjectFileExtension );

    if( !m_frame->GetSettingsManager()->LoadProject( projectFn.GetFullPath(), false ) )
    {
        wxString msg = wxString::Format( _( "Error importing settings from board:\n"
                                            "Associated project file %s could not be loaded" ),
                                         projectFn.GetFullPath() );
        DisplayErrorMessage( this, msg );

        return;
    }

    // Flag so user can stop work if it will result in deleted inner copper layers
    // and still clean up this function properly.
    bool okToProceed = true;

    PROJECT* otherPrj = m_frame->GetSettingsManager()->GetProject( projectFn.GetFullPath() );

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::KICAD_SEXP ) );

    BOARD* otherBoard = nullptr;

    try
    {
        WX_PROGRESS_REPORTER progressReporter( this, _( "Loading PCB" ), 1 );

        otherBoard = pi->Load( boardFn.GetFullPath(), nullptr, nullptr, nullptr,
                               &progressReporter );

        if( importDlg.m_LayersOpt->GetValue() )
        {
            BOARD* loadedBoard = m_frame->GetBoard();

            // Check if "Import Settings" board has more layers than the current board.
            okToProceed = m_layers->CheckCopperLayerCount( loadedBoard, otherBoard );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        // You wouldn't think boardFn.GetFullPath() would throw, but we get a stack buffer
        // underflow from ASAN.  While it's probably an ASAN error, a second try/catch doesn't
        // cost us much.
        try
        {
            if( ioe.Problem() != wxT( "CANCEL" ) )
            {
                wxString msg = wxString::Format( _( "Error loading board file:\n%s" ),
                                                 boardFn.GetFullPath() );
                DisplayErrorMessage( this, msg, ioe.What() );
            }

            if( otherPrj != &m_frame->Prj() )
                m_frame->GetSettingsManager()->UnloadProject( otherPrj, false );
        }
        catch(...)
        {
            // That was already our best-efforts
        }

        return;
    }


    if( okToProceed )
    {
        otherBoard->SetProject( otherPrj );

        // If layers options are imported, import also the stackup
        // layers options and stackup are linked, so they cannot be imported
        // separately, and stackup can be imported only after layers options
        if( importDlg.m_LayersOpt->GetValue() )
        {
            m_layers->ImportSettingsFrom( otherBoard );
            m_physicalStackup->ImportSettingsFrom( otherBoard );
        }

        if( importDlg.m_TextAndGraphicsOpt->GetValue() )
            m_textAndGraphics->ImportSettingsFrom( otherBoard );

        if( importDlg.m_ConstraintsOpt->GetValue() )
            m_constraints->ImportSettingsFrom( otherBoard );

        if( importDlg.m_NetclassesOpt->GetValue() )
            m_netclasses->ImportSettingsFrom( &otherBoard->GetDesignSettings().GetNetClasses() );

        if( importDlg.m_TracksAndViasOpt->GetValue() )
            m_tracksAndVias->ImportSettingsFrom( otherBoard );

        if( importDlg.m_MaskAndPasteOpt->GetValue() )
            m_maskAndPaste->ImportSettingsFrom( otherBoard );

        if( importDlg.m_SeveritiesOpt->GetValue() )
            m_severities->ImportSettingsFrom( otherBoard->GetDesignSettings().m_DRCSeverities );

        if( otherPrj != &m_frame->Prj() )
            otherBoard->ClearProject();
    }

    // Clean up and free memory before leaving
    if( otherPrj != &m_frame->Prj() )
        m_frame->GetSettingsManager()->UnloadProject( otherPrj, false );

    delete otherBoard;
}
