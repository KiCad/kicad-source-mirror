/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <confirm.h>
#include <symbol_lib_table.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <wildcards_and_files_ext.h>
#include <lib_manager.h>
#include <wx/filename.h>


void LIB_EDIT_FRAME::ImportPart()
{
    wxString msg;
    wxString libName = getTargetLib();

    if( !m_libMgr->LibraryExists( libName ) )
    {
        libName = SelectLibraryFromList();

        if( !m_libMgr->LibraryExists( libName ) )
            return;
    }

    wxFileDialog dlg( this, _( "Import Symbol" ), m_mruPath, wxEmptyString,
                      SchematicLibraryFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn = dlg.GetPath();
    m_mruPath = fn.GetPath();

    wxArrayString symbols;
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    // TODO dialog to select the part to be imported if there is more than one
    try
    {
        pi->EnumerateSymbolLib( symbols, fn.GetFullPath() );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Cannot import symbol library \"%s\"." ), fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
        return;
    }

    if( symbols.empty() )
    {
        msg.Printf( _( "Symbol library file \"%s\" is empty." ), fn.GetFullPath() );
        DisplayError( this,  msg );
        return;
    }

    wxString symbolName = symbols[0];
    LIB_ALIAS* entry = pi->LoadSymbol( fn.GetFullPath(), symbolName );

    if( m_libMgr->PartExists( symbols[0], libName ) )
    {
        msg.Printf( _( "Symbol \"%s\" already exists in library \"%s\"." ), symbolName, libName );
        DisplayError( this,  msg );
        return;
    }

    m_libMgr->UpdatePart( entry->GetPart(), libName );
    SyncLibraries( false );
    LoadPart( symbolName, libName, 1 );
}


void LIB_EDIT_FRAME::ExportPart()
{
    wxString msg, title;
    LIB_PART* part = getTargetPart();

    if( !part )
    {
        DisplayError( this, _( "There is no symbol selected to save." ) );
        return;
    }

    wxFileName fn;

    fn.SetName( part->GetName().Lower() );
    fn.SetExt( SchematicLibraryFileExtension );

    wxFileDialog dlg( this, _( "Export Symbol" ), m_mruPath, fn.GetFullName(),
                      SchematicLibraryFileWildcard(), wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();
    fn.MakeAbsolute();

    LIB_PART* old_part = NULL;

    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    if( fn.FileExists() )
    {
        try
        {
            LIB_ALIAS* alias = pi->LoadSymbol( fn.GetFullPath(), part->GetName() );

            if( alias )
                old_part = alias->GetPart();
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error occurred attempting to load symbol library file \"%s\"" ),
                        fn.GetFullPath() );
            DisplayErrorMessage( this, msg, ioe.What() );
            return;
        }

        if( old_part )
        {
            msg.Printf( _( "Symbol \"%s\" already exists in \"%s\"." ),
                        part->GetName(),
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
        msg.Printf( _( "Write permissions are required to save library \"%s\"." ),
                    fn.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    try
    {
        if( !fn.FileExists() )
            pi->CreateSymbolLib( fn.GetFullPath() );

        pi->SaveSymbol( fn.GetFullPath(), new LIB_PART( *part ) );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Failed to create symbol library file \"%s\"" ), fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
        msg.Printf( _( "Error creating symbol library \"%s\"" ), fn.GetFullName() );
        SetStatusText( msg );
        return;
    }

    m_mruPath = fn.GetPath();

    msg.Printf( _( "Symbol \"%s\" saved in library \"%s\"" ), part->GetName(), fn.GetFullPath() );
    SetStatusText( msg );

    // See if the user wants it added to a library table (global or project)
    SYMBOL_LIB_TABLE* libTable = selectSymLibTable( true );

    if( libTable )
    {
        if( !m_libMgr->AddLibrary( fn.GetFullPath(), libTable ) )
        {
            DisplayError( this, _( "Could not open the library file." ) );
            return;
        }

        bool globalTable = ( libTable == &SYMBOL_LIB_TABLE::GetGlobalLibTable() );
        saveSymbolLibTables( globalTable, !globalTable );
    }
}
