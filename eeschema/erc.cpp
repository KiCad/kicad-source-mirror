/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <class_drawpanel.h>
#include <kicad_string.h>
#include <schframe.h>

#include <netlist.h>
#include <class_netlist_object.h>
#include <lib_pin.h>
#include <erc.h>
#include <sch_marker.h>
#include <sch_component.h>
#include <sch_sheet.h>

#include <wx/ffile.h>


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
const wxString CommentERC_H[] =
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
    _( "No Connection......" )
};

// Messages for matrix columns
const wxString CommentERC_V[] =
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
    _( "No Connection" )
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
                        marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
                        marker->SetErrorLevel( ERR );
                        screen->Append( marker );
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

    marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
    marker->SetErrorLevel( WAR );
    screen = aNetItemRef->m_SheetPath.LastScreen();
    screen->Append( marker );

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
        cmp_ref = aNetItemRef->GetComponentParent()->GetRef( &aNetItemRef->m_SheetPath );

    if( aNetItemTst == NULL )
    {
        if( aMinConn == NOC )    /* Only 1 element in the net. */
        {
            msg.Printf( _( "Pin %s (%s) of component %s is unconnected." ),
                        GetChars( string_pinnum ),
                        GetChars( LIB_PIN::GetElectricalTypeName( ii ) ),
                        GetChars( cmp_ref ) );
            marker->SetData( ERCE_PIN_NOT_CONNECTED,
                             aNetItemRef->m_Start,
                             msg,
                             aNetItemRef->m_Start );
            return;
        }

        if( aMinConn == NOD )    /* Nothing driving the net. */
        {
            if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
                cmp_ref = aNetItemRef->GetComponentParent()->GetRef(
                    &aNetItemRef->m_SheetPath );

            msg.Printf( _( "Pin %s (%s) of component %s is not driven (Net %d)." ),
                        GetChars( string_pinnum ),
                        GetChars( LIB_PIN::GetElectricalTypeName( ii ) ),
                        GetChars( cmp_ref ),
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
            alt_cmp = aNetItemTst->GetComponentParent()->GetRef( &aNetItemTst->m_SheetPath );

        msg.Printf( _( "Pin %s (%s) of component %s is connected to " ),
                    GetChars( string_pinnum ),
                    GetChars( LIB_PIN::GetElectricalTypeName( ii ) ),
                    GetChars( cmp_ref ) );
        marker->SetData( errortype, aNetItemRef->m_Start, msg, aNetItemRef->m_Start );
        msg.Printf( _( "pin %s (%s) of component %s (net %d)." ),
                    GetChars( alt_string_pinnum ),
                    GetChars( LIB_PIN::GetElectricalTypeName( jj ) ),
                    GetChars( alt_cmp ),
                    aNetItemRef->GetNet() );
        marker->SetAuxiliaryData( msg, aNetItemTst->m_Start );
    }
}


void TestOthersItems( NETLIST_OBJECT_LIST* aList,
                      unsigned aNetItemRef, unsigned aNetStart,
                      int* aMinConnexion )
{
    unsigned netItemTst = aNetStart;
    int jj;
    int erc = OK;

    /* Analysis of the table of connections. */
    int ref_elect_type = aList->GetItem( aNetItemRef )->m_ElectricalType;
    int local_minconn = NOC;

    if( ref_elect_type == PIN_NC )
        local_minconn = NPI;

    /* Test pins connected to NetItemRef */
    for( ; ; netItemTst++ )
    {
        if( aNetItemRef == netItemTst )
            continue;

        // We examine only a given net. We stop the search if the net changes
        if( ( netItemTst >= aList->size() ) // End of list
            || ( aList->GetItemNet( aNetItemRef ) !=
                 aList->GetItemNet( netItemTst ) ) ) // End of net
        {
            /* End net code found: minimum connection test. */
            if( ( *aMinConnexion < NET_NC ) && ( local_minconn < NET_NC ) )
            {
                /* Not connected or not driven pin. */
                bool seterr = true;

                if( local_minconn == NOC &&
                    aList->GetItemType( aNetItemRef ) == NET_PIN )
                {
                    /* This pin is not connected: for multiple part per
                     * package, and duplicated pin,
                     * search for an other instance of this pin
                     * this will be flagged only if all instances of this pin
                     * are not connected
                     * TODO test also if instances connected are connected to
                     * the same net
                     */
                    for( unsigned duplicate = 0; duplicate < aList->size(); duplicate++ )
                    {
                        if( aList->GetItemType( duplicate ) != NET_PIN )
                            continue;

                        if( duplicate == aNetItemRef )
                            continue;

                        if( aList->GetItem( aNetItemRef )->m_PinNum !=
                            aList->GetItem( duplicate )->m_PinNum )
                            continue;

                        if( ( (SCH_COMPONENT*) aList->GetItem( aNetItemRef )->
                             m_Link )->GetRef( &aList->GetItem( aNetItemRef )-> m_SheetPath ) !=
                            ( (SCH_COMPONENT*) aList->GetItem( duplicate )->m_Link )
                           ->GetRef( &aList->GetItem( duplicate )->m_SheetPath ) )
                            continue;

                        // Same component and same pin. Do dot create error for this pin
                        // if the other pin is connected (i.e. if duplicate net has an other
                        // item)
                        if( (duplicate > 0)
                          && ( aList->GetItemNet( duplicate ) ==
                               aList->GetItemNet( duplicate - 1 ) ) )
                            seterr = false;

                        if( (duplicate < aList->size() - 1)
                          && ( aList->GetItemNet( duplicate ) ==
                               aList->GetItemNet( duplicate + 1 ) ) )
                            seterr = false;
                    }
                }

                if( seterr )
                    Diagnose( aList->GetItem( aNetItemRef ), NULL, local_minconn, WAR );

                *aMinConnexion = DRV;   // inhibiting other messages of this
                                       // type for the net.
            }
            return;
        }

        switch( aList->GetItemType( netItemTst ) )
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
            local_minconn = std::max( NET_NC, local_minconn );
            break;

        case NET_PIN:
            jj = aList->GetItem( netItemTst )->m_ElectricalType;
            local_minconn = std::max( MinimalReq[ref_elect_type][jj], local_minconn );

            if( netItemTst <= aNetItemRef )
                break;

            if( erc == OK )
            {
                erc = DiagErc[ref_elect_type][jj];

                if( erc != OK )
                {
                    if( aList->GetConnectionType( netItemTst ) == UNCONNECTED )
                    {
                        Diagnose( aList->GetItem( aNetItemRef ),
                                  aList->GetItem( netItemTst ),
                                  0, erc );
                        aList->SetConnectionType( netItemTst, NOCONNECT_SYMBOL_PRESENT );
                    }
                }
            }

            break;
        }
    }
}


int CountPinsInNet( NETLIST_OBJECT_LIST* aList, unsigned aNetStart )
{
    int count = 0;
    int curr_net = aList->GetItemNet( aNetStart );

    /* Test pins connected to NetItemRef */
    for( unsigned item = aNetStart; item < aList->size(); item++ )
    {
        // We examine only a given net. We stop the search if the net changes
        if( curr_net != aList->GetItemNet( item ) )   // End of net
            break;

        if( aList->GetItemType( item ) == NET_PIN )
            count++;
    }

    return count;
}

bool WriteDiagnosticERC( const wxString& aFullFileName )
{
    wxString    msg;

    wxFFile file( aFullFileName, wxT( "wt" ) );

    if( !file.IsOpened() )
        return false;

    msg = _( "ERC report" );
    msg << wxT(" (") << DateAndTime() << wxT( ", " )
        << _( "Encoding UTF8" ) << wxT( " )\n" );

    int err_count = 0;
    int warn_count = 0;
    int total_count = 0;
    SCH_SHEET_LIST sheetList;
    SCH_SHEET_PATH* sheet;

    for( sheet = sheetList.GetFirst(); sheet != NULL; sheet = sheetList.GetNext() )
    {
        msg << wxString::Format( _( "\n***** Sheet %s\n" ),
                                 GetChars( sheet->PathHumanReadable() ) );

        for( SCH_ITEM* item = sheet->LastDrawList(); item != NULL; item = item->Next() )
        {
            if( item->Type() != SCH_MARKER_T )
                continue;

            SCH_MARKER* marker = (SCH_MARKER*) item;

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            total_count++;

            if( marker->GetErrorLevel() == ERR )
                err_count++;

            if( marker->GetErrorLevel() == WAR )
                warn_count++;

            msg << marker->GetReporter().ShowReport();
        }
    }

    msg << wxString::Format( _( "\n ** ERC messages: %d  Errors %d  Warnings %d\n" ),
                             total_count, err_count, warn_count );

    // Currently: write report unsing UTF8 (as usual in Kicad).
    // TODO: see if we can use the current encoding page (mainly for Windows users),
    // Or other format (HTML?)
    file.Write( msg );

    // wxFFile dtor will close the file.

    return true;
}


void TestLabel( NETLIST_OBJECT_LIST* aList, unsigned aNetItemRef, unsigned aStartNet )
{
    unsigned netItemTst = aStartNet;
    int      erc = 1;

    // Review the list of labels connected to NetItemRef:
    for( ; ; netItemTst++ )
    {
        if( netItemTst == aNetItemRef )
            continue;

        /* Is always in the same net? */
        if( ( netItemTst == aList->size() )
          || ( aList->GetItemNet( aNetItemRef ) != aList->GetItemNet( netItemTst ) ) )
        {
            /* End Netcode found. */
            if( erc )
            {
                /* Glabel or SheetLabel orphaned. */
                Diagnose( aList->GetItem( aNetItemRef ), NULL, -1, WAR );
            }

            return;
        }

        if( aList->GetItem( aNetItemRef )->IsLabelConnected( aList->GetItem( netItemTst ) ) )
            erc = 0;

        //same thing, different order.
        if( aList->GetItem( netItemTst )->IsLabelConnected( aList->GetItem( aNetItemRef ) ) )
            erc = 0;
    }
}
