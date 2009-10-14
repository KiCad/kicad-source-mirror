/****************************/
/*  EESCHEMA - files-io.cpp */
/****************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "class_library.h"


/****************************************************************/
void WinEDA_SchematicFrame::Save_File( wxCommandEvent& event )
/****************************************************************/

/* Commands to save project or the current page.
 */
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
        DisplayError( this, wxT( "WinEDA_SchematicFrame::Save_File Internal Error" ) );
        break;
    }
}


/*
 *  Load an entire project
 *
 *  Schematic root file and its subhierarchies, the configuration and the libs
 *  which are not already loaded)
 */
int WinEDA_SchematicFrame::LoadOneEEProject( const wxString& FileName,
                                             bool IsNew )
{
    SCH_SCREEN*    screen;
    wxString       FullFileName, msg;
    bool           LibCacheExist = false;

    EDA_ScreenList ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        if( screen->IsModify() )
            break;
    }

    if( screen )
    {
        if( !IsOK( this, _( "Clear Schematic Hierarchy (modified!)?" ) ) )
            return FALSE;
        if( g_RootSheet->m_AssociatedScreen->m_FileName != m_DefaultSchematicFileName )
            SetLastProject( g_RootSheet->m_AssociatedScreen->m_FileName );
    }

    FullFileName = FileName;
    if( ( FullFileName.IsEmpty() ) && !IsNew )
    {
        wxFileDialog dlg( this, _( "Open Schematic" ), wxGetCwd(),
                          wxEmptyString, SchematicFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if( dlg.ShowModal() == wxID_CANCEL )
            return 0;

        FullFileName = dlg.GetPath();
    }

    if( g_RootSheet )
    {
        SAFE_DELETE( g_RootSheet );
    }
    CreateScreens();
    ActiveScreen = GetScreen();
    screen = (SCH_SCREEN*) GetScreen();

    wxSetWorkingDirectory( wxPathOnly( FullFileName ) );
    GetScreen()->m_FileName = FullFileName;
    g_RootSheet->SetFileName( FullFileName );
    Affiche_Message( wxEmptyString );
    ClearMsgPanel();

    memset( &g_EESchemaVar, 0, sizeof(g_EESchemaVar) );

    GetScreen()->ClrModify();

    //m_CurrentSheet->m_AssociatedScreen->Pnext = NULL; should be by default

    if( IsNew )
    {
        screen->m_CurrentSheetDesc = &g_Sheet_A4;
        screen->SetZoom( 32 );
        screen->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );
        screen->m_Title = wxT( "noname.sch" );
        GetScreen()->m_FileName = screen->m_Title;
        screen->m_Company.Empty();
        screen->m_Commentaire1.Empty();
        screen->m_Commentaire2.Empty();
        screen->m_Commentaire3.Empty();
        screen->m_Commentaire4.Empty();
        LoadProjectFile( wxEmptyString, TRUE );
        Zoom_Automatique( TRUE );
        SetSheetNumberAndCount();
        DrawPanel->Refresh();
        return 1;
    }

    // Rechargement de la configuration:
    msg = _( "Ready\nWorking dir: \n" ) + wxGetCwd();
    PrintMsg( msg );

    LoadProjectFile( wxEmptyString, FALSE );

    // Delete old caches.
    CMP_LIBRARY::RemoveCacheLibrary();

    if( IsNew )
    {
        if( DrawPanel )
            DrawPanel->Refresh( true );
        return 1;
    }

    /* Loading the project library cache
     * until apr 2009 the lib is named <root_name>.cache.lib
     * and after (due to code change): <root_name>-cache.lib
     * so if the <name>-cache.lib is not foun, the od way will be tried
    */
    bool use_oldcachename = false;
    wxFileName fn = g_RootSheet->m_AssociatedScreen->m_FileName;
    wxString cachename =  fn.GetName() + wxT("-cache");
    fn.SetName( cachename );
    fn.SetExt( CompLibFileExtension );
    if( ! fn.FileExists() )
    {
        fn = g_RootSheet->m_AssociatedScreen->m_FileName;
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
                fn.SetName(cachename);
                fn.SetExt( CompLibFileExtension );
                LibCache->SetFileName( fn );
            }

            LibCacheExist = true;
            CMP_LIBRARY::GetLibraryList().push_back( LibCache );
        }
        else
        {
            wxString prompt;

            prompt.Printf( _( "Component library <%s> failed to load.\n\n\
Error: %s" ),
                           ( const wxChar* ) fn.GetFullPath(),
                           ( const wxChar* ) errMsg );
            DisplayError( this, prompt );
            msg += wxT( " ->Error" );
        }

        PrintMsg( msg );
    }

    if( !wxFileExists( g_RootSheet->m_AssociatedScreen->m_FileName )
        && !LibCacheExist )
    {
        Zoom_Automatique( FALSE );
        msg.Printf( _( "File <%s> not found." ),
                    g_RootSheet->m_AssociatedScreen->m_FileName.GetData() );
        DisplayInfoMessage( this, msg, 0 );
        return -1;
    }

    //load the project.
    SAFE_DELETE( g_RootSheet->m_AssociatedScreen );
    bool diag = g_RootSheet->Load( this );

    /* Reaffichage ecran de base (ROOT) si necessaire */
    ActiveScreen = GetScreen();
    ActiveScreen->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );
    Zoom_Automatique( FALSE );
    SetSheetNumberAndCount();
    DrawPanel->Refresh( true );
    return diag;
}


/**********************************************************/
SCH_SCREEN* WinEDA_SchematicFrame::CreateNewScreen(
    SCH_SCREEN* OldScreen, int TimeStamp )
/**********************************************************/

/* Routine de creation ( par allocation memoire ) d'un nouvel ecran
  *     cet ecran est en chainage arriere avec OldScreen
  *     la valeur TimeStamp est attribuee au parametre NewScreen->TimeStamp
 */
{
    SCH_SCREEN* NewScreen;

    NewScreen = new SCH_SCREEN();

    NewScreen->SetRefreshReq();
    if( OldScreen )
        NewScreen->m_Company = OldScreen->m_Company;
    NewScreen->m_TimeStamp = TimeStamp;

    NewScreen->SetBack( OldScreen );

    return NewScreen;
}


/****************************************************/
void WinEDA_SchematicFrame::SaveProject()
/****************************************************/

/* Saves the entire project and creates an archive for components
 *  the library archive name is <root_name>.cache.lib
 */
{
    SCH_SCREEN*    screen;
    wxFileName     fn;
    EDA_ScreenList ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL;
        screen = ScreenList.GetNext() )
    {
        D( printf( "SaveEEFile, %s\n", CONV_TO_UTF8( screen->m_FileName ) ); )
        SaveEEFile( screen, FILE_SAVE_AS );
    }

    /* Creation du fichier d'archivage composants en repertoire courant */
    fn = g_RootSheet->GetFileName();
    wxString cachename =  fn.GetName() + wxT("-cache");
    fn.SetName( cachename );
    fn.SetExt( CompLibFileExtension );
    LibArchive( this, fn.GetFullPath() );
}


/************************/
int CountCmpNumber()
/************************/

/* Routine retournant le nombre de composants dans le schema,
 *  powers non comprises */
{
    return g_RootSheet->ComponentCount();

    /*
      * BASE_SCREEN*    Window;
      * EDA_BaseStruct* Phead;
      * int             Nb = 0;
     *
     *
     *
      * Window = ScreenSch;
      * while( Window )
      * {
      * for( Phead = Window->EEDrawList; Phead != NULL; Phead = Phead->Pnext )
      * {
      *     if( Phead->Type() == TYPE_SCH_COMPONENT )
      *     {
      *         DrawPartStruct* Cmp = (DrawPartStruct*) Phead;
      *         if( Cmp->m_Field[VALUE].m_Text.GetChar( 0 ) != '#' )
      *             Nb++;
      *     }
      * }
     *
      * Window = (BASE_SCREEN*) Window->Pnext;
      * }
     *
      * return Nb;
     */
}
