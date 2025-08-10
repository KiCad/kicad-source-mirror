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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/grid.h>

#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <dialog_footprint_wizard_list.h>
#include <footprint_wizard_frame.h>

#include <python/scripting/pcbnew_scripting.h>


enum FPGeneratorRowNames
{
    FP_GEN_ROW_NUMBER = 0,
    FP_GEN_ROW_NAME,
    FP_GEN_ROW_DESCR,
};


DIALOG_FOOTPRINT_WIZARD_LIST::DIALOG_FOOTPRINT_WIZARD_LIST( wxWindow* aParent ) :
        DIALOG_FOOTPRINT_WIZARD_LIST_BASE( aParent )
{
    OptOut( this );
    initLists();

    SetupStandardButtons();
    finishDialogSettings();

    Center();
}


void DIALOG_FOOTPRINT_WIZARD_LIST::initLists()
{
    // Current wizard selection, empty or first
    m_footprintWizard = NULL;

    int n_wizards = FOOTPRINT_WIZARD_LIST::GetWizardsCount();

    if( n_wizards )
        m_footprintWizard = FOOTPRINT_WIZARD_LIST::GetWizard( 0 );

    // Choose selection mode and insert the needed rows

    m_footprintGeneratorsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    int curr_row_cnt = m_footprintGeneratorsGrid->GetNumberRows();

    if( curr_row_cnt )
        m_footprintGeneratorsGrid->DeleteRows( 0, curr_row_cnt );

    if( n_wizards )
        m_footprintGeneratorsGrid->InsertRows( 0, n_wizards );

    // Put all wizards in the list
    for( int ii = 0; ii < n_wizards; ii++ )
    {
        wxString num = wxString::Format( wxT( "%d" ), ii+1 );
        FOOTPRINT_WIZARD *wizard = FOOTPRINT_WIZARD_LIST::GetWizard( ii );
        wxString name = wizard->GetName();
        wxString description = wizard->GetDescription();

        m_footprintGeneratorsGrid->SetCellValue( ii, FP_GEN_ROW_NUMBER, num );
        m_footprintGeneratorsGrid->SetCellValue( ii, FP_GEN_ROW_NAME, name );
        m_footprintGeneratorsGrid->SetCellValue( ii, FP_GEN_ROW_DESCR, description );
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

    // Display info about scripts: Search paths
    wxString message;
    pcbnewGetScriptsSearchPaths( message );
    m_tcSearchPaths->SetValue( message );

    // Display info about scripts: unloadable scripts (due to syntax errors is python source).
    pcbnewGetUnloadableScriptNames( message );

    if( message.IsEmpty() )
    {
        m_tcNotLoaded->SetValue( _( "All footprint generator scripts were loaded" ) );
        m_buttonShowTrace->Show( false );
    }
    else
        m_tcNotLoaded->SetValue( message );
}


void DIALOG_FOOTPRINT_WIZARD_LIST::onUpdatePythonModulesClick( wxCommandEvent& event )
{
    FOOTPRINT_WIZARD_FRAME* fpw_frame = static_cast<FOOTPRINT_WIZARD_FRAME*>( GetParent() );
    fpw_frame->PythonPluginsReload();

    initLists();
}


void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellFpGeneratorClick( wxGridEvent& event )
{
    int click_row = event.GetRow();
    m_footprintWizard = FOOTPRINT_WIZARD_LIST::GetWizard( click_row );
    m_footprintGeneratorsGrid->SelectRow( event.GetRow(), false );

    // Move the grid cursor to the active line, mainly for aesthetic reasons:
    m_footprintGeneratorsGrid->GoToCell( event.GetRow(), FP_GEN_ROW_NUMBER );
}


void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellFpGeneratorDoubleClick( wxGridEvent& event )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


void DIALOG_FOOTPRINT_WIZARD_LIST::onShowTrace( wxCommandEvent& event )
{
    wxString trace;
    pcbnewGetWizardsBackTrace( trace );

    // Now display the filtered trace in our dialog
    // (a simple wxMessageBox is really not suitable for long messages)
    DIALOG_FOOTPRINT_WIZARD_LOG logWindow( this );
    logWindow.m_Message->SetValue( trace );
    logWindow.ShowModal();
}


FOOTPRINT_WIZARD* DIALOG_FOOTPRINT_WIZARD_LIST::GetWizard()
{
    return m_footprintWizard;
}
