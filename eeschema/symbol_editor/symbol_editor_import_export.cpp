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
#include <sch_io/sch_io.h>
#include <symbol_edit_frame.h>
#include <wildcards_and_files_ext.h>
#include <lib_symbol_library_manager.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <string_utils.h>


void SYMBOL_EDIT_FRAME::ImportSymbol()
{
    wxString msg;
    wxString libName = getTargetLib();

    if( !m_libMgr->LibraryExists( libName ) )
        libName = SelectLibrary( _( "Import Symbol" ), _( "Import symbol to library:" ) );

    if( !m_libMgr->LibraryExists( libName ) )
        return;

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

    if( m_libMgr->SymbolNameInUse( entry->GetName(), libName ) )
    {
        msg.Printf( _( "Symbol %s already exists in library '%s'." ),
                    UnescapeString( symbolName ),
                    libName );

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


