/**************************************************************/
/*					libarch.cc								  */
/* Module de generation du fichier d'archivage des composants */
/**************************************************************/
#include <algorithm> // to use sort vector
#include <vector>

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"

#include "protos.h"

/* Local functions*/
static bool TriListEntry( const EDA_LibComponentStruct* Objet1,
                          const EDA_LibComponentStruct* Objet2 );


/*******************************************************************/
bool LibArchive( wxWindow* frame, const wxString& ArchFullFileName )
/*******************************************************************/

/*
 *  Creates a library that contains all components used in the whole hierarchy
 *  return  true if success
 */
{
    wxString DocFileName, msg;
    char     Line[256];
    FILE*    ArchiveFile, * DocFile;
    EDA_LibComponentStruct* Entry;

    std::vector <EDA_LibComponentStruct*> ListEntry;

    EDA_ScreenList s_list;

    // examine all screens used and build the list of components found in lib
    for( SCH_SCREEN* screen = s_list.GetFirst(); screen != NULL; screen = s_list.GetNext() )
    {
        for( SCH_ITEM* SchItem = screen->EEDrawList; SchItem; SchItem = SchItem->Next() )
        {
            if( SchItem->Type() != TYPE_SCH_COMPONENT )
                continue;

            SCH_COMPONENT* DrawLibItem = (SCH_COMPONENT*) SchItem;
            Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
            if( Entry )    // if NULL : component not found
                ListEntry.push_back( Entry );
        }
    }

    sort( ListEntry.begin(), ListEntry.end(), TriListEntry );

    /* calculate the file name for the associated doc file */
    DocFileName = ArchFullFileName;
    ChangeFileNameExt( DocFileName, DOC_EXT );

    if( ( ArchiveFile = wxFopen( ArchFullFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to create archive lib file " ) + ArchFullFileName;
        DisplayError( frame, msg );
        return FALSE;
    }

    if( ( DocFile = wxFopen( DocFileName, wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Failed to create doc lib file " ) + DocFileName;
        DisplayError( frame, msg );
    }

    fprintf( ArchiveFile, "%s  %s\n#\n", LIBFILE_IDENT, DateAndTime( Line ) );
    if( DocFile )
        fprintf( DocFile, "%s  %s\n", DOCFILE_IDENT, DateAndTime( Line ) );

    /* Save components in file */
    for( unsigned ii = 0; ii < ListEntry.size(); ii++ )
    {
        if( (ii == 0) || ( ListEntry[ii - 1] != ListEntry[ii] ) )
        {
            if( ListEntry[ii]->Type == ROOT )   // Must be always true, but just in case
                ListEntry[ii]->Save( ArchiveFile );
            if( DocFile )
                ListEntry[ii]->SaveDoc( DocFile );
        }
    }

    fprintf( ArchiveFile, "#\n#EndLibrary\n" );
    fclose( ArchiveFile );

    if( DocFile )
    {
        fprintf( DocFile, "#\n#End Doc Library\n" );
        fclose( DocFile );
    }

    return TRUE;
}


/***********************************************************************************************/
bool TriListEntry( const EDA_LibComponentStruct* Objet1, const EDA_LibComponentStruct* Objet2 )
/***********************************************************************************************/

/* Compare function for sort()
 *   lib components are sorted by name
 */
{
    int             ii;

    ii = Objet1->m_Name.m_Text.CmpNoCase( Objet2->m_Name.m_Text );
    return ii < 0;
}
