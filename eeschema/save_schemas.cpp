/*********************************************/
/*	eesave.cpp  Module to Save EESchema files */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
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
    const wxChar** LibNames;
    wxString       Name, msg;
    Ki_PageDescr*  PlotSheet;

    wxString datetime = DateAndTime(  );

    LibNames = GetLibNames();
    for( int ii = 0; LibNames[ii] != NULL; ii++ )
    {
        if( ii > 0 )
            Name += wxT( "," );
        Name += LibNames[ii];
    }

    MyFree( LibNames );

    // Creates header
    if( fprintf( aFile, "%s %s %d", EESCHEMA_FILE_STAMP,
            SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION ) == EOF )
        return FALSE;

    if( fprintf( aFile, "  date %s\n", CONV_TO_UTF8(datetime) ) == EOF )
        return FALSE;

    if( fprintf( aFile, "LIBS:%s\n", CONV_TO_UTF8( Name ) ) == EOF )
        return FALSE;

    SaveLayers( aFile );

    /* Write page info */

    PlotSheet = m_CurrentSheetDesc;
    fprintf( aFile, "$Descr %s %d %d\n", CONV_TO_UTF8( PlotSheet->m_Name ),
        PlotSheet->m_Size.x, PlotSheet->m_Size.y );

    /* Write ScreenNumber and NumberOfScreen; not very meaningfull for SheetNumber and Sheet Count
      * in a complex hierarchy, but usefull in simple hierarchy and flat hierarchy
      * Used also to serach the root sheet ( ScreenNumber = 1 ) withing the files
     */
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
    bool failed = FALSE;
    for( SCH_ITEM* item = EEDrawList;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_SCH_COMPONENT:                /* Its a library item. */
        case DRAW_SHEET_STRUCT_TYPE:            /* Its a Sheet item. */
        case DRAW_SEGMENT_STRUCT_TYPE:          /* Its a Segment item. */
        case DRAW_BUSENTRY_STRUCT_TYPE:         /* Its a Raccord item. */
        case DRAW_POLYLINE_STRUCT_TYPE:         /* Its a polyline item. */
        case DRAW_JUNCTION_STRUCT_TYPE:         /* Its a connection item. */
        case DRAW_NOCONNECT_STRUCT_TYPE:        /* Its a NoConnection item. */
        case TYPE_SCH_TEXT:                     /* Its a text item. */
        case TYPE_SCH_LABEL:                    /* Its a label item. */
        case TYPE_SCH_GLOBALLABEL:              /* Its a Global label item. */
        case TYPE_SCH_HIERLABEL:                /* Its a Hierarchical label item. */
        case DRAW_MARKER_STRUCT_TYPE:           /* Its a marker item. */
            if( !item->Save( aFile ) )
                failed = TRUE;
            break;

            /*
              * case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
              * case DRAW_PICK_ITEM_STRUCT_TYPE:
              * break;
             */

        default:
            break;
        }

        if( failed )
            break;
    }

    if( fprintf( aFile, "$EndSCHEMATC\n" ) == EOF )
        failed = TRUE;

    return !failed;
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
