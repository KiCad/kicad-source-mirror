/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <pgm_base.h>
#include <confirm.h>
#include <gestfich.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <bitmaps.h>
#include <project.h>            // For PROJECT_VAR_NAME definition
#include <footprint_library_adapter.h>       // For KICAD7_FOOTPRINT_DIR definition

#include <dialog_config_equfiles.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>


DIALOG_CONFIG_EQUFILES::DIALOG_CONFIG_EQUFILES( wxWindow* aParent ) :
    DIALOG_CONFIG_EQUFILES_BASE( aParent )
{
    SetTitle( wxString::Format( _( "Project file: '%s'" ), Prj().GetProjectFullName() ) );

    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_bpEdit->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    PROJECT_FILE& project = Prj().GetProjectFile();

    if( !project.m_EquivalenceFiles.empty() )
    {
        wxArrayString arr;

        for( const auto& entry : project.m_EquivalenceFiles )
            arr.Add( entry );

        m_filesListBox->InsertItems( arr, 0 );
    }

    m_gridEnvVars->ClearRows();
    m_gridEnvVars->AppendRows( 2 );
    m_gridEnvVars->SetCellValue( 0, 0, PROJECT_VAR_NAME );
    m_gridEnvVars->SetCellValue( 1, 0, FOOTPRINT_LIBRARY_ADAPTER::GlobalPathEnvVariableName() );

    for( int row = 0; row < m_gridEnvVars->GetTable()->GetRowsCount(); row++ )
    {
        wxString evValue;

        if( wxGetEnv( m_gridEnvVars->GetCellValue( row, 0 ), &evValue ) )
            m_gridEnvVars->SetCellValue( row, 1, evValue );
    }

    m_gridEnvVars->AutoSizeColumns();

    SetupStandardButtons();

    GetSizer()->SetSizeHints( this );
    Center();
}


void DIALOG_CONFIG_EQUFILES::OnEditEquFile( wxCommandEvent& event )
{
    wxString editorname = Pgm().GetTextEditor();

    if( editorname.IsEmpty() )
    {
        wxMessageBox( _( "No text editor selected in KiCad.  Please choose one." ) );
        return;
    }

    wxArrayInt selections;
    m_filesListBox->GetSelections( selections );

    for( unsigned ii = 0; ii < selections.GetCount(); ii++ )
        ExecuteFile( editorname, wxExpandEnvVars( m_filesListBox->GetString( selections[ii] ) ) );
}


void DIALOG_CONFIG_EQUFILES::OnOkClick( wxCommandEvent& event )
{
    PROJECT_FILE& project = Prj().GetProjectFile();

    // Recreate equ list
    project.m_EquivalenceFiles.clear();

    for( unsigned ii = 0; ii < m_filesListBox->GetCount(); ii++ )
        project.m_EquivalenceFiles.emplace_back( m_filesListBox->GetString( ii ) );

    Pgm().GetSettingsManager().SaveProject();

    EndModal( wxID_OK );
}


void DIALOG_CONFIG_EQUFILES::OnCloseWindow( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_CONFIG_EQUFILES::OnButtonMoveUp( wxCommandEvent& event )
{
    wxArrayInt selections;
    m_filesListBox->GetSelections( selections );

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    if( selections[0] == 0 )            // The first lib is selected. cannot move up it
        return;

    wxArrayString libnames = m_filesListBox->GetStrings();

    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        std::swap( libnames[jj],  libnames[jj-1] );
    }

    m_filesListBox->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_filesListBox->SetSelection( jj-1 );
    }
}


void DIALOG_CONFIG_EQUFILES::OnButtonMoveDown( wxCommandEvent& event )
{
    wxArrayInt selections;
    m_filesListBox->GetSelections( selections );

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    // The last lib is selected. cannot move down it
    if( selections.Last() == int( m_filesListBox->GetCount()-1 ) )
        return;

    wxArrayString libnames = m_filesListBox->GetStrings();

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        int jj = selections[ii];
        std::swap( libnames[jj],  libnames[jj+1] );
    }

    m_filesListBox->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_filesListBox->SetSelection( jj+1 );
    }
}


/* Remove a library to the library list.
 * The real list (g_LibName_List) is not changed, so the change can be canceled
 */
void DIALOG_CONFIG_EQUFILES::OnRemoveFiles( wxCommandEvent& event )
{
    wxArrayInt selections;
    m_filesListBox->GetSelections( selections );

    if( !selections.empty() )
    {
        std::sort( selections.begin(), selections.end() );

        for( int ii = (int) selections.GetCount() - 1; ii >= 0; ii-- )
            m_filesListBox->Delete( selections[ii] );
    }
}


void DIALOG_CONFIG_EQUFILES::OnAddFiles( wxCommandEvent& event )
{
    // Get a default path to open the file dialog:
    wxArrayInt selectedRows	= m_gridEnvVars->GetSelectedRows();
    int        row = selectedRows.GetCount() ? selectedRows[0] : m_gridEnvVars->GetGridCursorRow();
    wxString   libpath = m_gridEnvVars->GetCellValue( wxGridCellCoords( row, 1 ) );

    wxFileDialog dlg( this, _( "Footprint Association File" ), libpath, wxEmptyString,
                      FILEEXT::EquFileWildcard(), wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxArrayString filenames;
    dlg.GetPaths( filenames );

    for( unsigned jj = 0; jj < filenames.GetCount(); jj++ )
    {
        wxFileName fn = filenames[jj];
        wxString   filepath;

        for( row = 0; row < m_gridEnvVars->GetTable()->GetRowsCount(); row++ )
        {
            libpath = m_gridEnvVars->GetCellValue( wxGridCellCoords( row, 1 ) );

            if( fn.MakeRelativeTo( libpath ) )
            {
                filepath.Printf( wxT( "${%s}%c%s" ),
                                    m_gridEnvVars->GetCellValue( wxGridCellCoords( row, 0 ) ),
                                    wxFileName::GetPathSeparator(),
                                    fn.GetFullPath() );
                break;
            }
        }

        if( filepath.IsEmpty() )
            filepath = filenames[jj];

        // Add or insert new library name, if not already in list
        if( m_filesListBox->FindString( filepath, wxFileName::IsCaseSensitive() ) == wxNOT_FOUND )
        {
            filepath.Replace( wxT( "\\" ), wxT( "/" ) );     // Use unix separators only.
            m_filesListBox->Append( filepath );
        }
        else
        {
            DisplayErrorMessage( this, wxString::Format( _( "File '%s' already exists in list." ),
                                                         filepath.GetData() ) );
        }
    }
}
