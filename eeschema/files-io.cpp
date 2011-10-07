/****************************/
/*  EESCHEMA - files-io.cpp */
/****************************/

#include "fctsys.h"
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


bool SCH_EDIT_FRAME::SaveEEFile( SCH_SCREEN* aScreen, int aSaveType )
{
    wxString msg;
    wxFileName schematicFileName, backupFileName;
    FILE* f;

    if( aScreen == NULL )
        aScreen = GetScreen();

    /* If no name exists in the window yet - save as new. */
    if( aScreen->GetFileName().IsEmpty() )
        aSaveType = FILE_SAVE_NEW;

    switch( aSaveType )
    {
    case FILE_SAVE_AS:
        schematicFileName = aScreen->GetFileName();
        backupFileName = schematicFileName;

        if( !IsWritable( schematicFileName ) )
            return false;

        /* Rename the old file to a '.bak' one: */
        if( schematicFileName.FileExists() )
        {
            backupFileName.SetExt( wxT( "bak" ) );
            wxRemoveFile( backupFileName.GetFullPath() );

            if( !wxRenameFile( schematicFileName.GetFullPath(), backupFileName.GetFullPath() ) )
            {
                DisplayError( this, _( "Could not save backup of file <" ) +
                              schematicFileName.GetFullPath() + wxT( ">." ) );
            }
        }
        break;

    case FILE_SAVE_NEW:
    {
        schematicFileName = aScreen->GetFileName();

        wxFileDialog dlg( this, _( "Schematic Files" ), wxGetCwd(),
                          schematicFileName.GetFullName(), SchematicFileWildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        aScreen->SetFileName( dlg.GetPath() );
        schematicFileName = dlg.GetPath();

        if( !IsWritable( schematicFileName ) )
            return false;

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

    if( aSaveType == FILE_SAVE_NEW )
        aScreen->SetFileName( schematicFileName.GetFullPath() );

    bool success = aScreen->Save( f );

    if( !success )
    {
        DisplayError( this, _( "File write operation failed." ) );
    }
    else
    {
        aScreen->ClrModify();
        wxString msg;
        msg.Printf( _( "File %s saved" ), GetChars( aScreen->GetFileName() ) );
        SetStatusText( msg, 0 );
    }


    fclose( f );

    return success;
}


void SCH_EDIT_FRAME::Save_File( wxCommandEvent& event )
{
    int id = event.GetId();

    switch( id )
    {
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


bool SCH_EDIT_FRAME::LoadOneEEProject( const wxString& aFileName, bool aIsNew )
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
    }

    FullFileName = aFileName;

    if( ( FullFileName.IsEmpty() ) && !aIsNew )
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

    screen->ClrModify();

    if( aIsNew )
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
        LoadProjectFile( wxEmptyString, true );
        Zoom_Automatique( false );
        SetSheetNumberAndCount();
        DrawPanel->Refresh();
        return true;
    }

    // Reloading configuration.
    msg = _( "Ready\nWorking dir: \n" ) + wxGetCwd();
    PrintMsg( msg );

    LoadProjectFile( wxEmptyString, false );

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
        Zoom_Automatique( false );
        msg.Printf( _( "File <%s> not found." ),
                    GetChars( g_RootSheet->GetScreen()->GetFileName() ) );
        DisplayInfoMessage( this, msg, 0 );
        return false;
    }

    // load the project.
    g_RootSheet->SetScreen( NULL );
    bool diag = g_RootSheet->Load( this );
    SetScreen( m_CurrentSheet->LastScreen() );

    UpdateFileHistory( g_RootSheet->GetScreen()->GetFileName() );

    /* Redraw base screen (ROOT) if necessary. */
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    Zoom_Automatique( false );
    SetSheetNumberAndCount();
    DrawPanel->Refresh( true );
    return diag;
}


void SCH_EDIT_FRAME::OnSaveProject( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen;
    wxFileName  fn;
    wxFileName  tmp;
    SCH_SCREENS ScreenList;

    fn = g_RootSheet->GetFileName();
    tmp.AssignDir( fn.GetPath() );

    if( !IsWritable( tmp ) )
        return;

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        D( printf( "SaveEEFile, %s\n", TO_UTF8( screen->GetFileName() ) ); )
        SaveEEFile( screen, FILE_SAVE_AS );
    }

    wxString cachename = fn.GetName() + wxT( "-cache" );
    fn.SetName( cachename );
    fn.SetExt( CompLibFileExtension );
    LibArchive( this, fn.GetFullPath() );
}
