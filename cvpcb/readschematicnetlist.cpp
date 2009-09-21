/****************************/
/* readschematicnetlist.cpp */
/****************************/

/* Read a nelist type Eeschema or OrcadPCB2 and build the component list
 * Manages the lines like :
 * ( XXXXXX VALEUR|(pin1,pin2,...=newalim) ID VALEUR
 */

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "macros.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


#define SEPARATEUR '|'  /* caractere separateur dans netliste */

/* routines locales : */

static int ReadPinConnection( FILE* f, COMPONENT* CurrentCmp );

/* Tri la liste des composants par ordre alphabetique et met a jour le nouveau
 * chainage avant/arriere retourne un pointeur sur le 1er element de la liste */

#define BUFFER_CHAR_SIZE 1024   // Size of buffers used to  store netlist data


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
int WinEDA_CvpcbFrame::ReadSchematicNetlist()
{
    char       alim[1024];
    int        i, k, l;
    char*      LibName;
    char       Line[BUFFER_CHAR_SIZE + 1];
    wxString   component_reference;  /* buffer for component reference (U1, R4...) */
    wxString   schematic_timestamp;  /* buffer for component time stamp */
    wxString   footprint_name;       /* buffer for component footprint field */
    wxString   component_value;      /* buffer for component values (470K, 22nF ...) */
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
        DisplayError( this,  _( "File <" ) + m_NetlistFileName.GetFullPath() +
                      _( "> not found" )  );
        return -1;
    }

    /* Read the file header (must be  "( { OrCAD PCB" or "({ OrCAD PCB" )
     * or "# EESchema Netliste"
     */
    fgets( Line, BUFFER_CHAR_SIZE, source );
    /* test for netlist type PCB2 */
    i = strnicmp( Line, "( {", 3 );
    if( i != 0 )
        i = strnicmp( Line, "({", 2 );
    if( i != 0 )
    {
        i = strnicmp( Line, "# EESchema", 7 ); /* net type EESchema */
        if( i == 0 )
            m_isEESchemaNetlist = TRUE;
    }

    if( i != 0 )
    {
        wxString msg, Lineconv = CONV_FROM_UTF8( Line );
        msg.Printf( _( "Unknown file format <%s>" ), Lineconv.GetData() );
        DisplayError( this, msg );
        fclose( source );
        return -3;
    }

    SetStatusText( _( "Netlist Format: EESchema" ), 0 );


    /* Read the netlist */
    for( ; ; )
    {
        /* Search the beginning of a component description */

        if( fgets( Line, BUFFER_CHAR_SIZE, source )  == 0 )
            break;

        /* Remove blanks */
        i = 0; while( Line[i] == ' ' )
            i++;

        /* remove empty lines : */
        if( Line[i] < ' ' )
            continue;

        if( strnicmp( &Line[i], "{ Allowed footprints", 20 ) == 0 )
        {
            ReadFootprintFilterList( source );
            continue;
        }

        if( strnicmp( &Line[i], "( ", 2 ) != 0 )
            continue;

        /*******************************/
        /* Component description found */
        /*******************************/
        while( Line[i] != ' ' )
            i++;

        while( Line[i] == ' ' )
            i++;

        /* i points the beginning of the schematic time stamp */

        schematic_timestamp.Empty();
        while( Line[i] != ' ' )
            schematic_timestamp.Append( Line[i++] );

        /* search val/ref.lib */
        while( Line[i] == ' ' )
            i++;

        /* i points the component value */
        LibName = Line + i;

        component_reference.Empty();
        footprint_name.Empty();
        component_value.Empty();
        memset( alim, 0, sizeof(alim) );

        /* Read value */

        ptchar = strstr( &Line[i], " " );  // Search end of value field (space)
        if( ptchar == 0 )
        {
            wxString msg;
            msg.Printf( _( "Netlist error: %s" ), Line );
            DisplayError( this, msg );
            k = 0;
        }
        else
            k = ptchar - Line;

        for( ; i < k; i++ )
        {
            if( Line[i] == SEPARATEUR )
                break;
            footprint_name.Append( Line[i] );
        }

        if( (Line[++i] == '(') && (Line[k - 1] == ')' ) )
        {
            i++; l = 0; while( k - 1 > i )
                alim[l++] = Line[i++];
        }
        else
            i = k;

        /* Search component reference */
        while( Line[i] != ' ' )
            i++;

        /* goto end of value field */
        while( Line[i] == ' ' )
            i++;

        /* goto beginning of  reference */

        /* debut reference trouv‚ */
        for( ; ; i++ )
        {
#if defined(KICAD_GOST)
            if( Line[i] == ' ' )
#else
            if( Line[i] <= ' ' )
#endif
                break;
            component_reference.Append( Line[i] );
        }

        /* Search component value */
        while( Line[i] == ' ' )
            i++;

        /** goto beginning of  value */

        for( ; ; i++ )
        {
#if defined(KICAD_GOST)
            if( (Line[i] == ' ')  || (Line[i] == '\n') )
#else
            if( Line[i] <= ' ' )
#endif
                break;
            component_value.Append( Line[i] );
        }

        /* Store info for this component */
        Cmp = new COMPONENT();
        Cmp->m_Reference = component_reference;
        Cmp->m_Value     = component_value;
        m_components.push_back( Cmp );

        if(  m_isEESchemaNetlist )   /* copy footprint name: */
        {
            if( strnicmp( LibName, "$noname", 7 ) != 0 )
            {
                while( *LibName > ' ' )
                {
                    Cmp->m_Module.Append( *LibName );
                    LibName++;
                }
            }
        }
        Cmp->m_TimeStamp = schematic_timestamp;

        ReadPinConnection( source, Cmp );
    }

    fclose( source );

    m_components.sort();

    return 0;
}


int WinEDA_CvpcbFrame::ReadFootprintFilterList( FILE* f )
{
    char       Line[BUFFER_CHAR_SIZE + 1];
    wxString   CmpRef;
    COMPONENT* Cmp = NULL;

    for( ; ; )
    {
        if( fgets( Line, BUFFER_CHAR_SIZE, f )  == 0 )
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
            CmpRef = CONV_FROM_UTF8( Line + 11 );
            CmpRef.Trim( TRUE );
            CmpRef.Trim( FALSE );

            /* Search the new component in list */
            BOOST_FOREACH( COMPONENT& component, m_components )
            {
                Cmp = &component;

                if( Cmp->m_Reference == CmpRef )
                    break;
            }
        }
        else if( Cmp )
        {
            wxString fp = CONV_FROM_UTF8( Line + 1 );
            fp.Trim( FALSE );
            fp.Trim( TRUE );
            Cmp->m_FootprintFilter.Add( fp );
        }
    }

    return 1;
}


int ReadPinConnection( FILE* f, COMPONENT* Cmp )
{
    int      i, jj;
    wxString numpin;
    wxString net;
    char     Line[BUFFER_CHAR_SIZE + 1];
    PIN*     Pin     = NULL;

    for( ; ; )
    {
        /* debut description trouv‚ */
        for( ; ; )
        {
            if( fgets( Line, BUFFER_CHAR_SIZE, f ) == 0 )
                return -1;

            /* remove blanks from the beginning of the line */
            i = 0; while( Line[i] == ' ' )
                i++;

            while( Line[i] == '(' )
                i++;

            while( Line[i] == ' ' )
                i++;

            /* remove empty lines : */
            if( Line[i] < ' ' )
                continue;

            /* fin de description ? */
            if( Line[i] == ')' )
                return 0;

            net.Empty();
            numpin.Empty();

            /* Read pin name , 4 letters */
            for( jj = 0; jj < 4; jj++, i++ )
            {
                if( Line[i] == ' ' )
                    break;
                numpin.Append( Line[i] );
            }

            /* Read netname */
            while( Line[i] == ' ' )
                i++;

            for( ; ; i++ )
            {
                if( Line[i] <= ' ' )
                    break;
                net.Append( Line[i] );
            }

            Pin           = new PIN();
            Pin->m_Number = numpin;
            Pin->m_Net = net;
            Cmp->m_Pins.push_back( Pin );
        }
    }
}
