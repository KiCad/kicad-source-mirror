/*********************************************/
/*	eesave.cpp  Module to Save EESchema files */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "macros.h"

#include "protos.h"

/* Fonctions Locales */
static void SaveLayers( FILE* f );

/* Variables locales */

/*****************************************************************************
* Routine to save an EESchema file.											 *
* FileSave controls how the file is to be saved - under what name.			 *
* Returns TRUE if the file has been saved.									 *
*****************************************************************************/
bool WinEDA_SchematicFrame::SaveEEFile( SCH_SCREEN* screen, int FileSave )
{
    wxString msg;
    wxString Name, BakName;
    FILE*    f;
    wxString dirbuf;

    if( screen == NULL )
        screen = (SCH_SCREEN*) GetScreen();

    /* If no name exists in the window yet - save as new. */
    if( screen->m_FileName.IsEmpty() )
        FileSave = FILE_SAVE_NEW;

    switch( FileSave )
    {
    case FILE_SAVE_AS:
        dirbuf = wxGetCwd() + STRING_DIR_SEP;
        Name   = MakeFileName( dirbuf, screen->m_FileName, g_SchExtBuffer );
        /* Rename the old file to a '.bak' one: */
        BakName = Name;
        if( wxFileExists( Name ) )
        {
            ChangeFileNameExt( BakName, wxT( ".bak" ) );
            wxRemoveFile( BakName );    /* delete Old .bak file */
            if( !wxRenameFile( Name, BakName ) )
            {
                DisplayError( this, wxT( "Warning: unable to rename old file" ), 10 );
            }
        }
        break;

    case FILE_SAVE_NEW:
    {
        wxString mask = wxT( "*" ) + g_SchExtBuffer;
        Name = EDA_FileSelector( _( "Schematic files:" ),
            wxEmptyString,                  /* Chemin par defaut */
            screen->m_FileName,             /* nom fichier par defaut, et resultat */
            g_SchExtBuffer,                 /* extension par defaut */
            mask,                           /* Masque d'affichage */
            this,
            wxFD_SAVE,
            FALSE
            );
        if( Name.IsEmpty() )
            return FALSE;

        screen->m_FileName = Name;
        dirbuf = wxGetCwd() + STRING_DIR_SEP;
        Name   = MakeFileName( dirbuf, Name, g_SchExtBuffer );

        break;
    }

    default:
        break;
    }

    if( ( f = wxFopen( Name, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to create file " ) + Name;
        DisplayError( this, msg );
        return FALSE;
    }

    if( FileSave == FILE_SAVE_NEW )
        screen->m_FileName = Name;

    bool success = screen->Save( f );
    if( !success )
        DisplayError( this, _( "File write operation failed." ) );
    else
        screen->ClrModify();


    fclose( f );

    return success;
}


/*****************************************/
bool SCH_SCREEN::Save( FILE* aFile ) const
/*****************************************/

/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
{
    const wxChar**  LibNames;
    wxString        Name, msg;
    int             ii;
    bool            Failed = FALSE;
    EDA_BaseStruct* Phead;
    Ki_PageDescr*   PlotSheet;

    LibNames = GetLibNames();
    for( ii = 0; LibNames[ii] != NULL; ii++ )
    {
        if( ii > 0 )
            Name += wxT( "," );
        Name += LibNames[ii];
    }

    MyFree( LibNames );

    // Creates header
    if( fprintf( aFile, "%s %s %d\n", EESCHEMA_FILE_STAMP,
            SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION ) == EOF
        || fprintf( aFile, "LIBS:%s\n", CONV_TO_UTF8( Name ) ) == EOF )
    {
        return FALSE;
    }

    SaveLayers( aFile );

    /* Write page info */

    PlotSheet = m_CurrentSheetDesc;
    fprintf( aFile, "$Descr %s %d %d\n", CONV_TO_UTF8( PlotSheet->m_Name ),
        PlotSheet->m_Size.x, PlotSheet->m_Size.y );

    fprintf( aFile, "Sheet %d %d\n", m_ScreenNumber, m_NumberOfScreen );
    fprintf( aFile, "Title \"%s\"\n", CONV_TO_UTF8( m_Title ) );
    fprintf( aFile, "Date \"%s\"\n", CONV_TO_UTF8( m_Date ) );
    fprintf( aFile, "Rev \"%s\"\n", CONV_TO_UTF8( m_Revision ) );
    fprintf( aFile, "Comp \"%s\"\n", CONV_TO_UTF8( m_Company ) );
    fprintf( aFile, "Comment1 \"%s\"\n", CONV_TO_UTF8( m_Commentaire1 ) );
    fprintf( aFile, "Comment2 \"%s\"\n", CONV_TO_UTF8( m_Commentaire2 ) );
    fprintf( aFile, "Comment3 \"%s\"\n", CONV_TO_UTF8( m_Commentaire3 ) );
    fprintf( aFile, "Comment4 \"%s\"\n", CONV_TO_UTF8( m_Commentaire4 ) );

    fprintf( aFile, "$EndDescr\n" );

    /* Saving schematic items */
    Phead = EEDrawList;
    while( Phead )
    {
        switch( Phead->Type() )
        {
        case TYPE_SCH_COMPONENT:              /* Its a library item. */
            if( !( (SCH_COMPONENT*) Phead )->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_SHEET_STRUCT_TYPE:           /* Its a Sheet item. */
            if( !( (DrawSheetStruct*) Phead )->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:           /* Its a Segment item. */
                #undef STRUCT
                #define STRUCT ( (EDA_DrawLineStruct*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_BUSENTRY_STRUCT_TYPE:          /* Its a Raccord item. */
                #undef STRUCT
                #define STRUCT ( (DrawBusEntryStruct*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_POLYLINE_STRUCT_TYPE:           /* Its a polyline item. */
                #undef STRUCT
                #define STRUCT ( (DrawPolylineStruct*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:     /* Its a connection item. */
                #undef STRUCT
                #define STRUCT ( (DrawJunctionStruct*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_NOCONNECT_STRUCT_TYPE:        /* Its a NoConnection item. */
                #undef STRUCT
                #define STRUCT ( (DrawNoConnectStruct*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case TYPE_SCH_TEXT:             /* Its a text item. */
                #undef STRUCT
                #define STRUCT ( (SCH_TEXT*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;


        case TYPE_SCH_LABEL:            /* Its a label item. */
                #undef STRUCT
                #define STRUCT ( (SCH_LABEL*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case TYPE_SCH_GLOBALLABEL:     /* Its a Global label item. */
                #undef STRUCT
                #define STRUCT ( (SCH_GLOBALLABEL*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case TYPE_SCH_HIERLABEL:     /* Its a Hierarchical label item. */
                #undef STRUCT
                #define STRUCT ( (SCH_HIERLABEL*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_MARKER_STRUCT_TYPE:       /* Its a marker item. */
                #undef STRUCT
                #define STRUCT ( (DrawMarkerStruct*) Phead )
            if( !STRUCT->Save( aFile ) )
                Failed = TRUE;
            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        case DRAW_PICK_ITEM_STRUCT_TYPE:
            break;

        default:
            break;
        }

        if( Failed )
            break;

        Phead = Phead->Pnext;
    }

    if( fprintf( aFile, "$EndSCHEMATC\n" ) == EOF )
        Failed = TRUE;

    return !Failed;
}


/****************************/
static void SaveLayers( FILE* f )
/****************************/

/* Save a Layer Structure to a file
 * theses infos are not used in eeschema
 */
{
    fprintf( f, "EELAYER %2d %2d\n", g_LayerDescr.NumberOfLayers, g_LayerDescr.CurrentLayer );
    fprintf( f, "EELAYER END\n" );
}
