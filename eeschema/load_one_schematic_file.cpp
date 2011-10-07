/**
 * @file load_one_schematic_file.cpp
 * @brief Code to load and save Eeschema files.
 */

#include "fctsys.h"
#include "confirm.h"
#include "kicad_string.h"
#include "wxEeschemaStruct.h"
#include "richio.h"

#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"
#include "sch_marker.h"
#include "sch_junction.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_component.h"
#include "sch_polyline.h"
#include "sch_text.h"
#include "sch_sheet.h"
#include "sch_bitmap.h"


bool ReadSchemaDescr( LINE_READER* aLine, wxString& aMsgDiag, BASE_SCREEN* Window );

static void LoadLayers( LINE_READER* aLine );


bool SCH_EDIT_FRAME::LoadOneEEFile( SCH_SCREEN* aScreen, const wxString& aFullFileName )
{
    char            Name1[256];
    bool            itemLoaded = false;
    SCH_ITEM*       Phead;
    SCH_ITEM*       Pnext;
    SCH_ITEM*       item;
    wxString        MsgDiag;            // Error and log messages
    char*           line;

    if( aScreen == NULL )
        return FALSE;

    if( aFullFileName.IsEmpty() )
        return FALSE;

    aScreen->SetCurItem( NULL );
    aScreen->SetFileName( aFullFileName );

    // D(printf("LoadOneEEFile:%s\n", TO_UTF8( aFullFileName ) ); )

    FILE*           f;
    if( ( f = wxFopen( aFullFileName, wxT( "rt" ) ) ) == NULL )
    {
        MsgDiag = _( "Failed to open " ) + aFullFileName;
        DisplayError( this, MsgDiag );
        return FALSE;
    }

    // reader now owns the open FILE.
    FILE_LINE_READER    reader( f, aFullFileName );

    MsgDiag = _( "Loading " ) + aScreen->GetFileName();
    PrintMsg( MsgDiag );

    if( !reader.ReadLine()
        || strncmp( (char*)reader + 9, SCHEMATIC_HEAD_STRING, sizeof(SCHEMATIC_HEAD_STRING) - 1 ) != 0 )
    {
        MsgDiag = aFullFileName + _( " is NOT an Eeschema file!" );
        DisplayError( this, MsgDiag );
        return FALSE;
    }

    line = reader.Line();

    // get the file version here.
    char *strversion = line + 9 + sizeof(SCHEMATIC_HEAD_STRING);

    // Skip blanks
    while( *strversion && *strversion < '0' )
        strversion++;

    int  version = atoi(strversion);

    if( version > EESCHEMA_VERSION )
    {
        MsgDiag = aFullFileName + _( " was created by a more recent \
version of Eeschema and may not load correctly. Please consider updating!" );
        DisplayInfoMessage( this, MsgDiag );
    }

#if 0
    // Compile it if the new version is unreadable by previous Eeschema versions
    else if( version < EESCHEMA_VERSION )
    {
        MsgDiag = aFullFileName + _( " was created by an older version of \
Eeschema. It will be stored in the new file format when you save this file \
again." );

        DisplayInfoMessage( this, MsgDiag );
    }
#endif

    if( !reader.ReadLine() || strncmp( reader, "LIBS:", 5 ) != 0 )
    {
        MsgDiag = aFullFileName + _( " is NOT an Eeschema file!" );
        DisplayError( this, MsgDiag );
        return FALSE;
    }

    LoadLayers( &reader );

    while( reader.ReadLine() )
    {
        line = reader.Line();

        item = NULL;

        char* sline = line;
        while( (*sline != ' ' ) && *sline )
            sline++;

        switch( line[0] )
        {
        case '$':           // identification block
            if( line[1] == 'C' )
                item = new SCH_COMPONENT();
            else if( line[1] == 'S' )
                item = new SCH_SHEET();
            else if( line[1] == 'D' )
                itemLoaded = ReadSchemaDescr( &reader, MsgDiag, aScreen );
            else if( line[1] == 'B' )
                item = new SCH_BITMAP();
            break;

        case 'L':        // Its a library item.
            item = new SCH_COMPONENT();
            break;

        case 'W':        // Its a Segment (WIRE or BUS) item.
            item = new SCH_LINE();
            break;

        case 'E':        // Its a WIRE or BUS item.
            item = new SCH_BUS_ENTRY();
            break;

        case 'P':        // Its a polyline item.
            item = new SCH_POLYLINE();
            break;

        case 'C':        // It is a connection item.
            item = new SCH_JUNCTION();
            break;

        case 'K':                       // It is a Marker item.
            // Markers are no more read from file. they are only created on
            // demand in schematic
            break;

        case 'N':                       // It is a NoConnect item.
            item = new SCH_NO_CONNECT();
            break;

        case 'T':                       // It is a text item.
            if( sscanf( sline, "%s", Name1 ) != 1 )
            {
                MsgDiag.Printf( wxT( "Eeschema file text load error at line %d" ),
                                reader.LineNumber() );
                itemLoaded = false;
            }
            else if( Name1[0] == 'L' )
                item = new SCH_LABEL();
            else if( Name1[0] == 'G' && version > 1 )
                item = new SCH_GLOBALLABEL();
            else if( (Name1[0] == 'H') || (Name1[0] == 'G' && version == 1) )
                item = new SCH_HIERLABEL();
            else
                item = new SCH_TEXT();
            break;

        default:
            itemLoaded = false;
            MsgDiag.Printf( wxT( "Eeschema file undefined object at line %d, aborted" ),
                            reader.LineNumber() );
            MsgDiag << wxT( "\n" ) << FROM_UTF8( line );
        }

        if( item )
        {
            itemLoaded = item->Load( reader, MsgDiag );

            if( !itemLoaded )
            {
                SAFE_DELETE( item );
            }
            else
            {
                item->SetNext( aScreen->GetDrawItems() );
                aScreen->SetDrawItems( item );
            }
        }

        if( !itemLoaded )
        {
            DisplayError( this, MsgDiag );
            break;
        }
    }

    /* GetDrawItems() was constructed in reverse order - reverse it back: */
    Phead = NULL;

    while( aScreen->GetDrawItems() )
    {
        Pnext = aScreen->GetDrawItems();
        aScreen->SetDrawItems( aScreen->GetDrawItems()->Next() );
        Pnext->SetNext( Phead );
        Phead = Pnext;
    }

    aScreen->SetDrawItems( Phead );

#if 0 && defined (DEBUG)
    aScreen->Show( 0, std::cout );
#endif

    aScreen->TestDanglingEnds();

    MsgDiag = _( "Done Loading " ) + aScreen->GetFileName();
    PrintMsg( MsgDiag );

    return true;    // Although it may be that file is only partially loaded.
}


static void LoadLayers( LINE_READER* aLine )
{
    int  Number;

    //int Mode,Color,Layer;
    char Name[256];

    aLine->ReadLine();

    sscanf( *aLine, "%s %d %d", Name, &Number, &g_LayerDescr.CurrentLayer );
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

    while( aLine->ReadLine() )
    {
        if( strnicmp( *aLine, "EELAYER END", 11 ) == 0 )
            break;
    }
}


/* Read the schematic header. */
bool ReadSchemaDescr( LINE_READER* aLine, wxString& aMsgDiag, BASE_SCREEN* aScreen )
{
    char            Text[256];
    char            buf[1024];
    int             ii;
    Ki_PageDescr*   wsheet = &g_Sheet_A4;
    wxSize          PageSize;
    char*           line;

    static Ki_PageDescr* SheetFormatList[] =
    {
        &g_Sheet_A4,   &g_Sheet_A3,   &g_Sheet_A2,   &g_Sheet_A1, &g_Sheet_A0,
        &g_Sheet_A,    &g_Sheet_B,    &g_Sheet_C,    &g_Sheet_D,  &g_Sheet_E,
        &g_Sheet_user, NULL
    };

    line = aLine->Line();

    sscanf( line, "%s %s %d %d", Text, Text, &PageSize.x, &PageSize.y );

    wxString pagename = FROM_UTF8( Text );

    for( ii = 0; SheetFormatList[ii] != NULL; ii++ )
    {
        wsheet = SheetFormatList[ii];

        if( wsheet->m_Name.CmpNoCase( pagename ) == 0 ) /* Descr found ! */
        {
            // Get the user page size and make it the default
            if( wsheet == &g_Sheet_user )
            {
                g_Sheet_user.m_Size = PageSize;
            }

            break;
        }
    }

    if( SheetFormatList[ii] == NULL )
    {
        aMsgDiag.Printf( wxT( "Eeschema file dimension definition error \
line %d, \aAbort reading file.\n" ),
                         aLine->LineNumber() );
        aMsgDiag << FROM_UTF8( line );
    }

    aScreen->m_CurrentSheetDesc = wsheet;

    for( ; ; )
    {
        if( !aLine->ReadLine() )
            return true;

        line = aLine->Line();

        if( strnicmp( line, "$End", 4 ) == 0 )
            break;

        if( strnicmp( line, "Sheet", 2 ) == 0 )
            sscanf( line + 5, " %d %d",
                    &aScreen->m_ScreenNumber, &aScreen->m_NumberOfScreen );

        if( strnicmp( line, "Title", 2 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Title = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( line, "Date", 2 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Date = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( line, "Rev", 2 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Revision = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( line, "Comp", 4 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Company = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( line, "Comment1", 8 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Commentaire1 = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( line, "Comment2", 8 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Commentaire2 = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( line, "Comment3", 8 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Commentaire3 = FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( line, "Comment4", 8 ) == 0 )
        {
            ReadDelimitedText( buf, line, 256 );
            aScreen->m_Commentaire4 = FROM_UTF8( buf );
            continue;
        }
    }

    return true;
}
