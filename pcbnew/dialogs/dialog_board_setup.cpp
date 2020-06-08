/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <panel_setup_feature_constraints.h>
#include <panel_setup_netclasses.h>
#include <panel_setup_tracks_and_vias.h>
#include <panel_setup_mask_and_paste.h>
#include <../board_stackup_manager/panel_board_stackup.h>
#include <confirm.h>
#include <kiface_i.h>
#include <drc/drc.h>
#include <drc/drc_item.h>
#include <dialog_import_settings.h>
#include <io_mgr.h>
#include <panel_setup_severities.h>
#include <panel_text_variables.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include "dialog_board_setup.h"
#include "panel_setup_rules.h"

DIALOG_BOARD_SETUP::DIALOG_BOARD_SETUP( PCB_EDIT_FRAME* aFrame ) :
        PAGED_DIALOG( aFrame, _( "Board Setup" ), _( "Import Settings from Another Board..." ) ),
        m_frame( aFrame )
{
    m_layers = new PANEL_SETUP_LAYERS( this, aFrame );
    m_textAndGraphics = new PANEL_SETUP_TEXT_AND_GRAPHICS( this, aFrame );
    m_constraints = new PANEL_SETUP_FEATURE_CONSTRAINTS( this, aFrame );
    m_netclasses = new PANEL_SETUP_NETCLASSES( this, aFrame, m_constraints );
    m_rules = new PANEL_SETUP_RULES( this, aFrame );
    m_tracksAndVias = new PANEL_SETUP_TRACKS_AND_VIAS( this, aFrame, m_constraints );
    m_maskAndPaste = new PANEL_SETUP_MASK_AND_PASTE( this, aFrame );
    m_physicalStackup = new PANEL_SETUP_BOARD_STACKUP( this, aFrame, m_layers );

    BOARD_DESIGN_SETTINGS& bds = aFrame->GetDesignSettings();
    m_severities = new PANEL_SETUP_SEVERITIES( this, DRC_ITEM::GetItemsWithSeverities(),
                                               bds.m_DRCSeverities );

    m_textVars = new PANEL_TEXT_VARIABLES( m_treebook, &Prj() );

    /*
     * WARNING: If you change page names you MUST update calls to ShowBoardSetupDialog().
     */

    m_treebook->AddPage( new wxPanel( this ),  _( "Board Stackup" ) );
    m_treebook->AddSubPage( m_layers,  _( "Board Editor Layers" ) );
    m_treebook->AddSubPage( m_physicalStackup,  _( "Physical Stackup" ) );
    // Change this value if m_physicalStackup is not the page 2 of m_treebook
    m_physicalStackupPage = 2;  // The page number (from 0) to select the m_physicalStackup panel

    m_treebook->AddPage( new wxPanel( this ),  _( "Defaults" ) );
    m_treebook->AddSubPage( m_textAndGraphics,  _( "Text & Graphics" ) );
    m_treebook->AddSubPage( m_tracksAndVias, _( "Tracks & Vias" ) );
    m_treebook->AddSubPage( m_maskAndPaste,  _( "Solder Mask/Paste" ) );

    m_treebook->AddPage( new wxPanel( this ),  _( "Design Rules" ) );
    m_treebook->AddSubPage( m_constraints,  _( "Constraints" ) );
    m_treebook->AddSubPage( m_netclasses,  _( "Net Classes" ) );
    m_treebook->AddSubPage( m_rules, _( "Rules" ) );
    m_treebook->AddSubPage( m_severities, _( "Violation Severity" ) );

    m_treebook->AddPage( new wxPanel( this ), _( "Project" ) );
    m_treebook->AddSubPage( m_textVars, _( "Text Variables" ) );

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

	// Connect Events
	m_treebook->Connect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( DIALOG_BOARD_SETUP::OnPageChange ), NULL, this );

	FinishDialogSettings();
}


DIALOG_BOARD_SETUP::~DIALOG_BOARD_SETUP()
{
	m_treebook->Disconnect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( DIALOG_BOARD_SETUP::OnPageChange ), NULL, this );
}


void DIALOG_BOARD_SETUP::OnPageChange( wxBookCtrlEvent& event )
{
    if( event.GetSelection() == m_physicalStackupPage )
        m_physicalStackup->OnLayersOptionsChanged( m_layers->GetUILayerMask() );

#ifdef __WXMAC__
    // Work around an OSX bug where the wxGrid children don't get placed correctly until
    // the first resize event
    int page = event.GetSelection();

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


// Run Import Settings... action
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

    PROJECT* otherPrj = m_frame->GetSettingsManager()->GetProject( projectFn.GetFullPath() );

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::KICAD_SEXP ) );

    BOARD* otherBoard = new BOARD();

    try
    {
        otherBoard = pi->Load( boardFn.GetFullPath(), nullptr, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        if( ioe.Problem() != wxT( "CANCEL" ) )
        {
            wxString msg =
                    wxString::Format( _( "Error loading board file:\n%s" ), boardFn.GetFullPath() );
            DisplayErrorMessage( this, msg, ioe.What() );
        }

        m_frame->GetSettingsManager()->UnloadProject( otherPrj, false );

        return;
    }

    otherBoard->SetProject( otherPrj );

    if( importDlg.m_LayersOpt->GetValue() )
        m_layers->ImportSettingsFrom( otherBoard );
    if( importDlg.m_TextAndGraphicsOpt->GetValue() )
        m_textAndGraphics->ImportSettingsFrom( otherBoard );
    if( importDlg.m_ConstraintsOpt->GetValue() )
        m_constraints->ImportSettingsFrom( otherBoard );
    if( importDlg.m_NetclassesOpt->GetValue() )
        m_netclasses->ImportSettingsFrom( otherBoard );
    if( importDlg.m_TracksAndViasOpt->GetValue() )
        m_tracksAndVias->ImportSettingsFrom( otherBoard );
    if( importDlg.m_MaskAndPasteOpt->GetValue() )
        m_maskAndPaste->ImportSettingsFrom( otherBoard );

    // If layers options are imported, import also the stackup
    // layers options and stackup are linked, so they cannot be imported
    // separately, and stackup can be imported only after layers options
    //
    // Note also currently only the list of enabled layers can be imported, because
    // we import settings from a .pro project file, not the settings inside
    // a board, and info only living in the board is not imported.
    // TODO: Add import of physical settings now that we are actually loading the board here
    if( importDlg.m_LayersOpt->GetValue() )
        m_physicalStackup->ImportSettingsFrom( otherBoard );

    if( importDlg.m_SeveritiesOpt->GetValue() )
        m_severities->ImportSettingsFrom( otherBoard->GetDesignSettings().m_DRCSeverities );

    otherBoard->ClearProject();

    m_frame->GetSettingsManager()->UnloadProject( otherPrj, false );

    delete otherBoard;
}
