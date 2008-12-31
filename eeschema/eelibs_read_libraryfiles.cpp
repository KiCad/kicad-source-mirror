/*****************************************************************/
/*	Functions to handle component library files : read functions */
/*****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "trigo.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/* Local Functions */
static LibEDA_BaseStruct*   GetDrawEntry( WinEDA_DrawFrame* frame, FILE* f,
                                          char* Line, int* LineNum );
static bool                 GetLibEntryField( EDA_LibComponentStruct* LibEntry, char* line );
static bool                 AddAliasNames( EDA_LibComponentStruct* LibEntry, char* line );
static void                 InsertAlias( PriorQue** PQ,
                                         EDA_LibComponentStruct* LibEntry, int* NumOfParts );
static bool                 ReadLibEntryDateAndTime( EDA_LibComponentStruct* LibEntry, char* Line );
static int                  AddFootprintFilterList( EDA_LibComponentStruct* LibEntryLibEntry,
                                                    FILE* f, char* Line, int* LineNum );



static wxString currentLibraryName;     // If this code was written in C++ then this would not be needed.



/*************************************************************************************/
LibraryStruct* LoadLibraryName( WinEDA_DrawFrame* frame,
                                const wxString& FullLibName,
                                const wxString& LibName )
/*************************************************************************************/

/** Function LoadLibraryName
 * Routine to load the given library name. FullLibName should hold full path
 * of file name to open, while LibName should hold only its name.
 * IF library already exists, it is NOT reloaded.
 * @return : new lib or NULL
 */
{
    int            NumOfParts;
    FILE*          f;
    LibraryStruct* NewLib;
    PriorQue*      Entries;
    wxString       FullFileName;

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

        FullFileName = FullLibName;
        ChangeFileNameExt( FullFileName, DOC_EXT );
        LoadDocLib( frame, FullFileName, NewLib->m_Name );
    }
    else{
        SAFE_DELETE( NewLib );
    }
    fclose( f );
    return NewLib;
}


/******************************************/
void LoadLibraries (WinEDA_DrawFrame* frame)
/******************************************/

/* Function LoadLibraries
 * Clear all alredy loaded librries and load all librairies
 * given in g_LibName_List
 */
{
    wxString FullLibName, msg;
    wxString LibName;
    unsigned ii, iimax = g_LibName_List.GetCount();

    frame->PrintMsg( _( "Start loading schematic libs" ) );

    // Free the unwanted libraries (i.e. not in list) but keep the .cache lib
    LibraryStruct* nextlib, * lib = g_LibraryList;
    for( ; lib != NULL; lib = nextlib )
    {
        nextlib = lib->m_Pnext;
        if( lib->m_IsLibCache )
            continue;

        wxString libname = lib->m_Name;

        // is this library in "wanted list" g_LibName_List ?
        int      test = g_LibName_List.Index( libname );
        if( test == wxNOT_FOUND )
            FreeCmpLibrary( frame, libname );
    }

    // Load missing libraries (if any)
    for( ii = 0; ii < iimax; ii++ )
    {
        LibName = g_LibName_List[ii];

        if( LibName.IsEmpty() )
            continue;

        FullLibName = MakeFileName( g_RealLibDirBuffer, LibName, g_LibExtBuffer );

        // Loaded library statusbar message
        msg = _( "Library " ) + FullLibName;
        frame->PrintMsg( msg );

        if( LoadLibraryName( frame, FullLibName, LibName ) )
            msg += _( " loaded" );
        else
            msg += _( " error!" );

        frame->PrintMsg( msg );
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
    for( ii = 0; ii < g_LibName_List.GetCount(); ii++ )
    {
        if( jj >= NumOfLibs )
            break;
        lib = FindLibrary( g_LibName_List[ii] );
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
void FreeCmpLibrary (wxWindow* frame, const wxString& LibName)
/**************************************************************/

/** Function FreeCmpLibrary
 * Routine to remove and free a library from the current loaded libraries.
 */
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
        for( TempLib = g_LibraryList; TempLib->m_Pnext != Lib; TempLib = TempLib->m_Pnext )
            ;

        TempLib->m_Pnext = TempLib->m_Pnext->m_Pnext;
    }

    SAFE_DELETE( Lib );

    /* The removed librairy can be the current library in libedit.
      * If so, clear the current library in libedit */
    if( Lib == CurrentLib )
        CurrentLib = NULL;
}


/******************************/
const wxChar** GetLibNames()
/******************************/

/** GetLibNames()
 * Routine to return pointers to all library names.
 *  User is responsible to deallocate memory
 */
{
    int            ii, NumOfLibs = NumOfLibraries();
    const wxChar** Names;
    LibraryStruct* Lib;

    Names = (const wxChar**) MyZMalloc( sizeof(wxChar*) * (NumOfLibs + 1) );
    for( ii = 0, Lib = g_LibraryList; Lib != NULL; Lib = Lib->m_Pnext, ii++ )
    {
        Names[ii] = Lib->m_Name.GetData();
    }

    Names[ii] = NULL;

    return Names;
}


/** Function LibraryEntryCompare
 * Routine to compare two EDA_LibComponentStruct for the PriorQue module.
 * Comparison (insensitive  case) is based on Part name.
 */
int LibraryEntryCompare (EDA_LibComponentStruct* LE1, EDA_LibComponentStruct* LE2)
{
    return LE1->m_Name.m_Text.CmpNoCase( LE2->m_Name.m_Text );
}


/**************************************************/
/* Routine to load a library from given open file */
/**************************************************/
PriorQue* LoadLibraryAux( WinEDA_DrawFrame* frame,
                          LibraryStruct* Library,
                          FILE* libfile,
                          int* NumOfParts )
/**************************************************/
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
        msg = _( "File <" ) + Library->m_Name + _( "> is NOT EESCHEMA library!" );
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
            if( Library )
            {
                if( !Library->ReadHeader( libfile, &LineNum ) )
                {
                    msg = _( "Library <" ) + Library->m_Name + _( "> header read error" );
                    DisplayError( frame, msg, 30 );
                }
            }
            continue;
        }

        if( strnicmp( Line, "DEF", 3 ) == 0 )
        {
            /* Read one DEF/ENDDEF part entry from library: */
            LibEntry = Read_Component_Definition( frame, Line, libfile, &LineNum );
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


/*********************************************************************************************/
EDA_LibComponentStruct* Read_Component_Definition( WinEDA_DrawFrame* frame,
                                                   char* Line,
                                                   FILE* f,
                                                   int* LineNum )
/*********************************************************************************************/

/* Routine to Read a DEF/ENDDEF part entry from given open file.
 */
{
    int      unused;
    char*    p;
    char*    name;
    char*    prefix = NULL;

    EDA_LibComponentStruct* LibEntry = NULL;
    bool     Res;
    wxString Msg;

    p = strtok( Line, " \t\r\n" );

    if( strcmp( p, "DEF" ) != 0 )
    {
        Msg.Printf( wxT( "DEF command expected in line %d, aborted." ), *LineNum );
        DisplayError( frame, Msg );
        return NULL;
    }

    /* Read DEF line: */
    char drawnum = 0, drawname = 0;
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
        if( ( p = strtok( NULL, " \t\n" ) ) != NULL ) // m_UnitSelectionLocked param
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
        p   = strtok( Line, " \t\n" );
        Res = TRUE; /* This is the error flag ( if an error occurs, Res = FALSE) */

        if( (Line[0] == 'T') && (Line[1] == 'i') )
        {
            Res = ReadLibEntryDateAndTime( LibEntry, Line );
        }
        else if( Line[0] == 'F' )
        {
            Res = GetLibEntryField( LibEntry, Line );
        }
        else if( strcmp( p, "ENDDEF" ) == 0 )
        {
            break;
        }
        else if( strcmp( p, "DRAW" ) == 0 )
        {
            LibEntry->m_Drawings = GetDrawEntry( frame, f, Line, LineNum );
        }
        else if( strncmp( p, "ALIAS", 5 ) == 0 )
        {
            p   = strtok( NULL, "\r\n" );
            Res = AddAliasNames( LibEntry, p );
        }
        else if( strncmp( p, "$FPLIST", 5 ) == 0 )
        {
            Res = AddFootprintFilterList( LibEntry, f, Line, LineNum );
        }
        else
        {
            Msg.Printf( wxT( "Undefined command \"%s\" in line %d, skipped." ), p, *LineNum );
            frame->PrintMsg( Msg );
        }

        /* End line or block analysis: test for an error */
        if( !Res )
        {           /* Something went wrong there. */
            Msg.Printf( wxT( " Error at line %d of library \n\"%s\",\nlibrary not loaded" ),
                       *LineNum, currentLibraryName.GetData() );
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

static
LibEDA_BaseStruct* GetDrawEntry (WinEDA_DrawFrame* frame,
                                 FILE* f,
                                 char* Line,
                                 int* LineNum)
{
    int                i = 0, jj, ll, Unit, Convert, size1, size2;
    char*              p, Buffer[1024], BufName[256],
                       PinNum[256],
                       chartmp[256], chartmp1[256];
    wxString           MsgLine;
    bool               Error = FALSE;
    LibEDA_BaseStruct* Tail  = NULL,
    * New  = NULL,
    * Head = NULL;

    while( TRUE )
    {
        if( GetLine( f, Line, LineNum, 1024 ) == NULL )
        {
            DisplayError( frame, wxT( "File ended prematurely" ) );
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
        {
            int         startx, starty, endx, endy;
            LibDrawArc* Arc = new LibDrawArc();

            New = Arc;
            ll  = 0;
            int nbarg = sscanf( &Line[2], "%d %d %d %d %d %d %d %d %s %d %d %d %d",
                                &Arc->m_Pos.x, &Arc->m_Pos.y, &Arc->m_Rayon,
                                &Arc->t1, &Arc->t2, &Unit, &Convert,
                                &Arc->m_Width, chartmp, &startx, &starty, &endx, &endy );
            if( nbarg < 8 )
                Error = TRUE;

            Arc->m_Unit    = Unit;
            Arc->m_Convert = Convert;

            if( chartmp[0] == 'F' )
                Arc->m_Fill = FILLED_SHAPE;
            if( chartmp[0] == 'f' )
                Arc->m_Fill = FILLED_WITH_BG_BODYCOLOR;

            NORMALIZE_ANGLE( Arc->t1 );
            NORMALIZE_ANGLE( Arc->t2 );

            if( nbarg >= 13 )      // Actual Coordinates of arc ends are read from file
            {
                Arc->m_ArcStart.x = startx; Arc->m_ArcStart.y = starty;
                Arc->m_ArcEnd.x   = endx; Arc->m_ArcEnd.y = endy;
            }
            else      // Actual Coordinates of arc ends are not read from file (old library), calculate them
            {
                Arc->m_ArcStart.x = Arc->m_Rayon; Arc->m_ArcStart.y = 0;
                Arc->m_ArcEnd.x   = Arc->m_Rayon; Arc->m_ArcEnd.y = 0;
                RotatePoint( &Arc->m_ArcStart.x, &Arc->m_ArcStart.y, -Arc->t1 );
                Arc->m_ArcStart.x += Arc->m_Pos.x; Arc->m_ArcStart.y += Arc->m_Pos.y;
                RotatePoint( &Arc->m_ArcEnd.x, &Arc->m_ArcEnd.y, -Arc->t2 );
                Arc->m_ArcEnd.x += Arc->m_Pos.x; Arc->m_ArcEnd.y += Arc->m_Pos.y;
            }
        }
            break;

        case 'C':    /* Circle */
        {
            LibDrawCircle* Circle = new LibDrawCircle();

            New   = Circle; ll = 0;
            Error = sscanf( &Line[2], "%d %d %d %d %d %d %s",
                            &Circle->m_Pos.x, &Circle->m_Pos.y, &Circle->m_Rayon,
                            &Unit, &Convert, &Circle->m_Width, chartmp ) < 6;
            Circle->m_Unit    = Unit;
            Circle->m_Convert = Convert;
            if( chartmp[0] == 'F' )
                Circle->m_Fill = FILLED_SHAPE;
            if( chartmp[0] == 'f' )
                Circle->m_Fill = FILLED_WITH_BG_BODYCOLOR;
        }
            break;

        case 'T':    /* Text */
        {
            LibDrawText* Text = new LibDrawText();

            New = Text;
            Buffer[0] = 0;
			chartmp[0] = 0;			// For italic option, Not in old versions
			int thickness = 0;		// Not in old versions
            Error = sscanf( &Line[2], "%d %d %d %d %d %d %d %s %s %d",
                            &Text->m_Orient,
                            &Text->m_Pos.x, &Text->m_Pos.y,
                            &Text->m_Size.x, &Text->m_Attributs,
                            &Unit, &Convert, Buffer, chartmp, &thickness ) < 8;

            Text->m_Unit   = Unit; Text->m_Convert = Convert;
            Text->m_Size.y = Text->m_Size.x;
			if ( strnicmp(chartmp, "Italic", 6) == 0 )
				Text->m_Italic = true;

			Text->m_Width = thickness;

            if( !Error )
            {                                                   /* Convert '~' to spaces. */
                Text->m_Text = CONV_FROM_UTF8( Buffer );
                Text->m_Text.Replace( wxT( "~" ), wxT( " " ) ); // Les espaces sont restitu�s
            }
        }
            break;

        case 'S':    /* Square */
        {
            LibDrawSquare* Square = new LibDrawSquare();

            New   = Square; ll = 0;
            Error = sscanf( &Line[2], "%d %d %d %d %d %d %d %s",
                            &Square->m_Pos.x, &Square->m_Pos.y,
                            &Square->m_End.x, &Square->m_End.y,
                            &Unit, &Convert, &Square->m_Width, chartmp ) < 7;
            Square->m_Unit = Unit; Square->m_Convert = Convert;
            if( chartmp[0] == 'F' )
                Square->m_Fill = FILLED_SHAPE;
            if( chartmp[0] == 'f' )
                Square->m_Fill = FILLED_WITH_BG_BODYCOLOR;
        }
            break;

        case 'X':    /* Pin Description */
        {
            *Buffer = 0;
            LibDrawPin* Pin = new LibDrawPin();

            New = Pin;
            i   = sscanf( Line + 2, "%s %s %d %d %d %s %d %d %d %d %s %s",
                          BufName, PinNum,
                          &Pin->m_Pos.x, &Pin->m_Pos.y,
                          &ll, chartmp1,
                          &size1, &size2,
                          &Unit, &Convert, chartmp, Buffer );

            Pin->m_PinNumSize  = size1;         /* Parametres type short */
            Pin->m_PinNameSize = size2;
            Pin->m_PinLen = ll;
            Pin->m_Orient = chartmp1[0] & 255;

            Pin->m_Unit    = Unit;
            Pin->m_Convert = Convert;

            strncpy( (char*) &Pin->m_PinNum, PinNum, 4 );

            Error = (i != 11 && i != 12);

            Pin->m_PinName = CONV_FROM_UTF8( BufName );

            jj = *chartmp & 255;

            switch( jj )
            {
            case 'I':
                Pin->m_PinType = PIN_INPUT; break;

            case 'O':
                Pin->m_PinType = PIN_OUTPUT; break;

            case 'B':
                Pin->m_PinType = PIN_BIDI; break;

            case 'T':
                Pin->m_PinType = PIN_TRISTATE; break;

            case 'P':
                Pin->m_PinType = PIN_PASSIVE; break;

            case 'U':
                Pin->m_PinType = PIN_UNSPECIFIED; break;

            case 'W':
                Pin->m_PinType = PIN_POWER_IN; break;

            case 'w':
                Pin->m_PinType = PIN_POWER_OUT; break;

            case 'C':
                Pin->m_PinType = PIN_OPENCOLLECTOR; break;

            case 'E':
                Pin->m_PinType = PIN_OPENEMITTER; break;

            default:
                MsgLine.Printf( wxT( "Unknown Pin Type [%c] line %d" ),
                                jj, *LineNum );
                DisplayError( frame, MsgLine );
            }

            if( i == 12 )       /* Special Symbole defined */
            {
                for( jj = strlen( Buffer ); jj > 0; )
                {
                    switch( Buffer[--jj] )
                    {
                    case '~':
                        break;

                    case 'N':
                        Pin->m_Attributs |= PINNOTDRAW; break;

                    case 'I':
                        Pin->m_PinShape |= INVERT; break;

                    case 'C':
                        Pin->m_PinShape |= CLOCK; break;

                    case 'L':
                        Pin->m_PinShape |= LOWLEVEL_IN; break;

                    case 'V':
                        Pin->m_PinShape |= LOWLEVEL_OUT; break;

                    default:
                        MsgLine.Printf( wxT( "Unknown Pin Shape [%c] line %d" ),
                                        Buffer[jj], *LineNum );
                        DisplayError( frame, MsgLine ); break;
                    }
                }
            }
        }
            break;

        case 'P':    /* Polyline */
        {
            LibDrawPolyline* Polyl = new LibDrawPolyline();

            New = Polyl;

            if( sscanf( &Line[2], "%d %d %d %d",
                        &Polyl->m_CornersCount, &Unit, &Convert,
                        &Polyl->m_Width ) == 4
                && Polyl->m_CornersCount > 0 )
            {
                Polyl->m_Unit = Unit; Polyl->m_Convert = Convert;

                Polyl->m_PolyList = (int*)
                                  MyZMalloc( sizeof(int) * Polyl->m_CornersCount * 2 );

                p = strtok( &Line[2], " \t\n" );
                p = strtok( NULL, " \t\n" );
                p = strtok( NULL, " \t\n" );
                p = strtok( NULL, " \t\n" );

                for( i = 0; i < Polyl->m_CornersCount * 2 && !Error; i++ )
                {
                    p     = strtok( NULL, " \t\n" );
                    Error = sscanf( p, "%d", &Polyl->m_PolyList[i] ) != 1;
                }

                Polyl->m_Fill = NO_FILL;
                if( ( p = strtok( NULL, " \t\n" ) ) != NULL )
                {
                    if( p[0] == 'F' )
                        Polyl->m_Fill = FILLED_SHAPE;
                    if( p[0] == 'f' )
                        Polyl->m_Fill = FILLED_WITH_BG_BODYCOLOR;
                }
            }
            else
                Error = TRUE;
        }
            break;

        default:
            MsgLine.Printf( wxT( "Undefined DRAW command in line %d, aborted." ),
                            *LineNum );
            DisplayError( frame, MsgLine );
            return Head;
        }

        if( Error )
        {
            MsgLine.Printf( wxT( "Error in %c DRAW command in line %d, aborted." ),
                            Line[0], *LineNum );
            DisplayError( frame, MsgLine );
            SAFE_DELETE( New );

            /* FLush till end of draw: */
            do  {
                if( GetLine( f, Line, LineNum, 1024  ) == NULL )
                {
                    DisplayError( frame, wxT( "File ended prematurely" ) );
                    return Head;
                }
            }  while( strncmp( Line, "ENDDRAW", 7 ) != 0 );

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
int
NumOfLibraries()
{
    int            ii;
    LibraryStruct* Lib = g_LibraryList;

    for( ii = 0; Lib != NULL; Lib = Lib->m_Pnext )
        ii++;

    return ii;
}


/*****************************************************************************/
static bool GetLibEntryField (EDA_LibComponentStruct* LibEntry,
                  char* line)
/*****************************************************************************/

/* Analyse la ligne de description du champ de la forme:
 *  Fn "CA3130" 150 -200 50 H V
 *  ou n = 0 (REFERENCE), 1 (VALUE) , 2 .. 11 = autres champs, facultatifs
 */
{
    int   posx, posy, size, orient;
    bool  draw;
    char* Text,
          Char1[256], Char2[256],
          Char3[256], Char4[256],
          FieldUserName[1024];
    int           NumOfField, nbparam;
    LibDrawField* Field = NULL;

    if( sscanf( line + 1, "%d", &NumOfField ) != 1 )
        return 0;

    /* Recherche du debut des donnees (debut du texte suivant) */
    while( *line != 0 )
        line++;

    while( *line == 0 )
        line++;

    /* recherche du texte */
    while( *line && (*line != '"') )
        line++;

    if( *line == 0 )
        return 0;
    line++;
    Text = line;

    /* recherche fin de texte */
    while( *line && (*line != '"') )
        line++;

    if( *line == 0 )
        return 0;
    *line = 0;
    line++;

    FieldUserName[0] = 0;
	memset( Char4, 0, sizeof(Char4));
    nbparam = sscanf( line, " %d %d %d %c %c %c %s",
                      &posx, &posy, &size, Char1, Char2, Char3, Char4 );
    orient = TEXT_ORIENT_HORIZ;

    if( Char1[0] == 'V' )
        orient = TEXT_ORIENT_VERT;
    draw = TRUE; if( Char2[0] == 'I' )
        draw = FALSE;

    GRTextHorizJustifyType hjustify = GR_TEXT_HJUSTIFY_CENTER;
    GRTextVertJustifyType vjustify = GR_TEXT_VJUSTIFY_CENTER;

    if( nbparam >= 6 )
    {
        if( *Char3 == 'L' )
            hjustify = GR_TEXT_HJUSTIFY_LEFT;
        else if( *Char3 == 'R' )
            hjustify = GR_TEXT_HJUSTIFY_RIGHT;
        if( Char4[0] == 'B' )
            vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
        else if( Char4[0] == 'T' )
            vjustify = GR_TEXT_VJUSTIFY_TOP;
    }

    switch( NumOfField )
    {
    case REFERENCE:
        Field = &LibEntry->m_Prefix;
        Field->m_FieldId = REFERENCE;
        break;

    case VALUE:
        Field = &LibEntry->m_Name;
        Field->m_FieldId = VALUE;
        break;

    default:
        Field = new LibDrawField( NumOfField );
        LibEntry->m_Fields.PushBack( Field );
        break;
    }

    if( Field == NULL )
        return FALSE;

    Field->m_Pos.x  = posx; Field->m_Pos.y = posy;
    Field->m_Orient = orient;

    if ( Char4[1] == 'I' )		// Italic
            Field->m_Italic = true;
    if ( Char4[2] == 'B' )		// Bold
            Field->m_Width = size / 4;

    if( draw == FALSE )
        Field->m_Attributs |= TEXT_NO_VISIBLE;

    Field->m_Size.x = Field->m_Size.y = size;
    Field->m_Text   = CONV_FROM_UTF8( Text );

    if( NumOfField >= FIELD1 )
    {
        ReadDelimitedText( FieldUserName, line, sizeof(FieldUserName) );
        Field->m_Name = CONV_FROM_UTF8( FieldUserName );
    }

    Field->m_HJustify = hjustify;
    Field->m_VJustify = vjustify;
    return TRUE;
}


/********************************************************************/
static bool
AddAliasNames (EDA_LibComponentStruct* LibEntry,
               char* line )
/********************************************************************/

/* Read the alias names (in buffer line) and add them in alias list
 *  names are separated by spaces
 */
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
static void
InsertAlias (PriorQue** PQ,
             EDA_LibComponentStruct* LibEntry,
             int* NumOfParts)
/********************************************************************/
/* create in library (in list PQ) aliases of the "root" component LibEntry*/
{
    EDA_LibCmpAliasStruct* AliasEntry;
    unsigned ii;

    if( LibEntry->m_AliasList.GetCount() == 0 )
        return; /* No alias for this component */

    for( ii = 0; ii < LibEntry->m_AliasList.GetCount(); ii++ )
    {
        AliasEntry = new EDA_LibCmpAliasStruct( LibEntry->m_AliasList[ii],
                                               LibEntry->m_Name.m_Text.GetData() );

        ++ * NumOfParts;
        PQInsert( PQ, AliasEntry );
    }
}


/*******************************************************/
/* Routines de lecture des Documentation de composants */
/*******************************************************/

/**********************************************************************************************/
int
LoadDocLib (WinEDA_DrawFrame* frame,
            const wxString& FullDocLibName,
            const wxString& Libname)
/**********************************************************************************************/
/* Routine to load a library from given open file.*/
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
            msg.Printf( wxT( "$CMP command expected in line %d, aborted." ), LineNum );
            DisplayError( frame, msg );
            fclose( f );
            return 0;
        }

        /* Read one $CMP/$ENDCMP part entry from library: */
        Name = strtok( Line + 5, "\n\r" );
        wxString cmpname; cmpname = CONV_FROM_UTF8( Name );
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


/*********************************************************************************/
static bool
ReadLibEntryDateAndTime (EDA_LibComponentStruct* LibEntry,
                         char* Line )
/*********************************************************************************/

/* lit date et time de modif composant sous le format:
 *  "Ti yy/mm/jj hh:mm:ss"
 */
{
    int   year, mon, day, hour, min, sec;
    char* text;

    year = mon = day = hour = min = sec = 0;
    text = strtok( Line, " \r\t\n" );
    text = strtok( NULL, " \r\t\n" );  // text pointe donnees utiles

    sscanf( Line, "%d/%d/%d %d:%d:%d", &year, &mon, &day, &hour, &min, &sec );

    LibEntry->m_LastDate = (sec & 63)
                           + ( (min & 63) << 6 )
                           + ( (hour & 31) << 12 )
                           + ( (day & 31) << 17 )
                           + ( (mon & 15) << 22 )
                           + ( (year - 1990) << 26 );

    return TRUE;
}


/*******************************************/
static int
SortItemsFct (const void* ref,
              const void* item );

void
EDA_LibComponentStruct::SortDrawItems()
/*******************************************/
/* TODO translate comment to english TODO
 * Trie les �l�ments graphiques d'un composant lib pour am�liorer
 *  le trac�:
 *  items remplis en premier, pins en dernier
 *  En cas de superposition d'items, c'est plus lisible
 */
{
    LibEDA_BaseStruct** Bufentry, ** BufentryBase, * Entry = m_Drawings;
    int ii, nbitems;

    if( Entry == NULL )
        return; /* Pas d'alias pour ce composant */
    /* calcul du nombre d'items */
    for( nbitems = 0; Entry != NULL; Entry = Entry->Next() )
        nbitems++;

    BufentryBase =
        (LibEDA_BaseStruct**) MyZMalloc( (nbitems + 1) * sizeof(LibEDA_BaseStruct*) );

    /* memorisation du chainage : */
    for( Entry = m_Drawings, ii = 0; Entry != NULL; Entry = Entry->Next() )
        BufentryBase[ii++] = Entry;

    /* Tri du chainage */
    qsort( BufentryBase, nbitems, sizeof(LibEDA_BaseStruct*), SortItemsFct );

    /* Mise a jour du chainage. Remarque:
     *  le dernier element de BufEntryBase (BufEntryBase[nbitems]) est NULL*/
    m_Drawings = *BufentryBase;
    Bufentry   = BufentryBase;
    for( ii = 0; ii < nbitems; ii++ )
    {
        (*Bufentry)->SetNext( *(Bufentry + 1) );
        Bufentry++;
    }

    MyFree( BufentryBase );
}


int
SortItemsFct(const void* ref,
             const void* item)
{
#define Ref    ( *(LibEDA_BaseStruct**) (ref) )
#define Item   ( *(LibEDA_BaseStruct**) (item) )
#define BEFORE -1
#define AFTER  1

    int fill_ref = 0, fill_item = 0;

    switch( Ref->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
    {
        const LibDrawArc* draw = (const LibDrawArc*) Ref;
        fill_ref = draw->m_Fill;
        break;
    }

    case COMPONENT_CIRCLE_DRAW_TYPE:
    {
        const LibDrawCircle* draw = (const LibDrawCircle*) Ref;
        fill_ref = draw->m_Fill;
        break;
    }

    case COMPONENT_RECT_DRAW_TYPE:
    {
        const LibDrawSquare* draw = (const LibDrawSquare*) Ref;
        fill_ref = draw->m_Fill;
        break;
    }

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        const LibDrawPolyline* draw = (const LibDrawPolyline*) Ref;
        fill_ref = draw->m_Fill;
        break;
    }

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        if( Item->Type() == COMPONENT_PIN_DRAW_TYPE )
            return BEFORE;
        if( Item->Type() == COMPONENT_GRAPHIC_TEXT_DRAW_TYPE )
            return 0;
        return 1;
        break;

    case COMPONENT_PIN_DRAW_TYPE:
        if( Item->Type() == COMPONENT_PIN_DRAW_TYPE )
        {
            int ii;

            // We sort the pins by orientation
            ii = ( (LibDrawPin*) Ref )->m_Orient - ( (LibDrawPin*) Item )->m_Orient;
            if( ii )
                return ii;

            /* We sort the pins by position (x or y).
             *  note: at this point, most of pins have same x pos or y pos,
             *  because they are sorted by orientation and generally are vertically or
             *  horizontally aligned */
            wxPoint pos_ref, pos_tst;
            pos_ref = ( (LibDrawPin*) Ref )->m_Pos;
            pos_tst = ( (LibDrawPin*) Item )->m_Pos;
            if( (ii = pos_ref.x - pos_tst.x) )
                return ii;
            ii = pos_ref.y - pos_tst.y;
            return ii;
        }
        else
            return AFTER;
        break;

    default:
        ;
    }

    /* Test de l'item */
    switch( Item->Type() )
    {
    case COMPONENT_ARC_DRAW_TYPE:
    {
        const LibDrawArc* draw = (const LibDrawArc*) Item;
        fill_item = draw->m_Fill;
        break;
    }

    case COMPONENT_CIRCLE_DRAW_TYPE:
    {
        const LibDrawCircle* draw = (const LibDrawCircle*) Item;
        fill_item = draw->m_Fill;
        break;
    }

    case COMPONENT_RECT_DRAW_TYPE:
    {
        const LibDrawSquare* draw = (const LibDrawSquare*) Item;
        fill_item = draw->m_Fill;
        break;
    }

    case COMPONENT_POLYLINE_DRAW_TYPE:
    {
        const LibDrawPolyline* draw = (const LibDrawPolyline*) Item;
        fill_item = draw->m_Fill;
        break;
    }

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        return BEFORE;
        break;

    case COMPONENT_PIN_DRAW_TYPE:
        return BEFORE;
        break;

    default:
        ;
    }

    if( fill_ref & fill_item )
        return 0;
    if( fill_ref )
        return BEFORE;
    return AFTER;
}


/*****************************************************************************/
int
AddFootprintFilterList(EDA_LibComponentStruct* LibEntryLibEntry,
                       FILE* f,
                       char* Line,
                       int* LineNum)
/******************************************************************************/

/* read the FootprintFilter List stating with:
 *  FPLIST
 *  and ending with:
 *  ENDFPLIST
 */
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
