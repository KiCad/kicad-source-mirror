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

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "eeschema_id.h"
#include "richio.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"

#include <wx/filename.h>
#include <wx/wfstream.h>


extern int ExportPartId;


void LIB_EDIT_FRAME::OnImportPart( wxCommandEvent& event )
{
    wxString     errMsg;
    wxFileName   fn;
    CMP_LIBRARY* LibTmp;
    LIB_ALIAS*   LibEntry;

    m_lastDrawItem = NULL;

    wxFileDialog dlg( this, _( "Import Component" ), m_LastLibImportPath,
                      wxEmptyString, CompLibFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    LibTmp = CMP_LIBRARY::LoadLibrary( fn, errMsg );

    if( LibTmp == NULL )
        return;

    LibEntry = LibTmp->GetFirstEntry();

    if( LibEntry == NULL )
    {
        wxString msg;

        msg.Printf( _( "Component library file <%s> is empty." ), GetChars( fn.GetFullPath() ) );
        DisplayError( this,  msg );
        return;
    }

    if( LoadOneLibraryPartAux( LibEntry, LibTmp ) )
    {
        fn = dlg.GetPath();
        m_LastLibImportPath = fn.GetPath();
        DisplayLibInfos();
        GetScreen()->ClearUndoRedoList();
        DrawPanel->Refresh();
    }

    delete LibTmp;
}


void LIB_EDIT_FRAME::OnExportPart( wxCommandEvent& event )
{
    wxFileName   fn;
    wxString     msg, title;
    CMP_LIBRARY* CurLibTmp;
    bool         createLib = ( event.GetId() == ExportPartId ) ? false : true;

    if( m_component == NULL )
    {
        DisplayError( this, _( "There is no component selected to save." ) );
        return;
    }

    fn = m_component->GetName().Lower();
    fn.SetExt( CompLibFileExtension );

    title = createLib ? _( "New Library" ) : _( "Export Component" );

    wxFileDialog dlg( this, title, wxGetCwd(), fn.GetFullName(),
                      CompLibFileWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    CurLibTmp = m_library;

    m_library = new CMP_LIBRARY( LIBRARY_TYPE_EESCHEMA, fn );

    SaveOnePartInMemory();

    wxFFileOutputStream os( fn.GetFullPath(), wxT( "wt" ) );

    if( !os.IsOk() )
    {
        fn.MakeAbsolute();
        msg = wxT( "Failed to create component library file " ) + fn.GetFullPath();
        DisplayError( this, msg );
        return;
    }

    STREAM_OUTPUTFORMATTER formatter( os );

    bool success = m_library->Save( formatter );

    if( success )
        m_LastLibExportPath = fn.GetPath();

    delete m_library;
    m_library = CurLibTmp;

    if( success )
    {
        if( createLib )
        {
            msg = fn.GetFullPath() + _( " - OK" );
            DisplayInfoMessage( this, _( "This library will not be available \
until it is loaded by Eeschema.\n\nModify the Eeschema library configuration \
if you want to include it as part of this project." ) );
        }
        else
        {
            msg = fn.GetFullPath() + _( " - Export OK" );
        }
    }   // Error
    else
    {
        msg = _( "Error creating " ) + fn.GetFullName();
    }

    SetStatusText( msg );
}
