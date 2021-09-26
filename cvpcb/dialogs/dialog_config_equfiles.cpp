/**
 * @file dialog_config_equfiles.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <id.h>
#include <project.h>            // For PROJECT_VAR_NAME definition
#include <fp_lib_table.h>       // For KICAD6_FOOTPRINT_DIR definition

#include <cvpcb_mainframe.h>
#include <dialog_config_equfiles.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <wx/filedlg.h>


DIALOG_CONFIG_EQUFILES::DIALOG_CONFIG_EQUFILES( CVPCB_MAINFRAME* aParent ) :
    DIALOG_CONFIG_EQUFILES_BASE( aParent )
{
    m_Parent = aParent;

    PROJECT& prj = Prj();
    SetTitle( wxString::Format( _( "Project file: '%s'" ), prj.GetProjectFullName() ) );

    Init( );

    GetSizer()->SetSizeHints( this );
    Center();
}


void DIALOG_CONFIG_EQUFILES::Init()
{
    m_sdbSizerOK->SetDefault();
    m_ListChanged = false;

    PROJECT_FILE& project = Prj().GetProjectFile();

    if( !project.m_EquivalenceFiles.empty() )
    {
        wxArrayString arr;

        for( const auto& entry : project.m_EquivalenceFiles )
            arr.Add( entry );

        m_ListEquiv->InsertItems( arr, 0 );
    }

    if( getEnvVarCount() < 2 )
        m_gridEnvVars->AppendRows(2 - getEnvVarCount() );

    wxString evValue;
    int row = 0;

    m_gridEnvVars->SetCellValue( row++, 0, PROJECT_VAR_NAME );
    m_gridEnvVars->SetCellValue( row, 0, FP_LIB_TABLE::GlobalPathEnvVariableName() );

    for( row = 0; row < getEnvVarCount(); row++ )
    {
        if( wxGetEnv( m_gridEnvVars->GetCellValue( row, 0 ), &evValue ) )
            m_gridEnvVars->SetCellValue( row, 1, evValue );
    }

    m_gridEnvVars->AutoSizeColumns();

}


void DIALOG_CONFIG_EQUFILES::OnEditEquFile( wxCommandEvent& event )
{
    wxString    editorname = Pgm().GetTextEditor();

    if( editorname.IsEmpty() )
    {
        wxMessageBox( _( "No text editor selected in KiCad.  Please choose one." ) );
        return;
    }

    wxArrayInt selections;
    m_ListEquiv->GetSelections( selections );

    for( unsigned ii = 0; ii < selections.GetCount(); ii++ )
    {
        ExecuteFile( editorname, wxExpandEnvVars( m_ListEquiv->GetString( selections[ii] ) ) );
        m_ListChanged = true;
    }
}


void DIALOG_CONFIG_EQUFILES::OnOkClick( wxCommandEvent& event )
{
    // Save new equ file list if the files list was modified
    if( m_ListChanged  )
    {
        PROJECT_FILE& project = Prj().GetProjectFile();

        // Recreate equ list
        project.m_EquivalenceFiles.clear();

        for( unsigned ii = 0; ii < m_ListEquiv->GetCount(); ii++ )
            project.m_EquivalenceFiles.emplace_back( m_ListEquiv->GetString( ii ) );

        Pgm().GetSettingsManager().SaveProject();
    }

    EndModal( wxID_OK );
}


void DIALOG_CONFIG_EQUFILES::OnCloseWindow( wxCloseEvent& event )
{
    EndModal( wxID_CANCEL );
}


void DIALOG_CONFIG_EQUFILES::OnButtonMoveUp( wxCommandEvent& event )
{
    wxArrayInt selections;

    m_ListEquiv->GetSelections( selections );

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    if( selections[0] == 0 )            // The first lib is selected. cannot move up it
        return;

    wxArrayString libnames = m_ListEquiv->GetStrings();

    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        std::swap( libnames[jj],  libnames[jj-1] );
    }

    m_ListEquiv->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_ListEquiv->SetSelection( jj-1 );
    }

    m_ListChanged = true;
}


void DIALOG_CONFIG_EQUFILES::OnButtonMoveDown( wxCommandEvent& event )
{
    wxArrayInt selections;
    m_ListEquiv->GetSelections( selections );

    if ( selections.GetCount() <= 0 )   // No selection.
        return;

    // The last lib is selected. cannot move down it
    if( selections.Last() == int( m_ListEquiv->GetCount()-1 ) )
        return;

    wxArrayString libnames = m_ListEquiv->GetStrings();

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        int jj = selections[ii];
        std::swap( libnames[jj],  libnames[jj+1] );
    }

    m_ListEquiv->Set( libnames );

    // Reselect previously selected names
    for( size_t ii = 0; ii < selections.GetCount(); ii++ )
    {
        int jj = selections[ii];
        m_ListEquiv->SetSelection( jj+1 );
    }

    m_ListChanged = true;
}


/* Remove a library to the library list.
 * The real list (g_LibName_List) is not changed, so the change can be canceled
 */
void DIALOG_CONFIG_EQUFILES::OnRemoveFiles( wxCommandEvent& event )
{
    wxArrayInt selections;
    m_ListEquiv->GetSelections( selections );

    std::sort( selections.begin(), selections.end() );

    for( int ii = selections.GetCount()-1; ii >= 0; ii-- )
    {
        m_ListEquiv->Delete( selections[ii] );
        m_ListChanged = true;
    }
}


void DIALOG_CONFIG_EQUFILES::OnAddFiles( wxCommandEvent& event )
{
    wxString   equFilename;
    wxFileName fn;

    wxListBox* list = m_ListEquiv;

    // Get a default path to open the file dialog:
    wxString libpath;
    wxArrayInt selectedRows	= m_gridEnvVars->GetSelectedRows();

    int row = selectedRows.GetCount() ? selectedRows[0] :
                m_gridEnvVars->GetGridCursorRow();

    libpath = m_gridEnvVars->GetCellValue( wxGridCellCoords( row, 1 ) );

    wxFileDialog FilesDialog( this, _( "Footprint Association File" ), libpath,
                              wxEmptyString, EquFileWildcard(),
                              wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    wxArrayString Filenames;
    FilesDialog.GetPaths( Filenames );

    for( unsigned jj = 0; jj < Filenames.GetCount(); jj++ )
    {
        fn = Filenames[jj];
        equFilename.Empty();

        if( isPathRelativeAllowed() ) // try to use relative path
        {
            for( row = 0; row < getEnvVarCount(); row++ )
            {
                libpath = m_gridEnvVars->GetCellValue( wxGridCellCoords( row, 1 ) );

                if( fn.MakeRelativeTo( libpath ) )
                {
                    equFilename.Printf( wxT( "${%s}%c%s" ),
                            m_gridEnvVars->GetCellValue( wxGridCellCoords( row, 0 ) ),
                            fn.GetPathSeparator(), fn.GetFullPath() );
                    break;
                }
            }
        }

        if( equFilename.IsEmpty() )
            equFilename = Filenames[jj];

        // Add or insert new library name, if not already in list
        if( list->FindString( equFilename, fn.IsCaseSensitive() ) == wxNOT_FOUND )
        {
            m_ListChanged = true;
            equFilename.Replace( wxT( "\\" ), wxT( "/" ) );     // Use unix separators only.
            list->Append( equFilename );
        }
        else
        {
            wxString msg;
            msg.Printf( _( "File '%s' already exists in list." ), equFilename.GetData() );
            DisplayError( this, msg );
        }
    }
}
