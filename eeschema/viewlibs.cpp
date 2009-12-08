/****************************/
/*  EESchema - viewlibs.cpp */
/****************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libviewfrm.h"
#include "eeschema_id.h"
#include "class_library.h"


#define NEXT_PART      1
#define NEW_PART       0
#define PREVIOUS_PART -1


void WinEDA_ViewlibFrame::Process_Special_Functions( wxCommandEvent& event )
{
    wxString msg;
    CMP_LIB_ENTRY* LibEntry;
    int     ii, id = event.GetId();

    switch( id )
    {
    case ID_LIBVIEW_SELECT_LIB:
        SelectCurrentLibrary();
        break;

    case ID_LIBVIEW_SELECT_PART:
        SelectAndViewLibraryPart( NEW_PART );
        break;

    case ID_LIBVIEW_NEXT:
        SelectAndViewLibraryPart( NEXT_PART );
        break;

    case ID_LIBVIEW_PREVIOUS:
        SelectAndViewLibraryPart( PREVIOUS_PART );
        break;

    case ID_LIBVIEW_VIEWDOC:
        LibEntry = CMP_LIBRARY::FindLibraryEntry( m_entryName,
                                                  m_libraryName );

        if( LibEntry && ( !LibEntry->m_DocFile.IsEmpty() ) )
            GetAssociatedDocument( this, LibEntry->m_DocFile,
                                   &wxGetApp().GetLibraryPathList() );
        break;

    case ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT:
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, TRUE );
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, FALSE );
        m_convert = 1;
        DrawPanel->Refresh();
        break;

    case ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT:
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT, FALSE );
        m_HToolBar->ToggleTool( ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT, TRUE );
        m_convert = 2;
        DrawPanel->Refresh();
        break;

    case ID_LIBVIEW_SELECT_PART_NUMBER:
        ii = SelpartBox->GetChoice();
        if( ii < 0 )
            return;
        m_unit = ii + 1;
        DrawPanel->Refresh();
        break;

    default:
        msg << wxT( "WinEDA_ViewlibFrame::Process_Special_Functions error: id = " ) << id;
        DisplayError( this, msg );
        break;
    }
}


void WinEDA_ViewlibFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool WinEDA_ViewlibFrame::OnRightClick( const wxPoint& MousePos,
                                        wxMenu*        PopMenu )
{
    return true;
}


/* Displays the name of the current opened library in the caption */
void WinEDA_ViewlibFrame::DisplayLibInfos()
{
    wxString     msg;
    CMP_LIBRARY* Lib;

    Lib = CMP_LIBRARY::FindLibrary( m_libraryName );
    msg = _( "Library Browser" );

    msg << wxT( " [" );

    if( Lib )
        msg <<  Lib->GetFullFileName();
    else
        msg += _( "no library selected" );

    msg << wxT( "]" );
    SetTitle( msg );
}


/*****************************************/
/* Function to Select Current library      */
/*****************************************/
void WinEDA_ViewlibFrame::SelectCurrentLibrary()
{
    CMP_LIBRARY* Lib;

    Lib = SelectLibraryFromList( this );
    if( Lib )
    {
        m_entryName.Empty();
        m_libraryName = Lib->GetName();
        DisplayLibInfos();
        if( m_LibList )
        {
            ReCreateListCmp();
            DrawPanel->Refresh();
            DisplayLibInfos();
            ReCreateHToolbar();
            int id = m_LibList->FindString( m_libraryName.GetData() );
            if( id >= 0 )
                m_LibList->SetSelection( id );
        }
    }
}


/*
 * Routine to select and view library Part (NEW, NEXT or PREVIOUS)
 */
void WinEDA_ViewlibFrame::SelectAndViewLibraryPart( int option )
{
    CMP_LIBRARY* Lib;

    if( m_libraryName.IsEmpty() )
        SelectCurrentLibrary();
    if( m_libraryName.IsEmpty() )
        return;

    Lib = CMP_LIBRARY::FindLibrary( m_libraryName );
    if( Lib == NULL )
        return;

    if( ( m_entryName.IsEmpty() ) || ( option == NEW_PART ) )
    {
        ViewOneLibraryContent( Lib, NEW_PART );
        return;
    }

    CMP_LIB_ENTRY* LibEntry = Lib->FindEntry( m_entryName );

    if( LibEntry == NULL )
        return;

    if( option == NEXT_PART )
        ViewOneLibraryContent( Lib, NEXT_PART );

    if( option == PREVIOUS_PART )
        ViewOneLibraryContent( Lib, PREVIOUS_PART );
}


/*************************************************/
/* Routine to view one selected library content. */
/*************************************************/
void WinEDA_ViewlibFrame::ViewOneLibraryContent( CMP_LIBRARY* Lib, int Flag )
{
    int            NumOfParts = 0;
    CMP_LIB_ENTRY* LibEntry;
    wxString       CmpName;
    wxClientDC     dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    if( Lib )
        NumOfParts = Lib->GetCount();

    if( NumOfParts == 0 )
    {
        DisplayError( this, wxT( "No library or library is empty!" ) );
        return;
    }

    if( Lib == NULL )
        return;

    if( Flag == NEW_PART )
    {
        DisplayComponentsNamesInLib( this, Lib, CmpName,
                                     m_entryName );
    }

    if( Flag == NEXT_PART )
    {
        LibEntry = Lib->GetNextEntry( m_entryName );

        if( LibEntry )
            CmpName = LibEntry->GetName();
    }

    if( Flag == PREVIOUS_PART )
    {
        LibEntry = Lib->GetPreviousEntry( m_entryName );

        if( LibEntry )
            CmpName = LibEntry->GetName();
    }

    m_unit    = 1;
    m_convert = 1;

    LibEntry = Lib->FindEntry( CmpName );
    m_entryName = CmpName;
    DisplayLibInfos();
    Zoom_Automatique( false );
    RedrawActiveWindow( &dc, true );

    if( m_CmpList )
    {
        int id = m_CmpList->FindString( m_entryName.GetData() );
        if( id >= 0 )
            m_CmpList->SetSelection( id );
    }
    ReCreateHToolbar();
}


/** function RedrawActiveWindow
 * Display the current selected component.
 * If the component is an alias, the ROOT component is displayed
*/
void WinEDA_ViewlibFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    LIB_COMPONENT* component;
    CMP_LIB_ENTRY* entry;
    CMP_LIBRARY*   lib;
    wxString       msg;
    wxString       tmp;

    ActiveScreen = GetScreen();

    lib = CMP_LIBRARY::FindLibrary( m_libraryName );

    if( lib == NULL )
        return;

    entry = lib->FindEntry( m_entryName );

    if( entry == NULL )
        return;

    /* Forcage de la reinit de la brosse et plume courante */
    GRResetPenAndBrush( DC );
    DC->SetBackground( *wxBLACK_BRUSH );
    DC->SetBackgroundMode( wxTRANSPARENT );

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    if( entry->Type != ROOT )
    {
        LIB_ALIAS* alias = (LIB_ALIAS*) entry;
        component = alias->GetComponent();

        wxASSERT( component != NULL && component->Type == ROOT );

        msg = alias->GetName();

        /* Temporarily change the name field text to reflect the alias name. */
        tmp = component->GetName();
        component->SetName( alias->GetName() );
        if( m_unit < 1 )
            m_unit = 1;
        if( m_convert < 1 )
            m_convert = 1;
        component->SetName( tmp );
    }
    else
    {
        component = (LIB_COMPONENT*) entry;
        msg = _( "None" );
    }

    component->Draw( DrawPanel, DC, wxPoint( 0, 0 ), m_unit, m_convert,
                     GR_DEFAULT_DRAWMODE );

    if( !tmp.IsEmpty() )
        component->SetName( tmp );

    ClearMsgPanel();
    AppendMsgPanel( _( "Part" ), component->GetName(), BLUE, 6 );
    AppendMsgPanel( _( "Alias" ), msg, RED, 6 );
    AppendMsgPanel( _( "Description" ), entry->m_Doc, CYAN, 6 );
    AppendMsgPanel( _( "Key words" ), entry->m_KeyWord, DARKDARKGRAY );

    DrawPanel->Trace_Curseur( DC );
}
