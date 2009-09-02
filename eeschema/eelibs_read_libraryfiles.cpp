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


/**
 * Function LoadLibraryName
 *
 * Routine to load the given library name. FullLibName should hold full path
 * of file name to open, while LibName should hold only its name.
 * If library already exists, it is NOT reloaded.
 *
 * @return : new lib or NULL
 */
LibraryStruct* LoadLibraryName( WinEDA_DrawFrame* frame,
                                const wxString&   FullLibName,
                                const wxString&   LibName )
{
    LibraryStruct* NewLib;
    wxFileName     fn;
    wxString       errMsg;

    if( ( NewLib = FindLibrary( LibName ) ) != NULL )
    {
        if( NewLib->m_FullFileName == FullLibName )
            return NewLib;

        delete NewLib;
    }

    NewLib = new LibraryStruct( LIBRARY_TYPE_EESCHEMA, LibName, FullLibName );

    wxBusyCursor ShowWait;

    if( NewLib->Load( errMsg ) )
    {
        if( g_LibraryList == NULL )
            g_LibraryList = NewLib;
        else
        {
            LibraryStruct* tmplib = g_LibraryList;
            while( tmplib->m_Pnext )
                tmplib = tmplib->m_Pnext;

            tmplib->m_Pnext = NewLib;
        }

        NewLib->LoadDocs( errMsg );
    }
    else
    {
        wxString msg;
        msg.Printf( _( "Error <%s> occurred attempting to load component \
library <%s>" ),
                    ( const wxChar* ) errMsg,
                    ( const wxChar* ) FullLibName );
        DisplayError( frame, msg );
        SAFE_DELETE( NewLib );
    }

    return NewLib;
}


/**
 * Function LoadLibraries
 *
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


/**
 * Function FreeCmpLibrary
 *
 * Routine to remove and free a library from the current loaded libraries.
 */
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


/**
 * Function LibraryEntryCompare
 *
 * Routine to compare two EDA_LibComponentStruct for the PriorQue module.
 * Comparison (insensitive  case) is based on Part name.
 */
int LibraryEntryCompare( EDA_LibComponentStruct* LE1,
                         EDA_LibComponentStruct* LE2 )
{
    return LE1->m_Name.m_Text.CmpNoCase( LE2->m_Name.m_Text );
}


/*****************************************************************************
 * Routine to find the library given its name.
 *****************************************************************************/
LibraryStruct* FindLibrary( const wxString& Name )
{
    LibraryStruct* Lib = g_LibraryList;

    while( Lib )
    {
        if( *Lib == Name )
            return Lib;
        Lib = Lib->m_Pnext;
    }

    return NULL;
}


/*****************************************************************************
 * Routine to find the number of libraries currently loaded.
 *****************************************************************************/
int NumOfLibraries()
{
    int            ii;
    LibraryStruct* Lib = g_LibraryList;

    for( ii = 0; Lib != NULL; Lib = Lib->m_Pnext )
        ii++;

    return ii;
}
