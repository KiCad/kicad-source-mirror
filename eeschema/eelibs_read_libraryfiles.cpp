/*****************************************************************/
/*	Functions to handle component library files : read functions */
/*****************************************************************/

#include <iostream>

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"

/* Local Functions */
static LibEDA_BaseStruct* ReadDrawEntryItemDescription( EDA_LibComponentStruct* aParent, FILE* f,
                                        char* Line, int* LineNum );
static bool AddAliasNames( EDA_LibComponentStruct* LibEntry, char* line );
static void InsertAlias( PriorQue** PQ, EDA_LibComponentStruct* LibEntry,
                         int* NumOfParts );
static int AddFootprintFilterList( EDA_LibComponentStruct* LibEntryLibEntry,
                                   FILE* f, char* Line, int* LineNum );


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
                                const wxString& FullLibName,
                                const wxString& LibName )
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
    wxFileName fn;
    wxString msg, tmp;
    wxString libraries_not_found;
    unsigned ii, iimax = frame->m_ComponentLibFiles.GetCount();

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
		libraries_not_found += fn.GetName() + _("\n");
                continue;
            }
        }
        else
        {
            tmp = fn.GetFullPath();
        }

        // Loaded library statusbar message
        msg = _( "Library " ) + tmp;
        frame->PrintMsg( msg );

        if( LoadLibraryName( frame, tmp, fn.GetName() ) )
            msg += _( " loaded" );
        else
            msg += _( " error!" );

        frame->PrintMsg( msg );
    }

	/* Print the libraries not found */
	if( !libraries_not_found.IsEmpty() )
	{
		wxString message = _("The following libraries could not be found:");
		DIALOG_LOAD_ERROR *dialog = new DIALOG_LOAD_ERROR(NULL);
		dialog->Show();
		dialog->MessageSet(&message);
		dialog->ListSet(&libraries_not_found);
		libraries_not_found = wxT("");
	}



    // reorder the linked list to match the order filename list:
    int            NumOfLibs;
    for( NumOfLibs = 0, lib = g_LibraryList; lib != NULL; lib = lib->m_Pnext )
    {
        lib->m_Flags = 0;
        NumOfLibs++;
    }

    if( NumOfLibs == 0 )
        return;

    LibraryStruct** libs =
        (LibraryStruct**) MyZMalloc( sizeof(LibraryStruct*) * (NumOfLibs + 2) );

    int             jj = 0;
    for( ii = 0; ii < frame->m_ComponentLibFiles.GetCount(); ii++ )
    {
        if( jj >= NumOfLibs )
            break;
        fn = frame->m_ComponentLibFiles[ii];
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
void FreeCmpLibrary (wxWindow* frame, const wxString& LibName)
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
int LibraryEntryCompare (EDA_LibComponentStruct* LE1,
                         EDA_LibComponentStruct* LE2)
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
        msg = _( "File <" ) + Library->m_Name + _( "> is empty!" );
        DisplayError( frame, msg );
        return NULL;
    }

    if( strnicmp( Line, LIBFILE_IDENT, 10 ) != 0 )
    {
        msg = _( "File <" ) + Library->m_Name +
            _( "> is NOT EESCHEMA library!" );
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
                msg = _( "Library <" ) + Library->m_Name +
                    _( "> header read error" );
                DisplayError( frame, msg, 30 );
            }
            continue;
        }

        if( strnicmp( Line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            LibEntry = Read_Component_Definition( frame, Line, libfile,
                                                  &LineNum );
            if( LibEntry )
            {
                /* If we are here, this part is O.k. - put it in: */
                ++ * NumOfParts;
                PQInsert( &PQ, LibEntry );
                InsertAlias( &PQ, LibEntry, NumOfParts );
            }
        }
    }

    return PQ;
}


/*****************************************************************************/
/* Analyse la ligne de description du champ de la forme:
 *  Fn "CA3130" 150 -200 50 H V
 *  ou n = 0 (REFERENCE), 1 (VALUE) , 2 .. 11 = autres champs, facultatifs
 */
/*****************************************************************************/
static bool GetLibEntryField ( EDA_LibComponentStruct* LibEntry, char* line,
                               wxString& errorMsg )
{
    LibDrawField* field = new LibDrawField();

    if ( !field->Load( line, errorMsg ) )
    {
        SAFE_DELETE( field );
        return false;
    }

    switch( field->m_FieldId )
    {
    case REFERENCE:
        LibEntry->m_Prefix = *field;
        SAFE_DELETE( field );
        break;

    case VALUE:
        LibEntry->m_Name = *field;
        SAFE_DELETE( field );
        break;

    default:
        LibEntry->m_Fields.PushBack( field );
        break;
    }

    return true;
}


/*****************************************************************************/
/* Routine to Read a DEF/ENDDEF part entry from given open file.
 */
/*****************************************************************************/
EDA_LibComponentStruct* Read_Component_Definition( WinEDA_DrawFrame* frame,
                                                   char* Line,
                                                   FILE* f,
                                                   int* LineNum )
{
    int      unused;
    char*    p;
    char*    name;
    char*    prefix = NULL;

    EDA_LibComponentStruct* LibEntry = NULL;
    bool     Res;
    wxString Msg, errorMsg;

    p = strtok( Line, " \t\r\n" );

    if( strcmp( p, "DEF" ) != 0 )
    {
        Msg.Printf( wxT( "DEF command expected in line %d, aborted." ),
                    *LineNum );
        DisplayError( frame, Msg );
        return NULL;
    }

    /* Read DEF line: */
    char drawnum = 0;
    char drawname = 0;

    LibEntry = new EDA_LibComponentStruct( NULL );

    if( ( name = strtok( NULL, " \t\n" ) ) == NULL      /* Part name: */
        || ( prefix = strtok( NULL, " \t\n" ) ) == NULL  /* Prefix name: */
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* NumOfPins: */
        || sscanf( p, "%d", &unused ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* TextInside: */
        || sscanf( p, "%d", &LibEntry->m_TextInside ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* DrawNums: */
        || sscanf( p, "%c", &drawnum ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* DrawNums: */
        || sscanf( p, "%c", &drawname ) != 1
        || ( p = strtok( NULL, " \t\n" ) ) == NULL       /* m_UnitCount: */
        || sscanf( p, "%d", &LibEntry->m_UnitCount ) != 1 )
    {
        Msg.Printf( wxT( "Wrong DEF format in line %d, skipped." ), *LineNum );
        DisplayError( frame, Msg );
        while( GetLine( f, Line, LineNum, 1024 ) )
        {
            p = strtok( Line, " \t\n" );
            if( stricmp( p, "ENDDEF" ) == 0 )
                break;
        }

        return NULL;
    }
    else    /* Update infos read from the line "DEF" */
    {
        LibEntry->m_DrawPinNum  = (drawnum == 'N') ? FALSE : TRUE;
        LibEntry->m_DrawPinName = (drawname == 'N') ? FALSE : TRUE;

        /* Copy part name and prefix. */
        strupper( name );
        if( name[0] != '~' )
            LibEntry->m_Name.m_Text = CONV_FROM_UTF8( name );
        else
        {
            LibEntry->m_Name.m_Text       = CONV_FROM_UTF8( &name[1] );
            LibEntry->m_Name.m_Attributs |= TEXT_NO_VISIBLE;
        }

        if( strcmp( prefix, "~" ) == 0 )
        {
            LibEntry->m_Prefix.m_Text.Empty();
            LibEntry->m_Prefix.m_Attributs |= TEXT_NO_VISIBLE;
        }
        else
            LibEntry->m_Prefix.m_Text = CONV_FROM_UTF8( prefix );

        // Copy optional infos
        // m_UnitSelectionLocked param
        if( ( p = strtok( NULL, " \t\n" ) ) != NULL )
        {
            if( *p == 'L' )
                LibEntry->m_UnitSelectionLocked = TRUE;
        }
        if( ( p = strtok( NULL, " \t\n" ) ) != NULL )     /* Type Of Component */
        {
            if( *p == 'P' )
                LibEntry->m_Options = ENTRY_POWER;
        }
    }

    /* Read next lines */
    while( GetLine( f, Line, LineNum, 1024 ) )
    {
        p = strtok( Line, " \t\n" );

        /* This is the error flag ( if an error occurs, Res = FALSE) */
        Res = TRUE;

        if( (Line[0] == 'T') && (Line[1] == 'i') )
        {
            Res = LibEntry->LoadDateAndTime( Line );
        }
        else if( Line[0] == 'F' )
        {
            Res = GetLibEntryField( LibEntry, Line, errorMsg );
        }
        else if( strcmp( p, "ENDDEF" ) == 0 )
        {
            p = strtok( Line, " \t\n" );
            break;
        }
        else if( strcmp( p, "DRAW" ) == 0 )
        {
            LibEntry->m_Drawings = ReadDrawEntryItemDescription( LibEntry, f, Line, LineNum );
        }
        else if( strncmp( p, "ALIAS", 5 ) == 0 )
        {
            p = strtok( NULL, "\r\n" );
            Res = AddAliasNames( LibEntry, p );
        }
        else if( strncmp( p, "$FPLIST", 5 ) == 0 )
        {
            Res = AddFootprintFilterList( LibEntry, f, Line, LineNum );
        }
        else
        {
            Msg.Printf( wxT( "Undefined command \"%s\" in line %d, skipped." ),
                        p, *LineNum );
            frame->PrintMsg( Msg );
        }

        /* End line or block analysis: test for an error */
        if( !Res )
        {           /* Something went wrong there. */
            if( errorMsg.IsEmpty() )
                Msg.Printf( wxT( "Error at line %d of library \n\"%s\",\nlibrary not loaded" ),
                            *LineNum, currentLibraryName.GetData() );
            else
                Msg.Printf( wxT( "Error <%s> at line %d of library \n\"%s\",\nlibrary not loaded" ),
                            errorMsg.c_str(), *LineNum,
                            currentLibraryName.GetData() );
            DisplayError( frame, Msg );
            SAFE_DELETE( LibEntry );
            return NULL;
        }
    }

    /* If we are here, this part is O.k. - put it in: */
    LibEntry->SortDrawItems();
    return LibEntry;
}


/*****************************************************************************
* Routine to load a DRAW definition from given file. Note "DRAW" line has	 *
* been read already. Reads upto and include ENDDRAW, or an error (NULL ret). *
*****************************************************************************/

static LibEDA_BaseStruct* ReadDrawEntryItemDescription (EDA_LibComponentStruct* aParent, FILE* f,
                                        char* Line, int* LineNum)
{
    wxString           MsgLine, errorMsg;
    bool               entryLoaded;
    LibEDA_BaseStruct* Tail = NULL;
    LibEDA_BaseStruct* New = NULL;
    LibEDA_BaseStruct* Head = NULL;

    while( TRUE )
    {
        if( GetLine( f, Line, LineNum, 1024 ) == NULL )
        {
            DisplayError( NULL, wxT( "File ended prematurely" ) );
            return Head;
        }

        if( strncmp( Line, "ENDDRAW", 7 ) == 0 )
        {
            break;
        }

        New = NULL;

        switch( Line[0] )
        {
        case 'A':    /* Arc */
            New = ( LibEDA_BaseStruct* ) new LibDrawArc(aParent);
            entryLoaded = New->Load( Line, errorMsg );
            break;

        case 'C':    /* Circle */
            New = ( LibEDA_BaseStruct* ) new LibDrawCircle(aParent);
            entryLoaded = New->Load( Line, errorMsg );
            break;

        case 'T':    /* Text */
            New = ( LibEDA_BaseStruct* ) new LibDrawText(aParent);
            entryLoaded = New->Load( Line, errorMsg );
            break;

        case 'S':    /* Square */
            New = ( LibEDA_BaseStruct* ) new LibDrawSquare(aParent);
            entryLoaded = New->Load( Line, errorMsg );
            break;

        case 'X':    /* Pin Description */
            New = ( LibEDA_BaseStruct* ) new LibDrawPin(aParent);
            entryLoaded = New->Load( Line, errorMsg );
            break;

        case 'P':    /* Polyline */
            New = ( LibEDA_BaseStruct* ) new LibDrawPolyline(aParent);
            entryLoaded = New->Load( Line, errorMsg );
            break;

        case 'B':    /* Bezier */
            New = ( LibEDA_BaseStruct* ) new LibDrawBezier(aParent);
            entryLoaded = New->Load( Line, errorMsg );
            break;

        default:
            MsgLine.Printf( wxT( "Undefined DRAW command in line %d\n%s, aborted." ),
                            *LineNum, Line );
            DisplayError( NULL, MsgLine );
            return Head;
        }

        if( !entryLoaded )
        {
            MsgLine.Printf( wxT( "Error <%s %s> in DRAW command %c in line %d, aborted." ),
                            errorMsg.c_str(), MsgLine.c_str(),
                            Line[0], *LineNum );
            DisplayError( NULL, MsgLine );
            SAFE_DELETE( New );

            /* FLush till end of draw: */
            do
            {
                if( GetLine( f, Line, LineNum, 1024 ) == NULL )
                {
                    DisplayError( NULL, wxT( "File ended prematurely" ) );
                    return Head;
                }
            } while( strncmp( Line, "ENDDRAW", 7 ) != 0 );

            return Head;
        }
        else
        {
            if( Head == NULL )
                Head = Tail = New;
            else
            {
                Tail->SetNext( New );
                Tail = New;
            }
        }
    }

    return Head;
}


/*****************************************************************************
* Routine to find the library given its name.		                     *
*****************************************************************************/
LibraryStruct* FindLibrary (const wxString& Name)
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
* Routine to find the number of libraries currently loaded.	             *
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
/* Read the alias names (in buffer line) and add them in alias list
 *  names are separated by spaces
 */
/********************************************************************/
static bool AddAliasNames (EDA_LibComponentStruct* LibEntry, char* line )
{
    char*    text;
    wxString name;

    text = strtok( line, " \t\r\n" );

    while( text )
    {
        name = CONV_FROM_UTF8( text );
        LibEntry->m_AliasList.Add( name );
        text = strtok( NULL, " \t\r\n" );
    }

    return TRUE;
}


/********************************************************************/
/* create in library (in list PQ) aliases of the "root" component LibEntry*/
/********************************************************************/
static void InsertAlias (PriorQue** PQ, EDA_LibComponentStruct* LibEntry,
                         int* NumOfParts)
{
    EDA_LibCmpAliasStruct* AliasEntry;
    unsigned ii;

    if( LibEntry->m_AliasList.GetCount() == 0 )
        return; /* No alias for this component */

    for( ii = 0; ii < LibEntry->m_AliasList.GetCount(); ii++ )
    {
        AliasEntry =
            new EDA_LibCmpAliasStruct( LibEntry->m_AliasList[ii],
                                       LibEntry->m_Name.m_Text.GetData() );

        ++ * NumOfParts;
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
        Entry = FindLibPart( cmpname.GetData(), Libname, FIND_ALIAS );
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


/*****************************************************************************/
/* read the FootprintFilter List stating with:
 *  FPLIST
 *  and ending with:
 *  ENDFPLIST
 */
/*****************************************************************************/
int AddFootprintFilterList(EDA_LibComponentStruct* LibEntryLibEntry,
                           FILE* f, char* Line, int* LineNum)
{
    for( ; ; )
    {
        if( GetLine( f, Line, LineNum, 1024 ) == NULL )
        {
            DisplayError( NULL, wxT( "File ended prematurely" ) );
            return 0;
        }

        if( stricmp( Line, "$ENDFPLIST" ) == 0 )
        {
            break;  /*normal exit on end of list */
        }

        LibEntryLibEntry->m_FootprintList.Add( CONV_FROM_UTF8( Line + 1 ) );
    }

    return 1;
}
