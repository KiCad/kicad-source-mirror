/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <confirm.h>
#include <kidialog.h>
#include <common.h>
#include <io/eagle/eagle_parser.h>
#include <symbol_lib_table.h>
#include <symbol_edit_frame.h>
#include <symbol_library.h>
#include <wildcards_and_files_ext.h>
#include <lib_symbol_library_manager.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <string_utils.h>
#include <io/eagle/eagle_parser.h>


void SYMBOL_EDIT_FRAME::ImportSymbol()
{
    wxString msg;
    wxString libName = getTargetLib();

    if( !m_libMgr->LibraryExists( libName ) )
    {
        libName = SelectLibraryFromList();

        if( !m_libMgr->LibraryExists( libName ) )
            return;
    }

    wxString fileFiltersStr;
    wxString allWildcardsStr;

    for( const SCH_IO_MGR::SCH_FILE_T& fileType : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( fileType ) );

        if( !pi )
            continue;

        const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryFileDesc();

        if( desc.m_FileExtensions.empty() )
            continue;

        if( !fileFiltersStr.IsEmpty() )
            fileFiltersStr += wxChar( '|' );

        fileFiltersStr += desc.FileFilter();

        for( const std::string& ext : desc.m_FileExtensions )
            allWildcardsStr << wxT( "*." ) << formatWildcardExt( ext ) << wxT( ";" );
    }

    fileFiltersStr = _( "All supported formats" ) + wxT( "|" ) + allWildcardsStr + wxT( "|" )
                     + fileFiltersStr;

    wxFileDialog dlg( this, _( "Import Symbol" ), m_mruPath, wxEmptyString, fileFiltersStr,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn( dlg.GetPath() );

    m_mruPath = fn.GetPath();

    wxArrayString symbols;
    SCH_IO_MGR::SCH_FILE_T piType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

    if( piType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
    {
        msg.Printf( _( "Unable to find a reader for '%s'." ), fn.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( piType ) );

    // TODO dialog to select the symbol to be imported if there is more than one
    try
    {
        pi->EnumerateSymbolLib( symbols, fn.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Cannot import symbol library '%s'." ), fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
        return;
    }
    catch( const XML_PARSER_ERROR& ioe )
    {
        msg.Printf( _( "Cannot import symbol library '%s'." ), fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.what() );
        return;
    }

    if( symbols.empty() )
    {
        msg.Printf( _( "Symbol library file '%s' is empty." ), fn.GetFullPath() );
        DisplayError( this,  msg );
        return;
    }

    wxString symbolName = symbols[0];
    LIB_SYMBOL* entry = pi->LoadSymbol( fn.GetFullPath(), symbolName );

    entry->SetName( EscapeString( entry->GetName(), CTX_LIBID ) );

    if( m_libMgr->SymbolExists( entry->GetName(), libName ) )
    {
        msg.Printf( _( "Symbol %s already exists in library '%s'." ), symbolName, libName );

        KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        errorDlg.SetOKLabel( _( "Overwrite" ) );
        errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( errorDlg.ShowModal() == wxID_CANCEL )
            return;
    }

    m_libMgr->UpdateSymbol( entry, libName );
    SyncLibraries( false );
    LoadSymbol( entry->GetName(), libName, 1 );
}


void SYMBOL_EDIT_FRAME::ExportSymbol()
{
    wxString msg;
    LIB_SYMBOL* symbol = getTargetSymbol();

    if( !symbol )
    {
        ShowInfoBarError( _( "There is no symbol selected to save." ) );
        return;
    }

    wxFileName fn;

    fn.SetName( symbol->GetName().Lower() );
    fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

    wxFileDialog dlg( this, _( "Export Symbol" ), m_mruPath, fn.GetFullName(),
                      FILEEXT::KiCadSymbolLibFileWildcard(), wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();
    fn.MakeAbsolute();

    LIB_SYMBOL* old_symbol = nullptr;
    SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

    if( pluginType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        pluginType = SCH_IO_MGR::SCH_KICAD;

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( pluginType ) );

    if( fn.FileExists() )
    {
        try
        {
            old_symbol = pi->LoadSymbol( fn.GetFullPath(), symbol->GetName() );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error occurred attempting to load symbol library file '%s'." ),
                        fn.GetFullPath() );
            DisplayErrorMessage( this, msg, ioe.What() );
            return;
        }

        if( old_symbol )
        {
            msg.Printf( _( "Symbol %s already exists in library '%s'." ),
                        UnescapeString( symbol->GetName() ),
                        fn.GetFullName() );

            KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            errorDlg.SetOKLabel( _( "Overwrite" ) );
            errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( errorDlg.ShowModal() == wxID_CANCEL )
                return;
        }
    }

    if( fn.Exists() && !fn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to save library '%s'." ),
                    fn.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    try
    {
        if( !fn.FileExists() )
            pi->CreateLibrary( fn.GetFullPath() );

        // The flattened symbol is most likely what the user would want.  As some point in
        // the future as more of the symbol library inheritance is implemented, this may have
        // to be changes to save symbols of inherited symbols.
        pi->SaveSymbol( fn.GetFullPath(), symbol->Flatten().release() );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Failed to create symbol library file '%s'." ), fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
        msg.Printf( _( "Error creating symbol library '%s'." ), fn.GetFullName() );
        SetStatusText( msg );
        return;
    }

    m_mruPath = fn.GetPath();

    msg.Printf( _( "Symbol %s saved to library '%s'." ),
                UnescapeString( symbol->GetName() ),
                fn.GetFullPath() );
    SetStatusText( msg );
}
