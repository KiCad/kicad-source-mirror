/****************************/
/*  EESCHEMA - files-io.cpp */
/****************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "class_library.h"
#include "libeditframe.h"
#include "sch_sheet.h"



/*****************************************************************************
* Routine to save an EESchema file.                                          *
* FileSave controls how the file is to be saved - under what name.           *
* Returns TRUE if the file has been saved.                                   *
*****************************************************************************/
bool SCH_EDIT_FRAME::SaveEEFile( SCH_SCREEN* screen, int FileSave )
{
    wxString msg;
    wxFileName schematicFileName, backupFileName;
    FILE*    f;

    if( screen == NULL )
        screen = GetScreen();

    /* If no name exists in the window yet - save as new. */
    if( screen->GetFileName().IsEmpty() )
        FileSave = FILE_SAVE_NEW;

    switch( FileSave )
    {
    case FILE_SAVE_AS:
        schematicFileName = screen->GetFileName();
        backupFileName = schematicFileName;

        /* Rename the old file to a '.bak' one: */
        if( schematicFileName.FileExists() )
        {
            backupFileName.SetExt( wxT( "bak" ) );
            wxRemoveFile( backupFileName.GetFullPath() );

            if( !wxRenameFile( schematicFileName.GetFullPath(), backupFileName.GetFullPath() ) )
            {
                DisplayError( this, wxT( "Warning: unable to rename old file" ) );
            }
        }
        break;

    case FILE_SAVE_NEW:
    {
        wxFileDialog dlg( this, _( "Schematic Files" ), wxGetCwd(),
                          screen->GetFileName(), SchematicFileWildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        screen->SetFileName( dlg.GetPath() );
        schematicFileName = dlg.GetPath();

        break;
    }

    default:
        break;
    }

    if( ( f = wxFopen( schematicFileName.GetFullPath(), wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to create file " ) + schematicFileName.GetFullPath();
        DisplayError( this, msg );
        return false;
    }

    if( FileSave == FILE_SAVE_NEW )
        screen->SetFileName( schematicFileName.GetFullPath() );

    bool success = screen->Save( f );

    if( !success )
        DisplayError( this, _( "File write operation failed." ) );
    else
    {
        screen->ClrModify();
        wxString msg;
        msg.Printf( wxT("File %s saved"), GetChars(screen->GetFileName() ) );
        SetStatusText(msg, 0);
    }


    fclose( f );

    return success;
}


/* Commands to save project or the current page.
 */
void SCH_EDIT_FRAME::Save_File( wxCommandEvent& event )
{
    int id = event.GetId();

    switch( id )
    {
    case ID_SAVE_PROJECT:     /* Update Schematic File */
        SaveProject();
        break;

    case ID_SAVE_ONE_SHEET:     /* Update Schematic File */
        SaveEEFile( NULL, FILE_SAVE_AS );
        break;

    case ID_SAVE_ONE_SHEET_AS:     /* Save EED (new name) */
        SaveEEFile( NULL, FILE_SAVE_NEW );
        break;

    default:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::Save_File Internal Error" ) );
        break;
    }
}


/**
 *  Load an entire project
 *
 *  Schematic root file and its subhierarchies, the configuration and the libs
 *  which are not already loaded)
 */
bool SCH_EDIT_FRAME::LoadOneEEProject( const wxString& FileName, bool IsNew )
{
    SCH_SCREEN* screen;
    wxString    FullFileName, msg;
    bool        LibCacheExist = false;
    SCH_SCREENS ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        if( screen->IsModify() )
            break;
    }

    if( screen )
    {
        if( !IsOK( this, _( "Discard changes to the current schematic?" ) ) )
            return false;

        if( g_RootSheet->GetScreen()->GetFileName() != m_DefaultSchematicFileName )
            SetLastProject( g_RootSheet->GetScreen()->GetFileName() );
    }

    FullFileName = FileName;

    if( ( FullFileName.IsEmpty() ) && !IsNew )
    {
        wxFileDialog dlg( this, _( "Open Schematic" ), wxGetCwd(),
                          wxEmptyString, SchematicFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        FullFileName = dlg.GetPath();
    }

    if( g_RootSheet )
    {
        SAFE_DELETE( g_RootSheet );
    }

    CreateScreens();
    screen = GetScreen();

    wxFileName fn = FullFileName;

    if( fn.IsRelative() )
    {
        fn.MakeAbsolute();
        FullFileName = fn.GetFullPath();
    }
    wxLogDebug( wxT( "Loading schematic " ) + FullFileName );
    wxSetWorkingDirectory( fn.GetPath() );

    screen->SetFileName( FullFileName );
    g_RootSheet->SetFileName( FullFileName );
    SetStatusText( wxEmptyString );
    ClearMsgPanel();

    memset( &g_EESchemaVar, 0, sizeof(g_EESchemaVar) );

    screen->ClrModify();

    if( IsNew )
    {
        screen->m_CurrentSheetDesc = &g_Sheet_A4;
        screen->SetZoom( 32 );
        screen->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );
        screen->m_Title = NAMELESS_PROJECT;
        screen->m_Title += wxT( ".sch" );
        GetScreen()->SetFileName( screen->m_Title );
        screen->m_Company.Empty();
        screen->m_Commentaire1.Empty();
        screen->m_Commentaire2.Empty();
        screen->m_Commentaire3.Empty();
        screen->m_Commentaire4.Empty();
        LoadProjectFile( wxEmptyString, TRUE );
        Zoom_Automatique( TRUE );
        SetSheetNumberAndCount();
        DrawPanel->Refresh();
        return true;
    }

    // Reloading configuration.
    msg = _( "Ready\nWorking dir: \n" ) + wxGetCwd();
    PrintMsg( msg );

    LoadProjectFile( wxEmptyString, FALSE );

    // Clear (if needed) the current active library in libedit because it could be
    // removed from memory
    LIB_EDIT_FRAME::EnsureActiveLibExists();

    // Delete old caches.
    CMP_LIBRARY::RemoveCacheLibrary();

    /* Loading the project library cache
     * until apr 2009 the lib is named <root_name>.cache.lib
     * and after (due to code change): <root_name>-cache.lib
     * so if the <name>-cache.lib is not found, the old way will be tried
    */
    fn = g_RootSheet->GetScreen()->GetFileName();

    bool use_oldcachename = false;
    wxString cachename =  fn.GetName() + wxT( "-cache" );

    fn.SetName( cachename );
    fn.SetExt( CompLibFileExtension );

    if( ! fn.FileExists() )
    {
        fn = g_RootSheet->GetScreen()->GetFileName();
        fn.SetExt( wxT( "cache.lib" ) );
        use_oldcachename = true;
    }

    if( fn.FileExists() )
    {
        wxString errMsg;

        wxLogDebug( wxT( "LoadOneEEProject() load schematic cache library file <%s>" ),
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
                fn.SetExt( CompLibFileExtension );
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

    if( !wxFileExists( g_RootSheet->GetScreen()->GetFileName() ) && !LibCacheExist )
    {
        Zoom_Automatique( FALSE );
        msg.Printf( _( "File <%s> not found." ),
                    GetChars( g_RootSheet->GetScreen()->GetFileName() ) );
        DisplayInfoMessage( this, msg, 0 );
        return false;
    }

    // load the project.
    g_RootSheet->SetScreen( NULL );
    bool diag = g_RootSheet->Load( this );
    SetScreen( m_CurrentSheet->LastScreen() );

    /* Redraw base screen (ROOT) if necessary. */
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    Zoom_Automatique( FALSE );
    SetSheetNumberAndCount();
    DrawPanel->Refresh( true );
    return diag;
}


/**
 *  Save the entire project and create an archive for components.
 *
 *  The library archive name is &ltroot_name&gt-cache.lib
 */
void SCH_EDIT_FRAME::SaveProject()
{
    SCH_SCREEN* screen;
    wxFileName  fn;
    SCH_SCREENS ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        D( printf( "SaveEEFile, %s\n", CONV_TO_UTF8( screen->GetFileName() ) ); )
        SaveEEFile( screen, FILE_SAVE_AS );
    }

    /* Archive components in current directory. */
    fn = g_RootSheet->GetFileName();
    wxString cachename =  fn.GetName() + wxT( "-cache" );
    fn.SetName( cachename );
    fn.SetExt( CompLibFileExtension );
    LibArchive( this, fn.GetFullPath() );
}

