/****************************/
/* readschematicnetlist.cpp */
/****************************/

/* Read a nelist type Eeschema or OrcadPCB2 and build the component list
 */

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "macros.h"

#include "cvpcb.h"
#include "cvpcb_mainframe.h"
#include "richio.h"


#define SEPARATEUR '|'  /* Separator character in NetList */


static int ReadPinConnection( FILE_LINE_READER& aNetlistReader, COMPONENT* CurrentCmp );
static int ReadFootprintFilterList( FILE_LINE_READER& aNetlistReader, COMPONENT_LIST& aComponentsList );


/* Sort the list alphabetically by component and and returns
 * a pointer to the 1st element of list */

#define BUFFER_CHAR_SIZE 1024   // Size of buffers used to store netlist data


/**
 * Function ReadSchematicNetlist
 * Read a Eeschema (or OrcadPCB) netlist
 * like:
 * # EESchema Netlist Version 1.1 created  15/5/2008-12:09:21
 * (
 *  ( /32568D1E $noname  JP1 CONN_8X2 {Lib=CONN_8X2}
 *   (    1 GND )
 *   (    2 /REF10 )
 *   (    3 GND )
 *   (    4 /REF11 )
 *   (    5 GND )
 *   (    6 /REF7 )
 *   (    7 GND )
 *   (    8 /REF9 )
 *   (    9 GND )
 *   (   10 /REF6 )
 *   (   11 GND )
 *   (   12 /REF8 )
 *   (   13 GND )
 *   (   14 /REF4 )
 *   (   15 GND )
 *   (   16 /REF5 )
 *  )
 *  ( /325679C1 $noname  RR1 9x1K {Lib=RR9}
 *   (    1 VCC )
 *   (    2 /REF5 )
 *   (    3 /REF4 )
 *   (    4 /REF8 )
 *   (    5 /REF6 )
 *   (    6 /REF9 )
 *   (    7 /REF7 )
 *   (    8 /REF11 )
 *   (    9 /REF10 )
 *   (   10  ? )
 *  )
 * )
 * *
 * { Allowed footprints by component:
 * $component R5
 * R?
 * SM0603
 * SM0805
 * $endlist
 * $component C2
 * SM*
 * C?
 * C1-1
 * $endlist
 * $endfootprintlist
 * }
 */
int CVPCB_MAINFRAME::ReadSchematicNetlist()
{
    char       alim[1024];
    int        idx, jj, k, l;
    char       cbuffer[BUFFER_CHAR_SIZE];      /* temporary storage */
    char*      ptchar;
    COMPONENT* Cmp;
    FILE*      source;

    m_modified = false;
    m_isEESchemaNetlist = false;

    /* Clear components buffer */
    if( !m_components.empty() )
    {
        m_components.clear();
    }

    source = wxFopen( m_NetlistFileName.GetFullPath(), wxT( "rt" ) );

    if( source == 0 )
    {
        DisplayError( this, _( "File <" ) + m_NetlistFileName.GetFullPath() +
                     _( "> not found" ) );
        return -1;
    }

    // FILE_LINE_READER will close the file.
    FILE_LINE_READER netlistReader( source, m_NetlistFileName.GetFullPath() );

    // hopes netlistReader's line buffer does not move.  It won't unless you encounter
    // a line larger than LINE_READER_LINE_INITIAL_SIZE = 5000
    const char* Line = netlistReader.Line();

    /* Read the file header (must be  "( { OrCAD PCB" or "({ OrCAD PCB" )
     * or "# EESchema Netlist"
     */
    netlistReader.ReadLine();

    /* test for netlist type PCB2 */
    idx = strnicmp( Line, "( {", 3 );
    if( idx != 0 )
        idx = strnicmp( Line, "({", 2 );
    if( idx != 0 )
    {
        idx = strnicmp( Line, "# EESchema", 7 ); /* net type EESchema */
        if( idx == 0 )
            m_isEESchemaNetlist = true;
    }

    if( idx != 0 )
    {
        wxString msg, Lineconv = FROM_UTF8( Line );
        msg.Printf( _( "Unknown file format <%s>" ), Lineconv.GetData() );
        DisplayError( this, msg );
        return -3;
    }

    SetStatusText( _( "Netlist Format: EESchema" ), 0 );


    /* Read the netlist */
    for( ; ; )
    {
        /* Search the beginning of a component description */

        if( netlistReader.ReadLine( ) == 0 )
            break;

        /* Remove blanks */
        idx = 0;
        while( Line[idx] == ' ' )
            idx++;

        /* remove empty lines : */
        if( Line[idx] < ' ' )
            continue;

        if( strnicmp( &Line[idx], "{ Allowed footprints", 20 ) == 0 )
        {
            ReadFootprintFilterList( netlistReader, m_components );
            continue;
        }

        if( strnicmp( &Line[idx], "( ", 2 ) != 0 )
            continue;

        /*******************************/
        /* Component description found */
        /*******************************/
        Cmp = new COMPONENT();  // Creates the new component storage

        while( Line[idx] != ' ' )
            idx++;

        while( Line[idx] == ' ' )
            idx++;

        /* idx points the beginning of the schematic time stamp */
        jj = 0;
        while( Line[idx] != ' ' && Line[idx] )
            cbuffer[jj++] = Line[idx++];
        cbuffer[jj] = 0;
        Cmp->m_TimeStamp = FROM_UTF8(cbuffer);

        /* search val/ref.lib */
        while( Line[idx] == ' ' )
            idx++;

        /* idx points the component value.
         * Read value */
        ptchar = strstr( (char*) &Line[idx], " " );  // Search end of value field (space)
        if( ptchar == 0 )
        {
            wxString msg = _( "Netlist error: " );
            msg << FROM_UTF8( Line );
            DisplayError( this, msg );
            k = 0;
        }
        else
            k = ptchar - Line;

        for( jj = 0; idx < k; idx++ )
        {
            if( Line[idx] == SEPARATEUR )
                break;
            cbuffer[jj++] = Line[idx];
        }
        cbuffer[jj] = 0;
        // Copy footprint name:
        if( m_isEESchemaNetlist &&  strnicmp( cbuffer, "$noname", 7 ) != 0 )
            Cmp->m_Module = FROM_UTF8(cbuffer);

        if( (Line[++idx] == '(') && (Line[k - 1] == ')' ) )
        {
            idx++; l = 0;
            while( k - 1 > idx )
                alim[l++] = Line[idx++];
        }
        else
            idx = k;

        /* Search component reference */
        while( Line[idx] != ' ' && Line[idx] )
            idx++;

        /* goto end of value field */
        while( Line[idx] == ' ' && Line[idx] )
            idx++;

        /* goto beginning of reference */
        for( jj = 0; ; idx++ )
        {
            if( Line[idx] == ' ' || Line[idx] == 0)
                break;
            cbuffer[jj++] = Line[idx];
        }
        cbuffer[jj] = 0;
        Cmp->m_Reference = FROM_UTF8(cbuffer);

        /* Search component value */
        while( Line[idx] == ' ' && Line[idx] )
            idx++;

        /** goto beginning of value */

        for( jj = 0 ; ; idx++ )
        {
            if( (Line[idx] == ' ') || (Line[idx] == '\n') || (Line[idx] == '\r') || Line[idx] == 0)
                break;
            cbuffer[jj++] = Line[idx];
        }
        cbuffer[jj] = 0;
        Cmp->m_Value = FROM_UTF8(cbuffer);

        m_components.push_back( Cmp );

        ReadPinConnection( netlistReader, Cmp );
    }

    m_components.sort();

    return 0;
}


int ReadFootprintFilterList(  FILE_LINE_READER& aNetlistReader, COMPONENT_LIST& aComponentsList )
{
    const char* Line = aNetlistReader;
    wxString    CmpRef;
    COMPONENT*  Cmp = NULL;

    for( ; ; )
    {
        if( aNetlistReader.ReadLine( )  == 0 )
            break;
        if( strnicmp( Line, "$endlist", 8 ) == 0 )
        {
            Cmp = NULL;
            continue;
        }
        if( strnicmp( Line, "$endfootprintlist", 4 ) == 0 )
            return 0;

        if( strnicmp( Line, "$component", 10 ) == 0 ) // New component ref found
        {
            CmpRef = FROM_UTF8( Line + 11 );
            CmpRef.Trim( true );
            CmpRef.Trim( false );

            /* Search the new component in list */
            BOOST_FOREACH( COMPONENT & component, aComponentsList )
            {
                Cmp = &component;
                if( Cmp->m_Reference == CmpRef )
                    break;
            }
        }
        else if( Cmp )
        {
            wxString fp = FROM_UTF8( Line + 1 );
            fp.Trim( false );
            fp.Trim( true );
            Cmp->m_FootprintFilter.Add( fp );
        }
    }

    return 1;
}


int ReadPinConnection( FILE_LINE_READER& aNetlistReader, COMPONENT* Cmp )
{
    int         i, jj;
    char*       Line = aNetlistReader;
    char        cbuffer[BUFFER_CHAR_SIZE];

    for( ; ; )
    {
        /* Find beginning of description. */
        for( ; ; )
        {
            if( aNetlistReader.ReadLine() == 0 )
                return -1;

            /* Remove blanks from the beginning of the line. */
            i = 0; while( Line[i] == ' ' )
                i++;

            while( Line[i] == '(' )
                i++;

            while( Line[i] == ' ' )
                i++;

            /* remove empty lines : */
            if( Line[i] < ' ' )
                continue;

            /* End of description? */
            if( Line[i] == ')' )
                return 0;

            PIN * Pin = new PIN();

            /* Read pin name, usually 4 letters */
            for( jj = 0; ; i++ )
            {
                if( Line[i] == ' ' || Line[i] == 0 )
                    break;
                cbuffer[jj++] = Line[i];
            }
            cbuffer[jj] =  0;
            Pin->m_Number = FROM_UTF8(cbuffer);

            /* Read netname */
            while( Line[i] == ' ' )
                i++;

            for( jj = 0; ; i++ )
            {
                if( Line[i] == ' ' || Line[i] == '\n' || Line[i] == '\r' || Line[i] == 0 )
                    break;
                cbuffer[jj++] = Line[i];
            }
            cbuffer[jj] =  0;
            Pin->m_Net = FROM_UTF8(cbuffer);

            Cmp->m_Pins.push_back( Pin );
        }
    }
}
