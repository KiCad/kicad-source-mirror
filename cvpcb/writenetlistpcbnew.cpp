/***************************/
/* writenetlistpcbnew.cpp  */
/***************************/


#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "macros.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "cvpcb_mainframe.h"

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
                        GetChars( component.m_Reference ),
                        GetChars( component.m_Value ),
                        pin1->m_Number.GetData() );
            DisplayError( NULL, msg, 60 );
        }
        pin1 = pin2;
        i = component.m_Pins.erase( i );
        delete pin2;
    }
}


/**
 * Create KiCad net list file.
 *
 * @todo: None of the printf() call return values are checked for failure,
 *        a value less than zero.  Check all printf() return values and
 *        return a true(pass) or false(fail) to the caller.
 */
int CVPCB_MAINFRAME::GenNetlistPcbnew( FILE* file,bool isEESchemaNetlist )
{
#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"

    if( isEESchemaNetlist )
        fprintf( file, "# %s created  %s\n(\n", NETLIST_HEAD_STRING, TO_UTF8( DateAndTime() ) );
    else
        fprintf( file, "( { netlist created  %s }\n", TO_UTF8( DateAndTime() ) );


    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        fprintf( file, " ( %s ", TO_UTF8( component.m_TimeStamp ) );

        if( !component.m_Module.IsEmpty() )
            fprintf( file, "%s", TO_UTF8( component.m_Module ) );
        else
            fprintf( file, "$noname$" );

        fprintf( file, " %s ", TO_UTF8( component.m_Reference ) );

        fprintf( file, "%s\n", TO_UTF8( component.m_Value ) );

        component.m_Pins.sort();
        RemoveDuplicatePins( component );

        BOOST_FOREACH( PIN& pin, component.m_Pins )
        {
            if( !pin.m_Net.IsEmpty() )
                fprintf( file, "  ( %s %s )\n", TO_UTF8( pin.m_Number ), TO_UTF8( pin.m_Net ) );
            else
                fprintf( file, "  ( %s ? )\n", TO_UTF8( pin.m_Number ) );
        }

        fprintf( file, " )\n" );
    }

    fprintf( file, ")\n*\n" );

    if( isEESchemaNetlist )
        WriteFootprintFilterInfos( file, m_components );

    fclose( file );
    return 0;
}


/*
 * Write the allowed footprint list for each component
 */
void WriteFootprintFilterInfos( FILE* file, COMPONENT_LIST& list )
{
    bool       WriteHeader = false;

    BOOST_FOREACH( COMPONENT& component, list )
    {
        unsigned int FilterCount;
        FilterCount = component.m_FootprintFilter.GetCount();
        if( FilterCount == 0 )
            continue;
        if( !WriteHeader )
        {
            fprintf( file, "{ Allowed footprints by component:\n" );
            WriteHeader = true;
        }
        fprintf( file, "$component %s\n",
                 TO_UTF8( component.m_Reference ) );
        /* Write the footprint list */
        for( unsigned int jj = 0; jj < FilterCount; jj++ )
        {
            fprintf( file, " %s\n",
                     TO_UTF8( component.m_FootprintFilter[jj] ) );
        }

        fprintf( file, "$endlist\n" );
    }

    if( WriteHeader )
        fprintf( file, "$endfootprintlist\n}\n" );
}

