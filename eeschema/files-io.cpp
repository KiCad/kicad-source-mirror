/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file eeschema/files-io.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <schframe.h>
#include <pgm_base.h>
#include <kiface_i.h>

#include <eeschema_id.h>
#include <class_library.h>
#include <libeditframe.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <wildcards_and_files_ext.h>
#include <lib_cache_rescue.h>
#include <eeschema_config.h>


bool SCH_EDIT_FRAME::SaveEEFile( SCH_SCREEN* aScreen, bool aSaveUnderNewName, bool aCreateBackupFile )
{
    wxString msg;
    wxFileName schematicFileName;
    bool success;

    if( aScreen == NULL )
        aScreen = GetScreen();

    // If no name exists in the window yet - save as new.
    if( aScreen->GetFileName().IsEmpty() )
        aSaveUnderNewName = true;

    // Construct the name of the file to be saved
    schematicFileName = Prj().AbsolutePath( aScreen->GetFileName() );

    if( aSaveUnderNewName )
    {
        wxFileDialog dlg( this, _( "Schematic Files" ),
                wxPathOnly( Prj().GetProjectFullName() ),
                schematicFileName.GetFullName(), SchematicFileWildcard,
                wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        schematicFileName = dlg.GetPath();

        if( schematicFileName.GetExt() != SchematicFileExtension )
            schematicFileName.SetExt( SchematicFileExtension );
    }

    if( !IsWritable( schematicFileName ) )
        return false;

    // Create backup if requested
    if( aCreateBackupFile && schematicFileName.FileExists() )
    {
        wxFileName backupFileName = schematicFileName;

        // Rename the old file to a '.bak' one:
        backupFileName.SetExt( SchematicBackupFileExtension );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( schematicFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            msg.Printf( _( "Could not save backup of file '%s'" ),
                    GetChars( schematicFileName.GetFullPath() ) );
            DisplayError( this, msg );
        }
    }

    // Save
    wxLogTrace( traceAutoSave,
                wxT( "Saving file <" ) + schematicFileName.GetFullPath() + wxT( ">" ) );

    FILE* f = wxFopen( schematicFileName.GetFullPath(), wxT( "wt" ) );

    if( !f )
    {
        msg.Printf( _( "Failed to create file '%s'" ),
                    GetChars( schematicFileName.GetFullPath() ) );
        DisplayError( this, msg );
        return false;
    }

    success = aScreen->Save( f );

    if( success )
    {
        // Delete auto save file.
        wxFileName autoSaveFileName = schematicFileName;
        autoSaveFileName.SetName( AUTOSAVE_PREFIX_FILENAME + schematicFileName.GetName() );

        if( autoSaveFileName.FileExists() )
        {
            wxLogTrace( traceAutoSave,
                        wxT( "Removing auto save file <" ) + autoSaveFileName.GetFullPath() +
                        wxT( ">" ) );

            wxRemoveFile( autoSaveFileName.GetFullPath() );
        }

        // Update the screen and frame info.
        if( aSaveUnderNewName )
            aScreen->SetFileName( schematicFileName.GetFullPath() );
        aScreen->ClrSave();
        aScreen->ClrModify();

        msg.Printf( _( "File %s saved" ), GetChars( aScreen->GetFileName() ) );
        SetStatusText( msg, 0 );
    }
    else
    {
        DisplayError( this, _( "File write operation failed." ) );
    }

    fclose( f );

    return success;
}


void SCH_EDIT_FRAME::Save_File( wxCommandEvent& event )
{
    int id = event.GetId();

    switch( id )
    {
    case ID_UPDATE_ONE_SHEET:
        SaveEEFile( NULL );
        break;

    case ID_SAVE_ONE_SHEET_UNDER_NEW_NAME:
        if( SaveEEFile( NULL, true ) )
        {
            CreateArchiveLibraryCacheFile( true );
        }
        break;
    }

    UpdateTitle();
}


bool SCH_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // implement the pseudo code from KIWAY_PLAYER.h:

    SCH_SCREENS screenList;

    // This is for python:
    if( aFileSet.size() != 1 )
    {
        UTF8 msg = StrPrintf( "Eeschema:%s() takes only a single filename", __func__ );
        DisplayError( this, msg );
        return false;
    }

    wxString    fullFileName( aFileSet[0] );

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wxFileName( fullFileName ).IsAbsolute(),
        wxT( "bug in single_top.cpp or project manager." ) );

    if( !LockFile( fullFileName ) )
    {
        wxString msg = wxString::Format( _(
                "Schematic file '%s' is already open." ),
                GetChars( fullFileName )
                );
        DisplayError( this, msg );
        return false;
    }

    // save any currently open and modified project files.
    for( SCH_SCREEN* screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
    {
        if( screen->IsModify() )
        {
            int response = YesNoCancelDialog( this, _(
                "The current schematic has been modified.  Do you wish to save the changes?" ),
                wxEmptyString,
                _( "Save and Load" ),
                _( "Load Without Saving" )
                );

            if( response == wxID_CANCEL )
            {
                return false;
            }
            else if( response == wxID_YES )
            {
                wxCommandEvent dummy;
                OnSaveProject( dummy );
            }
            else
            {
                // response == wxID_NO, fall thru
            }
            break;
        }
    }

    wxFileName pro = fullFileName;
    pro.SetExt( ProjectFileExtension );

    bool is_new = !wxFileName::IsFileReadable( fullFileName );

    // If its a non-existent schematic and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        wxString ask = wxString::Format( _(
                "Schematic '%s' does not exist.  Do you wish to create it?" ),
                GetChars( fullFileName )
                );
        if( !IsOK( this, ask ) )
            return false;
    }

    // unload current project file before loading new
    {
        delete g_RootSheet;
        g_RootSheet = NULL;

        CreateScreens();
    }

    GetScreen()->SetFileName( fullFileName );
    g_RootSheet->SetFileName( fullFileName );

    SetStatusText( wxEmptyString );
    ClearMsgPanel();

    wxString msg = wxString::Format( _(
            "Ready\nProject dir: '%s'\n" ),
            GetChars( wxPathOnly( Prj().GetProjectFullName() ) )
            );
    SetStatusText( msg );

    // PROJECT::SetProjectFullName() is an impactful function.  It should only be
    // called under carefully considered circumstances.

    // The calling code should know not to ask me here to change projects unless
    // it knows what consequences that will have on other KIFACEs running and using
    // this same PROJECT.  It can be very harmful if that calling code is stupid.
    Prj().SetProjectFullName( pro.GetFullPath() );

    LoadProjectFile();

    // load the libraries here, not in SCH_SCREEN::Draw() which is a context
    // that will not tolerate DisplayError() dialog since we're already in an
    // event handler in there.
    // And when a schematic file is loaded, we need these libs to initialize
    // some parameters (links to PART LIB, dangling ends ...)
    Prj().SchLibs();

    if( is_new )
    {
        // mark new, unsaved file as modified.
        GetScreen()->SetModify();
    }
    else
    {
        g_RootSheet->SetScreen( NULL );

        DBG( printf( "%s: loading schematic %s\n", __func__, TO_UTF8( fullFileName ) );)

        bool diag = g_RootSheet->Load( this );
        (void) diag;

        SetScreen( m_CurrentSheet->LastScreen() );

        GetScreen()->ClrModify();

        UpdateFileHistory( fullFileName );

        // Check to see whether some old, cached library parts need to be rescued
        // Only do this if RescueNeverShow was not set.
        wxConfigBase *config = Kiface().KifaceSettings();
        bool rescueNeverShow = false;
        config->Read( RESCUE_NEVER_SHOW_KEY, &rescueNeverShow, false );

        if( !rescueNeverShow )
        {
            if( RescueCacheConflicts( false ) )
            {
                GetScreen()->CheckComponentsToPartsLinks();
                GetScreen()->TestDanglingEnds();
            }
        }
    }

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    Zoom_Automatique( false );
    SetSheetNumberAndCount();

    m_canvas->Refresh( true );

    return true;
}


bool SCH_EDIT_FRAME::AppendOneEEProject()
{
    wxString    fullFileName;
    wxString    msg;

    SCH_SCREEN* screen = GetScreen();

    if( !screen )
    {
        wxLogError( wxT("Document not ready, cannot import") );
        return false;
    }

    // open file chooser dialog
    wxString path = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Import Schematic" ), path,
                      wxEmptyString, SchematicFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    fullFileName = dlg.GetPath();

    wxFileName fn = fullFileName;

    if( fn.IsRelative() )
    {
        fn.MakeAbsolute();
        fullFileName = fn.GetFullPath();
    }

    wxString cache_name = PART_LIBS::CacheName( fullFileName );
    if( !!cache_name )
    {
        PART_LIBS*  libs = Prj().SchLibs();

        if( PART_LIB* lib = libs->AddLibrary( cache_name ) )
            lib->SetCache();
    }

    wxLogDebug( wxT( "Importing schematic " ) + fullFileName );

    // Keep trace of the last item in list.
    // New items will be loaded after this one.
    SCH_ITEM* bs = screen->GetDrawItems();

    if( bs )
        while( bs->Next() )
            bs = bs->Next();

    // load the project
    bool success = LoadOneEEFile( screen, fullFileName, true );

    if( success )
    {
        // the new loaded items need cleaning to avoid duplicate parameters
        // which should be unique (ref and time stamp).
        // Clear ref and set a new time stamp for new items
        if( bs == NULL )
            bs = screen->GetDrawItems();
        else
            bs = bs->Next();

        while( bs )
        {
            SCH_ITEM* nextbs = bs->Next();

            // To avoid issues with the current hieratchy,
            // do not load included sheets files and give new filenames
            // and new sheet names.
            // There are many tricky cases (loops, creation of complex hierarchies
            // with duplicate file names, duplicate sheet names...)
            // So the included sheets names are renamed if existing,
            // and filenames are just renamed to avoid loops and
            // creation of complex hierarchies.
            // If someone want to change it for a better append function, remember
            // these cases need work to avoid issues.
            if( bs->Type() == SCH_SHEET_T )
            {
                SCH_SHEET * sheet = (SCH_SHEET *) bs;
                time_t newtimestamp = GetNewTimeStamp();
                sheet->SetTimeStamp( newtimestamp );

                // Check for existing subsheet name in the current sheet
                wxString tmp = sheet->GetName();
                sheet->SetName( wxEmptyString );
                const SCH_SHEET* subsheet = GetScreen()->GetSheet( tmp );

                if( subsheet )
                    sheet->SetName( wxString::Format( wxT( "Sheet%8.8lX" ), (long) newtimestamp ) );
                else
                    sheet->SetName( tmp );

                sheet->SetFileName( wxString::Format( wxT( "file%8.8lX.sch" ), (long) newtimestamp ) );
                sheet->SetScreen( new SCH_SCREEN( &Kiway() ) );
                sheet->GetScreen()->SetFileName( sheet->GetFileName() );
            }
            // clear annotation and init new time stamp for the new components
            else if( bs->Type() == SCH_COMPONENT_T )
            {
                ( (SCH_COMPONENT*) bs )->SetTimeStamp( GetNewTimeStamp() );
                ( (SCH_COMPONENT*) bs )->ClearAnnotation( NULL );

                // Clear flags, which are set by these previous modifications:
                bs->ClearFlags();
            }

            bs = nextbs;
        }
    }

    // redraw base screen (ROOT) if necessary
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    Zoom_Automatique( false );
    SetSheetNumberAndCount();
    m_canvas->Refresh( true );
    return success;
}


void SCH_EDIT_FRAME::OnAppendProject( wxCommandEvent& event )
{
    wxString msg = _( "This operation cannot be undone. "
            "Besides, take into account that hierarchical sheets will not be appended.\n\n"
            "Do you want to save the current document before proceeding?" );

    if( IsOK( this, msg ) )
        OnSaveProject( event );

    AppendOneEEProject();
}


void SCH_EDIT_FRAME::OnSaveProject( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen;
    SCH_SCREENS screenList;

    // I want to see it in the debugger, show me the string!  Can't do that with wxFileName.
    wxString    fileName = Prj().AbsolutePath( g_RootSheet->GetFileName() );

    wxFileName  fn = fileName;

    if( !fn.IsDirWritable() )
    {
        wxString msg = wxString::Format( _(
                "Directory '%s' is not writable" ),
                GetChars( fn.GetPath() )
                );

        DisplayError( this, msg );
        return;
    }

    for( screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
        SaveEEFile( screen );

    CreateArchiveLibraryCacheFile();

    UpdateTitle();
}


bool SCH_EDIT_FRAME::doAutoSave()
{
    wxFileName  tmpFileName = g_RootSheet->GetFileName();
    wxFileName  fn = tmpFileName;
    wxFileName  tmp;
    SCH_SCREENS screens;

    bool autoSaveOk = true;

    tmp.AssignDir( fn.GetPath() );

    if( !tmp.IsOk() )
        return false;

    if( !IsWritable( tmp ) )
        return false;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        // Only create auto save files for the schematics that have been modified.
        if( !screen->IsSave() )
            continue;

        tmpFileName = fn = screen->GetFileName();

        // Auto save file name is the normal file name prefixed with AUTOSAVE_PREFIX_FILENAME.
        fn.SetName( AUTOSAVE_PREFIX_FILENAME + fn.GetName() );

        screen->SetFileName( fn.GetFullPath() );

        if( SaveEEFile( screen, false, NO_BACKUP_FILE ) )
            screen->SetModify();
        else
            autoSaveOk = false;

        screen->SetFileName( tmpFileName.GetFullPath() );
    }

    if( autoSaveOk )
        m_autoSaveState = false;

    return autoSaveOk;
}
