/***************************/
/* writenetlistpcbnew.cpp  */
/***************************/

/*
 * Complete la netliste (*.NET) en y placant les ref *.lib FORMAT PCBNEW ou
 * ORCADPCB
 */

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "macros.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "protos.h"

#define MAX_LEN_NETNAME 16

/* Routines locales */
static void ChangePinNet( COMPONENT_LIST& list, wxString& PinNet,
                          int* netNumber, bool rightJustify );
static void WriteFootprintFilterInfos( FILE* dest, COMPONENT_LIST& list );


static void RemoveDuplicatePins( COMPONENT& component )
{
    PIN_LIST::iterator i;
    PIN *pin1, *pin2;
    wxString msg;

    if( component.m_Pins.size() <= 1 )
        return;

    i = component.m_Pins.begin();
    pin1 = &(*i);
    ++i;

    while( i != component.m_Pins.end() )
    {
        pin2 = &(*i);

        wxASSERT( pin2 != NULL );

        if( !same_pin_number( pin1, pin2 ) )
        {
            pin1 = pin2;
            ++i;
            continue;
        }

        if( !same_pin_net( pin1, pin2 ) )
        {
            msg.Printf( _( "Component %s %s pin %s : Different Nets" ),
                        component.m_Reference.GetData(),
                        component.m_Value.GetData(),
                        pin1->m_Number.GetData() );
            DisplayError( NULL, msg, 60 );
        }

        wxLogDebug( wxT( "Removing duplicate pin %s from component %s: %s" ),
                    pin1->m_Number.c_str(), component.m_Reference.c_str(),
                    component.m_Value.c_str() );
        pin1 = pin2;
        i = component.m_Pins.erase( i );
        delete pin2;
    }
}


int GenNetlistPcbnew( FILE* file, COMPONENT_LIST& list, bool isEESchemaNetlist,
                      bool rightJustify )
{
#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"
    char       Line[1024];
    int        netNumber = 1;

    DateAndTime( Line );

    if( isEESchemaNetlist )
        fprintf( file, "# %s created  %s\n(\n", NETLIST_HEAD_STRING, Line );
    else
        fprintf( file, "( { netlist created  %s }\n", Line );

    /***********************/
    /* Lecture de la liste */
    /***********************/

    BOOST_FOREACH( COMPONENT& component, list )
    {
        fprintf( file, " ( %s ", CONV_TO_UTF8( component.m_TimeStamp ) );

        if( !component.m_Module.IsEmpty() )
            fprintf( file, CONV_TO_UTF8( component.m_Module ) );

        else
            fprintf( file, "$noname$" );

        fprintf( file, " %s ", CONV_TO_UTF8( component.m_Reference ) );

        /* placement de la valeur */
        fprintf( file, "%s\n", CONV_TO_UTF8( component.m_Value ) );

        component.m_Pins.sort();
        RemoveDuplicatePins( component );

        /* Placement de la liste des pins */
        BOOST_FOREACH( PIN& pin, component.m_Pins )
        {
            if( pin.m_Net.Len() > MAX_LEN_NETNAME )
                ChangePinNet( list, pin.m_Net, &netNumber, rightJustify );

            if( !pin.m_Net.IsEmpty() )
                fprintf( file, "  ( %s %s )\n",
                         CONV_TO_UTF8( pin.m_Number ),
                         CONV_TO_UTF8( pin.m_Net ) );
            else
                fprintf( file, "  ( %s ? )\n", CONV_TO_UTF8( pin.m_Number ) );
        }

        fprintf( file, " )\n" );
    }

    fprintf( file, ")\n*\n" );

    if( isEESchemaNetlist )
        WriteFootprintFilterInfos( file, list );

    fclose( file );
    return 0;
}


/*
 * Write the allowed footprint list for each component
 */
void WriteFootprintFilterInfos( FILE* file, COMPONENT_LIST& list )
{
    bool       WriteHeader = FALSE;

    BOOST_FOREACH( COMPONENT& component, list )
    {
        unsigned int FilterCount;
        FilterCount = component.m_FootprintFilter.GetCount();
        if( FilterCount == 0 )
            continue;
        if( !WriteHeader )
        {
            fprintf( file, "{ Allowed footprints by component:\n" );
            WriteHeader = TRUE;
        }
        fprintf( file, "$component %s\n",
                 CONV_TO_UTF8( component.m_Reference ) );
        /* Write the footprint list */
        for( unsigned int jj = 0; jj < FilterCount; jj++ )
        {
            fprintf( file, " %s\n",
                     CONV_TO_UTF8( component.m_FootprintFilter[jj] ) );
        }

        fprintf( file, "$endlist\n" );
    }

    if( WriteHeader )
        fprintf( file, "$endfootprintlist\n}\n" );
}


/*
 * Change le NetName PinNet par un nom compose des 8 derniers codes de PinNet
 * suivi de _Xnnnnn ou nnnnn est un nom de 0 a 99999
 */
static void ChangePinNet( COMPONENT_LIST& list, wxString& PinNet,
                          int* netNumber,  bool rightJustify )
{
    wxASSERT( netNumber != NULL );

    wxString   OldName;
    wxString   NewName;

    OldName = PinNet;

    if( rightJustify )  /* On conserve les 8 dernieres lettres du nom */
    {
        NewName = OldName.Right( 8 );
        NewName << *netNumber;
    }
    else             /* On conserve les 8 premieres lettres du nom */
    {
        NewName = OldName.Left( 8 );
        NewName << *netNumber;
    }

    *netNumber = *netNumber + 1;

    BOOST_FOREACH( COMPONENT& component, list )
    {
        BOOST_FOREACH( PIN& pin, component.m_Pins )
        {
            if( pin.m_Net != OldName )
                continue;

            pin.m_Net = NewName;
        }
    }
}
