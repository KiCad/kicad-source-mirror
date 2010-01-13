/**********************************************/
/*  eesave.cpp  Module to Save EESchema files */
/**********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "macros.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"

#include <boost/foreach.hpp>


/*****************************************************************************
* Routine to save an EESchema file.                                          *
* FileSave controls how the file is to be saved - under what name.           *
* Returns TRUE if the file has been saved.                                   *
*****************************************************************************/
bool WinEDA_SchematicFrame::SaveEEFile( SCH_SCREEN* screen, int FileSave )
{
    wxString msg, tmp;
    wxFileName schematicFileName, backupFileName;
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
        schematicFileName = screen->m_FileName;
        backupFileName = schematicFileName;

        /* Rename the old file to a '.bak' one: */
        if( schematicFileName.FileExists() )
        {
            backupFileName.SetExt( wxT( "bak" ) );
            wxRemoveFile( backupFileName.GetFullPath() );

            if( !wxRenameFile( schematicFileName.GetFullPath(),
                               backupFileName.GetFullPath() ) )
            {
                DisplayError( this,
                              wxT( "Warning: unable to rename old file" ), 10 );
            }
        }
        break;

    case FILE_SAVE_NEW:
    {
        wxFileDialog dlg( this, _( "Schematic Files" ), wxGetCwd(),
                          screen->m_FileName, SchematicFileWildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        screen->m_FileName = dlg.GetPath();
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
        screen->m_FileName = schematicFileName.GetFullPath();

    bool success = screen->Save( f );

    if( !success )
        DisplayError( this, _( "File write operation failed." ) );
    else
        screen->ClrModify();


    fclose( f );

    return success;
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.sch" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_SCREEN::Save( FILE* aFile ) const
{
    // Creates header
    if( fprintf( aFile, "%s %s %d", EESCHEMA_FILE_STAMP,
                 SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION ) < 0
        || fprintf( aFile, "  date %s\n", CONV_TO_UTF8( DateAndTime() ) ) < 0 )
        return FALSE;

    BOOST_FOREACH( const CMP_LIBRARY& lib, CMP_LIBRARY::GetLibraryList() )
    {
        if( fprintf( aFile, "LIBS:%s\n", CONV_TO_UTF8( lib.GetName() ) ) < 0 )
            return FALSE;
    }

    if( fprintf( aFile, "EELAYER %2d %2d\n", g_LayerDescr.NumberOfLayers,
                 g_LayerDescr.CurrentLayer ) < 0
        || fprintf( aFile, "EELAYER END\n" ) < 0 )
        return FALSE;

    /* Write page info, ScreenNumber and NumberOfScreen; not very meaningful for
     * SheetNumber and Sheet Count in a complex hierarchy, but useful in
     * simple hierarchy and flat hierarchy.  Used also to search the root
     * sheet ( ScreenNumber = 1 ) within the files
     */

    if( fprintf( aFile, "$Descr %s %d %d\n", CONV_TO_UTF8( m_CurrentSheetDesc->m_Name ),
                 m_CurrentSheetDesc->m_Size.x, m_CurrentSheetDesc->m_Size.y ) < 0
        || fprintf( aFile, "Sheet %d %d\n", m_ScreenNumber, m_NumberOfScreen ) < 0
        || fprintf( aFile, "Title \"%s\"\n", CONV_TO_UTF8( m_Title ) ) < 0
        || fprintf( aFile, "Date \"%s\"\n", CONV_TO_UTF8( m_Date ) ) < 0
        || fprintf( aFile, "Rev \"%s\"\n", CONV_TO_UTF8( m_Revision ) ) < 0
        || fprintf( aFile, "Comp \"%s\"\n", CONV_TO_UTF8( m_Company ) ) < 0
        || fprintf( aFile, "Comment1 \"%s\"\n", CONV_TO_UTF8( m_Commentaire1 ) ) < 0
        || fprintf( aFile, "Comment2 \"%s\"\n", CONV_TO_UTF8( m_Commentaire2 ) ) < 0
        || fprintf( aFile, "Comment3 \"%s\"\n", CONV_TO_UTF8( m_Commentaire3 ) ) < 0
        || fprintf( aFile, "Comment4 \"%s\"\n", CONV_TO_UTF8( m_Commentaire4 ) ) < 0
        || fprintf( aFile, "$EndDescr\n" ) < 0 )
        return FALSE;

    for( SCH_ITEM* item = EEDrawList; item; item = item->Next() )
    {
        if( !item->Save( aFile ) )
            return FALSE;
    }

    if( fprintf( aFile, "$EndSCHEMATC\n" ) < 0 )
        return FALSE;

    return TRUE;
}
