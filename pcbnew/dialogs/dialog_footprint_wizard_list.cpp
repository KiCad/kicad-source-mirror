/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_footprint_wizard_list.cpp
 */

#include <wx/grid.h>

#include <fctsys.h>
#include <pcbnew.h>
#include <kiface_i.h>
#include <dialog_footprint_wizard_list.h>
#include <class_footprint_wizard.h>
#include <footprint_wizard_frame.h>

#if defined(KICAD_SCRIPTING) || defined(KICAD_SCRIPTING_WXPYTHON)
#include <python_scripting.h>
#else
// Dummy functions, actually defined in python_scripting.h when KICAD_SCRIPTING is enabled
static void pcbnewGetWizardsBackTrace( wxString& aText ) {};
static void pcbnewGetScriptsSearchPaths( wxString& aText ) {};
static void pcbnewGetUnloadableScriptNames( wxString& aText ) {};
#endif

enum FPGeneratorRowNames
{
    FP_GEN_ROW_NUMBER = 0,
    FP_GEN_ROW_NAME,
    FP_GEN_ROW_DESCR,
};

#define FPWIZARTDLIST_HEIGHT_KEY wxT( "FpWizardListHeight" )
#define FPWIZARTDLIST_WIDTH_KEY  wxT( "FpWizardListWidth" )

DIALOG_FOOTPRINT_WIZARD_LIST::DIALOG_FOOTPRINT_WIZARD_LIST( wxWindow* aParent )
    : DIALOG_FOOTPRINT_WIZARD_LIST_BASE( aParent )
{
    m_config = Kiface().KifaceSettings();
    initLists();

    if( m_config )
    {
        wxSize size;
        m_config->Read( FPWIZARTDLIST_WIDTH_KEY, &size.x, -1 );
        m_config->Read( FPWIZARTDLIST_HEIGHT_KEY, &size.y, -1 );
        SetSize( size );
    }


    m_sdbSizerOK->SetDefault();
    FinishDialogSettings();

    Center();
}


DIALOG_FOOTPRINT_WIZARD_LIST::~DIALOG_FOOTPRINT_WIZARD_LIST()
{
    if( m_config && !IsIconized() )
    {
        m_config->Write( FPWIZARTDLIST_WIDTH_KEY, GetSize().x );
        m_config->Write( FPWIZARTDLIST_HEIGHT_KEY, GetSize().y );
    }
}


void DIALOG_FOOTPRINT_WIZARD_LIST::initLists()
{
    // Current wizard selection, empty or first
    m_footprintWizard = NULL;

    int n_wizards = FOOTPRINT_WIZARDS::GetWizardsCount();

    if( n_wizards )
        m_footprintWizard = FOOTPRINT_WIZARDS::GetWizard( 0 );

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
        wxString num = wxString::Format( "%d", ii+1 );
        FOOTPRINT_WIZARD *wizard = FOOTPRINT_WIZARDS::GetWizard( ii );
        wxString name = wizard->GetName();
        wxString description = wizard->GetDescription();
        wxString image = wizard->GetImage();

        m_footprintGeneratorsGrid->SetCellValue( ii, FP_GEN_ROW_NUMBER, num );
        m_footprintGeneratorsGrid->SetCellValue( ii, FP_GEN_ROW_NAME, name );
        m_footprintGeneratorsGrid->SetCellValue( ii, FP_GEN_ROW_DESCR, description );

    }

    m_footprintGeneratorsGrid->AutoSizeColumns();

    // Auto-expand the description column
    int width = m_footprintGeneratorsGrid->GetClientSize().GetWidth() -
                m_footprintGeneratorsGrid->GetRowLabelSize() -
                m_footprintGeneratorsGrid->GetColSize( FP_GEN_ROW_NAME );

    if ( width > m_footprintGeneratorsGrid->GetColMinimalAcceptableWidth() )
        m_footprintGeneratorsGrid->SetColSize( FP_GEN_ROW_DESCR, width );

    // Select the first row
    m_footprintGeneratorsGrid->ClearSelection();
    m_footprintGeneratorsGrid->SelectRow( 0, false );

    // Display info about scripts: Search paths
    wxString message;
    pcbnewGetScriptsSearchPaths( message );
    m_tcSearchPaths->SetValue( message );
    // Display info about scripts: unloadable scripts (due to syntax errors is python source)
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
#if defined(KICAD_SCRIPTING)
    FOOTPRINT_WIZARD_FRAME* fpw_frame = static_cast<FOOTPRINT_WIZARD_FRAME*>( GetParent() );
    fpw_frame->PythonPluginsReload();

    initLists();
#endif
}


void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellFpGeneratorClick( wxGridEvent& event )
{
    int click_row = event.GetRow();
    m_footprintWizard = FOOTPRINT_WIZARDS::GetWizard( click_row );
    m_footprintGeneratorsGrid->SelectRow( event.GetRow(), false );
    // Move the grid cursor to the active line, mainly for aesthetic reasons:
    m_footprintGeneratorsGrid->GoToCell( event.GetRow(), FP_GEN_ROW_NUMBER );
}


void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellFpGeneratorDoubleClick( wxGridEvent& event )
{
    EndModal( wxID_OK );
}

void DIALOG_FOOTPRINT_WIZARD_LIST::onShowTrace( wxCommandEvent& event )
{
    wxString trace;
    pcbnewGetWizardsBackTrace( trace );

    // Filter message before displaying them
    // a trace starts by "Traceback" and is followed by 2 useless lines
    // for our purpose
    wxArrayString traces;
    wxStringSplit( trace, traces, '\n' );

    // Build the filtered message (remove useless lines)
    trace.Clear();

    for( unsigned ii = 0; ii < traces.Count(); ++ii )
    {
        if( traces[ii].Contains( "Traceback" ) )
        {
            ii += 2;    // Skip this line and next lines which are related to pcbnew.py module

            if( !trace.IsEmpty() )  // Add separator for the next trace block
                trace << "\n**********************************\n";
        }
        else
            trace += traces[ii] + "\n";
    }

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
