/*****************************************************************/
/*  Functions to handle component library files : read functions */
/*****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"

#include "dialog_load_error.h"

/* Local Functions */
static void InsertAlias( PriorQue** PQ, EDA_LibComponentStruct* LibEntry,
                         int* NumOfParts );


// If this code was written in C++ then this would not be needed.
static wxString currentLibraryName;


/****************************************************************************/

/** Function LoadLibraryName
 * Routine to load the given library name. FullLibName should hold full path
 * of file name to open, while LibName should hold only its name.
 * If library already exists, it is NOT reloaded.
 * @return : new lib or NULL
 */
/****************************************************************************/
LibraryStruct* LoadLibraryName( WinEDA_DrawFrame* frame,
                                const wxString&   FullLibName,
                                const wxString&   LibName )
{
    int            NumOfParts;
    FILE*          f;
    LibraryStruct* NewLib;
    PriorQue*      Entries;
    wxFileName     fn;

    if( ( NewLib = FindLibrary( LibName ) ) != NULL )
    {
        if( NewLib->m_FullFileName == FullLibName )
            return NewLib;
        FreeCmpLibrary( frame, LibName );
    }

    NewLib = NULL;

    f = wxFopen( FullLibName, wxT( "rt" ) );
    if( f == NULL )
    {
        wxString msg;
        msg.Printf( _( "Library <%s> not found" ), FullLibName.GetData() );
        DisplayError( frame, msg );
        return NULL;
    }

    currentLibraryName = FullLibName;

    NewLib = new LibraryStruct( LIBRARY_TYPE_EESCHEMA, LibName, FullLibName );

    Entries = LoadLibraryAux( frame, NewLib, f, &NumOfParts );
    if( Entries != NULL )
    {
        NewLib->m_Entries    = Entries;
        NewLib->m_NumOfParts = NumOfParts;

        if( g_LibraryList == NULL )
            g_LibraryList = NewLib;
        else
        {
            LibraryStruct* tmplib = g_LibraryList;
            while( tmplib->m_Pnext )
                tmplib = tmplib->m_Pnext;

            tmplib->m_Pnext = NewLib;
        }

        fn = FullLibName;
        fn.SetExt( DOC_EXT );
        LoadDocLib( frame, fn.GetFullPath(), NewLib->m_Name );
    }
    else
    {
        SAFE_DELETE( NewLib );
    }
    fclose( f );
    return NewLib;
}


/** Function LoadLibraries
 * Clear all already loaded libraries and load all librairies
 * given in frame->m_ComponentLibFiles
 */
void LoadLibraries( WinEDA_SchematicFrame* frame )
{
    wxFileName     fn;
    wxString       msg, tmp;
    wxString       libraries_not_found;
    unsigned       ii, iimax = frame->m_ComponentLibFiles.GetCount();

    // Free the unwanted libraries (i.e. not in list) but keep the cache lib
    LibraryStruct* nextlib, * lib = g_LibraryList;

    for( ; lib != NULL; lib = nextlib )
    {
        nextlib = lib->m_Pnext;
        if( lib->m_IsLibCache )
            continue;

        // is this library in "wanted list" frame->m_ComponentLibFiles ?
        if( frame->m_ComponentLibFiles.Index( lib->m_Name ) == wxNOT_FOUND )
            FreeCmpLibrary( frame, lib->m_Name );
    }

    // Load missing libraries (if any)
    for( ii = 0; ii < iimax; ii++ )
    {
        fn = frame->m_ComponentLibFiles[ii];
        fn.SetExt( CompLibFileExtension );

        if( !fn.IsOk() )
            continue;

        if( !fn.FileExists() )
        {
            tmp = wxGetApp().FindLibraryPath( fn );
            if( !tmp )
            {
                libraries_not_found += fn.GetName() + _( "\n" );
                continue;
            }
        }
        else
        {
            tmp = fn.GetFullPath();
        }

        // Loaded library statusbar message
        msg = _( "Library " ) + tmp;

        if( LoadLibraryName( frame, tmp, fn.GetName() ) )
            msg += _( " loaded" );
        else
            msg += _( " error!" );

        frame->PrintMsg( msg );
    }

    /* Print the libraries not found */
    if( !libraries_not_found.IsEmpty() )
    {
        DIALOG_LOAD_ERROR dialog( frame );
        dialog.MessageSet( _( "The following libraries could not be found:" ) );
        dialog.ListSet( libraries_not_found );
        libraries_not_found.empty();
        dialog.ShowModal();
    }

    // reorder the linked list to match the order filename list:
    int NumOfLibs = 0;

    for( lib = g_LibraryList; lib != NULL; lib = lib->m_Pnext )
    {
        lib->m_Flags = 0;
        NumOfLibs++;
    }

    if( NumOfLibs == 0 )
        return;

    LibraryStruct** libs =
        (LibraryStruct**) MyZMalloc( sizeof(LibraryStruct*) * (NumOfLibs + 2) );

    int jj = 0;

    for( ii = 0; ii < frame->m_ComponentLibFiles.GetCount(); ii++ )
    {
        if( jj >= NumOfLibs )
            break;
        fn  = frame->m_ComponentLibFiles[ii];
        lib = FindLibrary( fn.GetName() );
        if( lib )
        {
            lib->m_Flags = 1;
            libs[jj++]   = lib;
        }
    }

    /* Put lib cache at end of list */
    for( lib = g_LibraryList; lib != NULL; lib = lib->m_Pnext )
    {
        if( lib->m_Flags == 0 )
            libs[jj++] = lib;
    }

    libs[jj] = NULL;

    /* Change the linked list pointers */
    for( ii = 0; libs[ii] != NULL; ii++ )
        libs[ii]->m_Pnext = libs[ii + 1];

    g_LibraryList = libs[0];

    MyFree( libs );

    for( lib = g_LibraryList; lib != NULL; lib = lib->m_Pnext )
        lib->m_Flags = 0;
}


/**************************************************************/

/** Function FreeCmpLibrary
 * Routine to remove and free a library from the current loaded libraries.
 */
/**************************************************************/
void FreeCmpLibrary( wxWindow* frame, const wxString& LibName )
{
    int            NumOfLibs = NumOfLibraries();
    LibraryStruct* Lib, * TempLib;

    if( NumOfLibs == 0 )
    {
        DisplayError( frame, wxT( "No libraries are loaded" ), 20 );
        return;
    }

    /* Search for this library name: */
    for( Lib = g_LibraryList; Lib != NULL; Lib = Lib->m_Pnext )
    {
        if( LibName == Lib->m_Name )
            break;
    }

    if( Lib == NULL )
        return;

    if( Lib == g_LibraryList )
        g_LibraryList = Lib->m_Pnext;
    else
    {
        for( TempLib = g_LibraryList; TempLib->m_Pnext != Lib;
             TempLib = TempLib->m_Pnext )
            ;

        TempLib->m_Pnext = TempLib->m_Pnext->m_Pnext;
    }

    SAFE_DELETE( Lib );

    /* The removed librairy can be the current library in libedit.
     * If so, clear the current library in libedit */
    if( Lib == CurrentLib )
        CurrentLib = NULL;
}


/** Function LibraryEntryCompare
 * Routine to compare two EDA_LibComponentStruct for the PriorQue module.
 * Comparison (insensitive  case) is based on Part name.
 */
int LibraryEntryCompare( EDA_LibComponentStruct* LE1,
                         EDA_LibComponentStruct* LE2 )
{
    return LE1->m_Name.m_Text.CmpNoCase( LE2->m_Name.m_Text );
}


/**************************************************/
/* Routine to load a library from given open file */
/**************************************************/
PriorQue* LoadLibraryAux( WinEDA_DrawFrame* frame, LibraryStruct* Library,
                          FILE* libfile, int* NumOfParts )
{
    int                     LineNum = 0;
    char                    Line[1024];
    PriorQue*               PQ = NULL;
    EDA_LibComponentStruct* LibEntry;
    wxString                msg;

    wxBusyCursor            ShowWait; // Display a Busy Cursor..

    *NumOfParts = 0;

    if( GetLine( libfile, Line, &LineNum, sizeof(Line) ) == NULL )
    {
        msg.Printf( _( "File <%s> is empty!" ),
                    (const wxChar*) Library->m_Name );
        DisplayError( frame, msg );
        return NULL;
    }

    if( strnicmp( Line, LIBFILE_IDENT, 10 ) != 0 )
    {
        msg.Printf( _( "File <%s> is NOT an EESCHEMA library!" ),
                    (const wxChar*) Library->m_Name );
        DisplayError( frame, msg );
        return NULL;
    }

    if( Library )
        Library->m_Header = CONV_FROM_UTF8( Line );

    PQInit( &PQ );
    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );

    while( GetLine( libfile, Line, &LineNum, sizeof(Line) ) )
    {
        if( strnicmp( Line, "$HEADER", 7 ) == 0 )
        {
            if( Library && !Library->ReadHeader( libfile, &LineNum ) )
            {
                msg.Printf( _( "Library <%s> header read error" ),
                            (const wxChar*) Library->m_Name );
                DisplayError( frame, msg, 30 );
            }
            continue;
        }

        if( strnicmp( Line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            LibEntry = new EDA_LibComponentStruct( NULL );

            if( LibEntry->Load( libfile, Line, &LineNum, msg ) )
            {
                /* If we are here, this part is O.k. - put it in: */
                ++*NumOfParts;
                PQInsert( &PQ, LibEntry );
                InsertAlias( &PQ, LibEntry, NumOfParts );
            }
            else
            {
                wxLogWarning( _( "Library <%s> component load error %s." ),
                              (const wxChar*) Library->m_Name,
                              (const wxChar*) msg );
                msg.Clear();
                delete LibEntry;
            }
        }
    }

    return PQ;
}


/*****************************************************************************
* Routine to find the library given its name.                            *
*****************************************************************************/
LibraryStruct* FindLibrary( const wxString& Name )
{
    LibraryStruct* Lib = g_LibraryList;

    while( Lib )
    {
        if( Lib->m_Name == Name )
            return Lib;
        Lib = Lib->m_Pnext;
    }

    return NULL;
}


/*****************************************************************************
* Routine to find the number of libraries currently loaded.              *
*****************************************************************************/
int NumOfLibraries()
{
    int            ii;
    LibraryStruct* Lib = g_LibraryList;

    for( ii = 0; Lib != NULL; Lib = Lib->m_Pnext )
        ii++;

    return ii;
}


/********************************************************************/
/* create in library (in list PQ) aliases of the "root" component LibEntry*/
/********************************************************************/
static void InsertAlias( PriorQue** PQ, EDA_LibComponentStruct* LibEntry,
                         int* NumOfParts )
{
    EDA_LibCmpAliasStruct* AliasEntry;
    unsigned ii;

    for( ii = 0; ii < LibEntry->m_AliasList.GetCount(); ii++ )
    {
        AliasEntry =
            new EDA_LibCmpAliasStruct( LibEntry->m_AliasList[ii],
                                      LibEntry->m_Name.m_Text.GetData() );

        ++*NumOfParts;
        PQInsert( PQ, AliasEntry );
    }
}


/*******************************************************/
/* Routines de lecture des Documentation de composants */
/*******************************************************/

/* Routine to load a library from given open file.*/
int LoadDocLib( WinEDA_DrawFrame* frame, const wxString& FullDocLibName,
                const wxString& Libname )
{
    int      LineNum = 0;
    char     Line[1024], * Name, * Text;
    EDA_LibComponentStruct* Entry;
    FILE*    f;
    wxString msg;

    f = wxFopen( FullDocLibName, wxT( "rt" ) );
    if( f == NULL )
        return 0;

    if( GetLine( f, Line, &LineNum, sizeof(Line) ) == NULL )
    {
        /* pas de lignes utiles */
        fclose( f );
        return 0;
    }

    if( strnicmp( Line, DOCFILE_IDENT, 10 ) != 0 )
    {
        DisplayError( frame, wxT( "File is NOT EESCHEMA doclib!" ) );
        fclose( f );
        return 0;
    }

    while( GetLine( f, Line, &LineNum, sizeof(Line) ) )
    {
        if( strncmp( Line, "$CMP", 4 ) != 0 )
        {
            msg.Printf( wxT( "$CMP command expected in line %d, aborted." ),
                        LineNum );
            DisplayError( frame, msg );
            fclose( f );
            return 0;
        }

        /* Read one $CMP/$ENDCMP part entry from library: */
        Name = strtok( Line + 5, "\n\r" );
        wxString cmpname;
        cmpname = CONV_FROM_UTF8( Name );
        Entry   = FindLibPart( cmpname.GetData(), Libname, FIND_ALIAS );
        while( GetLine( f, Line, &LineNum, sizeof(Line) ) )
        {
            if( strncmp( Line, "$ENDCMP", 7 ) == 0 )
                break;
            Text = strtok( Line + 2, "\n\r" );

            switch( Line[0] )
            {
            case 'D':
                if( Entry )
                    Entry->m_Doc = CONV_FROM_UTF8( Text );
                break;

            case 'K':
                if( Entry )
                    Entry->m_KeyWord = CONV_FROM_UTF8( Text );
                break;

            case 'F':
                if( Entry )
                    Entry->m_DocFile = CONV_FROM_UTF8( Text );
                break;
            }
        }
    }

    fclose( f );
    return 1;
}
