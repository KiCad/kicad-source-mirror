/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <confirm.h>
#include <gestfich.h>
#include <drawing_sheet/ds_data_model.h>
#include <paths.h>
#include <widgets/infobar.h>
#include <wildcards_and_files_ext.h>

#include "pl_editor_frame.h"
#include "pl_editor_id.h"
#include "properties_frame.h"

#include <wx/filedlg.h>

bool PL_EDITOR_FRAME::saveCurrentPageLayout()
{
    wxCommandEvent saveEvent;
    saveEvent.SetId( wxID_SAVE );
    Files_io( saveEvent );

    return !IsContentModified();
}


void PL_EDITOR_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString filename;

    filename = GetFileFromHistory( event.GetId(), _( "Page Layout Description File" ) );

    if( filename != wxEmptyString )
    {
        if( IsContentModified() )
        {
            if( !HandleUnsavedChanges( this, _( "The current page layout has been modified. "
                                                "Save changes?" ),
                                       [&]()->bool { return saveCurrentPageLayout(); } ) )
            {
                return;
            }
        }

        ::wxSetWorkingDirectory( ::wxPathOnly( filename ) );

        if( LoadPageLayoutDescrFile( filename ) )
        {
            wxString msg;
            msg.Printf( _( "File \"%s\" loaded"), filename );
            SetStatusText( msg );
        }

        OnNewPageLayout();
    }
}


void PL_EDITOR_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


/* File commands. */
void PL_EDITOR_FRAME::Files_io( wxCommandEvent& event )
{
    wxString       msg;
    int            id = event.GetId();
    wxString       filename = GetCurrentFileName();
    DS_DATA_MODEL& pglayout = DS_DATA_MODEL::GetTheInstance();

    if( filename.IsEmpty() && id == wxID_SAVE )
        id = wxID_SAVEAS;

    if( ( id == wxID_NEW || id == wxID_OPEN ) && IsContentModified() )
    {
        if( !HandleUnsavedChanges( this, _( "The current page layout has been modified.  "
                                            "Save changes?" ),
                                   [&]()->bool { return saveCurrentPageLayout(); } ) )
        {
            return;
        }
    }

    switch( id )
    {
    case wxID_NEW:
        pglayout.AllowVoidList( true );
        SetCurrentFileName( wxEmptyString );
        pglayout.ClearList();
        OnNewPageLayout();
        break;

    case ID_APPEND_DESCR_FILE:
    {
         wxFileDialog openFileDialog( this, _( "Append Existing Drawing Sheet" ),
                                      wxEmptyString, wxEmptyString,
                                      PageLayoutDescrFileWildcard(), wxFD_OPEN );

        if( openFileDialog.ShowModal() == wxID_CANCEL )
            return;

        filename = openFileDialog.GetPath();

        if( ! InsertPageLayoutDescrFile( filename ) )
        {
            msg.Printf( _( "Unable to load %s file" ), filename );
            DisplayErrorMessage( this, msg );
        }
        else
        {
            GetScreen()->SetModify();
            HardRedraw();
            msg.Printf( _( "File \"%s\" inserted" ), filename );
            SetStatusText( msg );
        }
    }
        break;

    case wxID_OPEN:
    {
         wxFileDialog openFileDialog( this, _( "Open" ), wxEmptyString, wxEmptyString,
                                      PageLayoutDescrFileWildcard(), wxFD_OPEN );

        if( openFileDialog.ShowModal() == wxID_CANCEL )
            return;

        filename = openFileDialog.GetPath();

        if( ! LoadPageLayoutDescrFile( filename ) )
        {
            msg.Printf( _( "Unable to load %s file" ), filename );
            DisplayErrorMessage( this, msg );
        }
        else
        {
            OnNewPageLayout();
            msg.Printf( _( "File \"%s\" saved." ), filename );
            SetStatusText( msg );
        }
    }
        break;

    case wxID_SAVE:
        if( !SavePageLayoutDescrFile( filename ) )
        {
            msg.Printf( _( "Unable to write \"%s\"" ), filename );
            DisplayErrorMessage( this, msg );
        }
        else
        {
            msg.Printf( _("File \"%s\" saved."), filename );
            SetStatusText( msg );
        }
        break;

    case wxID_SAVEAS:
    {
        wxString dir = PATHS::GetUserTemplatesPath();
        wxFileDialog openFileDialog( this, _( "Save As" ), dir, wxEmptyString,
                                     PageLayoutDescrFileWildcard(),
                                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( openFileDialog.ShowModal() == wxID_CANCEL )
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
            msg.Printf( _( "Unable to create \"%s\"" ), filename );
            DisplayErrorMessage( this, msg );
        }

        else
        {
            msg.Printf( _("File \"%s\" saved."), filename );
            SetStatusText( msg );

            if( GetCurrentFileName().IsEmpty() )
                SetCurrentFileName( filename );
        }
    }
        break;

    default:
        break;
    }
}


bool PL_EDITOR_FRAME::LoadPageLayoutDescrFile( const wxString& aFullFileName )
{
    if( wxFileExists( aFullFileName ) )
    {
        bool loaded = false;

        loaded = DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( aFullFileName );

        if( !loaded )
        {
            ShowInfoBarError( _( "Error reading drawing sheet" ), true );
            return false;
        }

        SetCurrentFileName( aFullFileName );
        UpdateFileHistory( aFullFileName );
        GetScreen()->ClrModify();

        wxFileName fn = aFullFileName;
        m_infoBar->Dismiss();

        if( fn.FileExists() && !fn.IsFileWritable() )
        {
            ShowInfoBarWarning( _( "Layout file is read only." ), true );
        }

        return true;
    }

    return false;
}


bool PL_EDITOR_FRAME::InsertPageLayoutDescrFile( const wxString& aFullFileName )
{
    if( wxFileExists( aFullFileName ) )
    {
        const bool append = true;
        SaveCopyInUndoList();
        DS_DATA_MODEL::GetTheInstance().LoadDrawingSheet( aFullFileName, append );
        return true;
    }

    return false;
}


bool PL_EDITOR_FRAME::SavePageLayoutDescrFile( const wxString& aFullFileName )
{
    if( !aFullFileName.IsEmpty() )
    {
        wxFileName tempFile( aFullFileName );
        tempFile.SetName( wxT( "." ) + tempFile.GetName() );
        tempFile.SetExt( tempFile.GetExt() + wxT( "$" ) );

        try
        {
            DS_DATA_MODEL::GetTheInstance().Save( tempFile.GetFullPath() );
        }
        catch( const IO_ERROR& )
        {
            // In case we started a file but didn't fully write it, clean up
            wxRemoveFile( tempFile.GetFullPath() );

            return false;
        }

        if( !wxRenameFile( tempFile.GetFullPath(), aFullFileName ) )
            return false;

        GetScreen()->ClrModify();
        return true;
    }

    return false;
}
