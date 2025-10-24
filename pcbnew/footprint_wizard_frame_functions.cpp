/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo Pelayo, miguelangel@nbee.es
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <pcbnew_id.h>
#include <wildcards_and_files_ext.h>
#include <dialogs/dialog_footprint_wizard_list.h>
#include <base_units.h>
#include <widgets/wx_grid.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <tool/tool_manager.h>
#include "footprint_wizard_frame.h"


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


void FOOTPRINT_WIZARD_FRAME::RegenerateFootprint()
{
    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if( !footprintWizard )
        return;

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    // Delete the current footprint
    GetBoard()->DeleteAllFootprints();

    // Creates the footprint
    wxString   msg;
    FOOTPRINT* footprint = footprintWizard->GetFootprint( &msg );
    DisplayBuildMessage( msg );

    if( footprint )
    {
        //  Add the object to board
        GetBoard()->Add( footprint, ADD_MODE::APPEND );
        footprint->SetPosition( VECTOR2I( 0, 0 ) );
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
        return nullptr;

    FOOTPRINT_WIZARD* footprintWizard = FOOTPRINT_WIZARD_LIST::GetWizard( m_wizardName );

    if( !footprintWizard )
    {
        wxMessageBox( _( "Couldn't reload footprint wizard" ) );
        return nullptr;
    }

    return footprintWizard;
}


FOOTPRINT* FOOTPRINT_WIZARD_FRAME::GetBuiltFootprint()
{
    FOOTPRINT_WIZARD* footprintWizard = FOOTPRINT_WIZARD_LIST::GetWizard( m_wizardName );

    if( footprintWizard && m_modal_ret_val )
    {
        wxString   msg;
        FOOTPRINT* footprint = footprintWizard->GetFootprint( &msg );
        DisplayBuildMessage( msg );

        return footprint;
    }

    return nullptr;
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

    RegenerateFootprint();
    Zoom_Automatique( false );
    DisplayWizardInfos();
    ReCreatePageList();
    ReCreateParameterList();
}


void FOOTPRINT_WIZARD_FRAME::SelectCurrentWizard( wxCommandEvent& aDummy )
{
    SelectFootprintWizard();
    updateView();
}

void FOOTPRINT_WIZARD_FRAME::DefaultParameters()
{
    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if ( footprintWizard == nullptr )
        return;

    footprintWizard->ResetParameters();

    // Reload
    ReCreateParameterList();
    RegenerateFootprint();
    DisplayWizardInfos();
}


void FOOTPRINT_WIZARD_FRAME::SelectWizardPreviousPage()
{
    int page = m_pageList->GetSelection() - 1;

    if( page < 0 )
        page = 0;

    m_pageList->SetSelection( page, true );

    wxCommandEvent event;
    ClickOnPageList( event );
}

void FOOTPRINT_WIZARD_FRAME::SelectWizardNextPage()
{
    int page = m_pageList->GetSelection() + 1;

    if( (int)m_pageList->GetCount() <= page )
        page = m_pageList->GetCount() - 1;

    m_pageList->SetSelection( page, true );

    wxCommandEvent event;
    ClickOnPageList( event );
}


// This is a flag to avoid reentering of ParametersUpdated
// that can happen in some cases
static bool lock_update_prms = false;


void FOOTPRINT_WIZARD_FRAME::ParametersUpdated( wxGridEvent& event )
{
    FOOTPRINT_WIZARD* footprintWizard = GetMyWizard();

    if( !footprintWizard )
        return;

    if( m_parameterGridPage < 0 )
        return;

    if( lock_update_prms )
        return;

    wxArrayString   prmValues = footprintWizard->GetParameterValues( m_parameterGridPage );
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

        RegenerateFootprint();
        DisplayWizardInfos();

        // The python script can have modified some other parameters.
        // So rebuild the current parameter list with new values, just in case.
        //
        // On wxWidgets 3.0.5, ReCreateParameterList() generates a EVT_GRID_CMD_CELL_CHANGED
        // that call ParametersUpdated() and creating an infinite loop
        // Note also it happens **only for languages using a comma** instead of a point
        // for floating point separator
        // It does not happen on wxWidgets 3.1.4
        //
        // So lock the next call.
        lock_update_prms = true;
        ReCreateParameterList();
    }

    // unlock ParametersUpdated() now the update is finished
    lock_update_prms = false;
}

