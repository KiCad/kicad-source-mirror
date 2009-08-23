/**************************/
/* EESchema - selpart.cpp */
/**************************/

/* Routine de selection d'un composant en librairie
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"


/* Routines locales */

/* Variables locales */


/***************************************************************/
LibraryStruct* SelectLibraryFromList( WinEDA_DrawFrame* frame )
/***************************************************************/
{
    int             ii, NumOfLibs = NumOfLibraries();
    LibraryStruct*  Lib = NULL;
    static wxString OldLibName;
    wxString        LibName;

    if( NumOfLibs == 0 )
    {
        DisplayError( frame, _( "No libraries are loaded" ) );
        return NULL;
    }

    WinEDAListBox ListBox( frame, _( "Select Lib" ),
                          NULL, OldLibName, NULL,
                          wxColour( 255, 255, 255 ) );  // Library browser background color

    wxArrayString  libNamesList;
    LibraryStruct* libcache = NULL;
    for( LibraryStruct* Lib = g_LibraryList; Lib != NULL; Lib = Lib->m_Pnext )
    {
        if( Lib->m_IsLibCache )
            libcache = Lib;
        else
            libNamesList.Add( Lib->m_Name );
    }

    libNamesList.Sort();

    // Add lib cache
    if( libcache )
        libNamesList.Add( libcache->m_Name );

    ListBox.InsertItems( libNamesList );


    ListBox.MoveMouseToOrigin();

    ii = ListBox.ShowModal();

    if( ii >= 0 )    /* Recherche de la librairie */
    {
        Lib = FindLibrary( libNamesList[ii] );
    }

    return Lib;
}


/******************************************************************************************/
int DisplayComponentsNamesInLib( WinEDA_DrawFrame* frame,
                                 LibraryStruct* Library, wxString& Buffer, wxString& OldName )
/******************************************************************************************/
{
    int                     ii;
    wxString                msg;
    EDA_LibComponentStruct* LibEntry;
    WinEDAListBox*          ListBox;
    const wxChar**          ListNames;

    if( Library == NULL )
        Library = SelectLibraryFromList( frame );

    if( Library == NULL )
        return 0;

    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );
    LibEntry = (EDA_LibComponentStruct*) PQFirst( &Library->m_Entries, FALSE );

    ii = 0;
    while( LibEntry )
    {
        ii++;
        LibEntry = (EDA_LibComponentStruct*) PQNext( Library->m_Entries, LibEntry, NULL );
    }

    ListNames = (const wxChar**) MyZMalloc( (ii + 1) * sizeof(wxChar*) );

    msg.Printf( _( "Select component (%d items)" ), ii );

    ii = 0;
    LibEntry = (EDA_LibComponentStruct*) PQFirst( &Library->m_Entries, FALSE );
    while( LibEntry )
    {
        ListNames[ii++] = LibEntry->m_Name.m_Text.GetData();
        LibEntry = (EDA_LibComponentStruct*) PQNext( Library->m_Entries, LibEntry, NULL );
    }

//	Qsort(ListNames,StrNumICmp);

    ListBox = new WinEDAListBox( frame, msg,
                                ListNames, OldName, DisplayCmpDoc,
                                wxColour( 255, 255, 255 ) ); // Component background listbox color

    ListBox->MoveMouseToOrigin();

    ii = ListBox->ShowModal(); ListBox->Destroy();
    if( ii >= 0 )
        Buffer = ListNames[ii];

    free( ListNames );

    if( ii < 0 )
        return 0;
    return 1;
}


/************************************************************/
int GetNameOfPartToLoad( WinEDA_DrawFrame* frame,
                         LibraryStruct* Library, wxString& BufName )
/************************************************************/
{
    int             ii;
    static wxString OldCmpName;

    ii = DisplayComponentsNamesInLib( frame, Library, BufName, OldCmpName );
    if( ii <= 0 )
        return 0;

    OldCmpName = BufName;
    return 1;
}
