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
#include <invoke_sch_dialog.h>
#include <project.h>
#include <kiface_i.h>
#include <bitmaps.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <sch_view.h>
#include <netlist_object.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <connection_graph.h>
#include <tools/ee_actions.h>
#include <tool/tool_manager.h>
#include <dialog_erc.h>
#include <erc.h>
#include <id.h>


extern int           DiagErc[PINTYPE_COUNT][PINTYPE_COUNT];
extern int           DefaultDiagErc[PINTYPE_COUNT][PINTYPE_COUNT];


bool DIALOG_ERC::m_diagErcTableInit = false;        // saved only for the current session

// Control identifiers for events
#define ID_MATRIX_0 1800


BEGIN_EVENT_TABLE( DIALOG_ERC, DIALOG_ERC_BASE )
    EVT_COMMAND_RANGE( ID_MATRIX_0, ID_MATRIX_0 + ( PINTYPE_COUNT * PINTYPE_COUNT ) - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED, DIALOG_ERC::ChangeErrorLevel )
END_EVENT_TABLE()


DIALOG_ERC::DIALOG_ERC( SCH_EDIT_FRAME* parent ) :
    DIALOG_ERC_BASE( parent, ID_DIALOG_ERC ),  // parent looks for this ID explicitly
    m_buttonList(),
    m_initialized( false ),
    m_settings()
{
    m_parent = parent;
    m_lastMarkerFound = nullptr;

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_textMarkers->SetFont( infoFont );
    m_titleMessages->SetFont( infoFont );

    Init();

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
    transferControlsToSettings();

    if( m_settings != m_parent->GetErcSettings() )
    {
        m_parent->UpdateErcSettings( m_settings );
        m_parent->SaveProjectSettings( false );
    }
}


void DIALOG_ERC::Init()
{
    m_initialized = false;

    for( auto& buttonRow : m_buttonList )
    {
        for( auto& button : buttonRow )
            button = NULL;
    }

    m_settings = m_parent->GetErcSettings();
    transferSettingsToControls();

    SCH_SCREENS screens;
    updateMarkerCounts( &screens );

    DisplayERC_MarkersList();

    // Init Panel Matrix
    ReBuildMatrixPanel();
}


void DIALOG_ERC::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_buttondelmarkers->Show( m_NoteBook->GetSelection() == 0 );
    m_ResetOptButton->Show( m_NoteBook->GetSelection() == 1 );
    m_buttonsSizer->Layout();
}


void DIALOG_ERC::transferSettingsToControls()
{
    m_WriteResultOpt->SetValue( m_settings.write_erc_file );
    m_cbTestSimilarLabels->SetValue( m_settings.check_similar_labels );
    m_cbTestUniqueGlbLabels->SetValue( m_settings.check_unique_global_labels );
    m_cbCheckBusDriverConflicts->SetValue( m_settings.check_bus_driver_conflicts );
    m_cbCheckBusEntries->SetValue( m_settings.check_bus_entry_conflicts );
    m_cbCheckBusToBusConflicts->SetValue( m_settings.check_bus_to_bus_conflicts );
    m_cbCheckBusToNetConflicts->SetValue( m_settings.check_bus_to_net_conflicts );
}


void DIALOG_ERC::transferControlsToSettings()
{
    m_settings.write_erc_file = m_WriteResultOpt->GetValue();
    m_settings.check_similar_labels = m_cbTestSimilarLabels->GetValue();
    m_settings.check_unique_global_labels = m_cbTestUniqueGlbLabels->GetValue();
    m_settings.check_bus_driver_conflicts = m_cbCheckBusDriverConflicts->GetValue();
    m_settings.check_bus_entry_conflicts = m_cbCheckBusEntries->GetValue();
    m_settings.check_bus_to_bus_conflicts = m_cbCheckBusToBusConflicts->GetValue();
    m_settings.check_bus_to_net_conflicts = m_cbCheckBusToNetConflicts->GetValue();
}


void DIALOG_ERC::updateMarkerCounts( SCH_SCREENS *screens )
{
    int markers = screens->GetMarkerCount( MARKER_BASE::MARKER_ERC,
                                           MARKER_BASE::MARKER_SEVERITY_UNSPEC );
    int warnings = screens->GetMarkerCount( MARKER_BASE::MARKER_ERC,
                                            MARKER_BASE::MARKER_SEVERITY_WARNING );
    int errors = screens->GetMarkerCount( MARKER_BASE::MARKER_ERC,
                                          MARKER_BASE::MARKER_SEVERITY_ERROR );

    wxString num;
    num.Printf( wxT( "%d" ), markers );
    m_TotalErrCount->SetValue( num );

    num.Printf( wxT( "%d" ), errors );
    m_LastErrCount->SetValue( num );

    num.Printf( wxT( "%d" ), warnings );
    m_LastWarningCount->SetValue( num );
}


/* Delete the old ERC markers, over the whole hierarchy
 */
void DIALOG_ERC::OnEraseDrcMarkersClick( wxCommandEvent& event )
{
    SCH_SCREENS ScreenList;

    ScreenList.DeleteAllMarkers( MARKER_BASE::MARKER_ERC );
    updateMarkerCounts( &ScreenList );

    m_MarkersList->ClearList();
    m_parent->GetCanvas()->Refresh();
}


// This is a modeless dialog so we have to handle these ourselves.
void DIALOG_ERC::OnButtonCloseClick( wxCommandEvent& event )
{
    Close();
}


void DIALOG_ERC::OnCloseErcDialog( wxCloseEvent& event )
{
    Destroy();
}


void DIALOG_ERC::OnResetMatrixClick( wxCommandEvent& event )
{
    ResetDefaultERCDiag( event );
}


void DIALOG_ERC::OnErcCmpClick( wxCommandEvent& event )
{
    wxBusyCursor busy;
    m_MarkersList->ClearList();

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


void DIALOG_ERC::OnLeftClickMarkersList( wxHtmlLinkEvent& event )
{
    wxString link = event.GetLinkInfo().GetHref();

    m_lastMarkerFound = nullptr;

    long index;

    if( !link.ToLong( &index ) )
        return;

    const SCH_MARKER* marker = m_MarkersList->GetItem( index );

    if( !marker )
        return;

    // Search for the selected marker
    unsigned i;
    SCH_SHEET_LIST  sheetList( g_RootSheet );
    bool found = false;

    for( i = 0;  i < sheetList.size(); i++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) sheetList[i].LastDrawList();

        for( ; item; item = item->Next() )
        {
            if( item == marker )
            {
                found = true;
                break;
            }
        }

        if( found )
            break;
    }

    if( !found ) // Error
    {
        wxMessageBox( _( "Marker not found" ) );

        // The marker was deleted, so rebuild marker list
        DisplayERC_MarkersList();
        return;
    }

    if( sheetList[i] != m_parent->GetCurrentSheet() )
    {
        m_parent->GetToolManager()->RunAction( ACTIONS::cancelInteractive, true );
        m_parent->GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );

        m_parent->SetCurrentSheet( sheetList[i] );
        m_parent->DisplayCurrentSheet();
        sheetList[i].LastScreen()->SetZoom( m_parent->GetScreen()->GetZoom() );
        m_parent->RedrawScreen( (wxPoint) m_parent->GetScreen()->m_ScrollCenter, false );
    }

    m_lastMarkerFound = marker;
    m_parent->FocusOnLocation( marker->m_Pos, true );
    RedrawDrawPanel();
}


void DIALOG_ERC::OnLeftDblClickMarkersList( wxMouseEvent& event )
{
    // Remember: OnLeftClickMarkersList was called just before and therefore m_lastMarkerFound
    // was initialized (NULL if not found).
    if( m_lastMarkerFound )
    {
        m_parent->FocusOnLocation( m_lastMarkerFound->m_Pos, true );
        RedrawDrawPanel();
    }

    Close();
}


void DIALOG_ERC::ReBuildMatrixPanel()
{
    // Try to know the size of bitmap button used in drc matrix
    wxBitmapButton * dummy = new wxBitmapButton( m_matrixPanel, wxID_ANY, KiBitmap( ercerr_xpm ) );
    wxSize bitmap_size = dummy->GetSize();
    delete dummy;

    if( !m_diagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        m_diagErcTableInit = true;
    }

    wxPoint pos;
    // Get the current text size:use a dummy text.
    wxStaticText* text = new wxStaticText( m_matrixPanel, -1, wxT( "W" ), pos );
    int text_height   = text->GetRect().GetHeight();
    bitmap_size.y = std::max( bitmap_size.y, text_height );
    delete text;

    // compute the Y pos interval:
    pos.y = text_height;

    if( !m_initialized )
    {
        std::vector<wxStaticText*> labels;

        // Print row labels
        for( int ii = 0; ii < PINTYPE_COUNT; ii++ )
        {
            int y = pos.y + (ii * bitmap_size.y);
            text = new wxStaticText( m_matrixPanel, -1, CommentERC_H[ii],
                                     wxPoint( 5, y + ( bitmap_size.y / 2) - (text_height / 2) ) );
            labels.push_back( text );

            int x = text->GetRect().GetRight();
            pos.x = std::max( pos.x, x );
        }

        // Right-align
        for( int ii = 0; ii < PINTYPE_COUNT; ii++ )
        {
            wxPoint labelPos = labels[ ii ]->GetPosition();
            labelPos.x = pos.x - labels[ ii ]->GetRect().GetWidth();
            labels[ ii ]->SetPosition( labelPos );
        }

        pos.x += 5;
    }
    else
        pos = m_buttonList[0][0]->GetPosition();

    for( int ii = 0; ii < PINTYPE_COUNT; ii++ )
    {
        int y = pos.y + (ii * bitmap_size.y);

        for( int jj = 0; jj <= ii; jj++ )
        {
            // Add column labels (only once)
            int diag = DiagErc[ii][jj];
            int x    = pos.x + (jj * bitmap_size.x);

            if( (ii == jj) && !m_initialized )
            {
                wxPoint txtpos;
                txtpos.x = x + (bitmap_size.x / 2);
                txtpos.y = y - text_height;
                text     = new wxStaticText( m_matrixPanel, -1, CommentERC_V[ii], txtpos );
            }

            int event_id = ID_MATRIX_0 + ii + ( jj * PINTYPE_COUNT );
            BITMAP_DEF bitmap_butt = erc_green_xpm;

            delete m_buttonList[ii][jj];
            m_buttonList[ii][jj] = new wxBitmapButton( m_matrixPanel,
                                                       event_id,
                                                       KiBitmap( bitmap_butt ),
                                                       wxPoint( x, y ) );
            setDRCMatrixButtonState( m_buttonList[ii][jj], diag );
        }
    }

    m_initialized = true;
}


void DIALOG_ERC::setDRCMatrixButtonState( wxBitmapButton *aButton, int aState )
{
    BITMAP_DEF bitmap_butt = nullptr;
    wxString tooltip;

    switch( aState )
    {
    case OK:
        bitmap_butt = erc_green_xpm;
        tooltip = _( "No error or warning" );
        break;

    case WAR:
        bitmap_butt = ercwarn_xpm;
        tooltip = _( "Generate warning" );
        break;

    case ERR:
        bitmap_butt = ercerr_xpm;
        tooltip = _( "Generate error" );
        break;

    default:
        break;
    }

    if( bitmap_butt )
    {
        aButton->SetBitmap( KiBitmap( bitmap_butt ) );
        aButton->SetToolTip( tooltip );
    }
}


void DIALOG_ERC::DisplayERC_MarkersList()
{
    SCH_SHEET_LIST sheetList( g_RootSheet);
    m_MarkersList->ClearList();

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        SCH_ITEM* item = sheetList[i].LastDrawList();

        for( ; item != NULL; item = item->Next() )
        {
            if( item->Type() != SCH_MARKER_T )
                continue;

            SCH_MARKER* marker = (SCH_MARKER*) item;

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            m_MarkersList->AppendToList( marker );
        }
    }

    m_MarkersList->DisplayList( GetUserUnits() );
}


void DIALOG_ERC::ResetDefaultERCDiag( wxCommandEvent& event )
{
    memcpy( DiagErc, DefaultDiagErc, sizeof( DiagErc ) );
    ReBuildMatrixPanel();
    m_settings.LoadDefaults();
    transferSettingsToControls();
}


void DIALOG_ERC::ChangeErrorLevel( wxCommandEvent& event )
{
    int id = event.GetId();
    int ii = id - ID_MATRIX_0;
    int x = ii / PINTYPE_COUNT;
    int y = ii % PINTYPE_COUNT;
    wxBitmapButton* butt = (wxBitmapButton*) event.GetEventObject();

    int level = ( DiagErc[y][x] + 1 ) % 3;

    setDRCMatrixButtonState( butt, level );

    DiagErc[y][x] = DiagErc[x][y] = level;
}


void DIALOG_ERC::TestErc( REPORTER& aReporter )
{
    wxFileName fn;

    transferControlsToSettings();

    // Build the whole sheet list in hierarchy (sheet, not screen)
    SCH_SHEET_LIST sheets( g_RootSheet );
    sheets.AnnotatePowerSymbols();

    if( m_parent->CheckAnnotate( aReporter, false ) )
    {
        if( aReporter.HasMessage() )
            aReporter.ReportTail( _( "Annotation required!" ), REPORTER::RPT_ERROR );

        return;
    }

    SCH_SCREENS screens;

    // Erase all previous DRC markers.
    screens.DeleteAllMarkers( MARKER_BASE::MARKER_ERC );

    // Test duplicate sheet names inside a given sheet.  While one can have multiple references
    // to the same file, each must have a unique name.
    TestDuplicateSheetNames( true );

    TestConflictingBusAliases();

    // The connection graph has a whole set of ERC checks it can run
    m_parent->RecalculateConnections();
    g_ConnectionGraph->RunERC( m_settings );

    // Test is all units of each multiunit component have the same footprint assigned.
    TestMultiunitFootprints( sheets );

    std::unique_ptr<NETLIST_OBJECT_LIST> objectsConnectedList( m_parent->BuildNetListBase() );

    // Reset the connection type indicator
    objectsConnectedList->ResetConnectionsType();

    unsigned lastItemIdx = 0;
    unsigned nextItemIdx = 0;
    int MinConn    = NOC;

    // Check that a pin appears in only one net.  This check is necessary because multi-unit
    // components that have shared pins could be wired to different nets.
    std::unordered_map<wxString, wxString> pin_to_net_map;

    // The netlist generated by SCH_EDIT_FRAME::BuildNetListBase is sorted by net number, which
    // means we can group netlist items into ranges that live in the same net. The range from
    // nextItem to the current item (exclusive) needs to be checked against the current item.
    // The lastItem variable is used as a helper to pass the last item's number from one loop
    // iteration to the next, which simplifies the initial pass.
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
        case NET_ITEM_UNSPECIFIED:
        case NET_SEGMENT:
        case NET_BUS:
        case NET_JUNCTION:
        case NET_LABEL:
        case NET_BUSLABELMEMBER:
        case NET_PINLABEL:
        case NET_GLOBBUSLABELMEMBER:
            break;

        // TODO(JE) Port this to the new system
        case NET_PIN:
        {
            // Check if this pin has appeared before on a different net
            if( item->m_Link )
            {
                auto ref = item->GetComponentParent()->GetRef( &item->m_SheetPath );
                wxString pin_name = ref + "_" + item->m_PinNum;

                if( pin_to_net_map.count( pin_name ) == 0 )
                {
                    pin_to_net_map[pin_name] = item->GetNetName();
                }
                else if( pin_to_net_map[pin_name] != item->GetNetName() )
                {
                    SCH_MARKER* marker = new SCH_MARKER();

                    marker->SetTimeStamp( GetNewTimeStamp() );
                    marker->SetData( ERCE_DIFFERENT_UNIT_NET, item->m_Start,
                        wxString::Format( _( "Pin %s on %s is connected to both %s and %s" ),
                        item->m_PinNum, ref, pin_to_net_map[pin_name], item->GetNetName() ),
                        item->m_Start );
                    marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
                    marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_ERROR );

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
    if( m_settings.check_similar_labels )
        objectsConnectedList->TestforSimilarLabels();

    // Displays global results:
    updateMarkerCounts( &screens );

    // Display diags:
    DisplayERC_MarkersList();

    // Display new markers from the current screen:
    KIGFX::VIEW* view = m_parent->GetCanvas()->GetView();

    for( auto item = m_parent->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_MARKER_T )
            view->Add( item );
    }

    m_parent->GetCanvas()->Refresh();

    // Display message
    aReporter.ReportTail( _( "Finished" ), REPORTER::RPT_INFO );

    if( m_settings.write_erc_file )
    {
        fn = g_RootSheet->GetScreen()->GetFileName();
        fn.SetExt( wxT( "erc" ) );

        wxFileDialog dlg( this, _( "ERC File" ), fn.GetPath(), fn.GetFullName(),
                          ErcFileWildcard(), wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        if( WriteDiagnosticERC( GetUserUnits(), dlg.GetPath() ) )
            ExecuteFile( this, Pgm().GetEditorName(), QuoteFullPath( fn ) );
    }
}


wxDialog* InvokeDialogERC( SCH_EDIT_FRAME* aCaller )
{
    // This is a modeless dialog, so new it rather than instantiating on stack.
    DIALOG_ERC* dlg = new DIALOG_ERC( aCaller );

    dlg->Show( true );

    return dlg;     // wxDialog is information hiding about DIALOG_ERC.
}
