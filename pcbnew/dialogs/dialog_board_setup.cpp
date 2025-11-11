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
#include <pcb_edit_frame.h>
#include <panel_setup_layers.h>
#include <panel_setup_defaults.h>
#include <panel_setup_constraints.h>
#include <panel_setup_tracks_and_vias.h>
#include <panel_setup_mask_and_paste.h>
#include <panel_setup_zone_hatch_offsets.h>
#include <../board_stackup_manager/panel_board_stackup.h>
#include <../board_stackup_manager/panel_board_finish.h>
#include <confirm.h>
#include <board_design_settings.h>
#include <kiface_base.h>
#include <drc/drc_item.h>
#include <dialog_import_settings.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <panel_embedded_files.h>
#include <dialogs/panel_setup_severities.h>
#include <dialogs/panel_setup_rules.h>
#include <dialogs/panel_setup_teardrops.h>
#include <dialogs/panel_setup_tuning_patterns.h>
#include <dialogs/panel_setup_netclasses.h>
#include <dialogs/panel_assign_component_classes.h>
#include <dialogs/panel_setup_tuning_profiles.h>
#include <panel_text_variables.h>
#include <project.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <widgets/resettable_panel.h>
#include <widgets/wx_progress_reporters.h>
#include <wildcards_and_files_ext.h>

#include "dialog_board_setup.h"

#include <advanced_config.h>
#include <dialog_board_setup.h>
#include <footprint.h>


#define RESOLVE_PAGE( T, pageIndex ) static_cast<T*>( m_treebook->ResolvePage( pageIndex ) )

DIALOG_BOARD_SETUP::DIALOG_BOARD_SETUP( PCB_EDIT_FRAME* aFrame, wxWindow* aParentWindow ) :
        PAGED_DIALOG( aParentWindow ? aParentWindow : aFrame, _( "Board Setup" ), false, false,
                      _( "Import Settings from Another Board..." ), wxSize( 980, 600 ) ),
        m_frame( aFrame ),
        m_layers( nullptr ),
        m_boardFinish( nullptr ),
        m_physicalStackup( nullptr ),
        m_zoneHatchOffsets( nullptr ),
        m_tuningProfiles( nullptr ),
        m_netClasses( nullptr ),
        m_currentPage( 0 ),
        m_layersPage( 0 ),
        m_physicalStackupPage( 0 ),
        m_boardFinishPage( 0 ),
        m_defaultsPage( 0 ),
        m_formattingPage( 0 ),
        m_maskAndPastePage( 0 ),
        m_zoneHatchOffsetsPage( 0 ),
        m_constraintsPage( 0 ),
        m_tracksAndViasPage( 0 ),
        m_teardropsPage( 0 ),
        m_tuningPatternsPage( 0 ),
        m_netclassesPage( 0 ),
        m_customRulesPage( 0 ),
        m_severitiesPage( 0 ),
        m_tuningProfilesPage( 0 )
{
    SetEvtHandlerEnabled( false );

    /*
     * WARNING: If you change page names you MUST update calls to ShowBoardSetupDialog().
     */

    m_treebook->AddPage( new wxPanel( GetTreebook() ), _( "Board Stackup" ) );

    /*
     * WARNING: Code currently relies on the layers setup coming before the physical stackup panel,
     * and thus transferring data to the board first.  See comment in
     * PANEL_SETUP_BOARD_STACKUP::TransferDataFromWindow and rework this logic if it is determined
     * that the order of these pages should be changed.
     */
    m_layersPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_LAYERS( aParent, m_frame );
            },  _( "Board Editor Layers" ) );

    m_physicalStackupPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                m_layers = RESOLVE_PAGE( PANEL_SETUP_LAYERS, m_layersPage );
                m_boardFinish = RESOLVE_PAGE( PANEL_SETUP_BOARD_FINISH, m_boardFinishPage );
                return new PANEL_SETUP_BOARD_STACKUP( aParent, m_frame, m_layers, m_boardFinish );
            },  _( "Physical Stackup" ) );

    m_boardFinishPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_BOARD_FINISH( aParent, m_frame );
            }, _( "Board Finish" ) );

    m_maskAndPastePage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_MASK_AND_PASTE( aParent, m_frame );
            }, _( "Solder Mask/Paste" ) );

    m_zoneHatchOffsetsPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

                return new PANEL_SETUP_ZONE_HATCH_OFFSETS( aParent, m_frame, bds );
            }, _( "Zone Hatch Offsets" ) );

    m_treebook->AddPage( new wxPanel( GetTreebook() ), _( "Text & Graphics" ) );

    m_defaultsPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_DEFAULTS( aParent, m_frame );
            }, _( "Defaults" ) );

    m_formattingPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_FORMATTING( aParent, m_frame );
            }, _( "Formatting" ) );

    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_TEXT_VARIABLES( aParent, &Prj() );
            }, _( "Text Variables" ) );

    m_treebook->AddPage( new wxPanel( GetTreebook() ), _( "Design Rules" ) );

    m_constraintsPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_CONSTRAINTS( aParent, m_frame );
            }, _( "Constraints" ) );

    m_tracksAndViasPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_TRACKS_AND_VIAS( aParent, m_frame );
            },  _( "Pre-defined Sizes" ) );

    m_teardropsPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_TEARDROPS( aParent, m_frame );
            }, _( "Teardrops" ) );

    m_tuningPatternsPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                BOARD_DESIGN_SETTINGS& bds = m_frame->GetBoard()->GetDesignSettings();

                return new PANEL_SETUP_TUNING_PATTERNS( aParent, m_frame,
                                                        bds.m_SingleTrackMeanderSettings,
                                                        bds.m_DiffPairMeanderSettings,
                                                        bds.m_SkewMeanderSettings );
            }, _( "Length-tuning Patterns" ) );

    m_tuningProfilesPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                BOARD* board = m_frame->GetBoard();
                return new PANEL_SETUP_TUNING_PROFILES( aParent, m_frame, board,
                                                        m_frame->Prj().GetProjectFile().TuningProfileParameters() );
            },
            _( "Tuning Profiles" ) );

    m_netclassesPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                BOARD* board = m_frame->GetBoard();
                return new PANEL_SETUP_NETCLASSES( aParent, m_frame,
                                                   m_frame->Prj().GetProjectFile().NetSettings(),
                                                   board->GetNetClassAssignmentCandidates(),
                                                   false );
            }, _( "Net Classes" ) );

    m_componentClassesPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                // Construct the panel
                return new PANEL_ASSIGN_COMPONENT_CLASSES(
                        aParent, m_frame, m_frame->Prj().GetProjectFile().ComponentClassSettings(),
                        this );
            },
            _( "Component Classes" ) );

    m_customRulesPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_RULES( aParent, m_frame );
            },
            _( "Custom Rules" ) );

    m_severitiesPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                BOARD* board = m_frame->GetBoard();
                return new PANEL_SETUP_SEVERITIES( aParent, DRC_ITEM::GetItemsWithSeverities(),
                                                   board->GetDesignSettings().m_DRCSeverities );
            },
            _( "Violation Severity" ) );

    m_treebook->AddPage( new wxPanel( GetTreebook() ), _( "Board Data" ) );
    m_embeddedFilesPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_EMBEDDED_FILES( aParent, m_frame->GetBoard(), NO_MARGINS );
            },
            _( "Embedded Files" ) );

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
        m_treebook->ExpandNode( i );

    SetEvtHandlerEnabled( true );

    finishDialogSettings();

    if( Prj().IsReadOnly() )
    {
        m_infoBar->ShowMessage( _( "Project is missing or read-only. Some settings will not be editable." ),
                                wxICON_WARNING );
    }

    wxBookCtrlEvent evt( wxEVT_TREEBOOK_PAGE_CHANGED, wxID_ANY, 0 );

    wxQueueEvent( m_treebook, evt.Clone() );
}


DIALOG_BOARD_SETUP::~DIALOG_BOARD_SETUP()
{
}


void DIALOG_BOARD_SETUP::onPageChanged( wxBookCtrlEvent& aEvent )
{
    PAGED_DIALOG::onPageChanged( aEvent );

    size_t page = aEvent.GetSelection();

    if( m_physicalStackupPage > 0 )     // Don't run this during initialization
    {
        if( m_currentPage == m_physicalStackupPage || page == m_physicalStackupPage || page == m_netclassesPage
            || page == m_tuningProfilesPage || page == m_zoneHatchOffsetsPage )
        {
            m_layers = RESOLVE_PAGE( PANEL_SETUP_LAYERS, m_layersPage );
            m_physicalStackup = RESOLVE_PAGE( PANEL_SETUP_BOARD_STACKUP, m_physicalStackupPage );
            m_tuningProfiles = RESOLVE_PAGE( PANEL_SETUP_TUNING_PROFILES, m_tuningProfilesPage );
            m_netClasses = RESOLVE_PAGE( PANEL_SETUP_NETCLASSES, m_netclassesPage );
            m_zoneHatchOffsets = RESOLVE_PAGE( PANEL_SETUP_ZONE_HATCH_OFFSETS, m_zoneHatchOffsetsPage );
        }

        // Ensure layer page always gets updated even if we aren't moving towards it
        if( m_currentPage == m_physicalStackupPage )
        {
            m_layers->SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );

            // Avoid calling SyncCopperLayers twice if moving from stackup to tuning profiles directly
            m_tuningProfiles->SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );
        }

        if( page == m_physicalStackupPage )
        {
            m_physicalStackup->OnLayersOptionsChanged( m_layers->GetUILayerMask() );
        }
        else if( page == m_netclassesPage || m_currentPage == m_tuningProfilesPage )
        {
            m_netClasses->UpdateDelayProfileNames( m_tuningProfiles->GetDelayProfileNames() );
        }
        else if( page == m_tuningProfilesPage )
        {
            m_tuningProfiles->SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );
        }
        else if( page == m_zoneHatchOffsetsPage )
        {
            m_zoneHatchOffsets->SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );
        }

        if( Prj().IsReadOnly() )
        {
            KIUI::Disable( m_treebook->GetPage( page ) );
        }
    }

    m_currentPage = page;
}


void DIALOG_BOARD_SETUP::onAuxiliaryAction( wxCommandEvent& aEvent )
{
    DIALOG_IMPORT_SETTINGS importDlg( this, m_frame );

    if( importDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName boardFn( importDlg.GetFilePath() );
    wxFileName projectFn( boardFn );

    projectFn.SetExt( FILEEXT::ProjectFileExtension );

    if( !m_frame->GetSettingsManager()->LoadProject( projectFn.GetFullPath(), false ) )
    {
        wxString msg = wxString::Format( _( "Error importing settings from board:\n"
                                            "Associated project file %s could not be loaded" ),
                                         projectFn.GetFullPath() );
        DisplayErrorMessage( this, msg );

        return;
    }

    m_layers = RESOLVE_PAGE( PANEL_SETUP_LAYERS, m_layersPage );
    m_physicalStackup = RESOLVE_PAGE( PANEL_SETUP_BOARD_STACKUP, m_physicalStackupPage );
    m_boardFinish = RESOLVE_PAGE( PANEL_SETUP_BOARD_FINISH, m_boardFinishPage );

    // Flag so user can stop work if it will result in deleted inner copper layers
    // and still clean up this function properly.
    bool okToProceed = true;

    PROJECT* otherPrj = m_frame->GetSettingsManager()->GetProject( projectFn.GetFullPath() );

    IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );
    BOARD*              otherBoard = nullptr;

    try
    {
        WX_PROGRESS_REPORTER progressReporter( this, _( "Load PCB" ), 1, PR_CAN_ABORT );

        pi->SetProgressReporter( &progressReporter );

        otherBoard = pi->LoadBoard( boardFn.GetFullPath(), nullptr );

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
                wxString msg = wxString::Format( _( "Error loading board file:\n%s" ), boardFn.GetFullPath() );
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
            m_physicalStackup->ImportSettingsFrom( otherBoard );
            m_layers->ImportSettingsFrom( otherBoard );
            m_boardFinish->ImportSettingsFrom( otherBoard );
        }

        if( importDlg.m_TextAndGraphicsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_DEFAULTS, m_defaultsPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_FormattingOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_FORMATTING, m_formattingPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_ConstraintsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_CONSTRAINTS, m_constraintsPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_NetclassesOpt->GetValue() )
        {
            PROJECT_FILE& otherProjectFile = otherPrj->GetProjectFile();

            RESOLVE_PAGE( PANEL_SETUP_NETCLASSES,
                          m_netclassesPage )->ImportSettingsFrom( otherProjectFile.m_NetSettings );
        }

        if( importDlg.m_ComponentClassesOpt->GetValue() )
        {
            PROJECT_FILE& otherProjectFile = otherPrj->GetProjectFile();

            RESOLVE_PAGE( PANEL_ASSIGN_COMPONENT_CLASSES,
                          m_componentClassesPage )->ImportSettingsFrom( otherProjectFile.m_ComponentClassSettings );
        }

        if( importDlg.m_TracksAndViasOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_TRACKS_AND_VIAS, m_tracksAndViasPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_TeardropsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_TEARDROPS, m_teardropsPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_TuningPatternsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_TUNING_PATTERNS, m_tuningPatternsPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_MaskAndPasteOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_MASK_AND_PASTE, m_maskAndPastePage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_ZoneHatchingOffsetsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_ZONE_HATCH_OFFSETS, m_zoneHatchOffsetsPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_CustomRulesOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_RULES, m_customRulesPage )->ImportSettingsFrom( otherBoard );

        if( importDlg.m_SeveritiesOpt->GetValue() )
        {
            BOARD_DESIGN_SETTINGS& otherSettings = otherBoard->GetDesignSettings();

            RESOLVE_PAGE( PANEL_SETUP_SEVERITIES,
                          m_severitiesPage )->ImportSettingsFrom( otherSettings.m_DRCSeverities );
        }

        if( importDlg.m_TuningProfilesOpt->GetValue() )
        {
            PROJECT_FILE& otherProjectFile = otherPrj->GetProjectFile();

            RESOLVE_PAGE( PANEL_SETUP_TUNING_PROFILES,
                          m_tuningProfilesPage )->ImportSettingsFrom( otherProjectFile.TuningProfileParameters() );
        }

        if( otherPrj != &m_frame->Prj() )
            otherBoard->ClearProject();
    }

    // Clean up and free memory before leaving
    if( otherPrj != &m_frame->Prj() )
        m_frame->GetSettingsManager()->UnloadProject( otherPrj, false );

    delete otherBoard;
}
