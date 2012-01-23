/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <appl_wxstruct.h>
#include <class_sch_screen.h>
#include <wxEeschemaStruct.h>

#include <general.h>
#include <netlist.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <lib_pin.h>
#include <protos.h>

#include <dialog_erc.h>
#include <dialog_erc_listbox.h>
#include <erc.h>


bool DIALOG_ERC::m_writeErcFile = false;


BEGIN_EVENT_TABLE( DIALOG_ERC, DIALOG_ERC_BASE )
    EVT_COMMAND_RANGE( ID_MATRIX_0, ID_MATRIX_0 + ( PIN_NMAX * PIN_NMAX ) - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED, DIALOG_ERC::ChangeErrorLevel )
END_EVENT_TABLE()


DIALOG_ERC::DIALOG_ERC( SCH_EDIT_FRAME* parent ) :
    DIALOG_ERC_BASE( parent )
{
    m_Parent = parent;
    Init();

    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_ERC::Init()
{
    SetFocus();

    m_Initialized = false;

    for( int ii = 0; ii < PIN_NMAX; ii++ )
        for( int jj = 0; jj < PIN_NMAX; jj++ )
            m_ButtonList[ii][jj] = NULL;

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


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERASE_DRC_MARKERS */
/* Delete the old ERC markers, over the whole hierarchy
 */
void DIALOG_ERC::OnEraseDrcMarkersClick( wxCommandEvent& event )
{
    SCH_SCREENS ScreenList;

    ScreenList.DeleteAllMarkers( MARK_ERC );
    m_MarkersList->ClearList();
    m_Parent->GetCanvas()->Refresh();
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL */
void DIALOG_ERC::OnCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
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


// Double click on a marker info:
void DIALOG_ERC::OnLeftDClickMarkersList( wxCommandEvent& event )
{
    int index = m_MarkersList->GetSelection();

    if( index < 0 )
        return;

    const SCH_MARKER* marker = m_MarkersList->GetItem( (unsigned) index );

    EndModal( 1 );


    // Search for the selected marker
    SCH_SHEET_PATH* sheet;
    bool            NotFound;
    wxPoint         pos = marker->m_Pos;
    wxPoint         curpos, old_cursor_position;

    SCH_SHEET_LIST  SheetList;

    NotFound = true;

    /* Search for the selected marker */
    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        SCH_ITEM* item = (SCH_ITEM*) sheet->LastDrawList();

        while( item && NotFound )
        {
            if( item == marker )
            {
                NotFound = false;
                break;
            }

            item = item->Next();
        }

        if( NotFound == false )
            break;
    }


    if( NotFound ) // Error
    {
        wxMessageBox( wxT( "OnLeftDClickMarkersList() error: Marker Not Found" ) );
        return;
    }

    if( *sheet != m_Parent->GetCurrentSheet() )
    {
        sheet->LastScreen()->SetZoom( m_Parent->GetScreen()->GetZoom() );
        m_Parent->SetCurrentSheet( *sheet );
        m_Parent->GetCurrentSheet().UpdateAllScreenReferences();
    }

    m_Parent->GetScreen()->SetCrossHairPosition( pos );
    m_Parent->RedrawScreen( pos, true );
}


/* Build or rebuild the panel showing the ERC conflict matrix
 */
void DIALOG_ERC::ReBuildMatrixPanel()
{
    int           ii, jj, event_id, text_height;
    wxPoint       pos, BoxMatrixPosition;
    wxStaticText* text;
    wxSize        BoxMatrixMinSize;

    // Try to know the size of bitmap button used in drc matrix
    wxBitmapButton * dummy = new wxBitmapButton( m_PanelERCOptions, wxID_ANY,
                                                 KiBitmap( ercerr_xpm ) );
    wxSize bitmap_size = dummy->GetSize();
    delete dummy;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = true;
    }

    // Get the current text size: this is a dummy text.
    text = new wxStaticText( m_PanelERCOptions, -1, wxT( "W" ), pos );

    text_height   = text->GetRect().GetHeight();
    bitmap_size.y = MAX( bitmap_size.y, text_height );
    SAFE_DELETE( text );

    // compute the Y pos interval:
    BoxMatrixMinSize.y = ( bitmap_size.y * (PIN_NMAX + 1) ) + 5;
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    pos = m_MatrixSizer->GetPosition();

    // Size computation is not made in constructor, in some wxWidgets version,
    // and m_BoxSizerForERC_Opt position is always 0,0. and we can't use it
    pos.x = MAX( pos.x, 5 );
    pos.y = MAX( pos.y, m_ResetOptButton->GetRect().GetHeight() + 30 );

    BoxMatrixPosition = pos;

    pos.y += text_height;

    if( m_Initialized == false )
    {
        // Print row labels
        for( ii = 0; ii < PIN_NMAX; ii++ )
        {
            int y = pos.y + (ii * bitmap_size.y);
            text = new wxStaticText( m_PanelERCOptions, -1, CommentERC_H[ii],
                                     wxPoint( 5, y + ( bitmap_size.y / 2) - (text_height / 2) ) );

            int x = text->GetRect().GetRight();
            pos.x = MAX( pos.x, x );
        }

        pos.x += 5;
    }
    else
    {
        pos = m_ButtonList[0][0]->GetPosition();
    }

    for( ii = 0; ii < PIN_NMAX; ii++ )
    {
        int y = pos.y + (ii * bitmap_size.y);

        for( jj = 0; jj <= ii; jj++ )
        {
            // Add column labels (only once)
            int diag = DiagErc[ii][jj];
            int x    = pos.x + (jj * bitmap_size.x);

            if( (ii == jj) && !m_Initialized )
            {
                wxPoint txtpos;
                txtpos.x = x + (bitmap_size.x / 2);
                txtpos.y = y - text_height;
                text     = new wxStaticText( m_PanelERCOptions,
                                             -1,
                                             CommentERC_V[ii],
                                             txtpos );

                BoxMatrixMinSize.x = MAX( BoxMatrixMinSize.x, text->GetRect().GetRight() );
            }

            event_id = ID_MATRIX_0 + ii + ( jj * PIN_NMAX );
            delete m_ButtonList[ii][jj];

            // Add button on matrix
            switch( diag )
            {
            case OK:
                m_ButtonList[ii][jj] = new wxBitmapButton( m_PanelERCOptions,
                                                           event_id,
                                                           KiBitmap( erc_green_xpm ),
                                                           wxPoint( x, y ) );

                break;

            case WAR:
                m_ButtonList[ii][jj] = new wxBitmapButton( m_PanelERCOptions,
                                                           event_id,
                                                           KiBitmap( ercwarn_xpm ),
                                                           wxPoint( x, y ) );

                break;

            case ERR:
                m_ButtonList[ii][jj] = new wxBitmapButton( m_PanelERCOptions,
                                                           event_id,
                                                           KiBitmap( ercerr_xpm ),
                                                           wxPoint( x, y ) );

                break;
            }
        }
    }

    if( !m_Initialized )
    {
        BoxMatrixMinSize.x += 5;
        m_MatrixSizer->SetMinSize( BoxMatrixMinSize );
        BoxMatrixMinSize.y += BoxMatrixPosition.y;
        m_PanelMatrixSizer->SetMinSize( BoxMatrixMinSize );
    }

    m_Initialized = true;
}


/**
 * Function DisplayERC_MarkersList
 * read the schematic and display the list of ERC markers
 */
void DIALOG_ERC::DisplayERC_MarkersList()
{
    SCH_SHEET_LIST SheetList;

    m_MarkersList->ClearList();

    for( SCH_SHEET_PATH* Sheet = SheetList.GetFirst(); Sheet != NULL; Sheet = SheetList.GetNext() )
    {
        SCH_ITEM* DrawStruct = Sheet->LastDrawList();

        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != SCH_MARKER_T )
                continue;

            SCH_MARKER* Marker = (SCH_MARKER*) DrawStruct;

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
    wxBitmapButton* Butt;
    BITMAP_DEF      new_bitmap_xpm = NULL;
    wxPoint         pos;

    id   = event.GetId();
    ii   = id - ID_MATRIX_0;
    Butt = (wxBitmapButton*) event.GetEventObject();
    pos  = Butt->GetPosition();

    x = ii / PIN_NMAX; y = ii % PIN_NMAX;

    level = DiagErc[y][x];

    switch( level )
    {
    case OK:
        level = WAR;
        new_bitmap_xpm = ercwarn_xpm;
        break;

    case WAR:
        level = ERR;
        new_bitmap_xpm = ercerr_xpm;
        break;

    case ERR:
        level = OK;
        new_bitmap_xpm = erc_green_xpm;
        break;
    }

    if( new_bitmap_xpm )
    {
        delete Butt;
        Butt = new wxBitmapButton( m_PanelERCOptions, id, KiBitmap( new_bitmap_xpm ), pos );

        m_ButtonList[y][x] = Butt;
        DiagErc[y][x] = DiagErc[x][y] = level;
    }
}


void DIALOG_ERC::TestErc( wxArrayString* aMessagesList )
{
    wxFileName fn;
    unsigned   net;
    unsigned   lastNet;
    unsigned   nextNet;

    int        NetNbItems, MinConn;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = true;
    }

    m_writeErcFile = m_WriteResultOpt->GetValue();

    /* Build the whole sheet list in hierarchy (sheet, not screen) */
    SCH_SHEET_LIST sheets;
    sheets.AnnotatePowerSymbols();

    if( m_Parent->CheckAnnotate( aMessagesList, false ) )
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

    m_Parent->BuildNetListBase();

    /* Reset the flag m_FlagOfConnection, that will be used next, in calculations */
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
        g_NetObjectslist[ii]->m_FlagOfConnection = UNCONNECTED;

    nextNet   = lastNet = 0;
    NetNbItems = 0;
    MinConn    = NOC;

    for( net = 0; net < g_NetObjectslist.size(); net++ )
    {
        if( g_NetObjectslist[lastNet]->GetNet() != g_NetObjectslist[net]->GetNet() )
        {
            // New net found:
            MinConn    = NOC;
            NetNbItems = 0;
            nextNet   = net;
        }

        switch( g_NetObjectslist[net]->m_Type )
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
            TestLabel( net, nextNet );
            break;

        case NET_NOCONNECT:

            // ERC problems when a noconnect symbol is connected to more than one pin.
            MinConn = NET_NC;

            if( NetNbItems != 0 )
                Diagnose( g_NetObjectslist[net], NULL, MinConn, UNC );

            break;

        case NET_PIN:

            // Look for ERC problems between pins:
            TestOthersItems( net, nextNet, &NetNbItems, &MinConn );
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
    m_Parent->GetCanvas()->Refresh();

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
            ExecuteFile( this, wxGetApp().GetEditorName(), QuoteFullPath( fn ) );
        }
    }
}
