/**
 * @file footprint_wizard.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <3d_viewer.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include "footprint_wizard_frame.h"
#include <wildcards_and_files_ext.h>
#include <dialogs/dialog_footprint_wizard_list.h>
#include <base_units.h>


void FOOTPRINT_WIZARD_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    wxString    msg;
    int         page;

    switch( event.GetId() )
    {
    case ID_FOOTPRINT_WIZARD_NEXT:
        m_pageList->SetSelection( m_pageList->GetSelection() + 1, true );
        ClickOnPageList( event );
        break;

    case ID_FOOTPRINT_WIZARD_PREVIOUS:
        page = m_pageList->GetSelection() - 1;

        if( page < 0 )
            page = 0;

        m_pageList->SetSelection( page, true );
        ClickOnPageList( event );
        break;

    default:
        msg << wxT( "FOOTPRINT_WIZARD_FRAME::Process_Special_Functions error: id = " )
            << event.GetId();
        wxMessageBox( msg );
        break;
    }
}


/* Function OnLeftClick
 * Captures a left click event in the dialog
 *
 */
void FOOTPRINT_WIZARD_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


/* Function OnRightClick
 * Captures a right click event in the dialog
 *
 */
bool FOOTPRINT_WIZARD_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


/* Displays the name of the current opened library in the caption */
void FOOTPRINT_WIZARD_FRAME::DisplayWizardInfos()
{
    wxString msg;

    msg = _( "Footprint Wizard" );
    msg << wxT( " [" );

    if( !m_wizardName.IsEmpty() )
        msg << m_wizardName;
    else
        msg += _( "no wizard selected" );

    msg << wxT( "]" );

    SetTitle( msg );
}


void FOOTPRINT_WIZARD_FRAME::ReloadFootprint()
{
    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if( !footprintWizard )
        return;

    SetCurItem( NULL );
    // Delete the current footprint
    GetBoard()->m_Modules.DeleteAll();

    // Creates the module
    MODULE* module = footprintWizard->GetModule();

    if( module )
    {
        //  Add the object to board
        GetBoard()->Add( module, ADD_APPEND );
        module->SetPosition( wxPoint( 0, 0 ) );
    }
    else
    {
        DBG(printf( "footprintWizard->GetModule() returns NULL\n" );)
    }

    m_canvas->Refresh();
}


FOOTPRINT_WIZARD* FOOTPRINT_WIZARD_FRAME::GetMyWizard()
{
    if( m_wizardName.Length()==0 )
        return NULL;

    FOOTPRINT_WIZARD* footprintWizard = FOOTPRINT_WIZARDS::GetWizard( m_wizardName );

    if( !footprintWizard )
    {
        wxMessageBox( _( "Couldn't reload footprint wizard" ) );
        return NULL;
    }

    return footprintWizard;
}


MODULE* FOOTPRINT_WIZARD_FRAME::GetBuiltFootprint()
{
    FOOTPRINT_WIZARD* footprintWizard = FOOTPRINT_WIZARDS::GetWizard( m_wizardName );

    if( footprintWizard && m_modal_ret_val )
    {
        return footprintWizard->GetModule();
    }

    return NULL;
}


void FOOTPRINT_WIZARD_FRAME::SelectFootprintWizard()
{
    DIALOG_FOOTPRINT_WIZARD_LIST wizardSelector( this );

    if( wizardSelector.ShowModal() != wxID_OK )
        return;

    FOOTPRINT_WIZARD* footprintWizard = wizardSelector.GetWizard();

    if( footprintWizard )
    {
        m_wizardName = footprintWizard->GetName();
        m_wizardDescription = footprintWizard->GetDescription();
    }
    else
    {
        m_wizardName.Empty();
        m_wizardDescription.Empty();
    }

    ReloadFootprint();
    Zoom_Automatique( false );
    DisplayWizardInfos();
    ReCreatePageList();
    ReCreateParameterList();
}


void FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard( wxCommandEvent& event )
{
    SelectFootprintWizard();
}


/**
 * Function SelectCurrentFootprint
 * Selects the current footprint name and display it
 */
void FOOTPRINT_WIZARD_FRAME::ParametersUpdated( wxGridEvent& event )
{
    int page = m_pageList->GetSelection();

    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if( !footprintWizard )
        return;

    if( page<0 )
        return;

    int             n = m_parameterGrid->GetNumberRows();
    wxArrayString   arr;
    wxArrayString   ptList = footprintWizard->GetParameterTypes( page );

    for( int i = 0; i < n; i++ )
    {
        wxString value = m_parameterGrid->GetCellValue( i, 1 );

        // if this parameter is expected to be an internal
        // unit convert it back from the user format
        if( ptList[i]==wxT( "IU" ) )
        {
            LOCALE_IO   toggle;
            double      dValue;

            value.ToDouble( &dValue );

            // convert from mils to inches where it's needed
            if( g_UserUnit==INCHES )
                dValue = dValue / 1000.0;

            dValue = From_User_Unit( g_UserUnit, dValue );
            value.Printf( wxT( "%f" ), dValue );
        }

        // If our locale is set to use , for decimal point, just change it
        // to be scripting compatible
        arr.Add( value );
    }

    wxString res = footprintWizard->SetParameterValues( page, arr );

    ReloadFootprint();
    DisplayWizardInfos();
}


/**
 * Function RedrawActiveWindow
 * Display the current selected component.
 * If the component is an alias, the ROOT component is displayed
 *
 */
void FOOTPRINT_WIZARD_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( !GetBoard() )
        return;

    m_canvas->DrawBackGround( DC );
    GetBoard()->Draw( m_canvas, DC, GR_COPY );

    MODULE* module = GetBoard()->m_Modules;

    if( module )
        SetMsgPanel( module );

    m_canvas->DrawCrossHair( DC );

    ClearMsgPanel();

    if( module )
        SetMsgPanel( module );
}
