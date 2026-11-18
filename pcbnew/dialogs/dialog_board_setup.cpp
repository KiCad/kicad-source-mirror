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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <jobs/scratch_doc.h>
#include <pcb_edit_frame.h>
#include <panel_setup_layers.h>
#include <panel_setup_defaults.h>
#include <panel_setup_constraints.h>
#include <panel_setup_tracks_and_vias.h>
#include <panel_setup_via_stacks.h>
#include <panel_setup_drill_chart.h>
#include <panel_setup_mask_and_paste.h>
#include <../board_stackup_manager/panel_board_stackup.h>
#include <../board_stackup_manager/panel_board_finish.h>
#include <confirm.h>
#include <board_design_settings.h>
#include <widgets/wx_infobar.h>
#include <kiface_base.h>
#include <drc/drc_item.h>
#include <dialog_import_settings.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <panel_embedded_files.h>
#include <dialogs/panel_setup_severities.h>
#include <dialogs/panel_setup_rules.h>
#include <dialogs/panel_setup_constraint_rules.h>
#include <drc/drc_engine.h>
#include <drc/rule_editor/drc_rule_document.h>
#include <ki_exception.h>
#include <dialogs/panel_setup_teardrops.h>
#include <dialogs/panel_setup_zones.h>
#include <dialogs/panel_setup_tuning_patterns.h>
#include <dialogs/panel_setup_netclasses.h>
#include <dialogs/panel_assign_component_classes.h>
#include <dialogs/panel_setup_tuning_profiles.h>
#include <panel_text_variables.h>
#include <project.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <settings/settings_manager.h>
#include <widgets/resettable_panel.h>
#include <widgets/wx_progress_reporters.h>
#include <wildcards_and_files_ext.h>

#include "dialog_board_setup.h"

#include <advanced_config.h>
#include <footprint.h>


#define RESOLVE_PAGE( T, pageIndex ) static_cast<T*>( m_treebook->ResolvePage( pageIndex ) )

DIALOG_BOARD_SETUP::DIALOG_BOARD_SETUP( PCB_EDIT_FRAME* aFrame, wxWindow* aParentWindow ) :
        PAGED_DIALOG( aParentWindow ? aParentWindow : aFrame, _( "Board Setup" ), false, false,
                      _( "Import Settings from Another Board..." ), wxSize( 980, 600 ) ),
        m_frame( aFrame ),
        m_layers( nullptr ),
        m_boardFinish( nullptr ),
        m_physicalStackup( nullptr ),
        m_zones( nullptr ),
        m_tuningProfiles( nullptr ),
        m_netClasses( nullptr ),
        m_currentPage( 0 ),
        m_layersPage( 0 ),
        m_physicalStackupPage( 0 ),
        m_boardFinishPage( 0 ),
        m_defaultsPage( 0 ),
        m_formattingPage( 0 ),
        m_textVarsPage( 0 ),
        m_customRulesPage( 0 ),
        m_tracksAndViasPage( 0 ),
        m_viaStacksPage( 0 ),
        m_teardropsPage( 0 ),
        m_tuningPatternsPage( 0 ),
        m_netclassesPage( 0 ),
        m_severitiesPage( 0 ),
        m_tuningProfilesPage( 0 ),
        m_initComplete( false )
{
    m_ruleDocument = std::make_unique<DRC_RULE_DOCUMENT>( m_frame->GetBoard() );
    m_ruleDocument->Load();

    m_stagedSettings = std::make_unique<BOARD_DESIGN_SETTINGS>( m_frame->GetBoard()->GetDesignSettings() );

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

    m_drillChartPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_DRILL_CHART( aParent, m_frame );
            }, _( "Drill Chart" ) );

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

    m_textVarsPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_TEXT_VARIABLES( aParent, &Prj() );
            }, _( "Text Variables" ) );

    m_treebook->AddPage( new wxPanel( GetTreebook() ), _( "Design Rules" ) );

    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_CONSTRAINTS( aParent, m_frame, m_ruleDocument.get(), *m_stagedSettings );
            },
            _( "Constraints" ) );

    const std::vector<std::pair<PANEL_SETUP_CONSTRAINT_RULES::FAMILY, wxString>> ruleFamilies = {
        { PANEL_SETUP_CONSTRAINT_RULES::FAMILY::CLEARANCE, _( "Clearance" ) },
        { PANEL_SETUP_CONSTRAINT_RULES::FAMILY::TRACKS_AND_VIAS, _( "Tracks and Vias" ) },
        { PANEL_SETUP_CONSTRAINT_RULES::FAMILY::HOLES, _( "Holes" ) },
        { PANEL_SETUP_CONSTRAINT_RULES::FAMILY::SILKSCREEN, _( "Silkscreen" ) },
        { PANEL_SETUP_CONSTRAINT_RULES::FAMILY::SOLDER_MASK, _( "Mask and Paste" ) },
        { PANEL_SETUP_CONSTRAINT_RULES::FAMILY::HIGH_SPEED, _( "High Speed" ) },
        { PANEL_SETUP_CONSTRAINT_RULES::FAMILY::FOOTPRINT_RULES, _( "Footprints" ) }
    };

    for( const auto& [family, familyTitle] : ruleFamilies )
    {
        size_t familyPage = m_treebook->GetPageCount();

        // An empty page, so selecting the family drops through to the first page under it, as
        // the other tree level nodes do.
        m_treebook->AddSubPage( new wxPanel( GetTreebook() ), familyTitle );

        if( family == PANEL_SETUP_CONSTRAINT_RULES::FAMILY::SOLDER_MASK )
        {
            // The rules in this family are read against these defaults, so they sit together.
            m_treebook->InsertLazySubPage(
                    familyPage,
                    [this]( wxWindow* aParent ) -> wxWindow*
                    {
                        return new PANEL_SETUP_MASK_AND_PASTE( aParent, m_frame, *m_stagedSettings );
                    },
                    _( "Solder Mask/Paste" ) );
        }

        for( const CONSTRAINT_SECTION& section : PANEL_SETUP_CONSTRAINT_RULES::SectionsFor( family ) )
        {
            m_ruleSearchSections.emplace( m_treebook->GetPageCount(), section );
            m_treebook->InsertLazySubPage(
                    familyPage,
                    [this, section]( wxWindow* aParent ) -> wxWindow*
                    {
                        return new PANEL_SETUP_CONSTRAINT_RULES( aParent, m_frame, m_ruleDocument.get(),
                                                                 *m_stagedSettings, section );
                    },
                    section.title );
        }
    }

    m_tracksAndViasPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_TRACKS_AND_VIAS( aParent, m_frame );
            },  _( "Pre-defined Sizes" ) );

    m_viaStacksPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_VIA_STACKS( aParent, m_frame );
            },
            _( "Microvia Stacks" ) );

    m_zonesPage = m_treebook->GetPageCount();
    m_treebook->AddLazySubPage(
            [this]( wxWindow* aParent ) -> wxWindow*
            {
                return new PANEL_SETUP_ZONES( aParent, m_frame, m_frame->GetBoard()->GetDesignSettings() );
            },
            _( "Zones" ) );

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
                return new PANEL_SETUP_RULES( aParent, m_frame, m_ruleDocument.get() );
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
                BOARD* board = m_frame->GetBoard();

                std::vector<EMBEDDED_FILES*> inheritedFiles;

                for( FOOTPRINT* fp : board->Footprints() )
                    inheritedFiles.push_back( fp->GetEmbeddedFiles() );

                return new PANEL_EMBEDDED_FILES( aParent, board->GetEmbeddedFiles(), NO_MARGINS, inheritedFiles );
            },
            _( "Embedded Files" ) );

    m_initComplete = true;

    std::set<size_t> reusablePages;

    for( const auto& [page, section] : m_ruleSearchSections )
        reusablePages.insert( page );

    reusablePages.insert( m_textVarsPage );
    reusablePages.insert( m_severitiesPage );

    // The rule pages hide their editor while the custom rules do not parse, and their fields name
    // the user's units, so the wording a page offers depends on both as well as on the project
    ReuseSearchLabels( wxS( "Board Setup" ),
                       wxString::Format( wxS( "%s:%d:%d" ), Prj().GetProjectFullName(),
                                         m_ruleDocument->HasUnparsedText(),
                                         static_cast<int>( m_frame->GetUserUnits() ) ),
                       reusablePages );

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
        m_treebook->ExpandNode( i );

    SetEvtHandlerEnabled( true );

    // Bound after DIALOG_SHIM's handlers so these run first
    Bind( wxEVT_BUTTON, &DIALOG_BOARD_SETUP::onCancelButton, this, wxID_CANCEL );
    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_BOARD_SETUP::onCloseWindow, this );

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
    Unbind( wxEVT_BUTTON, &DIALOG_BOARD_SETUP::onCancelButton, this, wxID_CANCEL );
    Unbind( wxEVT_CLOSE_WINDOW, &DIALOG_BOARD_SETUP::onCloseWindow, this );

    // Pages reading m_ruleDocument or m_stagedSettings would outlive them in ~wxWindow, so empty
    // their lazy holders now and leave the tree intact for ~PAGED_DIALOG
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        wxWindow* page = m_treebook->GetResolvedPage( i );

        if( page && page != m_treebook->GetPage( i )
            && ( dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( page ) || dynamic_cast<PANEL_SETUP_RULES*>( page )
                 || dynamic_cast<PANEL_SETUP_CONSTRAINTS*>( page )
                 || dynamic_cast<PANEL_SETUP_MASK_AND_PASTE*>( page ) ) )
        {
            m_treebook->GetPage( i )->DestroyChildren();
        }
    }
}


bool DIALOG_BOARD_SETUP::canDiscardChanges()
{
    auto* rules = static_cast<PANEL_SETUP_RULES*>( m_treebook->GetResolvedPage( m_customRulesPage ) );

    if( !m_ruleDocument->IsModified() && ( !rules || !rules->IsModified() ) )
        return true;

    return OKOrCancelDialog( this, _( "Confirmation" ), _( "Discard changes to custom rules?" ), wxEmptyString,
                             _( "Discard Changes" ) ) == wxID_OK;
}


void DIALOG_BOARD_SETUP::onCancelButton( wxCommandEvent& aEvent )
{
    if( canDiscardChanges() )
        aEvent.Skip();
}


void DIALOG_BOARD_SETUP::onCloseWindow( wxCloseEvent& aEvent )
{
    if( aEvent.CanVeto() && !canDiscardChanges() )
    {
        aEvent.Veto();
        return;
    }

    aEvent.Skip();
}


bool DIALOG_BOARD_SETUP::TransferDataFromWindow()
{
    if( !PAGED_DIALOG::TransferDataFromWindow() )
        return false;

    bool saveRules = m_ruleDocument->IsModified() && !m_frame->GetBoard()->GetDesignRulesPath().IsEmpty();

    if( saveRules )
    {
        wxString error;

        if( !m_ruleDocument->Save( &error ) )
        {
            SetError( wxString::Format( _( "Cannot save design rules: %s" ), error ), nullptr, nullptr );
            return false;
        }
    }

    BOARD_DESIGN_SETTINGS& boardSettings = m_frame->GetBoard()->GetDesignSettings();

    PANEL_SETUP_CONSTRAINTS::CopySettings( *m_stagedSettings, boardSettings );
    PANEL_SETUP_MASK_AND_PASTE::CopySettings( *m_stagedSettings, boardSettings );

    // The engine builds its implicit rules from the board defaults, so it starts after they land
    if( saveRules )
    {
        try
        {
            boardSettings.m_DRCEngine->InitEngine( m_frame->GetBoard()->GetDesignRulesPath() );
        }
        catch( PARSE_ERROR& )
        {
            // The rules are saved. DRC reports any compile problem when it runs.
        }
    }

    // References to renamed profiles can only be fixed up after all pages have saved
    auto* tuningProfiles =
            static_cast<PANEL_SETUP_TUNING_PROFILES*>( m_treebook->GetResolvedPage( m_tuningProfilesPage ) );

    if( tuningProfiles )
    {
        const std::map<wxString, wxString> renames = tuningProfiles->TakeProfileRenames();

        if( !renames.empty() )
        {
            std::shared_ptr<NET_SETTINGS>& netSettings = m_frame->Prj().GetProjectFile().NetSettings();

            auto remapProfile = [&renames]( const std::shared_ptr<NETCLASS>& aNetclass )
            {
                auto it = renames.find( aNetclass->GetTuningProfile() );

                if( it != renames.end() )
                    aNetclass->SetTuningProfile( it->second );
            };

            remapProfile( netSettings->GetDefaultNetclass() );

            for( const auto& [name, netclass] : netSettings->GetNetclasses() )
                remapProfile( netclass );

            netSettings->ClearAllCaches();
        }
    }

    return true;
}


void DIALOG_BOARD_SETUP::onPageChanged( wxBookCtrlEvent& aEvent )
{
    PAGED_DIALOG::onPageChanged( aEvent );

    size_t page = m_treebook->GetSelection();

    if( int selected = m_treebook->GetSelection(); selected != wxNOT_FOUND )
        refreshRulePage( selected );

    // A constraint can be set on its rule page or on the page it has always been set on, and
    // leaving either one writes it, so every open page showing that value reads it again
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        wxWindow* resolved = m_treebook->GetResolvedPage( i );

        if( auto* rulesPage = dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( resolved ) )
            rulesPage->RefreshDefaults();
        else if( auto* constraintsPage = dynamic_cast<PANEL_SETUP_CONSTRAINTS*>( resolved ) )
            constraintsPage->RefreshDefaults();
        else if( auto* maskPage = dynamic_cast<PANEL_SETUP_MASK_AND_PASTE*>( resolved ) )
            maskPage->RefreshDefaults();
    }

    if( m_initComplete )
    {
        if( m_currentPage == m_physicalStackupPage || page == m_physicalStackupPage || page == m_netclassesPage
            || page == m_tuningProfilesPage || page == m_zonesPage || page == m_boardFinishPage )
        {
            m_layers = RESOLVE_PAGE( PANEL_SETUP_LAYERS, m_layersPage );
            m_physicalStackup = RESOLVE_PAGE( PANEL_SETUP_BOARD_STACKUP, m_physicalStackupPage );
            m_tuningProfiles = RESOLVE_PAGE( PANEL_SETUP_TUNING_PROFILES, m_tuningProfilesPage );
            m_netClasses = RESOLVE_PAGE( PANEL_SETUP_NETCLASSES, m_netclassesPage );
            m_zones = RESOLVE_PAGE( PANEL_SETUP_ZONES, m_zonesPage );
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
            m_netClasses->RemapDelayProfileNames( m_tuningProfiles->TakeProfileRenames() );
            m_netClasses->UpdateDelayProfileNames( m_tuningProfiles->GetDelayProfileNames() );
        }
        else if( page == m_tuningProfilesPage )
        {
            m_tuningProfiles->SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );
        }
        else if( page == m_zonesPage )
        {
            m_zones->SyncCopperLayers( m_physicalStackup->GetCopperLayerCount() );
        }

        if( page == m_boardFinishPage )
        {
            m_physicalStackup->OnLayersOptionsChanged( m_layers->GetUILayerMask() );
            m_boardFinish->SetStackup( m_physicalStackup->GetStackup(), m_physicalStackup->GetCopperLayerCount() );
        }

        if( Prj().IsReadOnly() )
        {
            KIUI::Disable( m_treebook->GetPage( page ) );
        }
    }

    m_currentPage = page;
}


void DIALOG_BOARD_SETUP::forEachOpenRulePage( const std::function<void( PANEL_SETUP_CONSTRAINT_RULES* )>& aAction )
{
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        if( auto* page = dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( m_treebook->GetResolvedPage( i ) ) )
            aAction( page );
    }
}


void DIALOG_BOARD_SETUP::refreshStagedPages( const wxString& aGroup )
{
    // Pages of the other group may hold edits not yet written to the staged settings
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        wxWindow* resolved = m_treebook->GetResolvedPage( i );

        if( auto* rulesPage = dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( resolved ) )
        {
            if( rulesPage->ImportGroup() == aGroup )
                rulesPage->RefreshDefaults();
        }
        else if( dynamic_cast<PANEL_SETUP_CONSTRAINTS*>( resolved ) )
        {
            if( aGroup == _( "Constraints" ) )
                resolved->TransferDataToWindow();
        }
        else if( dynamic_cast<PANEL_SETUP_MASK_AND_PASTE*>( resolved ) )
        {
            if( aGroup == _( "Solder Mask/Paste" ) )
                resolved->TransferDataToWindow();
        }
    }
}


const DRC_RULE_DOCUMENT& DIALOG_BOARD_SETUP::searchDocument() const
{
    auto* textPage = static_cast<PANEL_SETUP_RULES*>( m_treebook->GetResolvedPage( m_customRulesPage ) );

    return textPage ? textPage->GetSearchDocument() : *m_ruleDocument;
}


void DIALOG_BOARD_SETUP::BeginSearchPass()
{
    m_searchRuleIndices.reset();

    int       selected = m_treebook->GetSelection();
    wxWindow* page = selected == wxNOT_FOUND ? nullptr : m_treebook->GetResolvedPage( selected );

    // Leaving the open page stages its defaults, and the pages sharing them search the staged values
    if( auto* rulesPage = dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( page ) )
        rulesPage->StageEditedDefaults();
    else if( auto* constraintsPage = dynamic_cast<PANEL_SETUP_CONSTRAINTS*>( page ) )
        constraintsPage->StageEditedDefaults();
    else if( auto* maskPage = dynamic_cast<PANEL_SETUP_MASK_AND_PASTE*>( page ) )
        maskPage->StageEditedDefaults();
}


void DIALOG_BOARD_SETUP::AppendSearchTerms( size_t aPage, std::vector<wxString>& aTerms ) const
{
    wxWindow* page = m_treebook->GetResolvedPage( aPage );

    if( auto ruleSection = m_ruleSearchSections.find( aPage ); ruleSection != m_ruleSearchSections.end() )
    {
        static const std::vector<int> noRules;

        CONSTRAINT_SECTION       section = ruleSection->second;
        const DRC_RULE_DOCUMENT& document = searchDocument();
        const std::vector<int>*  rules = &noRules;
        bool                     selected = static_cast<int>( aPage ) == m_treebook->GetSelection();

        // Leaving a page stages its defaults, so only the open page's own fields are current
        if( page && selected )
            section.defaults.clear();

        // Every rule page is asked in turn, so sort the rules onto pages once per query
        if( !m_searchRuleIndices )
            m_searchRuleIndices = document.IndicesByType();

        if( section.ruleType )
        {
            if( auto it = m_searchRuleIndices->find( *section.ruleType ); it != m_searchRuleIndices->end() )
                rules = &it->second;
        }

        PANEL_SETUP_CONSTRAINT_RULES::AppendSearchTerms( section, m_frame, *m_stagedSettings, document, *rules,
                                                         aTerms );

        if( auto* rulesPage = dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( page ) )
        {
            aTerms.push_back( rulesPage->GetSearchActionLabel() );

            // Forms on pages not shown may still hold values from before the raw text was edited
            AppendControlSearchTerms( rulesPage, aTerms, selected );
        }
        else if( !m_frame->GetBoard()->GetDesignRulesPath().IsEmpty() && !document.HasUnparsedText() )
        {
            aTerms.push_back( _( "Add Rule" ) );
        }

        return;
    }

    // A page reusing the labels of an earlier opening is answered from the project
    if( !page )
    {
        if( aPage == m_textVarsPage )
            AppendTextVarSearchTerms( aTerms );

        return;
    }

    bool                                       selected = static_cast<int>( aPage ) == m_treebook->GetSelection();
    std::vector<std::pair<wxString, wxString>> stale;

    if( PANEL_SETUP_RULES* textPage = dynamic_cast<PANEL_SETUP_RULES*>( page ) )
    {
        aTerms.push_back( textPage->GetSearchDocument().GetText() );
        return;
    }

    // A page not shown still holds what it last read, and searches the staged values it will read
    if( auto* constraintsPage = dynamic_cast<PANEL_SETUP_CONSTRAINTS*>( page ); constraintsPage && !selected )
        stale = constraintsPage->StaleSearchValues();
    else if( auto* maskPage = dynamic_cast<PANEL_SETUP_MASK_AND_PASTE*>( page ); maskPage && !selected )
        stale = maskPage->StaleSearchValues();

    const size_t first = aTerms.size();

    PAGED_DIALOG::AppendSearchTerms( aPage, aTerms );

    for( const auto& [shown, current] : stale )
        ReplaceSearchTerm( aTerms, first, shown, current );
}


void DIALOG_BOARD_SETUP::refreshRulePage( size_t aPage )
{
    wxWindow* resolved = m_treebook->GetResolvedPage( aPage );

    if( auto* rulesPage = dynamic_cast<PANEL_SETUP_CONSTRAINT_RULES*>( resolved ) )
        rulesPage->RefreshFromDocument();
    else if( auto* textPage = dynamic_cast<PANEL_SETUP_RULES*>( resolved ) )
        textPage->RefreshFromDocument();
}


void DIALOG_BOARD_SETUP::onAuxiliaryAction( wxCommandEvent& aEvent )
{
    DIALOG_IMPORT_SETTINGS importDlg( this, m_frame );

    if( importDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName boardFn( importDlg.GetFilePath() );

    SCRATCH_PROJECT scratch( *m_frame->GetSettingsManager(), boardFn.GetFullPath(),
                             /*aRequireProjectFile=*/true );

    if( !scratch.IsValid() )
    {
        wxFileName projectFn( boardFn );
        projectFn.SetExt( FILEEXT::ProjectFileExtension );

        wxString msg = wxString::Format( _( "Error importing settings from board:\n"
                                            "Associated project file %s could not be loaded" ),
                                         projectFn.GetFullPath() );
        DisplayErrorMessage( this, msg );

        return;
    }

    PROJECT* otherPrj = scratch.GetProject();

    m_layers = RESOLVE_PAGE( PANEL_SETUP_LAYERS, m_layersPage );
    m_physicalStackup = RESOLVE_PAGE( PANEL_SETUP_BOARD_STACKUP, m_physicalStackupPage );
    m_boardFinish = RESOLVE_PAGE( PANEL_SETUP_BOARD_FINISH, m_boardFinishPage );

    // Flag so user can stop work if it will result in deleted inner copper layers
    // and still clean up this function properly.
    bool okToProceed = true;

    IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );
    std::unique_ptr<BOARD> otherBoard;

    try
    {
        WX_PROGRESS_REPORTER progressReporter( this, _( "Load PCB" ), 1, PR_CAN_ABORT );

        pi->SetProgressReporter( &progressReporter );

        otherBoard = pi->LoadBoard( boardFn.GetFullPath() );

        if( importDlg.m_LayersOpt->GetValue() )
        {
            BOARD* loadedBoard = m_frame->GetBoard();

            // Check if "Import Settings" board has more layers than the current board.
            okToProceed = m_layers->CheckCopperLayerCount( loadedBoard, otherBoard.get() );
        }
    }
    catch( const IO_CANCELLED& )
    {
        // A user-cancelled load is not an error.
        return;
    }
    catch( const IO_ERROR& ioe )
    {
        // You wouldn't think boardFn.GetFullPath() would throw, but we get a stack buffer
        // underflow from ASAN.  While it's probably an ASAN error, a second try/catch doesn't
        // cost us much.
        try
        {
            wxString msg = wxString::Format( _( "Error loading board file:\n%s" ), boardFn.GetFullPath() );
            DisplayErrorMessage( this, msg, ioe.What() );
        }
        catch(...)
        {
            // That was already our best-efforts
        }

        return;
    }

    if( okToProceed )
    {
        BOARD* other = otherBoard.get();

        other->SetProject( otherPrj );

        // Always ClearProject before destruction, even if a later
        // ImportSettingsFrom throws. Skipping clear when otherPrj is the
        // active project preserves the live editor's BoardSettings.
        const bool activeProject = ( otherPrj == &m_frame->Prj() );

        struct BOARD_DETACH_GUARD
        {
            BOARD* board;
            bool   skip;
            ~BOARD_DETACH_GUARD() { if( board && !skip ) board->ClearProject(); }
        } detachGuard{ other, activeProject };

        // If layers options are imported, import also the stackup
        // layers options and stackup are linked, so they cannot be imported
        // separately, and stackup can be imported only after layers options
        if( importDlg.m_LayersOpt->GetValue() )
        {
            m_physicalStackup->ImportSettingsFrom( other );
            m_layers->ImportSettingsFrom( other );
            m_boardFinish->ImportSettingsFrom( other );
        }

        if( importDlg.m_TextAndGraphicsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_DEFAULTS, m_defaultsPage )->ImportSettingsFrom( other );

        if( importDlg.m_FormattingOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_FORMATTING, m_formattingPage )->ImportSettingsFrom( other );

        if( importDlg.m_ConstraintsOpt->GetValue() )
        {
            PANEL_SETUP_CONSTRAINTS::CopySettings( other->GetDesignSettings(), *m_stagedSettings );
            refreshStagedPages( _( "Constraints" ) );
        }

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
        {
            RESOLVE_PAGE( PANEL_SETUP_TRACKS_AND_VIAS, m_tracksAndViasPage )->ImportSettingsFrom( other );
            RESOLVE_PAGE( PANEL_SETUP_VIA_STACKS, m_viaStacksPage )->ImportSettingsFrom( other );
        }

        if( importDlg.m_ZonesOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_ZONES, m_zonesPage )->ImportSettingsFrom( other );

        if( importDlg.m_TeardropsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_TEARDROPS, m_teardropsPage )->ImportSettingsFrom( other );

        if( importDlg.m_TuningPatternsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_TUNING_PATTERNS, m_tuningPatternsPage )->ImportSettingsFrom( other );

        if( importDlg.m_MaskAndPasteOpt->GetValue() )
        {
            PANEL_SETUP_MASK_AND_PASTE::CopySettings( other->GetDesignSettings(), *m_stagedSettings );
            refreshStagedPages( _( "Solder Mask/Paste" ) );
        }

        if( importDlg.m_ZoneHatchingOffsetsOpt->GetValue() )
            RESOLVE_PAGE( PANEL_SETUP_ZONES, m_zonesPage )->ImportHatchOffsetsFrom( other );

        if( importDlg.m_CustomRulesOpt->GetValue() )
        {
            m_ruleDocument->LoadFrom( other->GetDesignRulesPath() );

            refreshRulePage( m_customRulesPage );

            forEachOpenRulePage(
                    []( PANEL_SETUP_CONSTRAINT_RULES* aPage )
                    {
                        aPage->RefreshFromDocument();
                    } );
        }

        if( importDlg.m_SeveritiesOpt->GetValue() )
        {
            BOARD_DESIGN_SETTINGS& otherSettings = other->GetDesignSettings();

            RESOLVE_PAGE( PANEL_SETUP_SEVERITIES,
                          m_severitiesPage )->ImportSettingsFrom( otherSettings.m_DRCSeverities );
        }

        if( importDlg.m_TuningProfilesOpt->GetValue() )
        {
            PROJECT_FILE& otherProjectFile = otherPrj->GetProjectFile();

            RESOLVE_PAGE( PANEL_SETUP_TUNING_PROFILES,
                          m_tuningProfilesPage )->ImportSettingsFrom( otherProjectFile.TuningProfileParameters() );
        }

    }
}
