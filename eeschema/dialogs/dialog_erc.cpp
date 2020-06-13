/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <sch_screen.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <invoke_sch_dialog.h>
#include <project.h>
#include <kiface_i.h>
#include <bitmaps.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <sch_view.h>
#include <netlist_object.h>
#include <sch_marker.h>
#include <sch_component.h>
#include <connection_graph.h>
#include <tools/ee_actions.h>
#include <tool/tool_manager.h>
#include <dialog_erc.h>
#include <erc.h>
#include <id.h>
#include <confirm.h>
#include <wx/ffile.h>
#include <erc_item.h>
#include <eeschema_settings.h>

DIALOG_ERC::DIALOG_ERC( SCH_EDIT_FRAME* parent ) :
        DIALOG_ERC_BASE( parent, ID_DIALOG_ERC ),  // parent looks for this ID explicitly
        m_parent( parent ),
        m_severities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING )
{
    EESCHEMA_SETTINGS* settings = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    m_severities = settings->m_Appearance.erc_severities;

    m_markerProvider = new SHEETLIST_ERC_ITEMS_PROVIDER( &m_parent->Schematic() );
    m_markerTreeModel = new RC_TREE_MODEL( parent, m_markerDataView );
    m_markerDataView->AssociateModel( m_markerTreeModel );

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_textMarkers->SetFont( infoFont );
    m_titleMessages->SetFont( infoFont );

    m_markerTreeModel->SetSeverities( m_severities );
    m_markerTreeModel->SetProvider( m_markerProvider );
    syncCheckboxes();
    updateDisplayedCounts();

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizer1OK->SetLabel( _( "Run" ) );
    m_sdbSizer1Cancel->SetLabel( _( "Close" ) );
    m_sdbSizer1->Layout();

    m_sdbSizer1OK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_ERC::~DIALOG_ERC()
{
    EESCHEMA_SETTINGS* settings = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( settings );

    if( settings )
        settings->m_Appearance.erc_severities = m_severities;

    m_markerTreeModel->DecRef();
}


void DIALOG_ERC::updateDisplayedCounts()
{
    int numErrors = 0;
    int numWarnings = 0;
    int numExcluded = 0;

    if( m_markerProvider )
    {
        numErrors += m_markerProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_markerProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_markerProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    m_errorsBadge->SetBitmap( MakeBadge( RPT_SEVERITY_ERROR, numErrors, m_errorsBadge ) );
    m_warningsBadge->SetBitmap( MakeBadge( RPT_SEVERITY_WARNING, numWarnings, m_warningsBadge ) );
    m_exclusionsBadge->SetBitmap( MakeBadge( RPT_SEVERITY_EXCLUSION, numExcluded, m_exclusionsBadge ) );
}


/* Delete the old ERC markers, over the whole hierarchy
 */
void DIALOG_ERC::OnEraseDrcMarkersClick( wxCommandEvent& event )
{
    deleteAllMarkers();

    updateDisplayedCounts();
    m_parent->GetCanvas()->Refresh();
}


// This is a modeless dialog so we have to handle these ourselves.
void DIALOG_ERC::OnButtonCloseClick( wxCommandEvent& event )
{
    m_parent->FocusOnItem( nullptr );

    Close();
}


void DIALOG_ERC::OnCloseErcDialog( wxCloseEvent& event )
{
    m_parent->FocusOnItem( nullptr );

    Destroy();
}


static int RPT_SEVERITY_ALL = RPT_SEVERITY_WARNING | RPT_SEVERITY_ERROR | RPT_SEVERITY_EXCLUSION;


void DIALOG_ERC::syncCheckboxes()
{
    m_showAll->SetValue( m_severities == RPT_SEVERITY_ALL );
    m_showErrors->SetValue( m_severities & RPT_SEVERITY_ERROR );
    m_showWarnings->SetValue( m_severities & RPT_SEVERITY_WARNING );
    m_showExclusions->SetValue( m_severities & RPT_SEVERITY_EXCLUSION );
}


void DIALOG_ERC::OnRunERCClick( wxCommandEvent& event )
{
    wxBusyCursor busy;
    deleteAllMarkers();

    m_MessagesList->Clear();
    wxSafeYield();      // m_MarkersList must be redraw

    WX_TEXT_CTRL_REPORTER reporter( m_MessagesList );
    TestErc( reporter );
}


void DIALOG_ERC::RedrawDrawPanel()
{
    WINDOW_THAWER thawer( m_parent );

    m_parent->GetCanvas()->Refresh();
}


void DIALOG_ERC::TestErc( REPORTER& aReporter )
{
    wxFileName fn;

    SCHEMATIC* sch = &m_parent->Schematic();

    // Build the whole sheet list in hierarchy (sheet, not screen)
    sch->GetSheets().AnnotatePowerSymbols();

    if( m_parent->CheckAnnotate( aReporter, false ) )
    {
        if( aReporter.HasMessage() )
            aReporter.ReportTail( _( "Annotation required!" ), RPT_SEVERITY_ERROR );

        return;
    }

    SCH_SCREENS screens( sch->Root() );
    ERC_SETTINGS* settings = sch->ErcSettings();

    // Test duplicate sheet names inside a given sheet.  While one can have multiple references
    // to the same file, each must have a unique name.
    if( settings->IsTestEnabled( ERCE_DUPLICATE_SHEET_NAME ) )
    {
        aReporter.ReportTail( _( "Checking sheet names...\n" ), RPT_SEVERITY_INFO );
        TestDuplicateSheetNames( sch, true );
    }

    if( settings->IsTestEnabled( ERCE_BUS_ALIAS_CONFLICT ) )
    {
        aReporter.ReportTail( _( "Checking bus conflicts...\n" ), RPT_SEVERITY_INFO );
        TestConflictingBusAliases( sch );
    }

    // The connection graph has a whole set of ERC checks it can run
    aReporter.ReportTail( _( "Checking conflicts...\n" ) );
    m_parent->RecalculateConnections( NO_CLEANUP );
    sch->ConnectionGraph()->RunERC();

    // Test is all units of each multiunit component have the same footprint assigned.
    if( settings->IsTestEnabled( ERCE_DIFFERENT_UNIT_FP ) )
    {
        aReporter.ReportTail( _( "Checking footprints...\n" ), RPT_SEVERITY_INFO );
        SCH_SHEET_LIST sheets = sch->GetSheets();
        TestMultiunitFootprints( sheets );
    }

    std::unique_ptr<NETLIST_OBJECT_LIST> objectsConnectedList( m_parent->BuildNetListBase() );

    // Reset the connection type indicator
    objectsConnectedList->ResetConnectionsType();

    unsigned lastItemIdx = 0;
    unsigned nextItemIdx = 0;
    int      MinConn = NOC;

    // Check that a pin appears in only one net.  This check is necessary because multi-unit
    // components that have shared pins could be wired to different nets.
    std::unordered_map<wxString, wxString> pin_to_net_map;

    // The netlist generated by SCH_EDIT_FRAME::BuildNetListBase is sorted by net number, which
    // means we can group netlist items into ranges that live in the same net. The range from
    // nextItem to the current item (exclusive) needs to be checked against the current item.
    // The lastItem variable is used as a helper to pass the last item's number from one loop
    // iteration to the next, which simplifies the initial pass.
    aReporter.ReportTail( _( "Checking connections...\n" ), RPT_SEVERITY_INFO );
    for( unsigned itemIdx = 0; itemIdx < objectsConnectedList->size(); itemIdx++ )
    {
        auto item = objectsConnectedList->GetItem( itemIdx );
        auto lastItem = objectsConnectedList->GetItem( lastItemIdx );

        auto lastNet = lastItem->GetNet();
        auto net = item->GetNet();

        wxASSERT_MSG( lastNet <= net, wxT( "Netlist not correctly ordered" ) );

        if( lastNet != net )
        {
            // New net found:
            MinConn      = NOC;
            nextItemIdx = itemIdx;
        }

        switch( item->m_Type )
        {
        // These items do not create erc problems
        case NETLIST_ITEM::ITEM_UNSPECIFIED:
        case NETLIST_ITEM::SEGMENT:
        case NETLIST_ITEM::BUS:
        case NETLIST_ITEM::JUNCTION:
        case NETLIST_ITEM::LABEL:
        case NETLIST_ITEM::BUSLABELMEMBER:
        case NETLIST_ITEM::PINLABEL:
        case NETLIST_ITEM::GLOBBUSLABELMEMBER:
            break;

        // TODO(JE) Port this to the new system
        case NETLIST_ITEM::PIN:
        {
            // Check if this pin has appeared before on a different net
            if( item->m_Link && settings->IsTestEnabled( ERCE_DIFFERENT_UNIT_NET ) )
            {
                wxString ref = item->GetComponentParent()->GetRef( &item->m_SheetPath );
                wxString pin_name = ref + "_" + item->m_PinNum;
                wxString msg;

                if( pin_to_net_map.count( pin_name ) == 0 )
                {
                    pin_to_net_map[pin_name] = item->GetNetName();
                }
                else if( pin_to_net_map[pin_name] != item->GetNetName() )
                {
                    msg.Printf( _( "Pin %s is connected to both %s and %s" ),
                                item->m_PinNum,
                                pin_to_net_map[pin_name],
                                item->GetNetName() );

                    ERC_ITEM* ercItem = new ERC_ITEM( ERCE_DIFFERENT_UNIT_NET );
                    ercItem->SetErrorMessage( msg );
                    ercItem->SetItems( item->m_Comp );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, item->m_Start );
                    item->m_SheetPath.LastScreen()->Append( marker );
                }
            }

            // Look for ERC problems between pins:
            TestOthersItems( objectsConnectedList.get(), itemIdx, nextItemIdx, &MinConn );
            break;
        }
        default:
        break;
        }

        lastItemIdx = itemIdx;
    }

    // Test similar labels (i;e. labels which are identical when
    // using case insensitive comparisons)
    if( settings->IsTestEnabled( ERCE_SIMILAR_LABELS ) )
    {
        aReporter.ReportTail( _( "Checking labels...\n" ), RPT_SEVERITY_INFO );
        objectsConnectedList->TestforSimilarLabels();
    }

    if( settings->IsTestEnabled( ERCE_UNRESOLVED_VARIABLE ) )
        TestTextVars( sch, m_parent->GetCanvas()->GetView()->GetWorksheet() );

    // Display diags:
    m_markerTreeModel->SetProvider( m_markerProvider );

    // Displays global results:
    updateDisplayedCounts();

    // Display new markers from the current screen:
    KIGFX::VIEW* view = m_parent->GetCanvas()->GetView();

    for( auto item : m_parent->GetScreen()->Items().OfType( SCH_MARKER_T ) )
        view->Add( item );

    m_parent->GetCanvas()->Refresh();

    // Display message
    aReporter.ReportTail( _( "Finished" ), RPT_SEVERITY_INFO );
}


void DIALOG_ERC::OnERCItemSelected( wxDataViewEvent& aEvent )
{
    const KIID&     itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    SCH_SHEET_PATH  sheet;
    SCH_ITEM*       item = m_parent->Schematic().GetSheets().GetItem( itemID, &sheet );

    if( item )
    {
        WINDOW_THAWER thawer( m_parent );

        if( sheet != m_parent->GetCurrentSheet() )
        {
            m_parent->GetToolManager()->RunAction( ACTIONS::cancelInteractive, true );
            m_parent->GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );

            m_parent->SetCurrentSheet( sheet );
            m_parent->DisplayCurrentSheet();
            m_parent->RedrawScreen( (wxPoint) m_parent->GetScreen()->m_ScrollCenter, false );
        }

        m_parent->FocusOnItem( item );
        RedrawDrawPanel();
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

    SCHEMATIC& sch = m_parent->Schematic();

    RC_ITEM*  rcItem = node->m_RcItem;
    wxString  listName;
    wxMenu    menu;
    wxString  msg;

    switch( sch.GetErcSeverity( rcItem->GetErrorCode() ) )
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
    else if( sch.GetErcSeverity( rcItem->GetErrorCode() ) == RPT_SEVERITY_WARNING )
    {
        menu.Append( 4, wxString::Format( _( "Change severity to Error for all '%s' violations" ),
                                          rcItem->GetErrorText( rcItem->GetErrorCode() ) ),
                     _( "Violation severities can also be edited in the Board Setup... dialog" ) );
    }
    else
    {
        menu.Append( 5, wxString::Format( _( "Change severity to Warning for all '%s' violations" ),
                                          rcItem->GetErrorText( rcItem->GetErrorCode() ) ),
                     _( "Violation severities can also be edited in the Board Setup... dialog" ) );
    }

    menu.Append( 6, wxString::Format( _( "Ignore all '%s' violations" ),
                                      rcItem->GetErrorText( rcItem->GetErrorCode() ) ),
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

    switch( GetPopupMenuSelectionFromUser( menu ) )
    {
    case 1:
        node->m_RcItem->GetParent()->SetExcluded( false );

        // Update view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
        updateDisplayedCounts();
        break;

    case 2:
        node->m_RcItem->GetParent()->SetExcluded( true );

        // Update view
        if( m_severities & RPT_SEVERITY_EXCLUSION )
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->ValueChanged( node );
        else
            static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->DeleteCurrentItem( false );

        updateDisplayedCounts();
        break;

    case 4:
        sch.SetErcSeverity( rcItem->GetErrorCode(), RPT_SEVERITY_ERROR );

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markerProvider );
        updateDisplayedCounts();
        break;

    case 5:
        sch.SetErcSeverity( rcItem->GetErrorCode(), RPT_SEVERITY_WARNING );

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markerProvider );
        updateDisplayedCounts();
        break;

    case 6:
    {
        sch.SetErcSeverity( rcItem->GetErrorCode(), RPT_SEVERITY_IGNORE );

        if( rcItem->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
            sch.SetErcSeverity( ERCE_PIN_TO_PIN_WARNING, RPT_SEVERITY_IGNORE );

        SCH_SCREENS ScreenList( sch.Root() );
        ScreenList.DeleteMarkers( MARKER_BASE::MARKER_ERC, rcItem->GetErrorCode() );

        // Rebuild model and view
        static_cast<RC_TREE_MODEL*>( aEvent.GetModel() )->SetProvider( m_markerProvider );
        updateDisplayedCounts();
    }
        break;

    case 7:
        m_parent->ShowSchematicSetupDialog( _( "Pin Conflicts Map" ) );
        break;

    case 8:
        m_parent->ShowSchematicSetupDialog( _( "Violation Severity" ) );
        break;
    }
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

    // Set the provider's severity levels through the TreeModel so that the old tree
    // can be torn down before the severity changes.
    //
    // It's not clear this is required, but we've had a lot of issues with wxDataView
    // being cranky on various platforms.

    m_markerTreeModel->SetSeverities( m_severities );

    updateDisplayedCounts();
}


void DIALOG_ERC::deleteAllMarkers()
{
    // Clear current selection list to avoid selection of deleted items
    m_parent->GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );

    m_markerTreeModel->DeleteAllItems();
}


void DIALOG_ERC::OnSaveReport( wxCommandEvent& aEvent )
{
    wxFileName fn( "./ERC." + ReportFileExtension );

    wxFileDialog dlg( this, _( "Save Report to File" ), fn.GetPath(), fn.GetFullName(),
                      ReportFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( ReportFileExtension );

    if( !fn.IsAbsolute() )
    {
        wxString prj_path = Prj().GetProjectPath();
        fn.MakeAbsolute( prj_path );
    }

    if( writeReport( fn.GetFullPath() ) )
    {
        m_MessagesList->AppendText( wxString::Format( _( "Report file '%s' created\n" ),
                                                      fn.GetFullPath() ) );
    }
    else
    {
        DisplayError( this, wxString::Format( _( "Unable to create report file '%s'" ),
                                              fn.GetFullPath() ) );
    }
}


bool DIALOG_ERC::writeReport( const wxString& aFullFileName )
{
    wxFFile file( aFullFileName, wxT( "wt" ) );

    if( !file.IsOpened() )
        return false;

    wxString msg = wxString::Format( _( "ERC report (%s, Encoding UTF8)\n" ), DateAndTime() );

    std::map<KIID, EDA_ITEM*> itemMap;

    int            err_count = 0;
    int            warn_count = 0;
    int            total_count = 0;
    SCH_SHEET_LIST sheetList = m_parent->Schematic().GetSheets();

    sheetList.FillItemMap( itemMap );

    for( unsigned i = 0;  i < sheetList.size(); i++ )
    {
        msg << wxString::Format( _( "\n***** Sheet %s\n" ), sheetList[i].PathHumanReadable() );

        for( SCH_ITEM* aItem : sheetList[i].LastScreen()->Items().OfType( SCH_MARKER_T ) )
        {
            const SCH_MARKER* marker = static_cast<const SCH_MARKER*>( aItem );

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            total_count++;

            switch( m_parent->Schematic().GetErcSeverity( marker->GetRCItem()->GetErrorCode() ) )
            {
            case RPT_SEVERITY_ERROR:   err_count++;  break;
            case RPT_SEVERITY_WARNING: warn_count++; break;
            default:                                 break;
            }

            msg << marker->GetRCItem()->ShowReport( GetUserUnits(), itemMap );
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


wxDialog* InvokeDialogERC( SCH_EDIT_FRAME* aCaller )
{
    // This is a modeless dialog, so new it rather than instantiating on stack.
    DIALOG_ERC* dlg = new DIALOG_ERC( aCaller );

    dlg->Show( true );

    return dlg;     // wxDialog is information hiding about DIALOG_ERC.
}
