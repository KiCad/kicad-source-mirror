/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_erc.cpp
// Purpose:
// Author:      jean-pierre Charras
// Modified by:
// Created:     02/07/2000
// License:     GPL
/////////////////////////////////////////////////////////////////////////////
#include "fctsys.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "netlist.h"
#include "sch_marker.h"
#include "sch_sheet.h"
#include "lib_pin.h"
#include "protos.h"

#include "dialog_erc.h"
#include "dialog_erc_listbox.h"
#include "erc.h"


bool DIALOG_ERC::m_writeErcFile = false;


BEGIN_EVENT_TABLE( DIALOG_ERC, DIALOG_ERC_BASE )
    EVT_COMMAND_RANGE( ID_MATRIX_0, ID_MATRIX_0 + ( PIN_NMAX * PIN_NMAX ) - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       DIALOG_ERC::ChangeErrorLevel )
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

    m_Initialized = FALSE;

    for( int ii = 0; ii < PIN_NMAX; ii++ )
        for( int jj = 0; jj < PIN_NMAX; jj++ )
            m_ButtonList[ii][jj] = NULL;

    m_WriteResultOpt->SetValue( m_writeErcFile );

    wxString num;
    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc );
    m_TotalErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc - g_EESchemaVar.NbWarningErc );
    m_LastErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbWarningErc );
    m_LastWarningCount->SetLabel( num );

    DisplayERC_MarkersList();

    // Init Panel Matrix
    ReBuildMatrixPanel();

    // Set the run ERC button as the default button.
    m_buttonERC->SetDefault();
}


/* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ERASE_DRC_MARKERS */
void DIALOG_ERC::OnEraseDrcMarkersClick( wxCommandEvent& event )
{
/* Delete the old ERC markers, over the whole hierarchy
 */
    SCH_SCREENS ScreenList;
    ScreenList.DeleteAllMarkers( MARK_ERC );
    m_MarkersList->ClearList();
    m_Parent->DrawPanel->Refresh();
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

    NotFound = TRUE;
    /* Search for the selected marker */
    for( sheet = SheetList.GetFirst();
        sheet != NULL;
        sheet = SheetList.GetNext() )
    {
        SCH_ITEM* item = (SCH_ITEM*) sheet->LastDrawList();
        while( item && NotFound )
        {
            if( item == marker )
            {
                NotFound = FALSE;
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

    if( sheet != m_Parent->GetSheet() )
    {
        sheet->LastScreen()->SetZoom( m_Parent->GetScreen()->GetZoom() );
        *m_Parent->m_CurrentSheet = *sheet;
        m_Parent->m_CurrentSheet->UpdateAllScreenReferences();
    }

    m_Parent->GetScreen()->SetCrossHairPosition(pos);
    m_Parent->RedrawScreen( pos, true );
}


/* Build or rebuild the panel showing the ERC conflict matrix
 */
void DIALOG_ERC::ReBuildMatrixPanel()
{
    int           ii, jj, event_id, text_height;
    wxPoint       pos, BoxMatrixPosition;

#define BITMAP_SIZE 19
    int           bitmap_size = BITMAP_SIZE;
    wxStaticText* text;
    int           x, y;
    wxSize        BoxMatrixMinSize;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = TRUE;
    }

    // Get the current text size: this is a dummy text.
    text = new wxStaticText( m_PanelERCOptions, -1, wxT( "W" ), pos );

    text_height = text->GetRect().GetHeight();
    bitmap_size = MAX( bitmap_size, text_height );
    SAFE_DELETE( text );

    // compute the Y pos interval:
    BoxMatrixMinSize.y = ( bitmap_size * (PIN_NMAX + 1) ) + 5;
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    pos = m_MatrixSizer->GetPosition();

    // Size computation is not made in constructor, in some wxWidgets version,
    // and m_BoxSizerForERC_Opt position is always 0,0. and we can't use it
    pos.x = MAX( pos.x, 5 );
    pos.y = MAX( pos.y, m_ResetOptButton->GetRect().GetHeight() + 30 );

    BoxMatrixPosition = pos;

    pos.y += text_height;

    if( m_Initialized == FALSE )
    {
        for( ii = 0; ii < PIN_NMAX; ii++ )
        {
            y    = pos.y + (ii * bitmap_size);
            text = new wxStaticText( m_PanelERCOptions, -1, CommentERC_H[ii],
                                     wxPoint( 5, y ) );

            x     = text->GetRect().GetRight();
            pos.x = MAX( pos.x, x );
        }

        pos.x += 5;
    }
    else
        pos = m_ButtonList[0][0]->GetPosition();

    for( ii = 0; ii < PIN_NMAX; ii++ )
    {
        y = pos.y + (ii * bitmap_size);
        for( jj = 0; jj <= ii; jj++ )
        {
            int diag = DiagErc[ii][jj];
            x = pos.x + (jj * bitmap_size);
            if( (ii == jj) && !m_Initialized )
            {
                wxPoint txtpos;
                txtpos.x = x + 6;
                txtpos.y = y - bitmap_size;
                text     = new wxStaticText( m_PanelERCOptions,
                                             -1,
                                             CommentERC_V[ii],
                                             txtpos );

                BoxMatrixMinSize.x = MAX( BoxMatrixMinSize.x,
                                          text->GetRect().GetRight() );
            }
            event_id = ID_MATRIX_0 + ii + ( jj * PIN_NMAX );
            delete m_ButtonList[ii][jj];

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
                                                           KiBitmap( warning_xpm ),
                                                           wxPoint( x, y ) );

                break;

            case ERR:
                m_ButtonList[ii][jj] = new wxBitmapButton( m_PanelERCOptions,
                                                           event_id,
                                                           KiBitmap( error_xpm ),
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
    m_Initialized = TRUE;
}


/**
 * Function DisplayERC_MarkersList
 * read the schematic and display the list of ERC markers
 */
void DIALOG_ERC::DisplayERC_MarkersList()
{
    SCH_SHEET_LIST SheetList;

    m_MarkersList->ClearList();

    for( SCH_SHEET_PATH* Sheet = SheetList.GetFirst();
        Sheet != NULL;
        Sheet = SheetList.GetNext() )
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
        new_bitmap_xpm = warning_xpm;
        break;

    case WAR:
        level = ERR;
        new_bitmap_xpm = error_xpm;
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
    unsigned   NetItemRef;
    unsigned   OldItem;
    unsigned   StartNet;

    int        NetNbItems, MinConn;

    if( !DiagErcTableInit )
    {
        memcpy( DiagErc, DefaultDiagErc, sizeof(DefaultDiagErc) );
        DiagErcTableInit = TRUE;
    }

    m_writeErcFile = m_WriteResultOpt->GetValue();

    /* Build the whole sheet list in hierarchy (sheet, not screen) */
    SCH_SHEET_LIST  sheets;
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

    /* Erase all DRC markers */
    SCH_SCREENS ScreenList;

    ScreenList.DeleteAllMarkers( MARK_ERC );

    g_EESchemaVar.NbErrorErc   = 0;
    g_EESchemaVar.NbWarningErc = 0;

    for( SCH_SCREEN* Screen = ScreenList.GetFirst();
         Screen != NULL;
         Screen = ScreenList.GetNext() )
    {
        bool ModifyWires;
        ModifyWires = Screen->SchematicCleanUp( NULL );

        /* if wire list has changed, delete Undo Redo list to avoid
         *  pointers on deleted data problems */
        if( ModifyWires )
            Screen->ClearUndoRedoList();
    }

    /* Test duplicate sheet names
     * inside a given sheet, one cannot have sheets with duplicate names (file
     * names can be duplicated).
     */
    int errcnt = TestDuplicateSheetNames( true );
    g_EESchemaVar.NbErrorErc   += errcnt;

    m_Parent->BuildNetListBase();

    /* Reset the flag m_FlagOfConnection, that will be used next, in
     * calculations */
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
        g_NetObjectslist[ii]->m_FlagOfConnection = UNCONNECTED;

    StartNet   = OldItem = 0;
    NetNbItems = 0;
    MinConn    = NOC;

    for( NetItemRef = 0; NetItemRef < g_NetObjectslist.size(); NetItemRef++ )
    {
        if( g_NetObjectslist[OldItem]->GetNet() != g_NetObjectslist[NetItemRef]->GetNet() )
        {   // New net found:
            MinConn    = NOC;
            NetNbItems = 0;
            StartNet   = NetItemRef;
        }

        switch( g_NetObjectslist[NetItemRef]->m_Type )
        {
        case NET_ITEM_UNSPECIFIED:
        case NET_SEGMENT:
        case NET_BUS:
        case NET_JONCTION:
        case NET_LABEL:
        case NET_BUSLABELMEMBER:
        case NET_PINLABEL:
        case NET_GLOBLABEL:
        case NET_GLOBBUSLABELMEMBER:

            // These items do not create erc problems
            break;

        case NET_HIERLABEL:
        case NET_HIERBUSLABELMEMBER:
        case NET_SHEETLABEL:
        case NET_SHEETBUSLABELMEMBER:

            // ERC problems when pin sheets do not match hierarchical labels.
            // Each pin sheet must match a hierarchical label
            // Each hierarchical label must match a pin sheet
            TestLabel( m_Parent->DrawPanel, NetItemRef, StartNet );
            break;

        case NET_NOCONNECT:

            // ERC problems when a noconnect symbol is connected to more than
            // one pin.
            MinConn = NET_NC;
            if( NetNbItems != 0 )
                Diagnose( m_Parent->DrawPanel, g_NetObjectslist[NetItemRef], NULL, MinConn, UNC );
            break;

        case NET_PIN:

            // Look for ERC problems between pins:
            TestOthersItems( m_Parent->DrawPanel, NetItemRef, StartNet, &NetNbItems, &MinConn );
            break;
        }

        OldItem = NetItemRef;
    }

    // Displays global results:
    wxString num;
    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc );
    m_TotalErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbErrorErc - g_EESchemaVar.NbWarningErc );
    m_LastErrCount->SetLabel( num );

    num.Printf( wxT( "%d" ), g_EESchemaVar.NbWarningErc );
    m_LastWarningCount->SetLabel( num );

    // Display diags:
    DisplayERC_MarkersList();

    // Display new markers:
    m_Parent->DrawPanel->Refresh();

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
            Close( TRUE );
            ExecuteFile( this, wxGetApp().GetEditorName(), QuoteFullPath( fn ) );
        }
    }
}
