/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file erc.cpp
 * @brief Electrical Rules Check implementation.
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "netlist.h"
#include "lib_pin.h"
#include "protos.h"
#include "erc.h"
#include "sch_marker.h"
#include "sch_component.h"
#include "sch_sheet.h"


/* ERC tests :
 *  1 - conflicts between connected pins ( example: 2 connected outputs )
 *  2 - minimal connections requirements ( 1 input *must* be connected to an
 * output, or a passive pin )
 */


/*
 *  Electrical type of pins:
 *  PIN_INPUT = usual pin input: must be connected
 *  PIN_OUTPUT = usual output
 *  PIN_BIDI = input or output (like port for a microprocessor)
 *  PIN_TRISTATE = tris state bus pin
 *  PIN_PASSIVE = pin for passive components: must be connected, and can be
 * connected to any pin
 *  PIN_UNSPECIFIED = unknown electrical properties: creates always a warning
 * when connected
 *  PIN_POWER_IN = power input (GND, VCC for ICs). Must be connected to a power
 * output.
 *  PIN_POWER_OUT = output of a regulator: intended to be connected to power
 * input pins
 *  PIN_OPENCOLLECTOR = pin type open collector
 *  PIN_OPENEMITTER = pin type open emitter
 *  PIN_NC = not connected (must be left open)
 *
 *  Minimal requirements:
 *  All pins *must* be connected (except PIN_NC).
 *  When a pin is not connected in schematic, the user must place a "non
 * connected" symbol to this pin.
 *  This ensures a forgotten connection will be detected.
 */

/* Messages for conflicts :
 *  PIN_INPUT, PIN_OUTPUT, PIN_BIDI, PIN_TRISTATE, PIN_PASSIVE,
 *  PIN_UNSPECIFIED, PIN_POWER_IN, PIN_POWER_OUT, PIN_OPENCOLLECTOR,
 *  PIN_OPENEMITTER, PIN_NC
 *  These messages are used to show the ERC matrix in ERC dialog
 */

// Messages for matrix rows:
const wxChar* CommentERC_H[] =
{
    _( "Input Pin.........." ),
    _( "Output Pin........." ),
    _( "Bidirectional Pin.." ),
    _( "Tri-State Pin......" ),
    _( "Passive Pin........" ),
    _( "Unspecified Pin...." ),
    _( "Power Input Pin...." ),
    _( "Power Output Pin..." ),
    _( "Open Collector....." ),
    _( "Open Emitter......." ),
    _( "No Connection......" ),
    NULL
};

// Messages for matrix columns
const wxChar* CommentERC_V[] =
{
    _( "Input Pin" ),
    _( "Output Pin" ),
    _( "Bidirectional Pin" ),
    _( "Tri-State Pin" ),
    _( "Passive Pin" ),
    _( "Unspecified Pin" ),
    _( "Power Input Pin" ),
    _( "Power Output Pin" ),
    _( "Open Collector" ),
    _( "Open Emitter" ),
    _( "No Connection" ),
    NULL
};


/* Look up table which gives the diag for a pair of connected pins
 *  Can be modified by ERC options.
 *  at start up: must be loaded by DefaultDiagErc
 */
int  DiagErc[PIN_NMAX][PIN_NMAX];
bool DiagErcTableInit;       // go to true after DiagErc init

/**
 * Default Look up table which gives the ERC error level for a pair of connected pins
 * Same as DiagErc, but cannot be modified.
 *  Used to init or reset DiagErc
 *  note also, to avoid inconsistancy:
 *    DefaultDiagErc[i][j] = DefaultDiagErc[j][i]
 */
int DefaultDiagErc[PIN_NMAX][PIN_NMAX] =
{
/*         I,   O,    Bi,   3S,   Pas,  UnS,  PwrI, PwrO, OC,   OE,   NC */
/* I */  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/* O */  { OK,  ERR,  OK,   WAR,  OK,   WAR,  OK,   ERR,  ERR,  ERR,  ERR },
/* Bi*/  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   WAR,  OK,   WAR,  ERR },
/* 3S*/  { OK,  WAR,  OK,   OK,   OK,   WAR,  WAR,  ERR,  WAR,  WAR,  ERR },
/*Pas*/  { OK,  OK,   OK,   OK,   OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/*UnS */ { WAR, WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  WAR,  ERR },
/*PwrI*/ { OK,  OK,   OK,   WAR,  OK,   WAR,  OK,   OK,   OK,   OK,   ERR },
/*PwrO*/ { OK,  ERR,  WAR,  ERR,  OK,   WAR,  OK,   ERR,  ERR,  ERR,  ERR },
/* OC */ { OK,  ERR,  OK,   WAR,  OK,   WAR,  OK,   ERR,  OK,   OK,   ERR },
/* OE */ { OK,  ERR,  WAR,  WAR,  OK,   WAR,  OK,   ERR,  OK,   OK,   ERR },
/* NC */ { ERR, ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR,  ERR }
};


/**
 * Look up table which gives the minimal drive for a pair of connected pins on
 * a net.
 * <p>
 * The initial state of a net is NOC (Net with No Connection).  It can be updated to
 * NPI (Pin Isolated), NET_NC (Net with a no connect symbol), NOD (Not Driven) or DRV
 * (DRIven).  It can be updated to NET_NC with no error only if there is only one pin
 * in net.  Nets are OK when their final state is NET_NC or DRV.   Nets with the state
 * NOD have no valid source signal.
 */
static int MinimalReq[PIN_NMAX][PIN_NMAX] =
{
/*         In   Out, Bi,  3S,  Pas, UnS, PwrI,PwrO,OC,  OE,  NC */
/* In*/  { NOD, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*Out*/  { DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NPI },
/* Bi*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/* 3S*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*Pas*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*UnS*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/*PwrI*/ { NOD, DRV, NOD, NOD, NOD, NOD, NOD, DRV, NOD, NOD, NPI },
/*PwrO*/ { DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, DRV, NPI },
/* OC*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/* OE*/  { DRV, DRV, DRV, DRV, DRV, DRV, NOD, DRV, DRV, DRV, NPI },
/* NC*/  { NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI, NPI }
};


int TestDuplicateSheetNames( bool aCreateMarker )
{
    SCH_SCREEN* screen;
    SCH_ITEM*   item;
    SCH_ITEM*   test_item;
    int         err_count = 0;
    SCH_SCREENS screenList;      // Created the list of screen

    for( screen = screenList.GetFirst(); screen != NULL; screen = screenList.GetNext() )
    {
        for( item = screen->GetDrawItems(); item != NULL; item = item->Next() )
        {
            // search for a sheet;
            if( item->Type() != SCH_SHEET_T )
                continue;

            for(  test_item = item->Next(); test_item != NULL; test_item = test_item->Next() )
            {
                if( test_item->Type() != SCH_SHEET_T )
                    continue;

                // We have found a second sheet: compare names
                if( ( (SCH_SHEET*) item )->GetName().CmpNoCase(
                        ( ( SCH_SHEET* ) test_item )->GetName() ) == 0 )
                {
                    if( aCreateMarker )
                    {
                        /* Create a new marker type ERC error*/
                        SCH_MARKER* marker = new SCH_MARKER();
                        marker->SetTimeStamp( GetNewTimeStamp() );
                        marker->SetData( ERCE_DUPLICATE_SHEET_NAME,
                                         ( (SCH_SHEET*) test_item )->GetPosition(),
                                         _( "Duplicate sheet name" ),
                                         ( (SCH_SHEET*) test_item )->GetPosition() );
                        marker->SetMarkerType( MARK_ERC );
                        marker->SetErrorLevel( ERR );
                        marker->SetNext( screen->GetDrawItems() );
                        screen->SetDrawItems( marker );
                    }

                    err_count++;
                }
            }
        }
    }

    return err_count;
}


void Diagnose( NETLIST_OBJECT* aNetItemRef, NETLIST_OBJECT* aNetItemTst,
               int aMinConn, int aDiag )
{
    SCH_MARKER* marker = NULL;
    SCH_SCREEN* screen;
    int         ii, jj;

    if( aDiag == OK )
        return;

    /* Create new marker for ERC error. */
    marker = new SCH_MARKER();
    marker->SetTimeStamp( GetNewTimeStamp() );

    marker->SetMarkerType( MARK_ERC );
    marker->SetErrorLevel( WAR );
    screen = aNetItemRef->m_SheetList.LastScreen();
    marker->SetNext( screen->GetDrawItems() );
    screen->SetDrawItems( marker );

    wxString msg;

    if( aMinConn < 0 )
    {
        if( (aNetItemRef->m_Type == NET_HIERLABEL)
            || (aNetItemRef->m_Type == NET_HIERBUSLABELMEMBER) )
        {
            msg.Printf( _( "Hierarchical label %s is not connected to a sheet label." ),
                        GetChars( aNetItemRef->m_Label ) );
        }
        else
        {
            msg.Printf( _( "Sheet label %s is not connected to a hierarchical label." ),
                        GetChars( aNetItemRef->m_Label ) );
        }


        marker->SetData( ERCE_HIERACHICAL_LABEL,
                         aNetItemRef->m_Start,
                         msg,
                         aNetItemRef->m_Start );
        return;
    }

    ii = aNetItemRef->m_ElectricalType;

    wxString string_pinnum, cmp_ref;
    char     ascii_buf[5];
    ascii_buf[4] = 0;
    memcpy( ascii_buf, &aNetItemRef->m_PinNum, 4 );
    string_pinnum = FROM_UTF8( ascii_buf );
    cmp_ref = wxT( "?" );

    if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
        cmp_ref = ( (SCH_COMPONENT*) aNetItemRef->m_Link )->GetRef( &aNetItemRef->m_SheetList );

    if( aNetItemTst == NULL )
    {
        if( aMinConn == NOC )    /* Only 1 element in the net. */
        {
            msg.Printf( _( "Pin %s (%s) of component %s is unconnected." ),
                        GetChars( string_pinnum ), MsgPinElectricType[ii], GetChars( cmp_ref ) );
            marker->SetData( ERCE_PIN_NOT_CONNECTED,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }

        if( aMinConn == NOD )    /* Nothing driving the net. */
        {
            if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
                cmp_ref = ( (SCH_COMPONENT*) aNetItemRef->m_Link )->GetRef(
                    &aNetItemRef->m_SheetList );

            msg.Printf( _( "Pin %s (%s) of component %s is not driven (Net %d)." ),
                        GetChars( string_pinnum ), MsgPinElectricType[ii], GetChars( cmp_ref ),
                        aNetItemRef->GetNet() );
            marker->SetData( ERCE_PIN_NOT_DRIVEN,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }

        if( aDiag == UNC )
        {
            msg.Printf( _( "More than 1 pin connected to an UnConnect symbol." ) );
            marker->SetData( ERCE_NOCONNECT_CONNECTED,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }
    }

    if( aNetItemTst )         /* Error between 2 pins */
    {
        jj = aNetItemTst->m_ElectricalType;
        int errortype = ERCE_PIN_TO_PIN_WARNING;

        if( aDiag == ERR )
        {
            marker->SetErrorLevel( ERR );
            errortype = ERCE_PIN_TO_PIN_ERROR;
        }

        wxString alt_string_pinnum, alt_cmp;
        memcpy( ascii_buf, &aNetItemTst->m_PinNum, 4 );
        alt_string_pinnum = FROM_UTF8( ascii_buf );
        alt_cmp = wxT( "?" );

        if( aNetItemTst->m_Type == NET_PIN && aNetItemTst->m_Link )
            alt_cmp = ( (SCH_COMPONENT*) aNetItemTst->m_Link )->GetRef( &aNetItemTst->m_SheetList );

        msg.Printf( _( "Pin %s (%s) of component %s is connected to " ),
                    GetChars( string_pinnum ), MsgPinElectricType[ii], GetChars( cmp_ref ) );
        marker->SetData( errortype, aNetItemRef->m_Start, msg, aNetItemRef->m_Start );
        msg.Printf( _( "pin %s (%s) of component %s (net %d)." ),
                    GetChars( alt_string_pinnum ), MsgPinElectricType[jj], GetChars( alt_cmp ),
                    aNetItemRef->GetNet() );
        marker->SetAuxiliaryData( msg, aNetItemTst->m_Start );
    }
}


void TestOthersItems( unsigned NetItemRef, unsigned netstart,
                      int* NetNbItems, int* MinConnexion )
{
    unsigned NetItemTst;

    int      ref_elect_type, jj, erc = OK, local_minconn;

    /* Analysis of the table of connections. */
    ref_elect_type = g_NetObjectslist[NetItemRef]->m_ElectricalType;

    NetItemTst    = netstart;
    local_minconn = NOC;

    if( ref_elect_type == PIN_NC )
        local_minconn = NPI;

    /* Test pins connected to NetItemRef */
    for( ; ; NetItemTst++ )
    {
        if( NetItemRef == NetItemTst )
            continue;

        // We examine only a given net. We stop the search if the net changes
        if( ( NetItemTst >= g_NetObjectslist.size() ) // End of list
            || ( g_NetObjectslist[NetItemRef]->GetNet() !=
                 g_NetObjectslist[NetItemTst]->GetNet() ) ) // End of net
        {
            /* End net code found: minimum connection test. */
            if( (*MinConnexion < NET_NC ) && (local_minconn < NET_NC ) )
            {
                /* Not connected or not driven pin. */
                bool seterr = true;

                if( local_minconn == NOC && g_NetObjectslist[NetItemRef]->m_Type == NET_PIN )
                {
                    /* This pin is not connected: for multiple part per
                     * package, and duplicated pin,
                     * search for an other instance of this pin
                     * this will be flagged only if all instances of this pin
                     * are not connected
                     * TODO test also if instances connected are connected to
                     * the same net
                     */
                    for( unsigned duplicate = 0; duplicate < g_NetObjectslist.size(); duplicate++ )
                    {
                        if( g_NetObjectslist[duplicate]->m_Type != NET_PIN )
                            continue;

                        if( duplicate == NetItemRef )
                            continue;

                        if( g_NetObjectslist[NetItemRef]->m_PinNum !=
                            g_NetObjectslist[duplicate]->m_PinNum )
                            continue;

                        if( ( (SCH_COMPONENT*) g_NetObjectslist[NetItemRef]->
                             m_Link )->GetRef( &g_NetObjectslist[NetItemRef]-> m_SheetList ) !=
                            ( (SCH_COMPONENT*) g_NetObjectslist[duplicate]->m_Link )
                           ->GetRef( &g_NetObjectslist[duplicate]->m_SheetList ) )
                            continue;

                        // Same component and same pin. Do dot create error for this pin
                        // if the other pin is connected (i.e. if duplicate net has an other
                        // item)
                        if( (duplicate > 0)
                          && ( g_NetObjectslist[duplicate]->GetNet() ==
                               g_NetObjectslist[duplicate - 1]->GetNet() ) )
                            seterr = false;

                        if( (duplicate < g_NetObjectslist.size() - 1)
                          && ( g_NetObjectslist[duplicate]->GetNet() ==
                               g_NetObjectslist[duplicate + 1]->GetNet() ) )
                            seterr = false;
                    }
                }

                if( seterr )
                    Diagnose( g_NetObjectslist[NetItemRef], NULL, local_minconn, WAR );

                *MinConnexion = DRV;   // inhibiting other messages of this
                                       // type for the net.
            }
            return;
        }

        switch( g_NetObjectslist[NetItemTst]->m_Type )
        {
        case NET_ITEM_UNSPECIFIED:
        case NET_SEGMENT:
        case NET_BUS:
        case NET_JUNCTION:
        case NET_LABEL:
        case NET_HIERLABEL:
        case NET_BUSLABELMEMBER:
        case NET_HIERBUSLABELMEMBER:
        case NET_SHEETBUSLABELMEMBER:
        case NET_SHEETLABEL:
        case NET_GLOBLABEL:
        case NET_GLOBBUSLABELMEMBER:
        case NET_PINLABEL:
            break;

        case NET_NOCONNECT:
            local_minconn = MAX( NET_NC, local_minconn );
            break;

        case NET_PIN:
            jj = g_NetObjectslist[NetItemTst]->m_ElectricalType;
            local_minconn = MAX( MinimalReq[ref_elect_type][jj], local_minconn );

            if( NetItemTst <= NetItemRef )
                break;

            *NetNbItems += 1;

            if( erc == OK )
            {
                erc = DiagErc[ref_elect_type][jj];

                if( erc != OK )
                {
                    if( g_NetObjectslist[NetItemTst]->m_FlagOfConnection == 0 )
                    {
                        Diagnose( g_NetObjectslist[NetItemRef],
                                  g_NetObjectslist[NetItemTst],
                                  0,
                                  erc );
                        g_NetObjectslist[NetItemTst]->m_FlagOfConnection =
                            NOCONNECT_SYMBOL_PRESENT;
                    }
                }
            }

            break;
        }
    }
}


bool WriteDiagnosticERC( const wxString& aFullFileName )
{
    SCH_ITEM*       item;
    SCH_MARKER*     marker;
    static FILE*    file;
    SCH_SHEET_PATH* sheet;
    wxString        msg;
    int             count = 0;

    if( ( file = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
        return FALSE;

    msg = _( "ERC report" );

    fprintf( file, "%s (%s)\n", TO_UTF8( msg ), TO_UTF8( DateAndTime() ) );

    SCH_SHEET_LIST sheetList;

    for( sheet = sheetList.GetFirst(); sheet != NULL; sheet = sheetList.GetNext() )
    {
        msg.Printf( _( "\n***** Sheet %s\n" ), GetChars( sheet->PathHumanReadable() ) );

        fprintf( file, "%s", TO_UTF8( msg ) );

        for( item = sheet->LastDrawList(); item != NULL; item = item->Next() )
        {
            if( item->Type() != SCH_MARKER_T )
                continue;

            marker = (SCH_MARKER*) item;

            if( marker->GetMarkerType() != MARK_ERC )
                continue;

            if( marker->GetMarkerType() == ERR )
                count++;

            msg = marker->GetReporter().ShowReport();
            fprintf( file, "%s", TO_UTF8( msg ) );
        }
    }

    msg.Printf( _( "\n >> Errors ERC: %d\n" ), count );
    fprintf( file, "%s", TO_UTF8( msg ) );
    fclose( file );

    return true;
}


void TestLabel( unsigned NetItemRef, unsigned StartNet )
{
    unsigned NetItemTst;
    int      erc = 1;

    NetItemTst = StartNet;

    /* Review the list of labels connected to NetItemRef. */
    for( ; ; NetItemTst++ )
    {
        if( NetItemTst == NetItemRef )
            continue;

        /* Is always in the same net? */
        if( ( NetItemTst == g_NetObjectslist.size() )
          || ( g_NetObjectslist[NetItemRef]->GetNet() != g_NetObjectslist[NetItemTst]->GetNet() ) )
        {
            /* End Netcode found. */
            if( erc )
            {
                /* Glabel or SheetLabel orphaned. */
                Diagnose( g_NetObjectslist[NetItemRef], NULL, -1, WAR );
            }

            return;
        }

        if( g_NetObjectslist[NetItemRef]->IsLabelConnected( g_NetObjectslist[NetItemTst] ) )
            erc = 0;

        //same thing, different order.
        if( g_NetObjectslist[NetItemTst]->IsLabelConnected( g_NetObjectslist[NetItemRef] ) )
            erc = 0;
    }
}
