/****************************************/
/*  Module to load/save EESchema files. */
/****************************************/

#include "fctsys.h"
#include "confirm.h"
#include "kicad_string.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_marker_sch.h"


/* in read_from_file_schematic_items_description.cpp */
SCH_ITEM* ReadTextDescr( FILE* aFile, wxString& aMsgDiag, char* aLine,
                         int aBufsize, int* aLineNum,
                         int aSchematicFileVersion );

extern int ReadSheetDescr( wxWindow* frame, char* Line, FILE* f,
                           wxString& aMsgDiag, int* aLineNum,
                           BASE_SCREEN* Window );

extern bool ReadSchemaDescr( wxWindow* frame, char* Line, FILE* f,
                             wxString& aMsgDiag, int* aLineNum,
                             BASE_SCREEN* Window );

extern int ReadPartDescr( wxWindow* frame, char* Line, FILE* f,
                          wxString& aMsgDiag, int* aLineNum,
                          BASE_SCREEN* Window );

static void LoadLayers( FILE* f, int* linecnt );


/**
 * Routine to load an EESchema file.
 *  Returns true if file has been loaded (at least partially.)
 */
bool WinEDA_SchematicFrame::LoadOneEEFile( SCH_SCREEN* screen,
                                           const wxString& FullFileName )
{
    char            Line[1024], * SLine;
    char            Name1[256],
                    Name2[256];
    int             ii, layer;
    wxPoint         pos;
    bool            Failed = FALSE;
    SCH_ITEM*       Phead, * Pnext;
    SCH_JUNCTION*   ConnectionStruct;
    SCH_POLYLINE*   PolylineStruct;
    SCH_LINE*       SegmentStruct;
    SCH_BUS_ENTRY*  busEntry;
    SCH_NO_CONNECT* NoConnectStruct;
    int             LineCount;
    wxString        MsgDiag;   /* Error and log messages */

    FILE*           f;

    if( screen == NULL )
        return FALSE;
    if( FullFileName.IsEmpty() )
        return FALSE;

    screen->SetCurItem( NULL );
    screen->m_FileName = FullFileName;

    LineCount = 1;
    if( ( f = wxFopen( FullFileName, wxT( "rt" ) ) ) == NULL )
    {
        MsgDiag = _( "Failed to open " ) + FullFileName;
        DisplayError( this, MsgDiag );
        return FALSE;
    }

    MsgDiag = _( "Loading " ) + screen->m_FileName;
    PrintMsg( MsgDiag );

    if( fgets( Line, sizeof(Line), f ) == NULL
        || strncmp( Line + 9, SCHEMATIC_HEAD_STRING,
                    sizeof(SCHEMATIC_HEAD_STRING) - 1 ) != 0 )
    {
        MsgDiag = FullFileName + _( " is NOT an EESchema file!" );
        DisplayError( this, MsgDiag );
        fclose( f );
        return FALSE;
    }

    //get the file version here. TODO: Support version numbers > 9
    char version = Line[9 + sizeof(SCHEMATIC_HEAD_STRING)];
    int  ver     = version - '0';
    if( ver > EESCHEMA_VERSION )
    {
        MsgDiag = FullFileName + _( " was created by a more recent \
version of EESchema and may not load correctly. Please consider updating!" );
        DisplayInfoMessage( this, MsgDiag );
    }

#if 0
    // Compile it if the new version is unreadable by previous eeschema versions
    else if( ver < EESCHEMA_VERSION )
    {
        MsgDiag = FullFileName + _( " was created by an older version of \
EESchema. It will be stored in the new file format when you save this file \
again." );

        DisplayInfoMessage( this, MsgDiag );
    }
#endif

    LineCount++;
    if( fgets( Line, sizeof(Line), f ) == NULL || strncmp( Line, "LIBS:", 5 ) != 0 )
    {
        MsgDiag = FullFileName + _( " is NOT an EESchema file!" );
        DisplayError( this, MsgDiag );
        fclose( f );
        return FALSE;
    }

    // Read the rest of a potentially very long line.  fgets() puts a '\n' into
    // the buffer if the end of line was reached.  Read until end of line if
    // necessary.
    if( Line[ strlen( Line )-1 ] != '\n' )
    {
        int c;
        while( !feof( f ) && (c = fgetc( f )) != '\n' )
            ;
    }

    LoadLayers( f, &LineCount );

    while( !feof( f ) && GetLine( f, Line, &LineCount, sizeof(Line) ) != NULL )
    {
        SLine = Line;
        while( (*SLine != ' ' ) && *SLine )
            SLine++;

        switch( Line[0] )
        {
        case '$':           /* identification block */
            if( Line[1] == 'C' )
                Failed = ReadPartDescr( this, Line, f, MsgDiag, &LineCount, screen );

            else if( Line[1] == 'S' )
                Failed = ReadSheetDescr( this, Line, f, MsgDiag, &LineCount, screen );

            else if( Line[1] == 'D' )
                Failed = ReadSchemaDescr( this, Line, f, MsgDiag, &LineCount, screen );
        else if( Line[1] == 'T' ) //text part
        {
            printf( "**** TEXT PART\n" );
            SCH_ITEM* Struct;
            Struct = ReadTextDescr( f, MsgDiag, Line, sizeof(Line), &LineCount, version );

            if( Struct )
            {
                Struct->SetNext( screen->EEDrawList );
                screen->EEDrawList = Struct;
            }
            else
            {
                Failed = true;
            }
        }

            break;

        case 'L':        /* Its a library item. */
            Failed = ReadPartDescr( this, Line, f, MsgDiag, &LineCount, screen );
            break;


        case 'W':        /* Its a Segment (WIRE or BUS) item. */
            if( sscanf( SLine, "%s %s", Name1, Name2 ) != 2  )
            {
                MsgDiag.Printf( wxT( "EESchema file segment error at line %d, aborted" ),
                                LineCount );
                MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                Failed = true;
                break;
            }
            layer = LAYER_NOTES;
            if( Name1[0] == 'W' )
                layer = LAYER_WIRE;
            if( Name1[0] == 'B' )
                layer = LAYER_BUS;

            SegmentStruct = new SCH_LINE( wxPoint( 0, 0 ), layer );

            LineCount++;
            if( fgets( Line, 256 - 1, f ) == NULL
                || sscanf( Line, "%d %d %d %d ", &SegmentStruct->m_Start.x,
                           &SegmentStruct->m_Start.y, &SegmentStruct->m_End.x,
                           &SegmentStruct->m_End.y ) != 4 )
            {
                MsgDiag.Printf( wxT( "EESchema file Segment struct error at line %d, aborted" ),
                                LineCount );
                MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                Failed = true;
                SAFE_DELETE( SegmentStruct );
                break;
            }

            if( !Failed )
            {
                SegmentStruct->SetNext( screen->EEDrawList );
                screen->EEDrawList = SegmentStruct;
            }
            break;


        case 'E':        /* Its a WIRE or BUS item. */
            if( sscanf( SLine, "%s %s", Name1, Name2 ) != 2  )
            {
                MsgDiag.Printf( wxT( "EESchema file record struct error at line %d, aborted" ),
                                LineCount );
                MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                Failed = true;
                break;
            }

            ii = WIRE_TO_BUS;
            if( Name1[0] == 'B' )
                ii = BUS_TO_BUS;
            busEntry = new SCH_BUS_ENTRY( wxPoint( 0, 0 ), '\\', ii );

            LineCount++;
            if( fgets( Line, 256 - 1, f ) == NULL
                || sscanf( Line, "%d %d %d %d ", &busEntry->m_Pos.x, &busEntry->m_Pos.y,
                           &busEntry->m_Size.x, &busEntry->m_Size.y ) != 4 )
            {
                MsgDiag.Printf( wxT( "EESchema file Bus Entry struct error at line %d, aborted" ),
                                LineCount );
                MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                Failed = true;
                SAFE_DELETE( busEntry );
                break;
            }

            if( !Failed )
            {
                busEntry->m_Size.x -= busEntry->m_Pos.x;
                busEntry->m_Size.y -= busEntry->m_Pos.y;
                busEntry->SetNext( screen->EEDrawList );
                screen->EEDrawList = busEntry;
            }
            break;

        case 'P':        /* Its a polyline item. */
            if( sscanf( SLine, "%s %s %d", Name1, Name2, &ii ) != 3 )
            {
                MsgDiag.Printf( wxT( "EESchema file polyline struct error at line %d, aborted" ),
                                LineCount );
                MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                Failed = true;
                break;
            }
            layer = LAYER_NOTES;
            if( Name2[0] == 'W' )
                layer = LAYER_WIRE;
            if( Name2[0] == 'B' )
                layer = LAYER_BUS;

            PolylineStruct = new SCH_POLYLINE( layer );
            for( unsigned jj = 0; jj < (unsigned)ii; jj++ )
            {
                LineCount++;
                wxPoint point;
                if( fgets( Line, 256 - 1, f ) == NULL
                    || sscanf( Line, "%d %d", &point.x, &point.y ) != 2 )
                {
                    MsgDiag.Printf( wxT( "EESchema file polyline struct error \
at line %d, aborted" ),
                                    LineCount );
                    MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                    Failed = true;
                    SAFE_DELETE( PolylineStruct );
                    break;
                }

                PolylineStruct->AddPoint( point );
            }

            if( !Failed )
            {
                PolylineStruct->SetNext( screen->EEDrawList );
                screen->EEDrawList = PolylineStruct;
            }
            break;

        case 'C':                       /* It is a connection item. */
            ConnectionStruct = new SCH_JUNCTION( wxPoint( 0, 0 ) );

            if( sscanf( SLine, "%s %d %d", Name1, &ConnectionStruct->m_Pos.x,
                        &ConnectionStruct->m_Pos.y ) != 3 )
            {
                MsgDiag.Printf( wxT( "EESchema file connection struct error \
at line %d, aborted" ),
                                LineCount );
                MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                Failed = true;
                SAFE_DELETE( ConnectionStruct );
            }
            else
            {
                ConnectionStruct->SetNext( screen->EEDrawList );
                screen->EEDrawList = ConnectionStruct;
            }
            break;

        case 'N':                       /* It is a NoConnect item. */
            if( sscanf( SLine, "%s %d %d", Name1, &pos.x, &pos.y ) != 3 )
            {
                MsgDiag.Printf( wxT( "EESchema file NoConnect struct error at line %d, aborted" ),
                                LineCount );
                MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
                Failed = true;
            }
            else
            {
                NoConnectStruct = new SCH_NO_CONNECT( pos );
                NoConnectStruct->SetNext( screen->EEDrawList );
                screen->EEDrawList = NoConnectStruct;
            }
            break;

        case 'K':                       /* It is a Marker item. */
            // Markers are no more read from file. they are only created on
            // demand in schematic
            break;

        case 'T':                       /* It is a text item. */
        {
            SCH_ITEM* Struct;
            Struct = ReadTextDescr( f, MsgDiag, Line, sizeof(Line), &LineCount, version);

            if( Struct )
            {
                Struct->SetNext( screen->EEDrawList );
                screen->EEDrawList = Struct;
            }
            else
                Failed = true;
            break;
        }

        default:
            Failed = true;
            MsgDiag.Printf( wxT( "EESchema file undefined object at line %d, aborted" ),
                            LineCount );
            MsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
            break;
        }

        if( Failed )
        {
            DisplayError( this, MsgDiag );
            break;
        }
    }

    /* EEDrawList was constructed in reverse order - reverse it back: */
    Phead = NULL;
    while( screen->EEDrawList )
    {
        Pnext = screen->EEDrawList;
        screen->EEDrawList = screen->EEDrawList->Next();
        Pnext->SetNext( Phead );
        Phead = Pnext;
    }

    screen->EEDrawList = Phead;

#if 0 && defined (DEBUG)
    screen->Show( 0, std::cout );
#endif

    fclose( f );

    TestDanglingEnds( screen->EEDrawList, NULL );

    MsgDiag = _( "Done Loading " ) + screen->m_FileName;
    PrintMsg( MsgDiag );

    return true;    /* Although it may be that file is only partially loaded. */
}


static void LoadLayers( FILE* f, int* linecnt )
{
    int  Number;
    char Line[1024];

    //int Mode,Color,Layer;
    char Name[256];

    GetLine( f, Line, linecnt, sizeof(Line) );       /* read line */

    sscanf( Line, "%s %d %d", Name, &Number, &g_LayerDescr.CurrentLayer );
    if( strcmp( Name, "EELAYER" ) !=0 )
    {
        /* error : init par default */
        Number = MAX_LAYER;
    }

    if( Number <= 0 )
        Number = MAX_LAYER;
    if( Number > MAX_LAYER )
        Number = MAX_LAYER;

    g_LayerDescr.NumberOfLayers = Number;

    while( GetLine( f, Line, linecnt, sizeof(Line) ) )
    {
        if( strnicmp( Line, "EELAYER END", 11 ) == 0 )
            break;
    }
}
