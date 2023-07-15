/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <gestfich.h>
#include <sch_screen.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <project.h>
#include <kiface_base.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <sch_marker.h>
#include <connection_graph.h>
#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <dialog_erc.h>
#include <erc.h>
#include <id.h>
#include <confirm.h>
#include <common.h>
#include <widgets/wx_html_report_box.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/hyperlink.h>
#include <erc_item.h>
#include <eeschema_settings.h>
#include <string_utils.h>
#include <kiplatform/ui.h>


wxDEFINE_EVENT( EDA_EVT_CLOSE_ERC_DIALOG, wxCommandEvent );


// wxWidgets spends *far* too long calcuating column widths (most of it, believe it or
// not, in repeatedly creating/destroying a wxDC to do the measurement in).
// Use default column widths instead.
static int DEFAULT_SINGLE_COL_WIDTH = 660;


static SCHEMATIC*            g_lastERCSchematic = nullptr;
static bool                  g_lastERCRun = false;
static std::vector<wxString> g_lastERCIgnored;


DIALOG_ERC::DIALOG_ERC( SCH_EDIT_FRAME* parent ) :
        DIALOG_ERC_BASE( parent ),
        PROGRESS_REPORTER_BASE( 1 ),
        m_parent( parent ),
        m_markerTreeModel( nullptr ),
        m_running( false ),
        m_ercRun( false ),
        m_centerMarkerOnIdle( nullptr ),
        m_severities( 0 )
{
    m_currentSchematic = &parent->Schematic();

    SetName( DIALOG_ERC_WINDOW_NAME ); // Set a window name to be able to find it

    EESCHEMA_SETTINGS* settings = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    m_severities = settings->m_Appearance.erc_severities;

    m_messages->SetImmediateMode();

    m_markerProvider = std::make_shared<SHEETLIST_ERC_ITEMS_PROVIDER>( &m_parent->Schematic() );

    m_markerTreeModel = new ERC_TREE_MODEL( parent, m_markerDataView );
    m_markerDataView->AssociateModel( m_markerTreeModel );
    m_markerTreeModel->Update( m_markerProvider, m_severities );

    m_ignoredList->InsertColumn( 0, wxEmptyString, wxLIST_FORMAT_LEFT, DEFAULT_SINGLE_COL_WIDTH );

    if( m_currentSchematic == g_lastERCSchematic )
    {
        m_ercRun = g_lastERCRun;

        for( const wxString& str : g_lastERCIgnored )
            m_ignoredList->InsertItem( m_ignoredList->GetItemCount(), str );
    }

    m_notebook->SetSelection( 0 );

    SetupStandardButtons( { { wxID_OK,     _( "Run ERC" ) },
                            { wxID_CANCEL, _( "Close" )   } } );

    m_violationsTitleTemplate = m_notebook->GetPageText( 0 );
    m_ignoredTitleTemplate    = m_notebook->GetPageText( 1 );

    m_errorsBadge->SetMaximumNumber( 999 );
    m_warningsBadge->SetMaximumNumber( 999 );
    m_exclusionsBadge->SetMaximumNumber( 999 );

    UpdateAnnotationWarning();

    Layout();

    SetFocus();

    syncCheckboxes();
    updateDisplayedCounts();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_ERC::~DIALOG_ERC()
{
    g_lastERCSchematic = m_currentSchematic;
    g_lastERCRun = m_ercRun;

    g_lastERCIgnored.clear();

    for( int ii = 0; ii < m_ignoredList->GetItemCount(); ++ii )
        g_lastERCIgnored.push_back( m_ignoredList->GetItemText( ii ) );

    EESCHEMA_SETTINGS* settings = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( settings );

    if( settings )
        settings->m_Appearance.erc_severities = m_severities;

    m_markerTreeModel->DecRef();
}


void DIALOG_ERC::UpdateAnnotationWarning()
{
    if( m_parent->CheckAnnotate( []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* )
                                 { } ) )
    {
        if( !m_infoBar->IsShown() )
        {
            wxHyperlinkCtrl* button = new wxHyperlinkCtrl( m_infoBar, wxID_ANY,
                                                           _( "Show Annotation dialog" ),
                                                           wxEmptyString );

            button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& aEvent )>(
                          [&]( wxHyperlinkEvent& aEvent )
                          {
                              wxHtmlLinkEvent htmlEvent( aEvent.GetId(),
                                                         wxHtmlLinkInfo( aEvent.GetURL() ) );
                              OnLinkClicked( htmlEvent );
                          } ) );

            m_infoBar->RemoveAllButtons();
            m_infoBar->AddButton( button );
            m_infoBar->ShowMessage( _( "Schematic is not fully annotated. "
                                       "ERC results will be incomplete." ) );
        }
    }
    else
    {
        if( m_infoBar->IsShown() )
        {
            m_infoBar->RemoveAllButtons();
            m_infoBar->Hide();
        }
    }
}


bool DIALOG_ERC::updateUI()
{
    // If ERC checks ever get slow enough we'll want a progress indicator...
    //
    // double cur = (double) m_progress.load() / m_maxProgress;
    // cur = std::max( 0.0, std::min( cur, 1.0 ) );
    //
    // m_gauge->SetValue( KiROUND( cur * 1000.0 ) );
    // wxSafeYield( this );

    return !m_cancelled;
}


void DIALOG_ERC::AdvancePhase( const wxString& aMessage )
{
    // Will also call Report( aMessage ):
    PROGRESS_REPORTER_BASE::AdvancePhase( aMessage );
    SetCurrentProgress( 0.0 );
}


void DIALOG_ERC::Report( const wxString& aMessage )
{
    m_messages->Report( aMessage );
}


void DIALOG_ERC::updateDisplayedCounts()
{
    int numErrors = 0;
    int numWarnings = 0;
    int numExcluded = 0;

    int numMarkers = 0;

    if( m_markerProvider )
    {
        numMarkers += m_markerProvider->GetCount();
        numErrors += m_markerProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_markerProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_markerProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    bool markersOverflowed = false;

    // We don't currently have a limit on ERC violations, so the above is always false.

    wxString num;
    wxString msg;

    if( m_ercRun )
    {
        num.Printf( markersOverflowed ? wxT( "%d+" ) : wxT( "%d" ), numMarkers );
        msg.Printf( m_violationsTitleTemplate, num );
    }
    else
    {
        msg = m_violationsTitleTemplate;
        msg.Replace( wxT( "(%s)" ), wxEmptyString );
    }

    m_notebook->SetPageText( 0, msg );

    if( m_ercRun )
    {
        num.Printf( wxT( "%d" ), m_ignoredList->GetItemCount() );
        msg.sprintf( m_ignoredTitleTemplate, num );
    }
    else
    {
        msg = m_ignoredTitleTemplate;
        msg.Replace( wxT( "(%s)" ), wxEmptyString );
    }

    m_notebook->SetPageText( 1, msg );

    if( !m_ercRun && numErrors == 0 )
        numErrors = -1;

    if( !m_ercRun && numWarnings == 0 )
        numWarnings = -1;

    m_errorsBadge->UpdateNumber( numErrors, RPT_SEVERITY_ERROR );
    m_warningsBadge->UpdateNumber( numWarnings, RPT_SEVERITY_WARNING );
    m_exclusionsBadge->UpdateNumber( numExcluded, RPT_SEVERITY_EXCLUSION );
}


void DIALOG_ERC::OnDeleteOneClick( wxCommandEvent& aEvent )
{
    if( m_notebook->GetSelection() == 0 )
    {
        // Clear the selection.  It may be the selected ERC marker.
        m_parent->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

        m_markerTreeModel->DeleteCurrentItem( true );

        // redraw the schematic
        redrawDrawPanel();
    }

    updateDisplayedCounts();
}


void DIALOG_ERC::OnDeleteAllClick( wxCommandEvent& event )
{
    bool includeExclusions = false;
    int  numExcluded = 0;

    if( m_markerProvider )
        numExcluded += m_markerProvider->GetCount( RPT_SEVERITY_EXCLUSION );

    if( numExcluded > 0 )
    {
        wxMessageDialog dlg( this, _( "Delete exclusions too?" ), _( "Delete All Markers" ),
                             wxYES_NO | wxCANCEL | wxCENTER | wxICON_QUESTION );
        dlg.SetYesNoLabels( _( "Errors and Warnings Only" ),
                            _( "Errors, Warnings and Exclusions" ) );

        int ret = dlg.ShowModal();

        if( ret == wxID_CANCEL )
            return;
        else if( ret == wxID_NO )
            includeExclusions = true;
    }

    deleteAllMarkers( includeExclusions );

    // redraw the schematic
    redrawDrawPanel();
    updateDisplayedCounts();
}


void DIALOG_ERC::OnCancelClick( wxCommandEvent& aEvent )
{
    if( m_running )
    {
        m_cancelled = true;
        return;
    }

    m_parent->FocusOnItem( nullptr );

    aEvent.Skip();
}


void DIALOG_ERC::OnCloseErcDialog( wxCloseEvent& aEvent )
{
    m_parent->FocusOnItem( nullptr );

    // Dialog is mode-less so let the parent know that it needs to be destroyed.
    if( !IsModal() && !IsQuasiModal() )
    {
        wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_ERC_DIALOG, wxID_ANY );

        wxWindow* parent = GetParent();

        if( parent )
            wxQueueEvent( parent, evt );
    }

    aEvent.Skip();
}


static int RPT_SEVERITY_ALL = RPT_SEVERITY_WARNING | RPT_SEVERITY_ERROR | RPT_SEVERITY_EXCLUSION;


void DIALOG_ERC::syncCheckboxes()
{
    m_showAll->SetValue( m_severities == RPT_SEVERITY_ALL );
    m_showErrors->SetValue( m_severities & RPT_SEVERITY_ERROR );
    m_showWarnings->SetValue( m_severities & RPT_SEVERITY_WARNING );
    m_showExclusions->SetValue( m_severities & RPT_SEVERITY_EXCLUSION );
}


void DIALOG_ERC::OnLinkClicked( wxHtmlLinkEvent& event )
{
    wxCommandEvent dummy;
    m_parent->OnAnnotate( dummy );
}


void DIALOG_ERC::OnRunERCClick( wxCommandEvent& event )
{
    wxBusyCursor busy;

    SCHEMATIC* sch = &m_parent->Schematic();

    UpdateAnnotationWarning();

    m_parent->RecordERCExclusions();
    deleteAllMarkers( true );

    std::vector<std::reference_wrapper<RC_ITEM>> violations = ERC_ITEM::GetItemsWithSeverities();
    m_ignoredList->DeleteAllItems();

    for( std::reference_wrapper<RC_ITEM>& item : violations )
    {
        if( sch->ErcSettings().GetSeverity( item.get().GetErrorCode() ) == RPT_SEVERITY_IGNORE )
        {
            m_ignoredList->InsertItem( m_ignoredList->GetItemCount(),
                                       wxT( " • " ) + item.get().GetErrorText() );
        }
    }

    m_ignoredList->SetColumnWidth( 0, m_ignoredList->GetParent()->GetClientSize().x - 20 );

    Raise();

    m_runningResultsBook->ChangeSelection( 0 );   // Display the "Tests Running..." tab
    m_messages->Clear();
    wxYield();                                    // Allow time slice to refresh Messages

    m_running = true;
    m_sdbSizer1Cancel->SetLabel( _( "Cancel" ) );
    m_sdbSizer1OK->Enable( false );
    m_deleteOneMarker->Enable( false );
    m_deleteAllMarkers->Enable( false );
    m_saveReport->Enable( false );

    sch->GetSheets().AnnotatePowerSymbols();

    int itemsNotAnnotated = m_parent->CheckAnnotate(
            []( ERCE_T aType, const wxString& aMsg, SCH_REFERENCE* aItemA, SCH_REFERENCE* aItemB )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( aType );
                ercItem->SetErrorMessage( aMsg );

                if( aItemB )
                    ercItem->SetItems( aItemA->GetSymbol(), aItemB->GetSymbol() );
                else
                    ercItem->SetItems( aItemA->GetSymbol() );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, aItemA->GetSymbol()->GetPosition() );
                aItemA->GetSheetPath().LastScreen()->Append( marker );
            } );

    testErc();

    if( itemsNotAnnotated )
        m_messages->ReportHead( wxString::Format( _( "%d symbol(s) require annotation.<br><br>" ),
                                                  itemsNotAnnotated ), RPT_SEVERITY_INFO );

    if( m_cancelled )
        m_messages->Report( _( "-------- ERC cancelled by user.<br><br>" ), RPT_SEVERITY_INFO );
    else
        m_messages->Report( _( "Done.<br><br>" ), RPT_SEVERITY_INFO );

    Raise();
    wxYield();                                    // Allow time slice to refresh Messages

    m_running = false;
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1OK->Enable( true );
    m_deleteOneMarker->Enable( true );
    m_deleteAllMarkers->Enable( true );
    m_saveReport->Enable( true );

    if( !m_cancelled )
    {
        wxMilliSleep( 500 );
        m_runningResultsBook->ChangeSelection( 1 );
#ifndef __WXGTK__
        KIPLATFORM::UI::ForceFocus( m_markerDataView );
#endif
    }

    m_ercRun = true;
    redrawDrawPanel();
    updateDisplayedCounts();
}


void DIALOG_ERC::redrawDrawPanel()
{
    WINDOW_THAWER thawer( m_parent );

    m_parent->GetCanvas()->Refresh();
}


void DIALOG_ERC::testErc()
{
    wxFileName fn;

    SCHEMATIC* sch = &m_parent->Schematic();

    SCH_SCREENS screens( sch->Root() );
    ERC_SETTINGS& settings = sch->ErcSettings();
    ERC_TESTER tester( sch );

    // Test duplicate sheet names inside a given sheet.  While one can have multiple references
    // to the same file, each must have a unique name.
    if( settings.IsTestEnabled( ERCE_DUPLICATE_SHEET_NAME ) )
    {
        AdvancePhase( _( "Checking sheet names..." ) );
        tester.TestDuplicateSheetNames( true );
    }

    if( settings.IsTestEnabled( ERCE_BUS_ALIAS_CONFLICT ) )
    {
        AdvancePhase( _( "Checking bus conflicts..." ) );
        tester.TestConflictingBusAliases();
    }

    // The connection graph has a whole set of ERC checks it can run
    AdvancePhase( _( "Checking conflicts..." ) );

    // If we are using the new connectivity, make sure that we do a full-rebuild
    if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
        m_parent->RecalculateConnections( nullptr, GLOBAL_CLEANUP );
    else
        m_parent->RecalculateConnections( nullptr, NO_CLEANUP );

    sch->ConnectionGraph()->RunERC();

    AdvancePhase( _( "Checking units..." ) );

    // Test is all units of each multiunit symbol have the same footprint assigned.
    if( settings.IsTestEnabled( ERCE_DIFFERENT_UNIT_FP ) )
    {
        AdvancePhase( _( "Checking footprints..." ) );
        tester.TestMultiunitFootprints();
    }

    if( settings.IsTestEnabled( ERCE_MISSING_UNIT )
            || settings.IsTestEnabled( ERCE_MISSING_INPUT_PIN )
            || settings.IsTestEnabled( ERCE_MISSING_POWER_INPUT_PIN )
            || settings.IsTestEnabled( ERCE_MISSING_BIDI_PIN ) )
    {
        tester.TestMissingUnits();
    }

    AdvancePhase( _( "Checking pins..." ) );

    if( settings.IsTestEnabled( ERCE_DIFFERENT_UNIT_NET ) )
        tester.TestMultUnitPinConflicts();

    // Test pins on each net against the pin connection table
    if( settings.IsTestEnabled( ERCE_PIN_TO_PIN_ERROR )
      || settings.IsTestEnabled( ERCE_POWERPIN_NOT_DRIVEN )
      || settings.IsTestEnabled( ERCE_PIN_NOT_DRIVEN ) )
    {
         tester.TestPinToPin();
    }

    // Test similar labels (i;e. labels which are identical when
    // using case insensitive comparisons)
    if( settings.IsTestEnabled( ERCE_SIMILAR_LABELS ) )
    {
        AdvancePhase( _( "Checking labels..." ) );
        tester.TestSimilarLabels();
    }

    if( settings.IsTestEnabled( ERCE_UNRESOLVED_VARIABLE ) )
    {
        AdvancePhase( _( "Checking for unresolved variables..." ) );
        tester.TestTextVars( m_parent->GetCanvas()->GetView()->GetDrawingSheet() );
    }

    if( settings.IsTestEnabled( ERCE_SIMULATION_MODEL ) )
    {
        AdvancePhase( _( "Checking SPICE models..." ) );
        tester.TestSimModelIssues();
    }

    if( settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) )
    {
        AdvancePhase( _( "Checking no connect pins for connections..." ) );
        tester.TestNoConnectPins();
    }

    if( settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES ) )
    {
        AdvancePhase( _( "Checking for library symbol issues..." ) );
        tester.TestLibSymbolIssues();
    }

    if( settings.IsTestEnabled( ERCE_ENDPOINT_OFF_GRID ) )
    {
        AdvancePhase( _( "Checking for off grid pins and wires..." ) );
        tester.TestOffGridEndpoints( m_parent->GetCanvas()->GetView()->GetGAL()->GetGridSize().x );
    }

    m_parent->ResolveERCExclusions();

    // Update marker list:
    m_markerTreeModel->Update( m_markerProvider, m_severities );

    // Display new markers from the current screen:
    for( SCH_ITEM* marker : m_parent->GetScreen()->Items().OfType( SCH_MARKER_T ) )
    {
        m_parent->GetCanvas()->GetView()->Remove( marker );
        m_parent->GetCanvas()->GetView()->Add( marker );
    }

    m_parent->GetCanvas()->Refresh();
}


void DIALOG_ERC::OnERCItemSelected( wxDataViewEvent& aEvent )
{
    const KIID&     itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    SCH_SHEET_PATH  sheet;
    SCH_ITEM*       item = m_parent->Schematic().GetSheets().GetItem( itemID, &sheet );

    if( m_centerMarkerOnIdle )
    {
        // we already came from a cross-probe of the marker in the document; don't go
        // around in circles
    }
    else if( item && item->GetClass() != wxT( "DELETED_SHEET_ITEM" ) )
    {
        const RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );

        if( node )
        {
            // Determine the owning sheet for sheet-specific items
            std::shared_ptr<ERC_ITEM> ercItem =
                    std::static_pointer_cast<ERC_ITEM>( node->m_RcItem );
            switch( node->m_Type )
            {
            case RC_TREE_NODE::MARKER:
                if( ercItem->IsSheetSpecific() )
                    sheet = ercItem->GetSpecificSheetPath();
                break;
            case RC_TREE_NODE::MAIN_ITEM:
                if( ercItem->MainItemHasSheetPath() )
                    sheet = ercItem->GetMainItemSheetPath();
                break;
            case RC_TREE_NODE::AUX_ITEM:
                if( ercItem->AuxItemHasSheetPath() )
                    sheet = ercItem->GetAuxItemSheetPath();
                break;
            default:
                break;
            }
        }

        WINDOW_THAWER thawer( m_parent );

        if( !sheet.empty() && sheet != m_parent->GetCurrentSheet() )
        {
            m_parent->GetToolManager()->RunAction( ACTIONS::cancelInteractive );
            m_parent->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

            m_parent->SetCurrentSheet( sheet );
            m_parent->DisplayCurrentSheet();
            m_parent->RedrawScreen(  m_parent->GetScreen()->m_ScrollCenter, false );
        }

        m_parent->FocusOnItem( item );
        redrawDrawPanel();
    }

    aEvent.Skip();
}


void DIALOG_ERC::OnERCItemDClick( wxDataViewEvent& aEvent )
{
    if( aEvent.GetItem().IsOk() )
    {
        // turn control over to m_parent, hide this DIALOG_ERC window,
        // no destruction so we can preserve listbox cursor
        if( !IsModal() )
            Show( false );
    }

    aEvent.Skip();
}


void DIALOG_ERC::OnERCItemRClick( wxDataViewEvent& aEvent )
{
    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );

    if( !node )
        return;

    ERC_SETTINGS& settings = m_parent->Schematic().ErcSettings();

    std::shared_ptr<RC_ITEM>  rcItem = node->m_RcItem;
    wxString  listName;
    wxMenu    menu;

    switch( settings.GetSeverity( rcItem->GetErrorCode() ) )
    {
    case RPT_SEVERITY_ERROR:   listName = _( "errors" );      break;
    case RPT_SEVERITY_WARNING: listName = _( "warnings" );    break;
    default:                   listName = _( "appropriate" ); break;
    }

    if( rcItem->GetParent()->IsExcluded() )
    {
        menu.Append( 1, _( "Remove exclusion for this violation" ),
                     wxString::Format( _( "It will be placed back in the %s list" ), listName ) );
    }
    else
    {
        menu.Append( 2, _( "Exclude this violation" ),
                     wxString::Format( _( "It will be excluded from the %s list" ), listName ) );
    }

    menu.AppendSeparator();

    if( rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_WARNING
      || rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
    {
        // Pin to pin severities edited through pin conflict map
    }
    else if( settings.GetSeverity( rcItem->GetErrorCode() ) == RPT_SEVERITY_WARNING )
    {
        menu.Append( 4, wxString::Format( _( "Change severity to Error for all '%s' violations" ),
                                          rcItem->GetErrorText() ),
                     _( "Violation severities can also be edited in the Schematic Setup... dialog" ) );
    }
    else
    {
        menu.Append( 5, wxString::Format( _( "Change severity to Warning for all '%s' violations" ),
                                          rcItem->GetErrorText() ),
                     _( "Violation severities can also be edited in the Schematic Setup... dialog" ) );
    }

    menu.Append( 6, wxString::Format( _( "Ignore all '%s' violations" ), rcItem->GetErrorText() ),
                 _( "Violations will not be checked or reported" ) );

    menu.AppendSeparator();

    if( rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_WARNING
        || rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
    {
        menu.Append( 7, _( "Edit pin-to-pin conflict map..." ) );
    }
    else
    {
        menu.Append( 8, _( "Edit violation severities..." ),
                     _( "Open the Schematic Setup... dialog" ) );
    }

    bool modified = false;

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case 1:
    {
        SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( node->m_RcItem->GetParent() );

        if( marker )
        {
            marker->SetExcluded( false );
            m_parent->GetCanvas()->GetView()->Update( marker );

            // Update view
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            modified = true;
        }
    }
        break;

    case 2:
    {
        SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( node->m_RcItem->GetParent() );

        if( marker )
        {
            marker->SetExcluded( true );
            m_parent->GetCanvas()->GetView()->Update( marker );

            // Update view
            if( m_severities & RPT_SEVERITY_EXCLUSION )
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            else
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->DeleteCurrentItem( false );

            modified = true;
        }
    }
        break;

    case 4:
        settings.SetSeverity( rcItem->GetErrorCode(), RPT_SEVERITY_ERROR );

        for( SCH_ITEM* item : m_parent->GetScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_parent->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markerProvider, m_severities );
        modified = true;
        break;

    case 5:
        settings.SetSeverity( rcItem->GetErrorCode(), RPT_SEVERITY_WARNING );

        for( SCH_ITEM* item : m_parent->GetScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );

            if( marker->GetRCItem()->GetErrorCode() == rcItem->GetErrorCode() )
                m_parent->GetCanvas()->GetView()->Update( marker );
        }

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markerProvider, m_severities );
        modified = true;
        break;

    case 6:
    {
        settings.SetSeverity( rcItem->GetErrorCode(), RPT_SEVERITY_IGNORE );

        if( rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
            settings.SetSeverity( ERCE_PIN_TO_PIN_WARNING, RPT_SEVERITY_IGNORE );

        SCH_SCREENS ScreenList( m_parent->Schematic().Root() );
        ScreenList.DeleteMarkers( MARKER_BASE::MARKER_ERC, rcItem->GetErrorCode() );

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markerProvider, m_severities );
        modified = true;
    }
        break;

    case 7:
        m_parent->ShowSchematicSetupDialog( _( "Pin Conflicts Map" ) );
        break;

    case 8:
        m_parent->ShowSchematicSetupDialog( _( "Violation Severity" ) );
        break;
    }

    if( modified )
    {
        updateDisplayedCounts();
        redrawDrawPanel();
        m_parent->OnModify();
    }
}


void DIALOG_ERC::PrevMarker()
{
    if( m_notebook->IsShown() )
    {
        if( m_notebook->GetSelection() != 0 )
            m_notebook->SetSelection( 0 );

        m_markerTreeModel->PrevMarker();
    }
}


void DIALOG_ERC::NextMarker()
{
    if( m_notebook->IsShown() )
    {
        if( m_notebook->GetSelection() != 0 )
            m_notebook->SetSelection( 0 );

        m_markerTreeModel->NextMarker();
    }
}


void DIALOG_ERC::SelectMarker( const SCH_MARKER* aMarker )
{
    if( m_notebook->IsShown() )
    {
        m_notebook->SetSelection( 0 );
        m_markerTreeModel->SelectMarker( aMarker );

        // wxWidgets on some platforms fails to correctly ensure that a selected item is
        // visible, so we have to do it in a separate idle event.
        m_centerMarkerOnIdle = aMarker;
        Bind( wxEVT_IDLE, &DIALOG_ERC::centerMarkerIdleHandler, this );
    }
}


void DIALOG_ERC::centerMarkerIdleHandler( wxIdleEvent& aEvent )
{
    m_markerTreeModel->CenterMarker( m_centerMarkerOnIdle );
    m_centerMarkerOnIdle = nullptr;
    Unbind( wxEVT_IDLE, &DIALOG_ERC::centerMarkerIdleHandler, this );
}


void DIALOG_ERC::ExcludeMarker( SCH_MARKER* aMarker )
{
    SCH_MARKER* marker = aMarker;

    if( marker != nullptr )
        m_markerTreeModel->SelectMarker( marker );

    if( m_notebook->GetSelection() != 0 )
        return;

    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( m_markerDataView->GetCurrentItem() );

    if( node && node->m_RcItem )
        marker = dynamic_cast<SCH_MARKER*>( node->m_RcItem->GetParent() );

    if( node && marker && !marker->IsExcluded() )
    {
        marker->SetExcluded( true );
        m_parent->GetCanvas()->GetView()->Update( marker );

        // Update view
        if( m_severities & RPT_SEVERITY_EXCLUSION )
            m_markerTreeModel->ValueChanged( node );
        else
            m_markerTreeModel->DeleteCurrentItem( false );

        updateDisplayedCounts();
        redrawDrawPanel();
        m_parent->OnModify();
    }
}


void DIALOG_ERC::OnEditViolationSeverities( wxHyperlinkEvent& aEvent )
{
    m_parent->ShowSchematicSetupDialog( _( "Violation Severity" ) );
}


void DIALOG_ERC::OnSeverity( wxCommandEvent& aEvent )
{
    int flag = 0;

    if( aEvent.GetEventObject() == m_showAll )
        flag = RPT_SEVERITY_ALL;
    else if( aEvent.GetEventObject() == m_showErrors )
        flag = RPT_SEVERITY_ERROR;
    else if( aEvent.GetEventObject() == m_showWarnings )
        flag = RPT_SEVERITY_WARNING;
    else if( aEvent.GetEventObject() == m_showExclusions )
        flag = RPT_SEVERITY_EXCLUSION;

    if( aEvent.IsChecked() )
        m_severities |= flag;
    else if( aEvent.GetEventObject() == m_showAll )
        m_severities = RPT_SEVERITY_ERROR;
    else
        m_severities &= ~flag;

    syncCheckboxes();

    m_markerTreeModel->Update( m_markerProvider, m_severities );
    updateDisplayedCounts();
}


void DIALOG_ERC::deleteAllMarkers( bool aIncludeExclusions )
{
    // Clear current selection list to avoid selection of deleted items
    m_parent->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

    m_markerTreeModel->DeleteItems( false, aIncludeExclusions, false );

    SCH_SCREENS screens( m_parent->Schematic().Root() );
    screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC, aIncludeExclusions );
}


void DIALOG_ERC::OnSaveReport( wxCommandEvent& aEvent )
{
    wxFileName fn( wxS( "ERC." ) + ReportFileExtension );

    wxFileDialog dlg( this, _( "Save Report to File" ), Prj().GetProjectPath(), fn.GetFullName(),
                      ReportFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = EnsureFileExtension( dlg.GetPath(), ReportFileExtension );

    if( !fn.IsAbsolute() )
    {
        wxString prj_path = Prj().GetProjectPath();
        fn.MakeAbsolute( prj_path );
    }

    if( writeReport( fn.GetFullPath() ) )
    {
        m_messages->Report( wxString::Format( _( "Report file '%s' created." ),
                                              fn.GetFullPath() ) );
    }
    else
    {
        DisplayError( this, wxString::Format( _( "Failed to create file '%s'." ),
                                              fn.GetFullPath() ) );
    }
}


bool DIALOG_ERC::writeReport( const wxString& aFullFileName )
{
    wxFFile file( aFullFileName, wxT( "wt" ) );

    if( !file.IsOpened() )
        return false;

    wxString msg = wxString::Format( _( "ERC report (%s, Encoding UTF8)\n" ), GetISO8601CurrentDateTime() );

    std::map<KIID, EDA_ITEM*> itemMap;

    int            err_count = 0;
    int            warn_count = 0;
    int            total_count = 0;
    SCH_SHEET_LIST sheetList = m_parent->Schematic().GetSheets();

    sheetList.FillItemMap( itemMap );

    ERC_SETTINGS& settings = m_parent->Schematic().ErcSettings();

    for( unsigned i = 0;  i < sheetList.size(); i++ )
    {
        msg << wxString::Format( _( "\n***** Sheet %s\n" ), sheetList[i].PathHumanReadable() );

        for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            const SCH_MARKER* marker = static_cast<const SCH_MARKER*>( aItem );
            RC_ITEM*          item = marker->GetRCItem().get();
            SEVERITY          severity = settings.GetSeverity( item->GetErrorCode() );

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            total_count++;

            switch( severity )
            {
            case RPT_SEVERITY_ERROR:   err_count++;  break;
            case RPT_SEVERITY_WARNING: warn_count++; break;
            default:                                 break;
            }

            msg << marker->GetRCItem()->ShowReport( m_parent, severity, itemMap );
        }
    }

    msg << wxString::Format( _( "\n ** ERC messages: %d  Errors %d  Warnings %d\n" ),
                             total_count, err_count, warn_count );

    // Currently: write report using UTF8 (as usual in Kicad).
    // TODO: see if we can use the current encoding page (mainly for Windows users),
    // Or other format (HTML?)
    file.Write( msg );

    // wxFFile dtor will close the file.

    return true;
}


