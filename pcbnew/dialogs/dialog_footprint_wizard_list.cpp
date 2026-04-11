/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Miguel Angel Ajo <miguelangel@nbee.es>
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

#include <wx/grid.h>

#include <api/api_plugin_manager.h>
#include <pgm_base.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <dialog_footprint_wizard_list.h>
#include <footprint_wizard_frame.h>


enum FPGeneratorRowNames
{
    FP_GEN_ROW_NAME = 0,
    FP_GEN_ROW_DESCR,
};


DIALOG_FOOTPRINT_WIZARD_LIST::DIALOG_FOOTPRINT_WIZARD_LIST( FOOTPRINT_WIZARD_FRAME* aParent ) :
        DIALOG_FOOTPRINT_WIZARD_LIST_BASE( aParent )
{
    OptOut( this );
    initLists();

    SetupStandardButtons();
    finishDialogSettings();

    Center();
}


FOOTPRINT_WIZARD_FRAME* DIALOG_FOOTPRINT_WIZARD_LIST::ParentFrame()
{
    return static_cast<FOOTPRINT_WIZARD_FRAME*>( m_parent );
}


void DIALOG_FOOTPRINT_WIZARD_LIST::initLists()
{
    m_selectedWizard = wxEmptyString;
    m_footprintGeneratorsGrid->ClearGrid();

    FOOTPRINT_WIZARD_MANAGER* manager = ParentFrame()->Manager();

    manager->ReloadWizards();
    std::vector<FOOTPRINT_WIZARD*> wizards = manager->Wizards();

    m_footprintGeneratorsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    if( !wizards.empty() )
    {
        m_selectedWizard = wizards[0]->Identifier();
        m_footprintGeneratorsGrid->InsertRows( 0, wizards.size() );
    }

    int idx = 0;

    for( FOOTPRINT_WIZARD* wizard : wizards )
    {
        wxString name = wizard->Info().meta.name;
        wxString description = wizard->Info().meta.description;

        m_footprintGeneratorsGrid->SetCellValue( idx, FP_GEN_ROW_NAME, name );
        m_footprintGeneratorsGrid->SetCellValue( idx++, FP_GEN_ROW_DESCR, description );
    }

    m_footprintGeneratorsGrid->AutoSizeColumns();

    // Auto-expand the description column
    int width = m_footprintGeneratorsGrid->GetClientSize().GetWidth()
                    - m_footprintGeneratorsGrid->GetRowLabelSize()
                    - m_footprintGeneratorsGrid->GetColSize( FP_GEN_ROW_NAME );

    if ( width > m_footprintGeneratorsGrid->GetColMinimalAcceptableWidth() )
        m_footprintGeneratorsGrid->SetColSize( FP_GEN_ROW_DESCR, width );

    // Select the first row if it exists
    m_footprintGeneratorsGrid->ClearSelection();

    if( m_footprintGeneratorsGrid->GetNumberRows() > 0 )
        m_footprintGeneratorsGrid->SelectRow( 0, false );
}


void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellFpGeneratorClick( wxGridEvent& event )
{
    int click_row = event.GetRow();

    FOOTPRINT_WIZARD_MANAGER* manager = ParentFrame()->Manager();
    std::vector<FOOTPRINT_WIZARD*> wizards = manager->Wizards();

    if( click_row >= 0 && click_row < static_cast<int>( wizards.size() ) )
        m_selectedWizard = wizards[click_row]->Identifier();

    m_footprintGeneratorsGrid->SelectRow( event.GetRow(), false );

    // Move the grid cursor to the active line, mainly for aesthetic reasons:
    m_footprintGeneratorsGrid->GoToCell( event.GetRow(), FP_GEN_ROW_NAME );
}


void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellFpGeneratorDoubleClick( wxGridEvent& event )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


const wxString& DIALOG_FOOTPRINT_WIZARD_LIST::GetWizard()
{
    return m_selectedWizard;
}
