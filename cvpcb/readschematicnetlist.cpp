/****************************/
/* readschematicnetlist.cpp */
/****************************/

/* Read a nelist type Eeschema or OrcadPCB2 and buid the component list
 * Manages the lines like :
 * ( XXXXXX VALEUR|(pin1,pin2,...=newalim) ID VALEUR
 */

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"

#include "protos.h"

#define SEPARATEUR '|'  /* caractere separateur dans netliste */

/* routines locales : */

static int ReadPinConnection( STORECMP* CurrentCmp );

#define BUFFER_CHAR_SIZE 1024   // Size of buffers used to  store netlist datas

/************************************************/
int WinEDA_CvpcbFrame::ReadSchematicNetlist()
/************************************************/
{
    int       i, j, k, l;
    char*     LibName;
    char      Line[BUFFER_CHAR_SIZE + 1];
    char      component_reference[BUFFER_CHAR_SIZE + 1];    /* buffer for component reference (U1, R4...) */
    char      schematic_timestamp[BUFFER_CHAR_SIZE + 1];    /* buffer for component time stamp */
    char      footprint_name[BUFFER_CHAR_SIZE + 1];         /* buffer for component footprint field */
    char      component_value[BUFFER_CHAR_SIZE + 1];        /* buffer for component values (470K, 22nF ...) */
    char*     ptchar;                                       /* pointeur de service */
    STORECMP* Cmp;

    modified = 0;
    Rjustify = 0;
    g_FlagEESchema = FALSE;

    /* Raz buffer et variable de gestion */
    if( g_BaseListeCmp )
        FreeMemoryComponants();

    /* Ouverture du fichier source */
    source = wxFopen( FFileName, wxT( "rt" ) );
    if( source == 0 )
    {
        wxString msg;
        msg.Printf( _( "File <%s> not found" ), FFileName.GetData() );
        DisplayError( this, msg ); return -1;
    }

    /* Read the file header (must be  "( { OrCAD PCB" or "({ OrCAD PCB" )
     * or "# EESchema Netliste"
     */
    fgets( Line, 255, source );
    /* test for netlist type PCB2 */
    i = strnicmp( Line, "( {", 3 );
    if( i != 0 )
        i = strnicmp( Line, "({", 2 );
    if( i != 0 )
    {
        i = strnicmp( Line, "# EESchema", 7 ); /* net type EESchema */
        if( i == 0 )
            g_FlagEESchema = TRUE;
    }

    if( i != 0 )
    {
        wxString msg, Lineconv = CONV_FROM_UTF8( Line );
        msg.Printf( _( "Unknown file format <%s>" ), Lineconv.GetData() );
        DisplayError( this, msg );
        fclose( source ); return -3;
    }

    SetStatusText( _( "Netlist Format: EESchema" ), 0 );


    /* Read the netlit */
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

        j = 0; while( Line[i] != ' ' )
            schematic_timestamp[j++] = Line[i++];

        schematic_timestamp[j] = 0;

        /* search val/ref.lib */
        while( Line[i] == ' ' )
            i++;

        /* i points the component value */
        LibName = Line + i;

        memset( schematic_timestamp, 0, sizeof(schematic_timestamp) );
        memset( component_reference, 0, sizeof(component_reference) );
        memset( footprint_name, 0, sizeof(footprint_name) );
        memset( component_value, 0, sizeof(component_value) );
        memset( alim, 0, sizeof(alim) );

        /* Read value */

        ptchar = strstr( &Line[i], " " );  // Search end of value field (space)
        if( ptchar == 0 )
        {
            wxString msg;
            msg.Printf( _( "Netlist error: %s" ), Line );
            DisplayError( NULL, msg );
            k = 0;
        }
        else
            k = ptchar - Line;

        for( j = 0; i < k;  i++ )
        {
            if( Line[i] == SEPARATEUR )
                break;
            if( j < (int) (sizeof(footprint_name) - 1) )
                footprint_name[j++] = Line[i];
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
        for( k = 0; k < (int) (sizeof(component_reference) - 1); i++, k++ )
        {
            if( Line[i] <= ' ' )
                break;
            component_reference[k] = Line[i];
        }

        /* Search component value */
        while( Line[i] == ' ' )
            i++;

        /** goto beginning of  value */

        for( k = 0; k < (int) (sizeof(component_value) - 1); i++, k++ )
        {
            if( Line[i] <= ' ' )
                break;
            component_value[k] = Line[i];
        }

        /* Store info for this component */
        Cmp = new STORECMP();
        Cmp->Pnext       = g_BaseListeCmp;
        g_BaseListeCmp   = Cmp;
        Cmp->m_Reference = CONV_FROM_UTF8( component_reference );
        Cmp->m_Valeur    = CONV_FROM_UTF8( component_value );

        if(  g_FlagEESchema )   /* copy footprint name: */
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
        Cmp->m_TimeStamp = CONV_FROM_UTF8( schematic_timestamp );

        ReadPinConnection( Cmp );

        nbcomp++;
    }

    fclose( source );

    /* Alpabetic sorting : */
    g_BaseListeCmp = TriListeComposantss( g_BaseListeCmp, nbcomp );

    return 0;
}


/********************************************************/
int WinEDA_CvpcbFrame::ReadFootprintFilterList( FILE* f )
/********************************************************/
{
    char      Line[BUFFER_CHAR_SIZE + 1];
    wxString  CmpRef;
    STORECMP* Cmp = NULL;

    for( ; ; )
    {
        if( fgets( Line, BUFFER_CHAR_SIZE, source )  == 0 )
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
            CmpRef.Trim( TRUE ); CmpRef.Trim( FALSE );
            /* Search the new component in list */
            for( Cmp = g_BaseListeCmp; Cmp != NULL; Cmp = Cmp->Pnext )
            {
                if( Cmp->m_Reference == CmpRef )
                    break;
            }
        }
        else if( Cmp )
        {
            wxString fp = CONV_FROM_UTF8( Line + 1 );
            fp.Trim( FALSE ); fp.Trim( TRUE );
            Cmp->m_FootprintFilter.Add( fp );
        }
    }

    return 1;
}


/***********************************/
int ReadPinConnection( STORECMP* Cmp )
/***********************************/
{
    int        i, jj;
    char       numpin[BUFFER_CHAR_SIZE + 1], net[BUFFER_CHAR_SIZE + 1];
    char       Line[BUFFER_CHAR_SIZE + 1];
    STOREPIN*  Pin     = NULL;
    STOREPIN** LastPin = &Cmp->m_Pins;

    for( ; ; )
    {
        /* debut description trouv‚ */
        for( ; ; )
        {
            if( fgets( Line, BUFFER_CHAR_SIZE, source ) == 0 )
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

            memset( net, 0, sizeof(net) );
            memset( numpin, 0, sizeof(numpin) );

            /* Read pi name , 4 letters */
            for( jj = 0; jj < 4; jj++, i++ )
            {
                if( Line[i] == ' ' )
                    break;
                numpin[jj] = Line[i];
            }

            /* Search for a net attribute  */
            if( reaffect( numpin, net ) != 0 )
            {
                Pin           = new STOREPIN();
                *LastPin      = Pin; LastPin = &Pin->Pnext;
                Pin->m_PinNum = CONV_FROM_UTF8( numpin );
                Pin->m_PinNet = CONV_FROM_UTF8( net );
                continue;
            }

            /* Read netname */
            while( Line[i] == ' ' )
                i++;

            for( jj = 0; jj < (int) sizeof(net) - 1; i++, jj++ )
            {
                if( Line[i] <= ' ' )
                    break;
                net[jj] = Line[i];
            }

            Pin           = new STOREPIN();
            *LastPin      = Pin; LastPin = &Pin->Pnext;
            Pin->m_PinNum = CONV_FROM_UTF8( numpin );
            Pin->m_PinNet = CONV_FROM_UTF8( net );
        }
    }
}


/****************************************************************/
STORECMP* TriListeComposantss( STORECMP* BaseListe, int nbitems )
/****************************************************************/

/* Sort the component list( this is a linked list)
 * retourn the beginning of the list
 */
{
    STORECMP** bufferptr, * Item;
    int        ii;

    if( nbitems <= 0 )
        return NULL;
    bufferptr = (STORECMP**) MyZMalloc( (nbitems + 2) * sizeof(STORECMP*) );

    for( ii = 1, Item = BaseListe; Item != NULL; Item = Item->Pnext, ii++ )
    {
        bufferptr[ii] = Item;
    }

    /* Here: bufferptr[0] = NULL and bufferptr[nbitem+1] = NULL.
     * These 2 values are the first item back link, and the last item forward link
     */

    qsort( bufferptr + 1, nbitems, sizeof(STORECMP*),
           ( int( * ) ( const void*, const void* ) )CmpCompare );
    /* Update linked list */
    for( ii = 1; ii <= nbitems; ii++ )
    {
        Item = bufferptr[ii];
        Item->m_Num = ii;
        Item->Pnext = bufferptr[ii + 1];
        Item->Pback = bufferptr[ii - 1];
    }

    return bufferptr[1];
}


/****************************************/
int CmpCompare( void* mod1, void* mod2 )
/****************************************/

/*
 * Function compare() for qsort() : alphabetic sorting, with numbering order
 */
{
    int       ii;
    STORECMP* pt1, * pt2;

    pt1 = *( (STORECMP**) mod1 );
    pt2 = *( (STORECMP**) mod2 );

    //FIXME:
    ii = StrNumICmp( (const wxChar*) pt1->m_Reference, (const wxChar*) pt2->m_Reference );
    return ii;
}
