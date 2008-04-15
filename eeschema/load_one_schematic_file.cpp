/****************************************/
/*	Module to load/save EESchema files.	*/
/****************************************/

#include "fctsys.h"
//#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "id.h"

/* Format des fichiers:
 *  - entete:
 *  EESchema Schematic File Version n
 *  - liste des librairies utilisees
 *  LIBS:lib1,lib2,...
 *
 *  - description des elements:
 *  - ici Dimensions du schema, cartouche..:
 *  $Descr  A3 xx yy		(format A3 (A..A0 / A..E / "user") xx yy = dims internes )
 *  $EndDescr
 *
 *  - ici: polyline
 *  P L  0 3
 *  2208 1008
 *  2208 1136
 *  2128 1136
 *
 *  - ici: Segment (wire, bus) ( 1; W = segment type Wire, 2: W = Wire B = Bus
 *                              3: L ou B = epaisseur ( L = ligne, B = bus)
 *  W W  L		(W B B si bus)
 *  1856 1008 1856 1136 (debut X,Y fin X,Y)
 *
 *  - ici: Raccord (wire, bus)
 *  R W  L
 *  1856 1008 1856 1136 (debut X,Y fin X,Y)
 *
 *  - ici: Sheet ( Sous-feuille de hierarchie)
 *  $Sheet
 *  S  1856 1008 1856 1136 (debut X,Y fin X,Y)
 *  F0 "texte" X X posx posy			; sheetname
 *  F1 "texte" X X posx posy			; filename
 *  Fn "label" type side posx posy size	; n lignes de label
 *  $EndSheet
 *
 *  - ici: composant
 *  $Comp
 *  L CAPACITOR  C1			H H -30863 -14794 1968 1184		nom, ref, dir et pos
 *  1	 2016 1136 1904 1024 2128 1248		multi, posx,y, rect encadrement
 *  1	 0	  0		 -1  					matrice de rotation/miroir
 *  $EndComp
 */

/* Fonctions locales */
static int  ReadPartDescr( wxWindow* frame, char* Line, FILE* f, BASE_SCREEN* Window );
static int ReadSheetDescr( wxWindow* frame, char* Line, FILE* f, BASE_SCREEN* Window);
static int  ReadSchemaDescr( wxWindow* frame, char* Line, FILE* f, BASE_SCREEN* Window );
static void LoadLayers( FILE* f, int* linecnt );

/* Variables locales */
static int      LineCount;  /* Decompte de num de ligne lue dans eeload() */
static wxString MsgDiag;    /* Error and log messages */

/************************************************************************************************/
bool WinEDA_SchematicFrame::LoadOneEEFile( SCH_SCREEN* screen, const wxString& FullFileName )
/************************************************************************************************/

/* Routine to load an EESchema file.
 *  Returns TRUE if file has been loaded (at least partially.)
 */
{
    char                 Line[1024], * SLine;
    char                 Name1[256],
                         Name2[256];
    int                  ii, layer, orient, size;
    wxPoint              pos;
    bool                 Failed = FALSE;
    SCH_ITEM*            Phead, * Pnext;
    DrawJunctionStruct*  ConnectionStruct;
    DrawPolylineStruct*  PolylineStruct;
    EDA_DrawLineStruct*  SegmentStruct;
    DrawBusEntryStruct*  RaccordStruct;
    DrawMarkerStruct*    MarkerStruct;
    DrawNoConnectStruct* NoConnectStruct;

    FILE*                f;

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

    if( fgets( Line, 1024 - 1, f ) == NULL
        || strncmp( Line + 9, SCHEMATIC_HEAD_STRING, sizeof(SCHEMATIC_HEAD_STRING) - 1 )
        != 0 )
    {
        MsgDiag = FullFileName + _( " is NOT an EESchema file!" );
        DisplayError( this, MsgDiag );
        fclose( f );
        return FALSE;
    }
    //get the file version here. TODO: Support version numbers > 9
    char version = Line[9 + sizeof(SCHEMATIC_HEAD_STRING)];
    int ver = version - '0';
    if ( ver > EESCHEMA_VERSION )
    {
        MsgDiag = FullFileName + _( " was created by a more recent version of EESchema and may not load correctly. Please consider updating!");
        DisplayInfo( this, MsgDiag);
    }
#if 0
    // Compile it if the new versiopn is unreadable by previous eeschema versions
    else if ( ver < EESCHEMA_VERSION )
    {
        MsgDiag = FullFileName + _( " was created by an older version of EESchema. It will be stored in the new file format when you save this file again.");
        DisplayInfo( this, MsgDiag);
    }
#endif

    LineCount++;
    if( fgets( Line, 1024 - 1, f ) == NULL || strncmp( Line, "LIBS:", 5 ) != 0 )
    {
        MsgDiag = FullFileName + _( " is NOT an EESchema file!" );
        DisplayError( this, MsgDiag );
        fclose( f );
        return FALSE;
    }

    LoadLayers( f, &LineCount );

    while( !feof( f ) && GetLine( f, Line, &LineCount, sizeof(Line) ) != NULL )
    {
        SLine = Line;
        while( (*SLine != ' ' ) && *SLine )
            SLine++;

        switch( Line[0] )
        {
        case '$':           /* identification de bloc */
            if( Line[1] == 'C' )
            {
                Failed = ReadPartDescr( this, Line, f, screen );
            }
            else if( Line[1] == 'S' )
            {
                Failed = ReadSheetDescr( this, Line, f, screen );
            }
            else if( Line[1] == 'D' )
            {
                Failed = ReadSchemaDescr( this, Line, f, screen );
            }
            break;

        case 'L':           /* Its a library item. */
            Failed = ReadPartDescr( this, Line, f, screen );
            break;          /* Fin lecture 1 composant */


        case 'W':        /* Its a Segment (WIRE or BUS) item. */
            if( sscanf( SLine, "%s %s", Name1, Name2 ) != 2  )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file Segment struct error at line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
                break;
            }
            layer = LAYER_NOTES;
            if( Name1[0] == 'W' )
                layer = LAYER_WIRE;
            if( Name1[0] == 'B' )
                layer = LAYER_BUS;

            SegmentStruct = new EDA_DrawLineStruct( wxPoint( 0, 0 ), layer );

            LineCount++;
            if( fgets( Line, 256 - 1, f ) == NULL
                || sscanf( Line, "%d %d %d %d ",
                           &SegmentStruct->m_Start.x, &SegmentStruct->m_Start.y,
                           &SegmentStruct->m_End.x, &SegmentStruct->m_End.y ) != 4 )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file Segment struct error at line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
                SAFE_DELETE( SegmentStruct ) ;
                break;
            }

            if( !Failed )
            {
                SegmentStruct->Pnext = screen->EEDrawList;
                screen->EEDrawList   = SegmentStruct;
            }
            break;


        case 'E':        /* Its a Raccord (WIRE or BUS) item. */
            if( sscanf( SLine, "%s %s", Name1, Name2 ) != 2  )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file record struct error at line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
                break;
            }

            ii = WIRE_TO_BUS;
            if( Name1[0] == 'B' )
                ii = BUS_TO_BUS;
            RaccordStruct = new DrawBusEntryStruct( wxPoint( 0, 0 ), '\\', ii );
            LineCount++;
            if( fgets( Line, 256 - 1, f ) == NULL
                || sscanf( Line, "%d %d %d %d ",
                           &RaccordStruct->m_Pos.x, &RaccordStruct->m_Pos.y,
                           &RaccordStruct->m_Size.x, &RaccordStruct->m_Size.y ) != 4 )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file Raccord struct error at line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
                SAFE_DELETE( RaccordStruct ) ;
                break;
            }

            if( !Failed )
            {
                RaccordStruct->m_Size.x -= RaccordStruct->m_Pos.x;
                RaccordStruct->m_Size.y -= RaccordStruct->m_Pos.y;
                RaccordStruct->Pnext = screen->EEDrawList;
                screen->EEDrawList   = RaccordStruct;
            }
            break;

        case 'P':        /* Its a polyline item. */
            if( sscanf( SLine, "%s %s %d", Name1, Name2, &ii ) != 3 )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file polyline struct error at line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
                break;
            }
            layer = LAYER_NOTES;
            if( Name2[0] == 'W' )
                layer = LAYER_WIRE;
            if( Name2[0] == 'B' )
                layer = LAYER_BUS;

            PolylineStruct = new DrawPolylineStruct( layer );

            PolylineStruct->m_NumOfPoints = ii;
            PolylineStruct->m_Points = (int*) MyZMalloc( sizeof(int) * 2 *
                                                         PolylineStruct->m_NumOfPoints );
            for( ii = 0; ii < PolylineStruct->m_NumOfPoints; ii++ )
            {
                LineCount++;
                if( fgets( Line, 256 - 1, f ) == NULL
                    || sscanf( Line, "%d %d", &PolylineStruct->m_Points[ii * 2],
                               &PolylineStruct->m_Points[ii * 2 + 1] ) != 2 )
                {
                    MsgDiag.Printf(
                        wxT( "EESchema file polyline struct error at line %d, aborted" ),
                        LineCount );
                    Failed = TRUE;
                    SAFE_DELETE( PolylineStruct )  ;
                    break;
                }
            }

            if( !Failed )
            {
                PolylineStruct->Pnext = screen->EEDrawList;
                screen->EEDrawList    = PolylineStruct;
            }
            break;

        case 'C':                       /* Its a connection item. */
            ConnectionStruct = new DrawJunctionStruct( wxPoint( 0, 0 ) );
            if( sscanf( SLine, "%s %d %d", Name1,
                        &ConnectionStruct->m_Pos.x,
                        &ConnectionStruct->m_Pos.y ) != 3 )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file connection struct error at line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
                SAFE_DELETE( ConnectionStruct ) ;
            }
            else
            {
                ConnectionStruct->Pnext = screen->EEDrawList;
                screen->EEDrawList = ConnectionStruct;
            }
            break;

        case 'N':                       /* Its a NoConnect item. */
            if( sscanf( SLine, "%s %d %d", Name1, &pos.x, &pos.y ) != 3 )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file NoConnect struct error at line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
            }
            else
            {
                NoConnectStruct = new DrawNoConnectStruct( pos );
                NoConnectStruct->Pnext = screen->EEDrawList;
                screen->EEDrawList = NoConnectStruct;
            }
            break;

        case 'K':                       /* Its a MarKer item. */
            if( sscanf( SLine, "%s %d %d", Name1, &pos.x, &pos.y ) != 3 )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file marker struct error line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
            }
            else
            {
                char* text;
                char  BufLine[1024];
                MarkerStruct = new DrawMarkerStruct( pos, wxEmptyString );
                ii = ReadDelimitedText( BufLine, Line, 256 );
                MarkerStruct->m_Type = (TypeMarker) ( (Name1[0] & 255) - 'A' );
                if( MarkerStruct->m_Type < 0 )
                    MarkerStruct->m_Type = MARQ_UNSPEC;
                if( ii )
                    MarkerStruct->m_Comment = CONV_FROM_UTF8( BufLine );
                text = strstr( Line, " F=" );
                if( text )
                {
                    sscanf( text + 3, "%X", &ii );
                    MarkerStruct->m_MarkFlags = ii;
                }
                MarkerStruct->Pnext = screen->EEDrawList;
                screen->EEDrawList  = MarkerStruct;
            }
            break;

        case 'T':                       /* Its a text item. */
        {
            SCH_ITEM* Struct = NULL;
            *Name1 = *Name2 = 0;
            ii = sscanf( SLine, "%s %d %d %d %d %s",
                         Name1, &pos.x, &pos.y, &orient, &size, Name2 );

            if( ii < 4 )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file text struct error line %d, aborted" ),
                    LineCount );
                Failed = TRUE;
            }
            else if( fgets( Line, 256 - 1, f ) == NULL )
            {
                LineCount++;
                MsgDiag.Printf(
                    wxT( "EESchema file text struct error line %d (No text), aborted" ),
                    LineCount );
                Failed = TRUE;
            }
            else
            {
                LineCount++;
                if( size == 0 )
                    size = DEFAULT_SIZE_TEXT;
                char* text = strtok( Line, "\n\r" );
                if( text == NULL )
                    break;

                if( Name1[0] == 'L' )
                {
                    SCH_LABEL* TextStruct =
                        new SCH_LABEL( pos, CONV_FROM_UTF8( text ) );
                    TextStruct->m_Size.x = TextStruct->m_Size.y = size;
                    TextStruct->m_Orient = orient;
                    Struct = TextStruct;
                }
                else if( Name1[0] == 'G' && version > '1')
                {
                    SCH_GLOBALLABEL* TextStruct = new SCH_GLOBALLABEL(pos, CONV_FROM_UTF8( text ) );
                    Struct = TextStruct;
                    TextStruct->m_Size.x = TextStruct->m_Size.y = size;
                    TextStruct->m_Orient = orient;
                    TextStruct->m_Shape = NET_INPUT;
                    if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
                        TextStruct->m_Shape = NET_OUTPUT;
                    if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
                        TextStruct->m_Shape = NET_BIDI;
                    if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
                        TextStruct->m_Shape = NET_TRISTATE;
                    if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
                        TextStruct->m_Shape = NET_UNSPECIFIED;
                }
                else if( (Name1[0] == 'H') || (Name1[0] == 'G' && version == '1'))
                { //in schematic file version 1, glabels were actually hierarchal labels.
                    SCH_HIERLABEL* TextStruct = new SCH_HIERLABEL(pos, CONV_FROM_UTF8( text ) );
                    Struct = TextStruct;
                    TextStruct->m_Size.x = TextStruct->m_Size.y = size;
                    TextStruct->m_Orient = orient;
                    TextStruct->m_Shape = NET_INPUT;
                    if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
                        TextStruct->m_Shape = NET_OUTPUT;
                    if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
                        TextStruct->m_Shape = NET_BIDI;
                    if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
                        TextStruct->m_Shape = NET_TRISTATE;
                    if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
                        TextStruct->m_Shape = NET_UNSPECIFIED;
                }
                else
                {
                    SCH_TEXT* TextStruct =
                        new SCH_TEXT( pos, CONV_FROM_UTF8( text ) );
                    TextStruct->m_Size.x = TextStruct->m_Size.y = size;
                    TextStruct->m_Orient = orient;
                    Struct = TextStruct;
                }
                if( Struct )
                {
                    Struct->Pnext      = screen->EEDrawList;
                    screen->EEDrawList = Struct;
                }
            }
            break;
        }

        default:
            Failed = FALSE;
            MsgDiag.Printf(
                wxT( "EESchema file undef structdef at line %d, aborted" ),
                LineCount );
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
        Pnext->Pnext = Phead;
        Phead = Pnext;
    }

    screen->EEDrawList = Phead;

#if 0 && defined(DEBUG)
    screen->Show( 0, std::cout );
#endif

    fclose( f );

    TestDanglingEnds( screen->EEDrawList, NULL );

    MsgDiag = _( "Done Loading " ) +  screen->m_FileName;
    PrintMsg( MsgDiag );

    return TRUE;    /* Although it may be that file is only partially loaded. */
}


/*************************************************************/
static int ReadPartDescr( wxWindow* frame, char* Line, FILE* f,
                          BASE_SCREEN* Window )
/*************************************************************/

/* Fonction utilisee par LoadEEFile().
 *  Lit les lignes relatives a la description d'un composant en schema
 */
{
    int   ii, fieldref;
    char  Name1[256], Name2[256],
          Char1[256], Char2[256], Char3[256];
    SCH_COMPONENT* LibItemStruct;
    int   Failed = 0, newfmt = 0;
    char* ptcar;

    LibItemStruct = new SCH_COMPONENT();
    LibItemStruct->m_Convert = 1;

    if( Line[0] == '$' )
    {
        newfmt = 1;
        LineCount++;
        if( fgets( Line, 256 - 1, f ) == 0 )
            return TRUE;
    }

    /* Traitement de la 1ere ligne de description */
    if( sscanf( &Line[1], "%s %s", Name1, Name2 ) != 2 )
    {
        MsgDiag.Printf(
            wxT( "EESchema Component descr error at line %d, aborted" ),
            LineCount );
        Failed = TRUE;
        return Failed;
    }

    if( strcmp( Name1, NULL_STRING ) != 0 )
    {
        for( ii = 0; ii < (int) strlen( Name1 ); ii++ )
            if( Name1[ii] == '~' )
                Name1[ii] = ' ';

        LibItemStruct->m_ChipName = CONV_FROM_UTF8( Name1 );
        if( !newfmt )
            LibItemStruct->m_Field[VALUE].m_Text = CONV_FROM_UTF8( Name1 );
    }
    else
    {
        LibItemStruct->m_ChipName.Empty();
        LibItemStruct->m_Field[VALUE].m_Text.Empty();
        LibItemStruct->m_Field[VALUE].m_Orient    = TEXT_ORIENT_HORIZ;
        LibItemStruct->m_Field[VALUE].m_Attributs = TEXT_NO_VISIBLE;
    }

    if( strcmp( Name2, NULL_STRING ) != 0 )
    {
        bool isDigit = false;
        for( ii = 0; ii < (int) strlen( Name2 ); ii++ ){
            if( Name2[ii] == '~' )
                Name2[ii] = ' ';
            //get RefBase from this, too. store in Name1.
            if(Name2[ii] >= '0' && Name2[ii] <= '9'){
                isDigit = true;
                Name1[ii] = 0;  //null-terminate.
            }
            if(!isDigit){
                Name1[ii] = Name2[ii];
            }
        }
        Name1[ii] = 0; //just in case
        int jj;
        for(jj=0; jj<ii && Name1[jj] == ' '; jj++);
        if(jj == ii){
            //blank string.
            LibItemStruct->m_PrefixString = wxT("U");
        }else{
            LibItemStruct->m_PrefixString = CONV_FROM_UTF8(&Name1[jj]);
            //printf("prefix: %s\n", CONV_TO_UTF8(LibItemStruct->m_PrefixString));
        }
        if( !newfmt )
            LibItemStruct->m_Field[REFERENCE].m_Text = CONV_FROM_UTF8( Name2 );
    }
    else
    {
        LibItemStruct->m_Field[REFERENCE].m_Attributs = TEXT_NO_VISIBLE;
    }

    /* Traitement des autres lignes de description */

    /* Ces lignes commencent par:
     *  "P " = position
     *  "U " = Num Unit, et Conversion
     *  "Fn" = Champs ( n = 0.. = numero de champ )
     *	"Ar" = AlternateReference, in the case of multiple sheets
     *			referring to one schematic file.
     */

    /* Lecture des champs */
    for( ; ; )
    {
        LineCount++;
        if( fgets( Line, 256 - 1, f ) == NULL )
            return TRUE;
        if( (Line[0] != 'F' )
           && (Line[0] != 'P' )
           && (Line[0] != 'A' )
           && (Line[0] != 'U' ) )
            break;

        if( Line[0] == 'U' ) /* Lecture num multi, conversion et time stamp */
        {
            sscanf( Line + 1, "%d %d %lX",
                    &LibItemStruct->m_Multi, &LibItemStruct->m_Convert,
                    &LibItemStruct->m_TimeStamp );
        }

        if( Line[0] == 'P' )
        {
            sscanf( Line + 1, "%d %d",
                    &LibItemStruct->m_Pos.x, &LibItemStruct->m_Pos.y );
        }
        if( Line[0] == 'A' && Line[1] == 'R' )
        {
            /*format:
            AR Path="/9086AF6E/67452AA0" Ref="C99"
            where 9086AF6E is the unique timestamp of the containing sheet
            and 67452AA0 is the timestamp of this component.
            C99 is the reference given this path.
            */
            int i=2;
            while(i<256 && Line[i] != '"'){ i++; } i++;
            //copy the path.
            int j = 0;
            while(i<256 && Line[i] != '"'){Name1[j] = Line[i]; i++; j++;} i++;
            Name1[j] = 0;
            wxString path = CONV_FROM_UTF8(Name1);
            //i should be one after the closing quote, match the next opening quote
            while(i<256 && Line[i] != '"'){ i++; } i++;
            j = 0;
            while(i<256 && Line[i] != '"'){Name1[j] = Line[i]; i++; j++;} i++;
            Name1[j] = 0;
            wxString ref = CONV_FROM_UTF8(Name1);

            LibItemStruct->AddHierarchicalReference(path, ref);
            LibItemStruct->m_Field[REFERENCE].m_Text = ref;
        }
        if( Line[0] == 'F' )
        {
            char FieldUserName[1024];
            int  hjustify = GR_TEXT_HJUSTIFY_CENTER;
            int  vjustify = GR_TEXT_VJUSTIFY_CENTER;

            FieldUserName[0] = 0;

            sscanf( Line + 1, "%d", &fieldref );
            if( fieldref >= NUMBER_OF_FIELDS )
            {
                MsgDiag.Printf(
                    wxT( "Component Field number error at line %d, aborted" ),
                    LineCount );
                return TRUE;
            }
            /* Lecture du champ */
            ptcar = Line; while( *ptcar && (*ptcar != '"') )
                ptcar++;

            if( *ptcar != '"' )
            {
                MsgDiag.Printf(
                    wxT( "EESchema file lib field F at line %d, aborted" ),
                    LineCount );
                return TRUE;
            }

            for( ptcar++, ii = 0; ; ii++, ptcar++ )
            {
                Name1[ii] = *ptcar;
                if( *ptcar == 0 )
                {
                    MsgDiag.Printf(
                        wxT( "Component field F at line %d, aborted" ),
                        LineCount );
                    return TRUE;
                }
                if( *ptcar == '"' )
                {
                    Name1[ii] = 0; ptcar++;
                    break;
                }
            }

            if( LibItemStruct->m_Field[fieldref].m_Text.IsEmpty() )
                LibItemStruct->m_Field[fieldref].m_Text = CONV_FROM_UTF8( Name1 );

            /* Lecture des coordonnees */
            if( ( ii = sscanf( ptcar, "%s %d %d %d %X %s %s", Char1,
                               &LibItemStruct->m_Field[fieldref].m_Pos.x,
                               &LibItemStruct->m_Field[fieldref].m_Pos.y,
                               &LibItemStruct->m_Field[fieldref].m_Size.x,
                               &LibItemStruct->m_Field[fieldref].m_Attributs,
                               Char2, Char3 ) ) < 4 )
            {
                MsgDiag.Printf(
                    wxT( "Component Field error line %d, aborted" ),
                    LineCount );
                DisplayError( frame, MsgDiag );
                continue;
            }

            ReadDelimitedText( FieldUserName, ptcar, sizeof(FieldUserName) );
            if( (LibItemStruct->m_Field[fieldref].m_Size.x == 0 ) || (ii == 4) )
                LibItemStruct->m_Field[fieldref].m_Size.x = DEFAULT_SIZE_TEXT;
            LibItemStruct->m_Field[fieldref].m_Orient = TEXT_ORIENT_HORIZ;
            LibItemStruct->m_Field[fieldref].m_Size.y = LibItemStruct->m_Field[fieldref].m_Size.x;
            if( Char1[0] == 'V' )
                LibItemStruct->m_Field[fieldref].m_Orient = TEXT_ORIENT_VERT;
            if( ii >= 7 )
            {
                if( *Char2 == 'L' )
                    hjustify = GR_TEXT_HJUSTIFY_LEFT;
                else if( *Char2 == 'R' )
                    hjustify = GR_TEXT_HJUSTIFY_RIGHT;
                if( *Char3 == 'B' )
                    vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
                else if( *Char3 == 'T' )
                    vjustify = GR_TEXT_VJUSTIFY_TOP;
                LibItemStruct->m_Field[fieldref].m_HJustify = hjustify;
                LibItemStruct->m_Field[fieldref].m_VJustify = vjustify;
            }

            if( fieldref >= FIELD1 )
            {
                LibItemStruct->m_Field[fieldref].m_Name = CONV_FROM_UTF8( FieldUserName );
            }

// 27 juin  2001: A Supprimer lorsque tous les schemas auront ete traites :
            if( fieldref == REFERENCE )
                if( LibItemStruct->m_Field[fieldref].m_Text[0] == '#' )
                    LibItemStruct->m_Field[fieldref].m_Attributs |= TEXT_NO_VISIBLE;
        }
    }

    /* Lecture multi et position du composant */
    if( sscanf( Line, "%d %d %d",
                &LibItemStruct->m_Multi,
                &LibItemStruct->m_Pos.x, &LibItemStruct->m_Pos.y ) != 3 )
    {
        MsgDiag.Printf(
            wxT( "Component unit & pos error at line %d, aborted" ),
            LineCount );
        Failed = TRUE;
        return Failed;
    }

    /* Lecture de la matrice de miroir / rotation */
    LineCount++;
    if( (fgets( Line, 256 - 1, f ) == NULL)
       || (sscanf( Line, "%d %d %d %d",
                   &LibItemStruct->m_Transform[0][0],
                   &LibItemStruct->m_Transform[0][1],
                   &LibItemStruct->m_Transform[1][0],
                   &LibItemStruct->m_Transform[1][1] ) != 4) )
    {
        MsgDiag.Printf(
            wxT( "Component orient error at line %d, aborted" ),
            LineCount );
        Failed = TRUE;
        return Failed;
    }

    if( newfmt )
    {
        LineCount++;
        if( fgets( Line, 256 - 1, f ) == NULL )
            return TRUE;
        if( strnicmp( "$End", Line, 4 ) != 0 )
        {
            MsgDiag.Printf(
                wxT( "Component End expected at line %d, aborted" ),
                LineCount );
            Failed = TRUE;
        }
    }

    if( !Failed )
    {
        LibItemStruct->Pnext = Window->EEDrawList;
        Window->EEDrawList   = LibItemStruct;
        LibItemStruct->m_Parent = Window;
    }

    return Failed;   /* Fin lecture 1 composant */
}


/*************************************************************************************/
static int ReadSheetDescr( wxWindow* frame, char* Line, FILE* f, BASE_SCREEN* Window )
/*************************************************************************************/

/* Fonction utilisee par LoadEEFile().
 *  Lit les lignes relatives a la description d'une feuille de hierarchie
 */
{
    int                   ii, fieldref, size;
    char                  Name1[256], Char1[256], Char2[256];
    DrawSheetStruct*      SheetStruct;
    Hierarchical_PIN_Sheet_Struct* SheetLabelStruct, * OldSheetLabel = NULL;
    int                   Failed = FALSE;
    char*                 ptcar;

    SheetStruct = new DrawSheetStruct();
    SheetStruct->m_TimeStamp = GetTimeStamp();
    //sheets are added to the EEDrawList like other schematic components.
    //however, in order to preserve the heirarchy (through m_Parent pointers),
    //a duplicate of the sheet is added to m_SubSheet array.
    //must be a duplicate, references just work for a two-layer structure.
    //this is accomplished through the Sync() function.

    if( Line[0] == '$' )   /* Ligne doit etre "$Sheet" */
    {
        LineCount++;
        if( fgets( Line, 256 - 1, f ) == 0 )
        {
            MsgDiag.Printf( wxT( "Read File Errror" ) ); return TRUE;
        }
    }

    /* Next line: must be "S xx yy nn mm" with xx, yy = sheet position
     *  ( upper left corner  ) et nn,mm = sheet size */
    if( (sscanf( &Line[1], "%d %d %d %d",
                 &SheetStruct->m_Pos.x, &SheetStruct->m_Pos.y,
                 &SheetStruct->m_Size.x, &SheetStruct->m_Size.y ) != 4)
       || (Line[0] != 'S' ) )
    {
        MsgDiag.Printf(
            wxT( " ** EESchema file sheet struct error at line %d, aborted" ),
            LineCount );
        Failed = TRUE;
        return Failed;
    }

    /* Lecture des champs */
    for( ; ; ) /* Analyse des lignes "Fn "texte" .." */
    {
        LineCount++;
        if( fgets( Line, 256 - 1, f ) == NULL )
            return TRUE;
        if( Line[0] == 'U' ){
            sscanf( Line + 1, "%lX", &(SheetStruct->m_TimeStamp) );
            if(SheetStruct->m_TimeStamp == 0) //zero is not unique!
                SheetStruct->m_TimeStamp = GetTimeStamp();
            continue;
        }
        if( Line[0] != 'F' )
            break;
        sscanf( Line + 1, "%d", &fieldref );

        /* Lecture du champ :
         *  si fieldref >= 2 :  Fn "texte" t s posx posy
         *  sinon F0 "texte" pour sheetname
         *  et F1 "texte" pour filename */

        ptcar = Line; while( *ptcar && (*ptcar != '"') )
            ptcar++;

        if( *ptcar != '"' )
        {
            MsgDiag.Printf(
                wxT( " ** EESchema file sheet label F%d at line %d, aborted" ),
                fieldref, LineCount );
            return TRUE;
        }

        for( ptcar++, ii = 0; ; ii++, ptcar++ )
        {
            Name1[ii] = *ptcar;
            if( *ptcar == 0 )
            {
                MsgDiag.Printf(
                    wxT( " ** EESchema file sheet field F at line %d, aborted" ), LineCount );
                return TRUE;
            }
            if( *ptcar == '"' )
            {
                Name1[ii] = 0; ptcar++;
                break;
            }
        }

        if( ( fieldref == 0 ) || ( fieldref == 1 ) )
        {
            if( sscanf( ptcar, "%d", &size ) != 1 )
            {
                MsgDiag.Printf(
                    wxT(
                        " ** EESchema file sheet Label Caract error line %d, aborted" ), LineCount );
                DisplayError( frame, MsgDiag );
            }
            if( size == 0 )
                size = DEFAULT_SIZE_TEXT;
            if( fieldref == 0 )
            {
                SheetStruct->m_SheetName     = CONV_FROM_UTF8( Name1 );
                SheetStruct->m_SheetNameSize = size;
            }
            else
            {
                SheetStruct->SetFileName(CONV_FROM_UTF8( Name1 ));
                //printf("in ReadSheetDescr : SheetStruct->m_FileName = %s \n", Name1);
                SheetStruct->m_FileNameSize = size;
            }
        }

        if( fieldref > 1 )
        {
            SheetLabelStruct = new Hierarchical_PIN_Sheet_Struct( SheetStruct,
                                                        wxPoint( 0, 0 ), CONV_FROM_UTF8( Name1 ) );
            if( SheetStruct->m_Label == NULL )
                OldSheetLabel = SheetStruct->m_Label = SheetLabelStruct;
            else
                OldSheetLabel->Pnext = (EDA_BaseStruct*) SheetLabelStruct;
            OldSheetLabel = SheetLabelStruct;

            /* Lecture des coordonnees */
            if( sscanf( ptcar, "%s %s %d %d %d", Char1, Char2,
                        &SheetLabelStruct->m_Pos.x, &SheetLabelStruct->m_Pos.y,
                        &size ) != 5 )
            {
                MsgDiag.Printf(
                    wxT(
                        " ** EESchema file Sheet Label Caract error line %d, aborted" ), LineCount );
                DisplayError( frame, MsgDiag );
                continue;
            }

            if( size == 0 )
                size = DEFAULT_SIZE_TEXT;
            SheetLabelStruct->m_Size.x = SheetLabelStruct->m_Size.y = size;

            /* Mise a jour des cadrage et type */
            switch( Char1[0] )
            {
            case 'I':
                SheetLabelStruct->m_Shape = NET_INPUT; break;

            case 'O':
                SheetLabelStruct->m_Shape = NET_OUTPUT; break;

            case 'B':
                SheetLabelStruct->m_Shape = NET_BIDI; break;

            case 'T':
                SheetLabelStruct->m_Shape = NET_TRISTATE; break;

            case 'U':
                SheetLabelStruct->m_Shape = NET_UNSPECIFIED; break;
            }

            if( Char2[0] == 'R' )
                SheetLabelStruct->m_Edge = 1;
        }
    }

    if( strnicmp( "$End", Line, 4 ) != 0 )
    {
        MsgDiag.Printf(
            wxT( " **EESchema file end_sheet struct error at line %d, aborted" ),
            LineCount );
        Failed = TRUE;
    }
    if( !Failed )
    {
        SheetStruct->Pnext = Window->EEDrawList;
        Window->EEDrawList   = SheetStruct;
        SheetStruct->m_Parent = Window;
    }
    return Failed;   /* Fin lecture 1 composant */
}


/******************************************************************/
static int ReadSchemaDescr( wxWindow* frame, char* Line, FILE* f, BASE_SCREEN* Window )
/******************************************************************/

/* Analyse de l'entete du schema ( dims feuille, cartouche..)
 */
{
    char                 Text[256], buf[1024];
    int                  ii;
    Ki_PageDescr*        wsheet = &g_Sheet_A4;
    static Ki_PageDescr* SheetFormatList[] = {
        &g_Sheet_A4,   &g_Sheet_A3, &g_Sheet_A2, &g_Sheet_A1, &g_Sheet_A0,
        &g_Sheet_A,    &g_Sheet_B,  &g_Sheet_C,  &g_Sheet_D,  &g_Sheet_E,
        &g_Sheet_user, NULL
    };
    wxSize               PageSize;

    sscanf( Line, "%s %s %d %d", Text, Text, &PageSize.x, &PageSize.y );
    /* Recherche de la descr correspondante: */
    wxString pagename = CONV_FROM_UTF8( Text );
    for( ii = 0; SheetFormatList[ii] != NULL; ii++ )
    {
        wsheet = SheetFormatList[ii];
        if( wsheet->m_Name.CmpNoCase( pagename ) == 0 )
        {                                   /* Descr found ! */
            if( wsheet == &g_Sheet_user )   // Get the user page size and make it the default
            {
                g_Sheet_user.m_Size = PageSize;
            }
            break;
        }
    }

    if( SheetFormatList[ii] == NULL )
    {
        /* Erreur ici: descr non trouvee */
        MsgDiag.Printf(
            wxT( "EESchema file Dims Caract error line %d, aborted" ), LineCount );
        DisplayError( frame, MsgDiag );
    }

    /* Ajuste ecran */
    Window->m_CurrentSheetDesc = wsheet;

    /* Recheche suite et fin de descr */
    for( ; ; )
    {
        if( GetLine( f, Line, &LineCount, 1024 ) == NULL )
            return TRUE;
        if( strnicmp( Line, "$End", 4 ) == 0 )
            break;

        if( strnicmp( Line, "Sheet", 2 ) == 0 )
            sscanf( Line + 5, " %d %d",
                    &Window->m_ScreenNumber, &Window->m_NumberOfScreen );

        if( strnicmp( Line, "Title", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Title = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Date", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Date = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Rev", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Revision = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comp", 4 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Company = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment1", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire1 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment2", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire2 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment3", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire3 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment4", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire4 = CONV_FROM_UTF8( buf );
            continue;
        }
    }

    return FALSE;
}


/***********************************/
static void LoadLayers( FILE* f, int* linecnt )
/***********************************/

/* Load the Layer Struct from a file
 */
{
    int  cnt = 0, Number;
    char Line[1024];

//int Mode,Color,Layer;
    char Name[256];

    GetLine( f, Line, NULL, sizeof(Line) );       /* read line */
    (*linecnt)++;
    sscanf( Line, "%s %d %d", Name, &Number, &g_LayerDescr.CurrentLayer );
    if( strcmp( Name, "EELAYER" ) !=0 )
    {
        /* error : init par defaut */
        Number = MAX_LAYER;
    }
    if( Number <= 0 )
        Number = MAX_LAYER;
    if( Number > MAX_LAYER )
        Number = MAX_LAYER;

    g_LayerDescr.NumberOfLayers = Number;

    while( GetLine( f, Line, NULL, sizeof(Line) ) )
    {
        (*linecnt)++;
        if( strnicmp( Line, "EELAYER END", 11 ) == 0 )
            break;
        cnt++;
    }
}
