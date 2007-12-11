/****************************************/
/*	Module to load/save EESchema files.	*/
/****************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "id.h"


static void LoadSubHierarchy( WinEDA_SchematicFrame* frame, EDA_BaseStruct* DrawList );

/* Variables locales */


/************************************************************************************/
int WinEDA_SchematicFrame::LoadOneEEProject( const wxString& FileName, bool IsNew )
/************************************************************************************/

/*
 *  Load an entire project ( shcematic root file and its subhierarchies, the configuration and the libs
 *  which are not already loaded)
 */
{
    SCH_SCREEN*    screen;
    wxString       FullFileName, msg;
    bool           LibCacheExist = FALSE;

    EDA_ScreenList ScreenList( NULL );

    for( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
    {
        if( screen->IsModify() )
            break;
    }

    if( screen )
    {
        if( !IsOK( this, _( "Clear Schematic Hierarchy (modified!)?" ) ) )
            return FALSE;
        if( ScreenSch->m_FileName != g_DefaultSchematicFileName )
            SetLastProject( ScreenSch->m_FileName );
    }


    screen = ScreenSch;

    FullFileName = FileName;
    if( ( FullFileName.IsEmpty() ) && !IsNew )
    {
        wxString mask = wxT( "*" ) + g_SchExtBuffer;
        FullFileName = EDA_FileSelector( _( "Schematic files:" ),
                                         wxEmptyString,     /* Chemin par defaut */
                                         wxEmptyString,     /* nom fichier par defaut */
                                         g_SchExtBuffer,    /* extension par defaut */
                                         mask,              /* Masque d'affichage */
                                         this,
                                         wxFD_OPEN,
                                         TRUE
                                         );
        if( FullFileName.IsEmpty() )
            return FALSE;
    }

    if( ClearProjectDrawList( screen, TRUE ) == FALSE )
        return 1;

    ActiveScreen = m_CurrentScreen = screen = ScreenSch;
    ScreenSch->ClearUndoRedoList();
    screen->SetCurItem( NULL );
    wxSetWorkingDirectory( wxPathOnly( FullFileName ) );
    m_CurrentScreen->m_FileName = FullFileName;
    Affiche_Message( wxEmptyString );
    MsgPanel->EraseMsgBox();

    memset( &g_EESchemaVar, 0, sizeof(g_EESchemaVar) );

    m_CurrentScreen->ClrModify();
    m_CurrentScreen->Pnext = NULL;

    if( IsNew )
    {
        screen->m_CurrentSheet = &g_Sheet_A4;
        screen->SetZoom( 32 );
        screen->m_SheetNumber = screen->m_NumberOfSheet = 1;
        screen->m_Title = wxT( "noname.sch" );
        m_CurrentScreen->m_FileName = screen->m_Title;
        screen->m_Company.Empty();
        screen->m_Commentaire1.Empty();
        screen->m_Commentaire2.Empty();
        screen->m_Commentaire3.Empty();
        screen->m_Commentaire4.Empty();
        Read_Config( wxEmptyString, TRUE );
        Zoom_Automatique( TRUE );
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
        ReDrawPanel();
        return 1;
    }

    // Loading the project library cache
    wxString       FullLibName;
    wxString       shortfilename;
    wxSplitPath( ScreenSch->m_FileName, NULL, &shortfilename, NULL );
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

    if( !wxFileExists( ScreenSch->m_FileName ) && !LibCacheExist )   // Nouveau projet prpbablement
    {
        msg.Printf( _( "File %s not found (new project ?)" ),
                   ScreenSch->m_FileName.GetData() );
        DisplayInfo( this, msg, 20 );
        return -1;
    }

    if( LoadOneEEFile( ScreenSch, ScreenSch->m_FileName ) == FALSE )
        return 0;

    /* load all subhierarchies fond in current list and new loaded list */
    LoadSubHierarchy( this, ScreenSch->EEDrawList );

    /* Reaffichage ecran de base (ROOT) si necessaire */
    ActiveScreen = ScreenSch;
    Zoom_Automatique( FALSE );

    return 1;
}


/*******************************************************************************/
void LoadSubHierarchy( WinEDA_SchematicFrame* frame, EDA_BaseStruct* DrawList )
/*******************************************************************************/

/* load subhierarcy when sheets are found in DrawList
 *  recursive function.
 */
{
    EDA_BaseStruct* EEDrawList = DrawList;

    while( EEDrawList )
    {
        if( EEDrawList->Type() == DRAW_SHEET_STRUCT_TYPE )
        {
            #undef STRUCT
            #define STRUCT ( (DrawSheetStruct*) EEDrawList )
            int timestamp = STRUCT->m_TimeStamp;
            if( timestamp == 0 )
            {
                timestamp = GetTimeStamp();
                STRUCT->m_TimeStamp = timestamp;
            }
            if( !STRUCT->m_FileName.IsEmpty() )
            {
				//problem -- must check for closed loops here, or we may never exit! 
				//search back up the linked list tree...
				EDA_BaseStruct* strct = EEDrawList;
				bool noRecurse = true; 
				while( strct->m_Parent ){
					strct = strct->m_Parent; 
					if( ((DrawSheetStruct*)strct)->m_FileName ==
										 STRUCT->m_FileName ){
						wxString msg; 
						msg += wxString::Format(_( "The sheet hierarchy has an infinite loop, halting recursive loads. file: "));
						msg += STRUCT->m_FileName; 
						DisplayError( frame, msg );
						noRecurse = false; 
					}
				}
                if( frame->LoadOneEEFile( STRUCT, STRUCT->m_FileName ) == TRUE && noRecurse)
                {
                    LoadSubHierarchy( frame, STRUCT->EEDrawList );
                }
            }
            else
                DisplayError( frame, _( "No FileName in SubSheet" ) );
        }
        EEDrawList = EEDrawList->Pnext;
    }
}
