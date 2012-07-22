/**
 * @file footprint_wizard.cpp
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
#include <dialogs/dialog_footprint_wizard_list.h>
#include <base_units.h>

#define NEXT_PART      1
#define NEW_PART       0
#define PREVIOUS_PART -1


void FOOTPRINT_WIZARD_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    wxString   msg;
    int page;

    switch( event.GetId() )
    {
    case ID_FOOTPRINT_WIZARD_NEXT:
        m_PageList->SetSelection(m_PageList->GetSelection()+1,true);
        break;

    case ID_FOOTPRINT_WIZARD_PREVIOUS:
        page = m_PageList->GetSelection()-1;
        if (page<0) page=0;
        m_PageList->SetSelection(page,true);
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

void FOOTPRINT_WIZARD_FRAME::ReloadFootprint()
{
    if( m_FootprintWizard == NULL )
        return;

    SetCurItem( NULL );
    // Delete the current footprint
    GetBoard()->m_Modules.DeleteAll();
    MODULE *m = m_FootprintWizard->GetModule();
    if (m)
    {
        /* Here we should make a copy of the object before adding to board*/
        m->SetParent((EDA_ITEM*)GetBoard());
        GetBoard()->m_Modules.Append(m);
        wxPoint p(0,0);
        m->SetPosition(p);
    }
    else
    {
        printf ("m_FootprintWizard->GetModule() returns NULL\n");
    }
    m_canvas->Refresh();
}

void FOOTPRINT_WIZARD_FRAME::SelectFootprintWizard()
{
    DIALOG_FOOTPRINT_WIZARD_LIST         *selectWizard =  
            new DIALOG_FOOTPRINT_WIZARD_LIST(this);
    
    selectWizard->ShowModal();
    
    m_FootprintWizard = selectWizard->GetWizard();

    if (m_FootprintWizard)
    {
        m_wizardName = m_FootprintWizard->GetName();
        m_wizardDescription = m_FootprintWizard->GetDescription();
    }

    ReloadFootprint();
    Zoom_Automatique(false);
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
    
    int page = m_PageList->GetSelection();
    
    if (page<0)
        return;
    
    int n=m_ParameterGrid->GetNumberRows();
    wxArrayString arr;
    wxArrayString ptList = m_FootprintWizard->GetParameterTypes(page);
    
    for (int i=0;i<n;i++)
    {    
        wxString value = m_ParameterGrid->GetCellValue(i,1);
        
        // if this parameter is expected to be an internal 
        // unit convert it back from the user format
        if (ptList[i]==wxT("IU")) 
        {
            double dValue;
            value.ToDouble(&dValue);

            // convert from mils to inches where it's needed
            if (g_UserUnit==INCHES)  dValue = dValue / 1000.0; 
            dValue = From_User_Unit(g_UserUnit,dValue);
                        
            value.Printf(wxT("%lf"),dValue); 
            
        }

        arr.Add(value);
    }
    
    wxString res = m_FootprintWizard->SetParameterValues(page,arr);
    
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

    if ( module )
        module->DisplayInfo( this );

    m_canvas->DrawCrossHair( DC );

    ClearMsgPanel();
    if( module )
        module->DisplayInfo( this );
}
