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
#include <wxEeschemaStruct.h>
#include <pgm_base.h>

#include <eeschema_id.h>
#include <class_library.h>
#include <libeditframe.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <wildcards_and_files_ext.h>


bool SCH_EDIT_FRAME::SaveEEFile( SCH_SCREEN* aScreen, bool aSaveUnderNewName, bool aCreateBackupFile )
{
    wxString msg;
    wxFileName schematicFileName;
    FILE* f;
    bool success;

    if( aScreen == NULL )
        aScreen = GetScreen();

    // If no name exists in the window yet - save as new.
    if( aScreen->GetFileName().IsEmpty() )
        aSaveUnderNewName = true;

    // Construct the name of the file to be saved
    schematicFileName = aScreen->GetFileName();

    if( aSaveUnderNewName )
    {
        wxFileDialog dlg( this, _( "Schematic Files" ), wxGetCwd(),
                schematicFileName.GetFullName(), SchematicFileWildcard,
                wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        schematicFileName = dlg.GetPath();

        if( schematicFileName.GetExt() != SchematicFileExtension )
            schematicFileName.SetExt( SchematicFileExtension );
    }
    else
    {
        // Sheet file names are relative to the root sheet path which is the current
        // working directory.  The IsWritable function expects the path to be set.
        if( schematicFileName.GetPath().IsEmpty() )
            schematicFileName.Assign( wxFileName::GetCwd(),
                                      schematicFileName.GetFullName() );
    }

    if( !IsWritable( schematicFileName ) )
        return false;

    /* Create backup if requested */
    if( aCreateBackupFile && schematicFileName.FileExists() )
    {
        wxFileName backupFileName = schematicFileName;

        /* Rename the old file to a '.bak' one: */
        backupFileName.SetExt( SchematicBackupFileExtension );
        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( schematicFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            msg.Printf( _( "Could not save backup of file <%s>" ),
                    GetChars( schematicFileName.GetFullPath() ) );
            DisplayError( this, msg );
        }
    }

    /* Save */
    wxLogTrace( traceAutoSave,
                wxT( "Saving file <" ) + schematicFileName.GetFullPath() + wxT( ">" ) );

    if( ( f = wxFopen( schematicFileName.GetFullPath(), wxT( "wt" ) ) ) == NULL )
    {
        msg.Printf( _( "Failed to create file <%s>" ),
                    GetChars( schematicFileName.GetFullPath() ) );
        DisplayError( this, msg );
        return false;
    }

    success = aScreen->Save( f );

    if( success )
    {
        // Delete auto save file.
        wxFileName autoSaveFileName = schematicFileName;
        autoSaveFileName.SetName( wxT( "$" ) + schematicFileName.GetName() );

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


bool SCH_EDIT_FRAME::LoadCacheLibrary( const wxString& aFilename )
{
    wxString msg;
    bool LibCacheExist = false;
    wxFileName fn = aFilename;

    /* Loading the project library cache
     * until apr 2009 the lib is named <root_name>.cache.lib
     * and after (due to code change): <root_name>-cache.lib
     * so if the <name>-cache.lib is not found, the old way will be tried
     */
    bool use_oldcachename = false;
    wxString cachename =  fn.GetName() + wxT( "-cache" );

    fn.SetName( cachename );
    fn.SetExt( SchematicLibraryFileExtension );

    if( ! fn.FileExists() )
    {
        fn = aFilename;
        fn.SetExt( wxT( "cache.lib" ) );
        use_oldcachename = true;
    }

    if( fn.FileExists() )
    {
        wxString errMsg;

        wxLogDebug( wxT( "Load schematic cache library file <%s>" ),
                    GetChars( fn.GetFullPath() ) );
        msg = wxT( "Load " ) + fn.GetFullPath();

        CMP_LIBRARY* LibCache = CMP_LIBRARY::LoadLibrary( fn, errMsg );

        if( LibCache )
        {
            LibCache->SetCache();
            msg += wxT( " OK" );

            if ( use_oldcachename )     // set the new name
            {
                fn.SetName( cachename );
                fn.SetExt( SchematicLibraryFileExtension );
                LibCache->SetFileName( fn );
            }

            LibCacheExist = true;
            CMP_LIBRARY::GetLibraryList().push_back( LibCache );
        }
        else
        {
            wxString prompt;

            prompt.Printf( _( "Component library <%s> failed to load.\nError: %s" ),
                           GetChars( fn.GetFullPath() ),
                           GetChars( errMsg ) );
            DisplayError( this, prompt );
            msg += _( " ->Error" );
        }

        PrintMsg( msg );
    }

    return LibCacheExist;
}


bool SCH_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    SCH_SCREEN* screen;
    wxString    fullFileName( aFileSet[0] );
    wxString    msg;
    SCH_SCREENS screenList;

    for( screen = screenList.GetFirst(); screen != NULL; screen = screenList.GetNext() )
    {
        if( screen->IsModify() )
            break;
    }

    if( screen )
    {
        int response = YesNoCancelDialog( this,
            _( "The current schematic has been modified.  Do you wish to save the changes?" ),
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
    }

/*
    if( fullFileName.IsEmpty() && !aIsNew )
    {
        wxFileDialog dlg( this, _( "Open Schematic" ), wxGetCwd(),
                          wxEmptyString, SchematicFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        FullFileName = dlg.GetPath();
    }
*/

    wxFileName fn = fullFileName;

    if( fn.IsRelative() )
    {
        fn.MakeAbsolute();
        fullFileName = fn.GetFullPath();
    }

    if( !Pgm().LockFile( fullFileName ) )
    {
        DisplayError( this, _( "This file is already open." ) );
        return false;
    }

    // Clear the screen before open a new file
    if( g_RootSheet )
    {
        delete g_RootSheet;
        g_RootSheet = NULL;
    }

    CreateScreens();
    screen = GetScreen();

    wxLogDebug( wxT( "Loading schematic " ) + fullFileName );

    // @todo: this is bad:
    wxSetWorkingDirectory( fn.GetPath() );

    screen->SetFileName( fullFileName );
    g_RootSheet->SetFileName( fullFileName );
    SetStatusText( wxEmptyString );
    ClearMsgPanel();

    screen->ClrModify();

#if 0
    if( aIsNew )
    {
        /* SCH_SCREEN constructor does this now
        screen->SetPageSettings( PAGE_INFO( wxT( "A4" ) ) );
        */

        screen->SetZoom( 32 );
        m_LastGridSizeId = screen->SetGrid( ID_POPUP_GRID_LEVEL_50 );

        TITLE_BLOCK tb;
        wxString    title;

        title += NAMELESS_PROJECT;
        title += wxT( ".sch" );
        tb.SetTitle( title );
        screen->SetTitleBlock( tb );

        GetScreen()->SetFileName( title );

        LoadProjectFile( wxEmptyString, true );
        Zoom_Automatique( false );
        SetSheetNumberAndCount();
        m_canvas->Refresh();
        return true;
    }
#endif

    // Reloading configuration.
    msg.Printf( _( "Ready\nWorking dir: '%s'\n" ), GetChars( wxGetCwd() ) );
    PrintMsg( msg );

    LoadProjectFile( wxEmptyString, true );

    // Clear (if needed) the current active library in libedit because it could be
    // removed from memory
    LIB_EDIT_FRAME::EnsureActiveLibExists();

    // Delete old caches.
    CMP_LIBRARY::RemoveCacheLibrary();

    if( !wxFileExists( g_RootSheet->GetScreen()->GetFileName() ) )
    {
        Zoom_Automatique( false );

        if( aCtl == 0 )
        {
            msg.Printf( _( "File '%s' not found." ),
                        GetChars( g_RootSheet->GetScreen()->GetFileName() ) );
            DisplayInfoMessage( this, msg );
        }

        return true;    // do not close Eeschema if the file if not found:
                        // we may have to create a new schematic file.
    }

    // load the project.
    bool libCacheExist = LoadCacheLibrary( g_RootSheet->GetScreen()->GetFileName() );

    g_RootSheet->SetScreen( NULL );

    bool diag = g_RootSheet->Load( this );

    SetScreen( m_CurrentSheet->LastScreen() );

    UpdateFileHistory( g_RootSheet->GetScreen()->GetFileName() );

    // Redraw base screen (ROOT) if necessary.
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    Zoom_Automatique( false );
    SetSheetNumberAndCount();
    m_canvas->Refresh( true );

    (void) libCacheExist;
    (void) diag;

//    return diag;
    return true;    // do not close Eeschema if the file if not found:
                    // we may have to create a new schematic file.
}


bool SCH_EDIT_FRAME::AppendOneEEProject()
{
    SCH_SCREEN* screen;
    wxString    fullFileName;
    wxString    msg;

    screen = GetScreen();

    if( !screen )
    {
        wxLogError( wxT("Document not ready, cannot import") );
        return false;
    }

    // open file chooser dialog
    wxFileDialog dlg( this, _( "Import Schematic" ), wxGetCwd(),
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

    LoadCacheLibrary( fullFileName );

    wxLogDebug( wxT( "Importing schematic " ) + fullFileName );

    // load the project
    bool success = LoadOneEEFile( screen, fullFileName, true );
    if( success )
    {
        // load sub-sheets
        EDA_ITEM* bs = screen->GetDrawItems();
        while( bs )
        {
            // do not append hierarchical sheets
            if( bs->Type() ==  SCH_SHEET_T )
            {
                screen->Remove( (SCH_SHEET*) bs );
            }
            // clear annotation and init new time stamp for the new components
            else if( bs->Type() == SCH_COMPONENT_T )
            {
                ( (SCH_COMPONENT*) bs )->SetTimeStamp( GetNewTimeStamp() );
                ( (SCH_COMPONENT*) bs )->ClearAnnotation( NULL );
                // Clear flags, which are set by these previous modifications:
                bs->ClearFlags();
            }

            bs = bs->Next();
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
    wxFileName  fn;
    wxFileName  tmp;
    SCH_SCREENS ScreenList;

    fn = g_RootSheet->GetFileName();

    // Ensure a path exists. if no path, assume the cwd is used
    // The IsWritable function expects the path to be set
    if( !fn.GetPath().IsEmpty() )
        tmp.AssignDir( fn.GetPath() );
    else
        tmp.AssignDir( wxGetCwd() );

    if( !IsWritable( tmp ) )
        return;

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
        SaveEEFile( screen );

    CreateArchiveLibraryCacheFile();

    UpdateTitle();
}


bool SCH_EDIT_FRAME::doAutoSave()
{
    wxFileName tmpFileName = g_RootSheet->GetFileName();
    wxFileName fn = tmpFileName;
    wxFileName  tmp;
    SCH_SCREENS screens;
    bool autoSaveOk = true;

    tmp.AssignDir( fn.GetPath() );

    if( !IsWritable( tmp ) )
        return false;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        // Only create auto save files for the schematics that have been modified.
        if( !screen->IsSave() )
            continue;

        tmpFileName = fn = screen->GetFileName();

        // Auto save file name is the normal file name prefixed with $.
        fn.SetName( wxT( "$" ) + fn.GetName() );

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
