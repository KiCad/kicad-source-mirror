/**
 * @file pagelayout_editor/files.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <macros.h>
#include <worksheet_shape_builder.h>

#include <pl_editor_frame.h>
#include <properties_frame.h>
#include <pl_editor_id.h>
#include <wildcards_and_files_ext.h>

void PL_EDITOR_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString filename;

    filename = GetFileFromHistory( event.GetId(), _( "Page Layout Description File" ) );

    if( filename != wxEmptyString )
    {
        if( GetScreen()->IsModify() && !IsOK( this,
                   _( "The current page layout has been modified.\n"
                      "Do you wish to discard the changes?" ) ) )
            return;

         m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
        ::wxSetWorkingDirectory( ::wxPathOnly( filename ) );
        if( LoadPageLayoutDescrFile( filename ) )
        {
            wxString msg;
            msg.Printf( _("File <%s> loaded"), GetChars( filename ) );
            SetStatusText( msg );
        }

        OnNewPageLayout();
    }
}

/* File commands. */
void PL_EDITOR_FRAME::Files_io( wxCommandEvent& event )
{
    wxString msg;
    int        id = event.GetId();
    wxString   filename = GetCurrFileName();
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();

    if( filename.IsEmpty() && id == wxID_SAVE )
        id = wxID_SAVEAS;

    switch( id )
    {
    case ID_LOAD_DEFAULT_PAGE_LAYOUT:
    case wxID_NEW:
    case wxID_OPEN:
        if( GetScreen()->IsModify() && !IsOK( this,
                   _( "The current page layout has been modified.\n"
                      "Do you wish to discard the changes?" ) ) )
            return;
        break;

    default:
        break;
    }


    switch( id )
    {
    case ID_LOAD_DEFAULT_PAGE_LAYOUT:
        pglayout.SetPageLayout();
        OnNewPageLayout();
        break;

    case wxID_NEW:
        pglayout.AllowVoidList( true );
        SetCurrFileName( wxEmptyString );
        pglayout.ClearList();
        OnNewPageLayout();
        break;

    case ID_APPEND_DESCR_FILE:
    {
         wxFileDialog openFileDialog(this, _("Append Page Layout Descr File"),
                wxEmptyString,
                wxEmptyString, PageLayoutDescrFileWildcard, wxFD_OPEN);

        if (openFileDialog.ShowModal() == wxID_CANCEL)
            return;

        filename = openFileDialog.GetPath();
        if( ! InsertPageLayoutDescrFile( filename ) )
        {
            msg.Printf( _("Unable to load %s file"), GetChars( filename ) );
            wxMessageBox( msg );
        }
        else
        {
            GetScreen()->SetModify();
            RebuildDesignTree();
            m_canvas->Refresh();
            msg.Printf( _("File <%s> inserted"), GetChars( filename ) );
            SetStatusText( msg );
        }
    }
        break;

    case wxID_OPEN:
    {
         wxFileDialog openFileDialog(this, _("Open file"), wxEmptyString,
                wxEmptyString, PageLayoutDescrFileWildcard, wxFD_OPEN);

        if (openFileDialog.ShowModal() == wxID_CANCEL)
            return;

        filename = openFileDialog.GetPath();
        if( ! LoadPageLayoutDescrFile( filename ) )
        {
            msg.Printf( _("Unable to load %s file"), GetChars( filename ) );
            wxMessageBox( msg );
        }
        else
        {
            OnNewPageLayout();
            msg.Printf( _("File <%s> loaded"), GetChars( filename ) );
            SetStatusText( msg );
        }
    }
        break;

    case wxID_SAVE:
        if( !SavePageLayoutDescrFile( filename ) )
        {
            msg.Printf( _("Unable to write <%s>"), GetChars( filename ) );
            wxMessageBox( msg );
        }
        else
        {
            msg.Printf( _("File <%s> written"), GetChars( filename ) );
            SetStatusText( msg );
        }
        break;

    case wxID_SAVEAS:
    {
         wxFileDialog openFileDialog(this, _("Create file"), wxEmptyString,
                wxEmptyString, PageLayoutDescrFileWildcard, wxFD_SAVE);

        if (openFileDialog.ShowModal() == wxID_CANCEL)
            return;

        filename = openFileDialog.GetPath();
        // Ensure the file has the right extension:
        // because a name like name.subname.subsubname is legal,
        // add the right extension without replacing the wxFileName
        // extension
        wxFileName fn(filename);

        if( fn.GetExt() != PageLayoutDescrFileExtension )
            filename << wxT(".") << PageLayoutDescrFileExtension;

        if( !SavePageLayoutDescrFile( filename ) )
        {
            msg.Printf( _("Unable to create <%s>"), GetChars( filename ) );
            wxMessageBox( msg );
        }

        else
        {
            msg.Printf( _("File <%s> written"), GetChars( filename ) );
            SetStatusText( msg );
            if( GetCurrFileName().IsEmpty() )
                SetCurrFileName( filename );
        }
    }
        break;

    default:
        wxMessageBox( wxT( "File_io: unexpected command id" ) );
        break;
    }
}

/* Loads a .kicad_wks page layout descr file
 */
bool PL_EDITOR_FRAME::LoadPageLayoutDescrFile( const wxString& aFullFileName )
{
    if( wxFileExists( aFullFileName ) )
    {
        WORKSHEET_LAYOUT::GetTheInstance().SetPageLayout( aFullFileName );
        SetCurrFileName( aFullFileName );
        UpdateFileHistory( aFullFileName );
        GetScreen()->ClrModify();
        return true;
    }

    return false;
}

/* Inserts a .kicad_wks page layout descr file
 * same as LoadPageLayoutDescrFile, but the new data is added
 * to the previous data.
 */
bool PL_EDITOR_FRAME::InsertPageLayoutDescrFile( const wxString& aFullFileName )
{
    if( wxFileExists( aFullFileName ) )
    {
        const bool append = true;
        SaveCopyInUndoList();
        WORKSHEET_LAYOUT::GetTheInstance().SetPageLayout( aFullFileName, append );
        return true;
    }

    return false;
}


/* Save the current layout in a .kicad_wks page layout descr file
 */
bool PL_EDITOR_FRAME::SavePageLayoutDescrFile( const wxString& aFullFileName )
{
    if( ! aFullFileName.IsEmpty() )
    {
        WORKSHEET_LAYOUT::GetTheInstance().Save( aFullFileName );
        GetScreen()->ClrModify();
        return true;
    }

    return false;
}
