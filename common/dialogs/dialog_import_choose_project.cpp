/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialog_import_choose_project.h>

#include <wx/msgdlg.h>
#include <wx/listctrl.h>


DIALOG_IMPORT_CHOOSE_PROJECT::DIALOG_IMPORT_CHOOSE_PROJECT( wxWindow* aParent,
                                                            const std::vector<IMPORT_PROJECT_DESC>& aProjectDesc ) :
        DIALOG_IMPORT_CHOOSE_PROJECT_BASE( aParent )
{
    m_project_desc = aProjectDesc;

    // Initialize columns in the wxListCtrl elements:
    int comboNameColId = m_listCtrl->AppendColumn( _( "Project Name" ) );
    int pcbNameColId = m_listCtrl->AppendColumn( _( "PCB" ) );
    int schNameColId = m_listCtrl->AppendColumn( _( "Schematic" ) );

    // Load the project/PCB/schematic names
    int row = 0;

    auto convertName =
            []( const wxString& aName, const wxString& aId ) -> wxString
            {
                if( aId.empty() )
                    return wxEmptyString;

                return aName;
            };

    for( const IMPORT_PROJECT_DESC& desc : m_project_desc )
    {
        m_listCtrl->InsertItem( row, convertName( desc.ComboName, desc.ComboId ) );
        m_listCtrl->SetItem( row, pcbNameColId, convertName( desc.PCBName, desc.PCBId ) );
        m_listCtrl->SetItem( row, schNameColId, convertName( desc.SchematicName,
                                                             desc.SchematicId ) );

        ++row;
    }

    // Auto select the first item to improve ease-of-use
    m_listCtrl->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

    m_listCtrl->SetColumnWidth( comboNameColId, wxLIST_AUTOSIZE_USEHEADER );
    m_listCtrl->SetColumnWidth( pcbNameColId, wxLIST_AUTOSIZE_USEHEADER );
    m_listCtrl->SetColumnWidth( schNameColId, wxLIST_AUTOSIZE_USEHEADER );

    SetupStandardButtons();

    Fit();
    finishDialogSettings();
}


void DIALOG_IMPORT_CHOOSE_PROJECT::onItemActivated( wxListEvent& event )
{
    EndModal( wxID_OK );
}


void DIALOG_IMPORT_CHOOSE_PROJECT::onClose( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


std::vector<IMPORT_PROJECT_DESC> DIALOG_IMPORT_CHOOSE_PROJECT::GetProjects()
{
    std::vector<IMPORT_PROJECT_DESC> result;

    long selected = -1;

    do
    {
        selected = m_listCtrl->GetNextItem( selected, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

        if( selected != -1 && selected < long( m_project_desc.size() ) )
            result.emplace_back( m_project_desc[selected] );

    } while( selected != -1 );

    return result;
}


std::vector<IMPORT_PROJECT_DESC>
DIALOG_IMPORT_CHOOSE_PROJECT::RunModal( wxWindow* aParent, const std::vector<IMPORT_PROJECT_DESC>& aProjectDesc )
{
    DIALOG_IMPORT_CHOOSE_PROJECT dlg( aParent, aProjectDesc );

    if( dlg.ShowModal() != wxID_OK )
        return {};

    return dlg.GetProjects();
}
