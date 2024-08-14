/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <erc/erc.h>
#include <erc/erc_report.h>
#include <id.h>
#include <confirm.h>
#include <common.h>
#include <widgets/wx_html_report_box.h>
#include <dialogs/dialog_text_entry.h>
#include <erc/erc_item.h>
#include <eeschema_settings.h>
#include <string_utils.h>
#include <kiplatform/ui.h>

#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/hyperlink.h>
#include <wx/msgdlg.h>


wxDEFINE_EVENT( EDA_EVT_CLOSE_ERC_DIALOG, wxCommandEvent );


// wxWidgets spends *far* too long calcuating column widths (most of it, believe it or
// not, in repeatedly creating/destroying a wxDC to do the measurement in).
// Use default column widths instead.
static int DEFAULT_SINGLE_COL_WIDTH = 660;


static SCHEMATIC* g_lastERCSchematic = nullptr;
static bool       g_lastERCRun = false;

static std::vector<std::pair<wxString, int>> g_lastERCIgnored;


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
    KIPLATFORM::UI::SetFloatLevel( this );

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

        for( const auto& [ str, code ] : g_lastERCIgnored )
        {
            wxListItem listItem;
            listItem.SetId( m_ignoredList->GetItemCount() );
            listItem.SetText( str );
            listItem.SetData( code );

            m_ignoredList->InsertItem( listItem );
        }
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
    {
        g_lastERCIgnored.push_back( { m_ignoredList->GetItemText( ii ),
                                      m_ignoredList->GetItemData( ii ) } );
    }

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
        if( !m_infoBar->IsShownOnScreen() )
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
        if( m_infoBar->IsShownOnScreen() )
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

    sch->RecordERCExclusions();
    deleteAllMarkers( true );

    std::vector<std::reference_wrapper<RC_ITEM>> violations = ERC_ITEM::GetItemsWithSeverities();
    m_ignoredList->DeleteAllItems();

    for( std::reference_wrapper<RC_ITEM>& item : violations )
    {
        if( sch->ErcSettings().GetSeverity( item.get().GetErrorCode() ) == RPT_SEVERITY_IGNORE )
        {
            wxListItem listItem;
            listItem.SetId( m_ignoredList->GetItemCount() );
            listItem.SetText( wxT( " • " ) + item.get().GetErrorText() );
            listItem.SetData( item.get().GetErrorCode() );

            m_ignoredList->InsertItem( listItem );
        }
    }

    m_ignoredList->SetColumnWidth( 0, m_ignoredList->GetParent()->GetClientSize().x - 20 );

    m_cancelled = false;
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
        m_sdbSizer1Cancel->SetDefault();
        // wxWidgets has a tendency to keep both buttons highlighted without the following:
        m_sdbSizer1OK->Enable( false );

        wxMilliSleep( 500 );
        m_runningResultsBook->ChangeSelection( 1 );
        KIPLATFORM::UI::ForceFocus( m_notebook );

        // now re-enable m_sdbSizerOK button
        m_sdbSizer1OK->Enable( true );
    }

    m_ercRun = true;
    redrawDrawPanel();
    updateDisplayedCounts();
    // set float level again, it can be lost due to window events during test run
    KIPLATFORM::UI::SetFloatLevel( this );
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
    ERC_TESTER tester( sch );

    {
        wxBusyCursor dummy;
        tester.RunTests( m_parent->GetCanvas()->GetView()->GetDrawingSheet(), m_parent,
                         m_parent->Kiway().KiFACE( KIWAY::FACE_CVPCB ), &m_parent->Prj(), this );
    }

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
    SCH_ITEM*       item = m_parent->Schematic().GetItem( itemID, &sheet );

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

            // Store the current zoom level into the current screen before switching
            m_parent->GetScreen()->m_LastZoomLevel = m_parent->GetCanvas()->GetView()->GetScale();

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
    TOOL_MANAGER*       toolMgr = m_parent->GetToolManager();
    EE_INSPECTION_TOOL* inspectionTool = toolMgr->GetTool<EE_INSPECTION_TOOL>();
    RC_TREE_NODE*       node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );

    if( !node )
        return;

    ERC_SETTINGS& settings = m_parent->Schematic().ErcSettings();

    std::shared_ptr<RC_ITEM> rcItem = node->m_RcItem;
    wxString                 listName;
    wxMenu                   menu;

    switch( settings.GetSeverity( rcItem->GetErrorCode() ) )
    {
    case RPT_SEVERITY_ERROR:   listName = _( "errors" );      break;
    case RPT_SEVERITY_WARNING: listName = _( "warnings" );    break;
    default:                   listName = _( "appropriate" ); break;
    }

    enum MENU_IDS
    {
        ID_EDIT_EXCLUSION_COMMENT = 4467,
        ID_REMOVE_EXCLUSION,
        ID_REMOVE_EXCLUSION_ALL,
        ID_ADD_EXCLUSION,
        ID_ADD_EXCLUSION_WITH_COMMENT,
        ID_ADD_EXCLUSION_ALL,
        ID_INSPECT_VIOLATION,
        ID_EDIT_PIN_CONFLICT_MAP,
        ID_EDIT_CONNECTION_GRID,
        ID_SET_SEVERITY_TO_ERROR,
        ID_SET_SEVERITY_TO_WARNING,
        ID_SET_SEVERITY_TO_IGNORE,
        ID_EDIT_SEVERITIES,
    };

    if( rcItem->GetParent()->IsExcluded() )
    {
        menu.Append( ID_REMOVE_EXCLUSION,
                     _( "Remove exclusion for this violation" ),
                     wxString::Format( _( "It will be placed back in the %s list" ), listName ) );

        menu.Append( ID_EDIT_EXCLUSION_COMMENT,
                     _( "Edit exclusion comment..." ) );
    }
    else
    {
        menu.Append( ID_ADD_EXCLUSION,
                     _( "Exclude this violation" ),
                     wxString::Format( _( "It will be excluded from the %s list" ), listName ) );

        menu.Append( ID_ADD_EXCLUSION_WITH_COMMENT,
                     _( "Exclude with comment..." ),
                     wxString::Format( _( "It will be excluded from the %s list" ), listName ) );
    }

    wxString inspectERCErrorMenuText = inspectionTool->InspectERCErrorMenuText( rcItem );

    if( !inspectERCErrorMenuText.IsEmpty() )
        menu.Append( ID_INSPECT_VIOLATION, inspectERCErrorMenuText );

    menu.AppendSeparator();

    if( rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_WARNING
      || rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
    {
        // Pin to pin severities edited through pin conflict map
    }
    else if( settings.GetSeverity( rcItem->GetErrorCode() ) == RPT_SEVERITY_WARNING )
    {
        menu.Append( ID_SET_SEVERITY_TO_ERROR,
                     wxString::Format( _( "Change severity to Error for all '%s' violations" ),
                                       rcItem->GetErrorText() ),
                     _( "Violation severities can also be edited in the Schematic Setup... dialog" ) );
    }
    else
    {
        menu.Append( ID_SET_SEVERITY_TO_WARNING,
                     wxString::Format( _( "Change severity to Warning for all '%s' violations" ),
                                       rcItem->GetErrorText() ),
                     _( "Violation severities can also be edited in the Schematic Setup... dialog" ) );
    }

    menu.Append( ID_SET_SEVERITY_TO_IGNORE,
                 wxString::Format( _( "Ignore all '%s' violations" ), rcItem->GetErrorText() ),
                 _( "Violations will not be checked or reported" ) );

    menu.AppendSeparator();

    if( rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_WARNING
        || rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
    {
        menu.Append( ID_EDIT_PIN_CONFLICT_MAP,
                     _( "Edit pin-to-pin conflict map..." ),
                     _( "Open the Schematic Setup... dialog" ) );
    }
    else
    {
        menu.Append( ID_EDIT_SEVERITIES,
                     _( "Edit violation severities..." ),
                     _( "Open the Schematic Setup... dialog" ) );
    }

    if( rcItem->GetErrorCode() == ERCE_ENDPOINT_OFF_GRID )
    {
        menu.Append( ID_EDIT_CONNECTION_GRID,
                     _( "Edit connection grid spacing..." ),
                     _( "Open the Schematic Setup... dialog" ) );
    }

    bool modified = false;
    int  command = GetPopupMenuSelectionFromUser( menu );

    switch( command )
    {
    case ID_EDIT_EXCLUSION_COMMENT:
        if( SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( node->m_RcItem->GetParent() ) )
        {
            WX_TEXT_ENTRY_DIALOG dlg( this, wxEmptyString, _( "Exclusion Comment" ),
                                      marker->GetComment(), true );

            if( dlg.ShowModal() == wxID_CANCEL )
                break;

            marker->SetExcluded( true, dlg.GetValue() );

            // Update view
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            modified = true;
        }

        break;

    case ID_REMOVE_EXCLUSION:
        if( SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( node->m_RcItem->GetParent() ) )
        {
            marker->SetExcluded( false );
            m_parent->GetCanvas()->GetView()->Update( marker );

            // Update view
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            modified = true;
        }

        break;

    case ID_ADD_EXCLUSION:
    case ID_ADD_EXCLUSION_WITH_COMMENT:
        if( SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( node->m_RcItem->GetParent() ) )
        {
            wxString comment;

            if( command == ID_ADD_EXCLUSION_WITH_COMMENT )
            {
                WX_TEXT_ENTRY_DIALOG dlg( this, wxEmptyString, _( "Exclusion Comment" ),
                                          wxEmptyString, true );

                if( dlg.ShowModal() == wxID_CANCEL )
                    break;

                comment = dlg.GetValue();
            }

            marker->SetExcluded( true, comment );

            m_parent->GetCanvas()->GetView()->Update( marker );

            // Update view
            if( m_severities & RPT_SEVERITY_EXCLUSION )
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
            else
                static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->DeleteCurrentItem( false );

            modified = true;
        }

        break;

    case ID_INSPECT_VIOLATION:
        inspectionTool->InspectERCError( node->m_RcItem );
        break;

    case ID_SET_SEVERITY_TO_ERROR:
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

    case ID_SET_SEVERITY_TO_WARNING:
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

    case ID_SET_SEVERITY_TO_IGNORE:
    {
        settings.SetSeverity( rcItem->GetErrorCode(), RPT_SEVERITY_IGNORE );

        if( rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
            settings.SetSeverity( ERCE_PIN_TO_PIN_WARNING, RPT_SEVERITY_IGNORE );

        wxListItem listItem;
        listItem.SetId( m_ignoredList->GetItemCount() );
        listItem.SetText( wxT( " • " ) + rcItem->GetErrorText() );
        listItem.SetData( rcItem->GetErrorCode() );

        m_ignoredList->InsertItem( listItem );

        // Clear the selection before deleting markers. It may be some selected ERC markers.
        // Deleting a selected marker without deselecting it first generates a crash
        m_parent->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

        SCH_SCREENS ScreenList( m_parent->Schematic().Root() );
        ScreenList.DeleteMarkers( MARKER_BASE::MARKER_ERC, rcItem->GetErrorCode() );

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->Update( m_markerProvider, m_severities );
        modified = true;
    }
        break;

    case ID_EDIT_PIN_CONFLICT_MAP:
        m_parent->ShowSchematicSetupDialog( _( "Pin Conflicts Map" ) );
        break;

    case ID_EDIT_SEVERITIES:
        m_parent->ShowSchematicSetupDialog( _( "Violation Severity" ) );
        break;

    case ID_EDIT_CONNECTION_GRID:
        m_parent->ShowSchematicSetupDialog( _( "Formatting" ) );
        break;
    }

    if( modified )
    {
        updateDisplayedCounts();
        redrawDrawPanel();
        m_parent->OnModify();
    }
}


void DIALOG_ERC::OnIgnoredItemRClick( wxListEvent& event )
{
    ERC_SETTINGS& settings = m_parent->Schematic().ErcSettings();
    int           errorCode = (int) event.m_item.GetData();
    wxMenu        menu;

    menu.Append( RPT_SEVERITY_ERROR,   _( "Error" ),   wxEmptyString, wxITEM_CHECK );
    menu.Append( RPT_SEVERITY_WARNING, _( "Warning" ), wxEmptyString, wxITEM_CHECK );
    menu.Append( RPT_SEVERITY_IGNORE,  _( "Ignore" ),  wxEmptyString, wxITEM_CHECK );

    menu.Check( settings.GetSeverity( errorCode ), true );

    int severity = GetPopupMenuSelectionFromUser( menu );

    if( severity > 0 )
    {
        if( settings.GetSeverity( errorCode ) != severity )
        {
            settings.SetSeverity( errorCode, (SEVERITY) severity );

            updateDisplayedCounts();
            redrawDrawPanel();
            m_parent->OnModify();
        }
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
    wxFileName fn( wxS( "ERC." ) + wxString( FILEEXT::ReportFileExtension ) );

    wxFileDialog dlg( this, _( "Save Report File" ), Prj().GetProjectPath(), fn.GetFullName(),
                      FILEEXT::ReportFileWildcard() + wxS( "|" ) + FILEEXT::JsonFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( FILEEXT::ReportFileExtension );

    if( !fn.IsAbsolute() )
    {
        wxString prj_path = Prj().GetProjectPath();
        fn.MakeAbsolute( prj_path );
    }

    ERC_REPORT reportWriter( &m_parent->Schematic(), m_parent->GetUserUnits() );

    bool success = false;
    if( fn.GetExt() == FILEEXT::JsonFileExtension )
        success = reportWriter.WriteJsonReport( fn.GetFullPath() );
    else
        success = reportWriter.WriteTextReport( fn.GetFullPath() );

    if( success )
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
