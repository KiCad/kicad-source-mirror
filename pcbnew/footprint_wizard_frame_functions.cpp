/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo Pelayo, miguelangel@nbee.es
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include "footprint_wizard_frame.h"
#include <wildcards_and_files_ext.h>
#include <dialogs/dialog_footprint_wizard_list.h>
#include <base_units.h>
#include <widgets/wx_grid.h>

#include <tool/tool_manager.h>

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
        wxLogDebug( wxT( "FOOTPRINT_WIZARD_FRAME::Process_Special_Functions error: id = %d" ),
                event.GetId() );
        break;
    }
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

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    // Delete the current footprint
    GetBoard()->DeleteAllModules();

    // Creates the module
    wxString msg;
    MODULE* module = footprintWizard->GetFootprint( &msg );
    DisplayBuildMessage( msg );

    if( module )
    {
        //  Add the object to board
        GetBoard()->Add( module, ADD_APPEND );
        module->SetPosition( wxPoint( 0, 0 ) );
    }
    else
    {
        DBG(printf( "footprintWizard->GetFootprint() returns NULL\n" );)
    }

    updateView();
    GetCanvas()->Refresh();
}


void FOOTPRINT_WIZARD_FRAME::DisplayBuildMessage( wxString& aMessage )
{
    m_buildMessageBox->SetValue( aMessage );
}


FOOTPRINT_WIZARD* FOOTPRINT_WIZARD_FRAME::GetMyWizard()
{
    if( m_wizardName.Length() == 0 )
        return NULL;

    FOOTPRINT_WIZARD* footprintWizard = FOOTPRINT_WIZARD_LIST::GetWizard( m_wizardName );

    if( !footprintWizard )
    {
        wxMessageBox( _( "Couldn't reload footprint wizard" ) );
        return NULL;
    }

    return footprintWizard;
}


MODULE* FOOTPRINT_WIZARD_FRAME::GetBuiltFootprint()
{
    FOOTPRINT_WIZARD* footprintWizard = FOOTPRINT_WIZARD_LIST::GetWizard( m_wizardName );

    if( footprintWizard && m_modal_ret_val )
    {
        wxString msg;
        MODULE * footprint = footprintWizard->GetFootprint( &msg );
        DisplayBuildMessage( msg );

        return footprint;
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

        footprintWizard->ResetParameters();
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
    updateView();
}

void FOOTPRINT_WIZARD_FRAME::DefaultParameters( wxCommandEvent& event )
{
    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if ( footprintWizard == NULL )
        return;

    footprintWizard->ResetParameters();

    // Reload
    ReCreateParameterList();
    ReloadFootprint();
    DisplayWizardInfos();
}


void FOOTPRINT_WIZARD_FRAME::ParametersUpdated( wxGridEvent& event )
{
    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if( !footprintWizard )
        return;

    if( m_parameterGridPage < 0 )
        return;

    wxArrayString   prmValues = footprintWizard->GetParameterValues( m_parameterGridPage );
    wxArrayString   ptList = footprintWizard->GetParameterTypes( m_parameterGridPage );

    bool            has_changed = false;
    int             count = m_parameterGrid->GetNumberRows();

    for( int prm_id = 0; prm_id < count; ++prm_id )
    {
        wxString value = m_parameterGrid->GetCellValue( prm_id, WIZ_COL_VALUE );

        if( prmValues[prm_id] != value )
        {
            has_changed = true;
            prmValues[prm_id] = value;
        }
    }

    if( has_changed )
    {
        wxString res = footprintWizard->SetParameterValues( m_parameterGridPage, prmValues );

        if( !res.IsEmpty() )
            wxMessageBox( res );

        ReloadFootprint();
        DisplayWizardInfos();

        // The python script can have modified some other parameters.
        // So rebuild the current parameter list with new values, just in case.
        ReCreateParameterList();
    }
}

