/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_erc.cpp
 * @brief Electrical Rules Check dialog implementation.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>
#include <invoke_sch_dialog.h>

#include <netlist.h>
#include <class_netlist_object.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <lib_pin.h>

#include <dialog_erc.h>
#include <dialog_erc_listbox.h>
#include <erc.h>
#include <id.h>


bool DIALOG_ERC::m_writeErcFile = false;


BEGIN_EVENT_TABLE( DIALOG_ERC, DIALOG_ERC_BASE )
    EVT_COMMAND_RANGE( ID_MATRIX_0, ID_MATRIX_0 + ( PIN_NMAX * PIN_NMAX ) - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED, DIALOG_ERC::ChangeErrorLevel )
END_EVENT_TABLE()


DIALOG_ERC::DIALOG_ERC( SCH_EDIT_FRAME* parent ) :
    DIALOG_ERC_BASE(
        parent,
        ID_DIALOG_ERC       // parent looks for this ID explicitly
        )
{
    m_parent = parent;
    Init();

    GetSizer()->SetSizeHints( this );
    Centre();
}

DIALOG_ERC::~DIALOG_ERC()
{
}


void DIALOG_ERC::Init()
{
    m_initialized = false;

    for( int ii = 0; ii < PIN_NMAX; ii++ )
        for( int jj = 0; jj < PIN_NMAX; jj++ )
            m_buttonList[ii][jj] = NULL;

    m_WriteResultOpt->SetValue( m_writeErcFile );

    SCH_SCREENS screens;
    int markers = screens.GetMarkerCount();
    int warnings = screens.GetMarkerCount( WAR );

    wxString num;
    num.Printf( wxT( "%d" ), markers );
    m_TotalErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), markers - warnings );
    m_LastErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), warnings );
    m_LastWarningCount->SetLabel( num );

    DisplayERC_MarkersList();

    // Init Panel Matrix
    ReBuildMatrixPanel();

    // Set the run ERC button as the default button.
    m_buttonERC->SetDefault();
}

/* Delete the old ERC markers, over the whole hierarchy
 */
void DIALOG_ERC::OnEraseDrcMarkersClick( wxCommandEvent& event )
{
    SCH_SCREENS ScreenList;

    ScreenList.DeleteAllMarkers( MARK_ERC );
    m_MarkersList->ClearList();
    m_parent->GetCanvas()->Refresh();
}


/* event handler for Close button
*/
void DIALOG_ERC::OnButtonCloseClick( wxCommandEvent& event )
{
    Close();
}

void DIALOG_ERC::OnCloseErcDialog( wxCloseEvent& event )
{
    Destroy();
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESET_MATRIX */
void DIALOG_ERC::OnResetMatrixClick( wxCommandEvent& event )
{
    ResetDefaultERCDiag( event );
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERC_CMP */
void DIALOG_ERC::OnErcCmpClick( wxCommandEvent& event )
{
    wxBusyCursor();
    m_MarkersList->Clear();
    m_MessagesList->Clear();
    wxSafeYield();      // m_MarkersList must be redraw
    wxArrayString messageList;
    TestErc( &messageList );

    for( unsigned ii = 0; ii < messageList.GetCount(); ii++ )
        m_MessagesList->AppendText( messageList[ii] );
}

// Single click on a marker info:
void DIALOG_ERC::OnLeftClickMarkersList( wxCommandEvent& event )
{
    m_lastMarkerFound = NULL;

    int index = m_MarkersList->GetSelection();

    if( index < 0 )
        return;

    const SCH_MARKER* marker = m_MarkersList->GetItem( (unsigned) index );

    // Search for the selected marker
    SCH_SHEET_PATH* sheet;
    SCH_SHEET_LIST  SheetList;
    bool notFound = true;

    for( sheet = SheetList.GetFirst(); sheet; sheet = SheetList.GetNext() )
    {
        SCH_ITEM* item = (SCH_ITEM*) sheet->LastDrawList();
        for( ; item; item = item->Next() )
        {
            if( item == marker )
            {
                notFound = false;
                break;
            }
        }

        if( notFound == false )
            break;
    }

    if( notFound ) // Error
    {
        wxMessageBox( _( "Marker not found" ) );

        // The marker was deleted, so rebuild marker list
        DisplayERC_MarkersList();
        return;
    }

    if( *sheet != m_parent->GetCurrentSheet() )
    {
        sheet->LastScreen()->SetZoom( m_parent->GetScreen()->GetZoom() );
        m_parent->SetCurrentSheet( *sheet );
        m_parent->GetCurrentSheet().UpdateAllScreenReferences();
    }

    m_lastMarkerFound = marker;
    m_parent->SetCrossHairPosition( marker->m_Pos );
    m_parent->RedrawScreen( marker->m_Pos, false);
}

// Double click on a marker info:
// Close the dialog and jump to the selected marker
void DIALOG_ERC::OnLeftDblClickMarkersList( wxCommandEvent& event )
{
    // Remember: OnLeftClickMarkersList was called just berfore
    // and therefore m_lastMarkerFound was initialized.
    // (NULL if not found)
    if( m_lastMarkerFound )
    {
        m_parent->SetCrossHairPosition( m_lastMarkerFound->m_Pos );
        m_parent->RedrawScreen( m_lastMarkerFound->m_Pos, true);
        // prevent a mouse left button release event in
        // coming from the ERC dialog double click
        // ( the button is released after closing this dialog and will generate
        // an unwanted event in  parent frame)
        m_parent->SkipNextLeftButtonReleaseEvent();
    }

    Close();
}


/* Build or rebuild the panel showing the ERC conflict matrix
 */
void DIALOG_ERC::ReBuildMatrixPanel()
{
    // Try to know the size of bitmap button used in drc matrix
    wxBitmapButton * dummy = new wxBitmapButton( m_matrixPanel, wxID_ANY,
                                                 KiBitmap( ercerr_xpm ) );
    wxSize bitmap_size = dummy->GetSize();
    delete dummy;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = true;
    }

    wxPoint pos;
    // Get the current text size:use a dummy text.
    wxStaticText* text = new wxStaticText( m_matrixPanel, -1, wxT( "W" ), pos );
    int text_height   = text->GetRect().GetHeight();
    bitmap_size.y = std::max( bitmap_size.y, text_height );
    delete text;

    // compute the Y pos interval:
    pos.y = text_height;

    if( m_initialized == false )
    {
        // Print row labels
        for( int ii = 0; ii < PIN_NMAX; ii++ )
        {
            int y = pos.y + (ii * bitmap_size.y);
            text = new wxStaticText( m_matrixPanel, -1, CommentERC_H[ii],
                                     wxPoint( 5, y + ( bitmap_size.y / 2) - (text_height / 2) ) );

            int x = text->GetRect().GetRight();
            pos.x = std::max( pos.x, x );
        }

        pos.x += 5;
    }
    else
        pos = m_buttonList[0][0]->GetPosition();

    for( int ii = 0; ii < PIN_NMAX; ii++ )
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

            int event_id = ID_MATRIX_0 + ii + ( jj * PIN_NMAX );
            BITMAP_DEF bitmap_butt = NULL;

            // Add button on matrix
            switch( diag )
            {
            case OK:
                bitmap_butt = erc_green_xpm;
                break;

            case WAR:
                bitmap_butt = ercwarn_xpm ;
                break;

            default:
            case ERR:
                bitmap_butt = ercerr_xpm;
                break;
            }

            delete m_buttonList[ii][jj];
            m_buttonList[ii][jj] = new wxBitmapButton( m_matrixPanel,
                                                       event_id,
                                                       KiBitmap( bitmap_butt ),
                                                       wxPoint( x, y ) );
        }
    }

    m_initialized = true;
}


/*
 * Function DisplayERC_MarkersList
 * read the schematic and display the list of ERC markers
 */
void DIALOG_ERC::DisplayERC_MarkersList()
{
    SCH_SHEET_LIST sheetList;
    m_MarkersList->ClearList();

    SCH_SHEET_PATH* sheet = sheetList.GetFirst();
    for( ; sheet != NULL; sheet = sheetList.GetNext() )
    {
        SCH_ITEM* item = sheet->LastDrawList();

        for( ; item != NULL; item = item->Next() )
        {
            if( item->Type() != SCH_MARKER_T )
                continue;

            SCH_MARKER* Marker = (SCH_MARKER*) item;

            if( Marker->GetMarkerType() != MARK_ERC )
                continue;

            // Add marker without refresh the displayed list:
            m_MarkersList->AppendToList( Marker, false );
        }
    }

    m_MarkersList->Refresh();
}


/* Resets the default values of the ERC matrix.
 */
void DIALOG_ERC::ResetDefaultERCDiag( wxCommandEvent& event )
{
    memcpy( DiagErc, DefaultDiagErc, sizeof(DiagErc) );
    ReBuildMatrixPanel();
}


/* Change the error level for the pressed button, on the matrix table
 */
void DIALOG_ERC::ChangeErrorLevel( wxCommandEvent& event )
{
    int             id, level, ii, x, y;
    BITMAP_DEF      new_bitmap_butt = NULL;
    wxPoint         pos;

    id   = event.GetId();
    ii   = id - ID_MATRIX_0;
    wxBitmapButton* butt = (wxBitmapButton*) event.GetEventObject();
    pos  = butt->GetPosition();

    x = ii / PIN_NMAX; y = ii % PIN_NMAX;

    level = DiagErc[y][x];

    switch( level )
    {
    case OK:
        level = WAR;
        new_bitmap_butt = ercwarn_xpm;
        break;

    case WAR:
        level = ERR;
        new_bitmap_butt = ercerr_xpm;
        break;

    case ERR:
        level = OK;
        new_bitmap_butt = erc_green_xpm;
        break;
    }

    if( new_bitmap_butt )
    {
        delete butt;
        butt = new wxBitmapButton( m_matrixPanel, id,
                                   KiBitmap( new_bitmap_butt ), pos );

        m_buttonList[y][x] = butt;
        DiagErc[y][x] = DiagErc[x][y] = level;
    }
}


void DIALOG_ERC::TestErc( wxArrayString* aMessagesList )
{
    wxFileName fn;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = true;
    }

    m_writeErcFile = m_WriteResultOpt->GetValue();

    /* Build the whole sheet list in hierarchy (sheet, not screen) */
    SCH_SHEET_LIST sheets;
    sheets.AnnotatePowerSymbols();

    if( m_parent->CheckAnnotate( aMessagesList, false ) )
    {
        if( aMessagesList )
        {
            wxString msg = _( "Annotation required!" );
            msg += wxT( "\n" );
            aMessagesList->Add( msg );
        }

        return;
    }

    SCH_SCREENS screens;

    // Erase all previous DRC markers.
    screens.DeleteAllMarkers( MARK_ERC );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        /* Ff wire list has changed, delete Undo Redo list to avoid pointers on deleted
         * data problems.
         */
        if( screen->SchematicCleanUp( NULL ) )
            screen->ClearUndoRedoList();
    }

    /* Test duplicate sheet names inside a given sheet, one cannot have sheets with
     * duplicate names (file names can be duplicated).
     */
    TestDuplicateSheetNames( true );

    NETLIST_OBJECT_LIST* objectsConnectedList = m_parent->BuildNetListBase();

    // Reset the connection type indicator
    objectsConnectedList->ResetConnectionsType();

    unsigned lastNet;
    unsigned nextNet = lastNet = 0;
    int NetNbItems = 0;
    int MinConn    = NOC;

    for( unsigned net = 0; net < objectsConnectedList->size(); net++ )
    {
        if( objectsConnectedList->GetItemNet( lastNet ) !=
            objectsConnectedList->GetItemNet( net ) )
        {
            // New net found:
            MinConn    = NOC;
            NetNbItems = 0;
            nextNet   = net;
        }

        switch( objectsConnectedList->GetItemType( net ) )
        {
        // These items do not create erc problems
        case NET_ITEM_UNSPECIFIED:
        case NET_SEGMENT:
        case NET_BUS:
        case NET_JUNCTION:
        case NET_LABEL:
        case NET_BUSLABELMEMBER:
        case NET_PINLABEL:
        case NET_GLOBLABEL:
        case NET_GLOBBUSLABELMEMBER:
            break;

        case NET_HIERLABEL:
        case NET_HIERBUSLABELMEMBER:
        case NET_SHEETLABEL:
        case NET_SHEETBUSLABELMEMBER:

            // ERC problems when pin sheets do not match hierarchical labels.
            // Each pin sheet must match a hierarchical label
            // Each hierarchical label must match a pin sheet
            TestLabel( objectsConnectedList, net, nextNet );
            break;

        case NET_NOCONNECT:

            // ERC problems when a noconnect symbol is connected to more than one pin.
            MinConn = NET_NC;

            if( NetNbItems != 0 )
                Diagnose( objectsConnectedList->GetItem( net ), NULL, MinConn, UNC );

            break;

        case NET_PIN:

            // Look for ERC problems between pins:
            TestOthersItems( objectsConnectedList, net, nextNet, &NetNbItems, &MinConn );
            break;
        }

        lastNet = net;
    }

    // Displays global results:
    wxString num;
    int markers = screens.GetMarkerCount();
    int warnings = screens.GetMarkerCount( WAR );

    num.Printf( wxT( "%d" ), markers );
    m_TotalErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), markers - warnings );
    m_LastErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), warnings );
    m_LastWarningCount->SetLabel( num );

    // Display diags:
    DisplayERC_MarkersList();

    // Display new markers:
    m_parent->GetCanvas()->Refresh();

    if( m_writeErcFile )
    {
        fn = g_RootSheet->GetScreen()->GetFileName();
        fn.SetExt( wxT( "erc" ) );

        wxFileDialog dlg( this, _( "ERC File" ), fn.GetPath(), fn.GetFullName(),
                          _( "Electronic rule check file (.erc)|*.erc" ),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        if( WriteDiagnosticERC( dlg.GetPath() ) )
        {
            Close( true );
            ExecuteFile( this, Pgm().GetEditorName(), QuoteFullPath( fn ) );
        }
    }
}


wxDialog* InvokeDialogERC( SCH_EDIT_FRAME* aCaller )
{
    // This is a modeless dialog, so new it rather than instantiating on stack.
    DIALOG_ERC* dlg = new DIALOG_ERC( aCaller );

    dlg->Show( true );

    return dlg;     // wxDialog is information hiding about DIALOG_ERC.
}
