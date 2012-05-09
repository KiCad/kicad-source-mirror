/**
 * @file footprint wizard.cpp
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
#include "footprint_wizard_frame.h"
#include <wildcards_and_files_ext.h>


#define NEXT_PART      1
#define NEW_PART       0
#define PREVIOUS_PART -1


void FOOTPRINT_WIZARD_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    wxString   msg;

    switch( event.GetId() )
    {
    case ID_FOOTPRINT_WIZARD_NEXT:
        //SelectAndViewFootprint( NEXT_PART );
        break;

    case ID_FOOTPRINT_WIZARD_PREVIOUS:
        //SelectAndViewFootprint( PREVIOUS_PART );
        break;

    default:
        msg << wxT( "FOOTPRINT_WIZARD_FRAME::Process_Special_Functions error: id = " )
            << event.GetId();
        wxMessageBox( msg );
        break;
    }
}


void FOOTPRINT_WIZARD_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool FOOTPRINT_WIZARD_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


/* Displays the name of the current opened library in the caption */
void FOOTPRINT_WIZARD_FRAME::DisplayWizardInfos()
{
    wxString     msg;

    msg = _( "Footprint Wizard" );
    msg << wxT( " [" );

    if( ! m_wizardName.IsEmpty() )
        msg << m_wizardName;
    else
        msg += _( "no wizard selected" );

    msg << wxT( "]" );

    SetTitle( msg );
}


void FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard( wxCommandEvent& event )
{
    wxString msg;

    if( g_LibraryNames.GetCount() == 0 )
        return;

    EDA_LIST_DIALOG dlg( this, _( "Select Current Wizard:" ),
                         g_LibraryNames, m_wizardName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    if( m_wizardName == dlg.GetTextSelection() )
        return;

    m_wizardName = dlg.GetTextSelection();

    DisplayWizardInfos();
    ReCreatePageList();
    ReCreateParameterList();

    
}

/**
 * Function SelectCurrentFootprint
 * Selects the current footprint name and display it
 */
void FOOTPRINT_WIZARD_FRAME::ParametersUpdated( wxCommandEvent& event )
{
    /*
    // will pick it from the wizard
    MODULE * module = new MODULE(NULL);
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
     * */
}


/**
 * Function RedrawActiveWindow
 * Display the current selected component.
 * If the component is an alias, the ROOT component is displayed
*/
void FOOTPRINT_WIZARD_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
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
