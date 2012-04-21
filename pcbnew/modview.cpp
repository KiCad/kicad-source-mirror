/**
 * @file viewlibs.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <3d_viewer.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <modview_frame.h>
#include <wildcards_and_files_ext.h>


#define NEXT_PART      1
#define NEW_PART       0
#define PREVIOUS_PART -1


void FOOTPRINT_VIEWER_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    wxString   msg;

    switch( event.GetId() )
    {
    case ID_MODVIEW_NEXT:
        SelectAndViewFootprint( NEXT_PART );
        break;

    case ID_MODVIEW_PREVIOUS:
        SelectAndViewFootprint( PREVIOUS_PART );
        break;

    default:
        msg << wxT( "FOOTPRINT_VIEWER_FRAME::Process_Special_Functions error: id = " )
            << event.GetId();
        wxMessageBox( msg );
        break;
    }
}


void FOOTPRINT_VIEWER_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool FOOTPRINT_VIEWER_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


/* Displays the name of the current opened library in the caption */
void FOOTPRINT_VIEWER_FRAME::DisplayLibInfos()
{
    wxString     msg;

    msg = _( "Library Browser" );
    msg << wxT( " [" );

    if( ! m_libraryName.IsEmpty() )
        msg << m_libraryName;
    else
        msg += _( "no library selected" );

    msg << wxT( "]" );

    SetTitle( msg );
}


void FOOTPRINT_VIEWER_FRAME::SelectCurrentLibrary( wxCommandEvent& event )
{
    wxString msg;

    if( g_LibraryNames.GetCount() == 0 )
        return;

    EDA_LIST_DIALOG dlg( this, _( "Select Current Library:" ),
                         g_LibraryNames, m_libraryName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    if( m_libraryName == dlg.GetTextSelection() )
        return;

    m_libraryName = dlg.GetTextSelection();
    m_footprintName.Empty();
    DisplayLibInfos();
    ReCreateFootprintList();

    int id = m_LibList->FindString( m_libraryName );
    if( id >= 0 )
        m_LibList->SetSelection( id );
}

/**
 * Function SelectCurrentFootprint
 * Selects the current footprint name and display it
 */
void FOOTPRINT_VIEWER_FRAME::SelectCurrentFootprint( wxCommandEvent& event )
{
    wxString libname = m_libraryName + wxT(".") + FootprintLibFileExtension;
    MODULE* oldmodule = GetBoard()->m_Modules;
    MODULE * module = Load_Module_From_Library( libname, false );
    if( module )
    {
        module->SetPosition( wxPoint( 0, 0 ) );

        // Only one fotprint allowed: remove the previous footprint (if exists)
        if( oldmodule )
        {
            GetBoard()->Remove( oldmodule );
            delete oldmodule;
        }
        m_footprintName = module->GetLibRef();
        module->ClearFlags();
        SetCurItem( NULL );

        Zoom_Automatique( false );
        m_canvas->Refresh( );
        Update3D_Frame();
        m_FootprintList->SetStringSelection( m_footprintName );
   }
}


/* Routine to view one selected library content. */
void FOOTPRINT_VIEWER_FRAME::SelectAndViewFootprint( int aMode )
{
    if( m_libraryName.IsEmpty() )
        return;

    int selection = m_FootprintList->FindString( m_footprintName );

    if( aMode == NEXT_PART )
    {
        if( selection != wxNOT_FOUND && selection < (int)m_FootprintList->GetCount()-1 )
            selection++;
    }

    if( aMode == PREVIOUS_PART )
    {
        if( selection != wxNOT_FOUND && selection > 0)
            selection--;
    }

    if( selection != wxNOT_FOUND )
    {
        m_footprintName = m_FootprintList->GetString( selection );
        SetCurItem( NULL );
        // Delete the current footprint
        GetBoard()->m_Modules.DeleteAll();
        GetModuleLibrary( m_libraryName + wxT(".") + FootprintLibFileExtension,
                          m_footprintName, true );
        Update3D_Frame();
    }

    DisplayLibInfos();
    Zoom_Automatique( false );
    m_canvas->Refresh( );
}


/**
 * Function RedrawActiveWindow
 * Display the current selected component.
 * If the component is an alias, the ROOT component is displayed
*/
void FOOTPRINT_VIEWER_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    m_canvas->DrawBackGround( DC );
    GetBoard()->Draw( m_canvas, DC, GR_COPY );

    MODULE* module = GetBoard()->m_Modules;

    if ( module )
        module->DisplayInfo( this );

    m_canvas->DrawCrossHair( DC );

    ClearMsgPanel();
    if( module )
        module->DisplayInfo( this );
}
