/****************************/
/*	EESCHEMA - files-io.cpp	*/
/****************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "id.h"

/* Fonctions locales */


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


/************************************************************************************/
int WinEDA_SchematicFrame::LoadOneEEProject( const wxString& FileName, bool IsNew )
/************************************************************************************/
{
    /*
     *  Load an entire project
     * ( schematic root file and its subhierarchies, the configuration and the libs
     *  which are not already loaded)
     */
    SCH_SCREEN*    screen;
    wxString       FullFileName, msg;
    bool           LibCacheExist = FALSE;

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
        if( g_RootSheet->m_AssociatedScreen->m_FileName != g_DefaultSchematicFileName )
            SetLastProject( g_RootSheet->m_AssociatedScreen->m_FileName );
    }

    FullFileName = FileName;
    if( ( FullFileName.IsEmpty() ) && !IsNew )
    {
        wxString mask = wxT( "*" ) + g_SchExtBuffer;
        FullFileName = EDA_FileSelector( _( "Schematic files:" ),
            wxEmptyString,              /* Chemin par defaut */
            wxEmptyString,              /* nom fichier par defaut */
            g_SchExtBuffer,             /* extension par defaut */
            mask,                       /* Masque d'affichage */
            this,
            wxFD_OPEN,
            TRUE
        );
        if( FullFileName.IsEmpty() )
            return 0;
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
    MsgPanel->EraseMsgBox();

    memset( &g_EESchemaVar, 0, sizeof(g_EESchemaVar) );

    GetScreen()->ClrModify();

    //m_CurrentSheet->m_AssociatedScreen->Pnext = NULL; should be by default

    if( IsNew )
    {
        screen->m_CurrentSheetDesc = &g_Sheet_A4;
        screen->SetZoom( 32 );
        screen->m_Title = wxT( "noname.sch" );
        GetScreen()->m_FileName = screen->m_Title;
        screen->m_Company.Empty();
        screen->m_Commentaire1.Empty();
        screen->m_Commentaire2.Empty();
        screen->m_Commentaire3.Empty();
        screen->m_Commentaire4.Empty();
        Read_Config( wxEmptyString, TRUE );
        Zoom_Automatique( TRUE );
        SetSheetNumberAndCount();
        ReDrawPanel();
        return 1;
    }

    // Rechargement de la configuration:
    msg = _( "Ready\nWorking dir: \n" ) + wxGetCwd();
    PrintMsg( msg );

    Read_Config( wxEmptyString, FALSE );

    // Delete old caches.
    LibraryStruct* nextlib, * lib = g_LibraryList;
    for( ; lib != NULL; lib = nextlib )
    {
        nextlib = lib->m_Pnext;
        if( lib->m_IsLibCache )
            FreeCmpLibrary( this, lib->m_Name );
    }

    if( IsNew )
    {
        if( DrawPanel )
            DrawPanel->Refresh( true );
        return 1;
    }

    // Loading the project library cache
    wxString       FullLibName;
    wxString       shortfilename;
    wxSplitPath( g_RootSheet->m_AssociatedScreen->m_FileName, NULL, &shortfilename, NULL );
    FullLibName << wxT( "." ) << STRING_DIR_SEP << shortfilename << wxT( ".cache" ) <<
    g_LibExtBuffer;
    if( wxFileExists( FullLibName ) )
    {
        wxString libname;
        libname = FullLibName;
        ChangeFileNameExt( libname, wxEmptyString );
        msg = wxT( "Load " ) + FullLibName;
        LibraryStruct* LibCache = LoadLibraryName( this, FullLibName, libname );
        if( LibCache )
        {
            LibCache->m_IsLibCache = TRUE;
            msg += wxT( " OK" );
        }
        else
            msg += wxT( " ->Error" );
        PrintMsg( msg );
        LibCacheExist = TRUE;
    }

    if( !wxFileExists( g_RootSheet->m_AssociatedScreen->m_FileName ) && !LibCacheExist )   // Nouveau projet prpbablement
    {
        Zoom_Automatique( FALSE );
        msg.Printf( _( "File <%s> not found." ),
            g_RootSheet->m_AssociatedScreen->m_FileName.GetData() );
        DisplayInfo( this, msg, 20 );
        return -1;
    }

    //load the project.
    SAFE_DELETE( g_RootSheet->m_AssociatedScreen );
    bool diag = g_RootSheet->Load( this );

    /* Reaffichage ecran de base (ROOT) si necessaire */
    ActiveScreen = GetScreen();
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

    NewScreen = new SCH_SCREEN( SCHEMATIC_FRAME );

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
    wxString       LibArchiveFileName;

    EDA_ScreenList ScreenList;

    for( screen = ScreenList.GetFirst(); screen != NULL;
        screen = ScreenList.GetNext() )
    {
        D( printf( "SaveEEFile, %s\n", CONV_TO_UTF8( screen->m_FileName ) ); )
        SaveEEFile( screen, FILE_SAVE_AS );
    }

    /* Creation du fichier d'archivage composants en repertoire courant */
    LibArchiveFileName = MakeFileName( wxEmptyString, g_RootSheet->GetFileName(), wxEmptyString );
    ChangeFileNameExt( LibArchiveFileName, wxEmptyString );

    /* mise a jour extension  */
    LibArchiveFileName += wxT( ".cache" ) + g_LibExtBuffer;
    LibArchive( this, LibArchiveFileName );
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
