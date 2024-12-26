/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <bitmaps.h>
#include <dialogs/panel_embedded_files.h>
#include <embedded_files.h>
#include <kidialog.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>

#include <wx/clipbrd.h>
#include <wx/dirdlg.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/menu.h>

PANEL_EMBEDDED_FILES::PANEL_EMBEDDED_FILES( wxWindow* parent, EMBEDDED_FILES* aFiles ) :
    PANEL_EMBEDDED_FILES_BASE( parent ),
    m_files( aFiles )
{
    m_localFiles = new EMBEDDED_FILES();

    for( auto& [name, file] : m_files->EmbeddedFileMap() )
    {
        EMBEDDED_FILES::EMBEDDED_FILE* newFile = new EMBEDDED_FILES::EMBEDDED_FILE( *file );
        m_localFiles->AddFile( newFile );
    }

    // Set up the standard buttons
    m_delete_button->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_browse_button->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_files_grid->SetMargins( 0 - wxSYS_VSCROLL_X, 0 );
    m_files_grid->EnableAlternateRowColors();
}


void PANEL_EMBEDDED_FILES::onSize( wxSizeEvent& event )
{
    resizeGrid();
}


void PANEL_EMBEDDED_FILES::resizeGrid()
{
    int panel_width = GetClientRect().GetWidth();
    int first_width = m_files_grid->GetColSize( 0 );
    int second_width = m_files_grid->GetColSize( 1 );

    double ratio;

    if( first_width + second_width > 0 )
        ratio = (double)first_width / (double)( first_width + second_width );
    else
        ratio = 0.3;


    m_files_grid->SetColSize( 0, panel_width * ratio );
    m_files_grid->SetColSize( 1, panel_width * ( 1 - ratio ) );
    Layout();
}


void PANEL_EMBEDDED_FILES::onGridRightClick( wxGridEvent& event )
{
    wxMenu menu;
    const wxWindowID id = NewControlId();
    menu.Append( id, _( "Copy Embedded Reference" ) );

    menu.Bind(
            wxEVT_COMMAND_MENU_SELECTED,
            [&]( wxCommandEvent& )
            {
                int row = event.GetRow();
                if( row >= 0 && row < m_files_grid->GetNumberRows() )
                {
                    wxString cellValue = m_files_grid->GetCellValue( row, 1 );

                    if( wxTheClipboard->Open() )
                    {
                        wxTheClipboard->SetData( new wxTextDataObject( cellValue ) );
                        wxTheClipboard->Close();
                    }
                }
            },
            id );

    PopupMenu( &menu );
}


bool PANEL_EMBEDDED_FILES::TransferDataToWindow()
{
    m_files_grid->ClearGrid();

    if( m_files_grid->GetNumberRows() > 0 )
        m_files_grid->DeleteRows( 0, m_files_grid->GetNumberRows() );

    int ii = 0;
    for( auto& [name, file] : m_localFiles->EmbeddedFileMap() )
    {
        while( m_files_grid->GetNumberRows() < ii + 1 )
            m_files_grid->AppendRows( 1 );

        m_files_grid->SetCellValue( ii, 0, name );
        m_files_grid->SetCellValue( ii, 1, file->GetLink() );

        ii++;
    }

    m_cbEmbedFonts->SetValue( m_files->GetAreFontsEmbedded() );

    resizeGrid();

    return true;
}


bool PANEL_EMBEDDED_FILES::TransferDataFromWindow()
{
    m_files->ClearEmbeddedFiles();

    std::vector<EMBEDDED_FILES::EMBEDDED_FILE*> files;

    for( auto it = m_localFiles->EmbeddedFileMap().begin(); it != m_localFiles->EmbeddedFileMap().end(); it++ )
        files.push_back( it->second );

    for( auto& file : files )
    {
        m_files->AddFile( file );
        m_localFiles->RemoveFile( file->name, false );
    }

    m_files->SetAreFontsEmbedded( m_cbEmbedFonts->IsChecked() );

    return true;
}


void PANEL_EMBEDDED_FILES::onAddEmbeddedFile( wxCommandEvent& event )
{
    wxFileDialog fileDialog( this, _( "Select a file to embed" ), wxEmptyString, wxEmptyString,
                             _( "All files|*.*" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( fileDialog.ShowModal() == wxID_OK )
    {
        wxFileName fileName( fileDialog.GetPath() );
        wxString name = fileName.GetFullName();

        if( m_localFiles->HasFile( name ) )
        {
            wxString msg = wxString::Format( _( "File '%s' already exists." ),
                        name );

            KIDIALOG errorDlg( m_parent, msg, _( "Confirmation" ),
                                wxOK | wxCANCEL | wxICON_WARNING );
            errorDlg.SetOKLabel( _( "Overwrite" ) );

            if( errorDlg.ShowModal() != wxID_OK )
                return;

            for( int ii = 0; ii < m_files_grid->GetNumberRows(); ii++ )
            {
                if( m_files_grid->GetCellValue( ii, 0 ) == name )
                {
                    m_files_grid->DeleteRows( ii );
                    break;
                }
            }
        }

        EMBEDDED_FILES::EMBEDDED_FILE* result = m_localFiles->AddFile( fileName, true );

        if( !result )
        {
            wxString msg = wxString::Format( _( "Failed to add file '%s'." ),
                        name );

            KIDIALOG errorDlg( m_parent, msg, _( "Error" ), wxOK | wxICON_ERROR );
            errorDlg.ShowModal();
            return;
        }

        m_files_grid->AppendRows( 1 );
        int ii = m_files_grid->GetNumberRows() - 1;
        m_files_grid->SetCellValue( ii, 0, name );
        m_files_grid->SetCellValue( ii, 1, result->GetLink() );
    }
}

void PANEL_EMBEDDED_FILES::onDeleteEmbeddedFile( wxCommandEvent& event )
{
    int row = m_files_grid->GetGridCursorRow();

    if( row < 0 )
        return;

    wxString name = m_files_grid->GetCellValue( row, 0 );

    m_localFiles->RemoveFile( name );

    m_files_grid->DeleteRows( row );

    if( row < m_files_grid->GetNumberRows() )
        m_files_grid->SetGridCursor( row, 0 );
    else if( m_files_grid->GetNumberRows() > 0 )
        m_files_grid->SetGridCursor( m_files_grid->GetNumberRows() - 1, 0 );
}


void PANEL_EMBEDDED_FILES::onExportFiles( wxCommandEvent& event )
{
    wxDirDialog dirDialog( this, _( "Select a directory to export files" ) );

    if( dirDialog.ShowModal() != wxID_OK )
        return;

    wxString path = dirDialog.GetPath();

    for( auto& [name, file] : m_localFiles->EmbeddedFileMap() )
    {
        wxFileName fileName( path, name );

        if( fileName.FileExists() )
        {
            wxString msg = wxString::Format( _( "File '%s' already exists." ),
                        fileName.GetFullName() );

            KIDIALOG errorDlg( m_parent, msg, _( "Confirmation" ),
                                wxOK | wxCANCEL | wxICON_WARNING );
            errorDlg.SetOKCancelLabels( _( "Overwrite" ), _( "Skip" ) );
            errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( errorDlg.ShowModal() != wxID_OK )
                continue;
        }

        bool skip_file = false;

        while( 1 )
        {
            if( !fileName.IsDirWritable() )
            {
#ifndef __WXMAC__
                wxString msg = wxString::Format( _( "Directory '%s' is not writable." ),
                                                    fileName.GetFullName() );
#else
                wxString msg = wxString::Format( _( "Folder '%s' is not writable." ),
                                                    fileName.GetPath() );
#endif
                // Don't set a 'do not show again' checkbox for this dialog
                KIDIALOG errorDlg( m_parent, msg, _( "Error" ), wxYES_NO | wxCANCEL | wxICON_ERROR );
                errorDlg.SetYesNoCancelLabels( _( "Retry" ), _( "Skip" ), _( "Cancel" ) );

                int result = errorDlg.ShowModal();

                if( result == wxID_CANCEL )
                {
                    return;
                }
                else if( result == wxID_NO )
                {
                    skip_file = true;
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if( skip_file )
            continue;

        wxFFile ffile( fileName.GetFullPath(), wxT( "w" ) );

        if( !ffile.IsOpened() )
        {
            wxString msg = wxString::Format( _( "Failed to open file '%s'." ),
                        fileName.GetFullName() );

            KIDIALOG errorDlg( m_parent, msg, _( "Error" ), wxOK | wxICON_ERROR );
            errorDlg.ShowModal();
            continue;
        }

        if( !ffile.Write( file->decompressedData.data(), file->decompressedData.size() ) )
        {
            wxString msg = wxString::Format( _( "Failed to write file '%s'." ),
                        fileName.GetFullName() );

            KIDIALOG errorDlg( m_parent, msg, _( "Error" ), wxOK | wxICON_ERROR );
            errorDlg.ShowModal();
        }
    }
}