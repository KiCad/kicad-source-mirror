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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <pcbnew_id.h>
#include <wildcards_and_files_ext.h>
#include <dialogs/dialog_footprint_wizard_list.h>
#include <wx/msgdlg.h>
#include <tool/tool_manager.h>
#include "footprint_wizard_frame.h"


/* Displays the name of the current opened library in the caption */
void FOOTPRINT_WIZARD_FRAME::DisplayWizardInfos()
{
    wxString msg;

    msg = _( "Footprint Wizard" );
    msg << wxT( " [" );

    wxString wizardName = m_currentWizard ? m_currentWizard->Info().meta.name : _( "no wizard selected" );
    msg << wizardName;

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
    tl::expected<FOOTPRINT*, wxString> result = Manager()->Generate( footprintWizard );

    if( result )
    {
        m_buildMessageBox->SetValue( footprintWizard->Info().meta.description );
        //  Add the object to board
        GetBoard()->Add( *result, ADD_MODE::APPEND );
        ( *result )->SetPosition( VECTOR2I( 0, 0 ) );
    }
    else
    {
        m_buildMessageBox->SetValue( result.error() );
    }

    updateView();
    GetCanvas()->Refresh();
}


FOOTPRINT_WIZARD* FOOTPRINT_WIZARD_FRAME::GetMyWizard()
{
    return m_currentWizard;
}


FOOTPRINT* FOOTPRINT_WIZARD_FRAME::GetBuiltFootprint()
{
    // TODO(JE) should this be cached?
    tl::expected<FOOTPRINT*, wxString> result = Manager()->Generate( m_currentWizard );
    return result.value_or( nullptr );
}


void FOOTPRINT_WIZARD_FRAME::SelectFootprintWizard()
{
    DIALOG_FOOTPRINT_WIZARD_LIST wizardSelector( this );

    if( wizardSelector.ShowModal() != wxID_OK )
        return;

    m_currentWizard = nullptr;
    wxString wizardIdentifier = wizardSelector.GetWizard();

    if( !wizardIdentifier.IsEmpty() )
    {
        if( std::optional<FOOTPRINT_WIZARD*> wizard = Manager()->GetWizard( wizardIdentifier ) )
            m_currentWizard = *wizard;
    }

    RegenerateFootprint();
    Zoom_Automatique( false );
    DisplayWizardInfos();
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


void FOOTPRINT_WIZARD_FRAME::RebuildWizardParameters()
{
    ReCreateParameterList();
}


void FOOTPRINT_WIZARD_FRAME::OnWizardParametersChanged()
{
    RegenerateFootprint();
    DisplayWizardInfos();
}

