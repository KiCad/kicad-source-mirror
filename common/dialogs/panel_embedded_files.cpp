/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <font/outline_font.h>
#include <kidialog.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>

#include <wx/clipbrd.h>
#include <wx/dirdlg.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/wfstream.h>

/* ---------- GRID_TRICKS for embedded files grid ---------- */

EMBEDDED_FILES_GRID_TRICKS::EMBEDDED_FILES_GRID_TRICKS( WX_GRID* aGrid ) :
        GRID_TRICKS( aGrid ), m_curRow( -1 )
{
}


void EMBEDDED_FILES_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    if( const int row = aEvent.GetRow(); row >= 0 && row < m_grid->GetNumberRows() )
    {
        m_curRow = row;
        menu.Append( EMBEDDED_FILES_GRID_TRICKS_COPY_FILENAME, _( "Copy Embedded Reference" ),
                     _( "Copy the reference for this embedded file" ) );
        menu.AppendSeparator();
        GRID_TRICKS::showPopupMenu( menu, aEvent );
    }
    else
    {
        m_curRow = -1;
    }
}


void EMBEDDED_FILES_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    if( event.GetId() == EMBEDDED_FILES_GRID_TRICKS_COPY_FILENAME )
    {
        if( m_curRow >= 0 )
        {
            const wxString cellValue = m_grid->GetCellValue( m_curRow, 1 );

            if( wxTheClipboard->Open() )
            {
                wxTheClipboard->SetData( new wxTextDataObject( cellValue ) );
                wxTheClipboard->Close();
            }
        }
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


/* ---------- End of GRID_TRICKS for embedded files grid ---------- */


PANEL_EMBEDDED_FILES::PANEL_EMBEDDED_FILES( wxWindow* parent, EMBEDDED_FILES* aFiles ) :
        PANEL_EMBEDDED_FILES_BASE( parent ), m_files( aFiles ), m_localFiles( new EMBEDDED_FILES() )
{
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

    m_files_grid->PushEventHandler( new EMBEDDED_FILES_GRID_TRICKS( m_files_grid ) );

    m_localFiles->SetFileAddedCallback( [this](EMBEDDED_FILES::EMBEDDED_FILE* file) {

        for( int ii = 0; ii < m_files_grid->GetNumberRows(); ii++ )
        {
            if( m_files_grid->GetCellValue( ii, 1 ) == file->GetLink() )
            {
                m_files_grid->DeleteRows( ii );
                break;
            }
        }

        m_files_grid->AppendRows( 1 );
        int ii = m_files_grid->GetNumberRows() - 1;
        m_files_grid->SetCellValue( ii, 0, file->name );
        m_files_grid->SetCellValue( ii, 1, file->GetLink() );

    });
}


PANEL_EMBEDDED_FILES::~PANEL_EMBEDDED_FILES()
{
    // Remove the GRID_TRICKS handler
    m_files_grid->PopEventHandler( true );
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

    for( auto it = m_localFiles->EmbeddedFileMap().begin();
         it != m_localFiles->EmbeddedFileMap().end(); it++ )
        files.push_back( it->second );

    for( auto& file : files )
    {
        m_files->AddFile( file );
        m_localFiles->RemoveFile( file->name, false );
    }

    m_files->SetAreFontsEmbedded( m_cbEmbedFonts->IsChecked() );

    return true;
}


void PANEL_EMBEDDED_FILES::onFontEmbedClick( wxCommandEvent& event )
{
    Freeze();
    int row_pos = m_files_grid->GetGridCursorRow();
    int col_pos = m_files_grid->GetGridCursorCol();
    wxString row_name;

    if( row_pos >= 0 )
        row_name = m_files_grid->GetCellValue( row_pos, 0 );

    for( int ii = 0; ii < m_files_grid->GetNumberRows(); ii++ )
    {
        wxString name = m_files_grid->GetCellValue( ii, 0 );

        EMBEDDED_FILES::EMBEDDED_FILE* file = m_localFiles->GetEmbeddedFile( name );

        if( file && file->type == EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::FONT )
        {
            m_files_grid->DeleteRows( ii );
            ii--;
            m_localFiles->RemoveFile( name );
        }
    }

    if( m_cbEmbedFonts->IsChecked() )
    {
        std::set<KIFONT::OUTLINE_FONT*> fonts = m_files->GetFonts();

        for( KIFONT::OUTLINE_FONT* font : fonts )
        {
            EMBEDDED_FILES::EMBEDDED_FILE* result =
                    m_localFiles->AddFile( font->GetFileName(), true );

            if( !result )
            {
                wxLogTrace( wxT( "KICAD_EMBED" ), wxString::Format( "Could not embed font %s",
                                                                    font->GetFileName() ) );
                continue;
            }

            m_files_grid->AppendRows( 1 );
            int ii = m_files_grid->GetNumberRows() - 1;
            m_files_grid->SetCellValue( ii, 0, result->name );
            m_files_grid->SetCellValue( ii, 1, result->GetLink() );
        }
    }

    if( row_pos >= 0 )
    {
        col_pos = std::max( std::min( col_pos, m_files_grid->GetNumberCols() - 1 ), 0 );
        row_pos = std::max( std::min( row_pos, m_files_grid->GetNumberRows() - 1 ), 0 );
        m_files_grid->SetGridCursor( row_pos, col_pos );

        for( int ii = 0; ii < m_files_grid->GetNumberRows(); ++ii )
        {
            if( m_files_grid->GetCellValue( ii, 0 ) == row_name )
            {
                m_files_grid->SetGridCursor( ii, col_pos );
                break;
            }
        }
    }

    Thaw();
}


EMBEDDED_FILES::EMBEDDED_FILE* PANEL_EMBEDDED_FILES::AddEmbeddedFile( const wxString& aFile )
{
    wxFileName fileName( aFile );
    wxString   name = fileName.GetFullName();

    if( m_localFiles->HasFile( name ) )
    {
        wxString msg = wxString::Format( _( "File '%s' already exists." ), name );

        KIDIALOG errorDlg( m_parent, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        errorDlg.SetOKLabel( _( "Overwrite" ) );

        if( errorDlg.ShowModal() != wxID_OK )
            return nullptr;

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
        wxString msg = wxString::Format( _( "Failed to add file '%s'." ), name );

        KIDIALOG errorDlg( m_parent, msg, _( "Error" ), wxOK | wxICON_ERROR );
        errorDlg.ShowModal();
        return nullptr;
    }

    return result;
}


void PANEL_EMBEDDED_FILES::onAddEmbeddedFiles( wxCommandEvent& event )
{
    // TODO: Update strings to reflect that multiple files can be selected.
    wxFileDialog fileDialog( this, _( "Select a file to embed" ), wxEmptyString, wxEmptyString,
                             _( "All Files" ) + wxT( " (*.*)|*.*" ),
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

    if( fileDialog.ShowModal() == wxID_OK )
    {
        wxArrayString paths;
        fileDialog.GetPaths( paths );

        for( wxString path : paths )
            AddEmbeddedFile( path );
    }
}


bool PANEL_EMBEDDED_FILES::RemoveEmbeddedFile( const wxString& aFileName )
{
    wxString name = aFileName;

    if( name.StartsWith( FILEEXT::KiCadUriPrefix ) )
        name = name.Mid( FILEEXT::KiCadUriPrefix.size() + 3 );

    int row = std::max( 0, m_files_grid->GetGridCursorRow() );

    for( int ii = 0; ii < m_files_grid->GetNumberRows(); ii++ )
    {
        if( m_files_grid->GetCellValue( ii, 0 ) == name )
        {
            m_files_grid->DeleteRows( ii );
            m_localFiles->RemoveFile( name );

            if( row < m_files_grid->GetNumberRows() )
                m_files_grid->SetGridCursor( row, 0 );
            else if( m_files_grid->GetNumberRows() > 0 )
                m_files_grid->SetGridCursor( m_files_grid->GetNumberRows() - 1, 0 );

            return true;
        }
    }

    return false;
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


        wxFFileOutputStream out( fileName.GetFullPath() );

        if( !out.IsOk() )
        {
            wxString msg = wxString::Format( _( "Failed to open file '%s'." ),
                        fileName.GetFullName() );

            KIDIALOG errorDlg( m_parent, msg, _( "Error" ), wxOK | wxICON_ERROR );
            errorDlg.ShowModal();
            continue;
        }

        out.Write( file->decompressedData.data(), file->decompressedData.size() );

        if( !out.IsOk() || ( out.LastWrite() != file->decompressedData.size() ) )
        {
            wxString msg = wxString::Format( _( "Failed to write file '%s'." ),
                        fileName.GetFullName() );

            KIDIALOG errorDlg( m_parent, msg, _( "Error" ), wxOK | wxICON_ERROR );

            errorDlg.ShowModal();
        }
    }
}
