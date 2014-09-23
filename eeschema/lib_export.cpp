/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file lib_export.cpp
 * @brief Eeschema library maintenance routines to backup modified libraries and
 *        create, edit, and delete components.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>

#include <general.h>
#include <libeditframe.h>
#include <class_library.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


extern int ExportPartId;


void LIB_EDIT_FRAME::OnImportPart( wxCommandEvent& event )
{
    m_lastDrawItem = NULL;

    wxFileDialog dlg( this, _( "Import Component" ), m_lastLibImportPath,
                      wxEmptyString, SchematicLibraryFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName  fn = dlg.GetPath();

    std::auto_ptr<PART_LIB> lib;

    try
    {
        std::auto_ptr<PART_LIB> new_lib( PART_LIB::LoadLibrary( fn.GetFullPath() ) );
        lib = new_lib;
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _(
            "Unable to import library '%s'.  Error:\n"
            "%s" ),
            GetChars( fn.GetFullPath() )
            );

        DisplayError( this, msg );
        return;
    }

    LIB_ALIAS* entry = lib->GetFirstEntry();

    if( !entry )
    {
        wxString msg = wxString::Format( _(
            "Part library file '%s' is empty." ),
            GetChars( fn.GetFullPath() )
            );
        DisplayError( this,  msg );
        return;
    }

    if( LoadOneLibraryPartAux( entry, lib.get() ) )
    {
        fn = dlg.GetPath();
        m_lastLibImportPath = fn.GetPath();
        DisplayLibInfos();
        GetScreen()->ClearUndoRedoList();
        m_canvas->Refresh();
    }
}


void LIB_EDIT_FRAME::OnExportPart( wxCommandEvent& event )
{
    wxString    msg, title;
    bool        createLib = ( event.GetId() == ExportPartId ) ? false : true;

    LIB_PART*   part = GetCurPart();

    if( !part )
    {
        DisplayError( this, _( "There is no component selected to save." ) );
        return;
    }

    wxFileName fn = part->GetName().Lower();

    fn.SetExt( SchematicLibraryFileExtension );

    title = createLib ? _( "New Library" ) : _( "Export Component" );

    wxFileDialog dlg( this, title, m_lastLibExportPath, fn.GetFullName(),
            SchematicLibraryFileWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    std::auto_ptr<PART_LIB> temp_lib( new PART_LIB( LIBRARY_TYPE_EESCHEMA, fn.GetFullPath() ) );

    SaveOnePart( temp_lib.get() );

    bool result = false;

    try
    {
        FILE_OUTPUTFORMATTER    formatter( fn.GetFullPath() );

        result = temp_lib.get()->Save( formatter );
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        fn.MakeAbsolute();
        msg = wxT( "Failed to create component library file " ) + fn.GetFullPath();
        DisplayError( this, msg );
        return;
    }

    if( result )
        m_lastLibExportPath = fn.GetPath();

    if( result )
    {
        if( createLib )
        {
            msg.Printf( _( "'%s' - OK" ), GetChars( fn.GetFullPath() ) );
            DisplayInfoMessage( this, _(
                "This library will not be available until it is loaded by Eeschema.\n\n"
                "Modify the Eeschema library configuration if you want to include it"
                " as part of this project." ) );
        }
        else
        {
            msg.Printf( _( "'%s' - Export OK" ), GetChars( fn.GetFullPath() ) );
        }
    }
    else    // Error
    {
        msg.Printf( _( "Error creating '%s'" ), GetChars( fn.GetFullName() ) );
    }

    SetStatusText( msg );
}
