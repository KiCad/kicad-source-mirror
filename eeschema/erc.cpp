/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_draw_panel.h>
#include <kicad_string.h>
#include <sch_edit_frame.h>
#include <netlist_object.h>
#include <lib_pin.h>
#include <erc.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_reference_list.h>
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
 *  Can be modified in dialog ERC
 */
int  DiagErc[PINTYPE_COUNT][PINTYPE_COUNT];

/**
 * Default Look up table which gives the ERC error level for a pair of connected pins
 * Same as DiagErc, but cannot be modified.
 *  Used to init or reset DiagErc
 *  note also, to avoid inconsistancy:
 *    DefaultDiagErc[i][j] = DefaultDiagErc[j][i]
 */
int DefaultDiagErc[PINTYPE_COUNT][PINTYPE_COUNT] =
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
static int MinimalReq[PINTYPE_COUNT][PINTYPE_COUNT] =
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
                // we are using case insensitive comparison to avoid mistakes between
                // similar names like Mysheet and mysheet
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
                        marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_ERROR );
                        screen->Append( marker );
                    }

                    err_count++;
                }
            }
        }
    }

    return err_count;
}


int TestConflictingBusAliases( bool aCreateMarker )
{
    wxString    msg;
    wxPoint     dummyPos( 0, 0 );
    int         err_count = 0;
    SCH_SCREENS screens;
    std::vector< std::shared_ptr<BUS_ALIAS> > aliases;

    for( auto screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        std::unordered_set< std::shared_ptr<BUS_ALIAS> > screen_aliases = screen->GetBusAliases();

        for( const std::shared_ptr<BUS_ALIAS>& alias : screen_aliases )
        {
            for( const std::shared_ptr<BUS_ALIAS>& test : aliases )
            {
                if( alias->GetName() == test->GetName() && alias->Members() != test->Members() )
                {
                    if( aCreateMarker )
                    {
                        msg = wxString::Format( _( "Bus alias %s has conflicting definitions on"
                                                   " multiple sheets: %s and %s" ),
                                                alias->GetName(),
                                                alias->GetParent()->GetFileName(),
                                                test->GetParent()->GetFileName() );

                        SCH_MARKER* marker = new SCH_MARKER();
                        marker->SetData( ERCE_BUS_ALIAS_CONFLICT, dummyPos, msg, dummyPos );
                        marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
                        marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_ERROR );

                        test->GetParent()->Append( marker );
                    }

                    ++err_count;
                }
            }
        }

        aliases.insert( aliases.end(), screen_aliases.begin(), screen_aliases.end() );
    }

    return err_count;
}


int TestMultiunitFootprints( SCH_SHEET_LIST& aSheetList )
{
    int errors = 0;
    std::map<wxString, LIB_ID> footprints;
    SCH_MULTI_UNIT_REFERENCE_MAP refMap;
    aSheetList.GetMultiUnitComponents( refMap, true );

    for( auto& component : refMap )
    {
        auto& refList = component.second;

        if( refList.GetCount() == 0 )
        {
            wxFAIL;   // it should not happen
            continue;
        }

        // Reference footprint
        wxString fp;
        wxString unitName;

        for( unsigned i = 0; i < component.second.GetCount(); ++i )
        {
            SCH_COMPONENT* cmp = refList.GetItem( i ).GetComp();
            SCH_SHEET_PATH sheetPath = refList.GetItem( i ).GetSheetPath();
            fp = cmp->GetField( FOOTPRINT )->GetText();

            if( !fp.IsEmpty() )
            {
                unitName = cmp->GetRef( &sheetPath )
                    + LIB_PART::SubReference( cmp->GetUnit(), false );
                break;
            }
        }

        for( unsigned i = 0; i < component.second.GetCount(); ++i )
        {
            SCH_REFERENCE& ref = refList.GetItem( i );
            SCH_COMPONENT* unit = ref.GetComp();
            SCH_SHEET_PATH sheetPath = refList.GetItem( i ).GetSheetPath();
            const wxString curFp = unit->GetField( FOOTPRINT )->GetText();

            if( !curFp.IsEmpty() && fp != curFp )
            {
                wxString curUnitName = unit->GetRef( &sheetPath )
                    + LIB_PART::SubReference( unit->GetUnit(), false );
                wxString msg = wxString::Format( _( "Unit %s has '%s' assigned, "
                                                    "whereas unit %s has '%s' assigned" ),
                                                 unitName,
                                                 fp,
                                                 curUnitName,
                                                 curFp );
                wxPoint pos = unit->GetPosition();

                SCH_MARKER* marker = new SCH_MARKER();
                marker->SetTimeStamp( GetNewTimeStamp() );
                marker->SetData( ERCE_DIFFERENT_UNIT_FP, pos, msg, pos );
                marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
                marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
                ref.GetSheetPath().LastScreen()->Append( marker );

                ++errors;
            }
        }
    }

    return errors;
}


void Diagnose( NETLIST_OBJECT* aNetItemRef, NETLIST_OBJECT* aNetItemTst, int aMinConn, int aDiag )
{
    SCH_MARKER*     marker = NULL;
    SCH_SCREEN*     screen;
    ELECTRICAL_PINTYPE ii, jj;

    if( aDiag == OK || aMinConn < 1 )
        return;

    /* Create new marker for ERC error. */
    marker = new SCH_MARKER();
    marker->SetTimeStamp( GetNewTimeStamp() );

    marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
    marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
    screen = aNetItemRef->m_SheetPath.LastScreen();
    screen->Append( marker );

    wxString msg;

    ii = aNetItemRef->m_ElectricalPinType;

    wxString cmp_ref( "?" );

    if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
        cmp_ref = aNetItemRef->GetComponentParent()->GetRef( &aNetItemRef->m_SheetPath );

    if( aNetItemTst == NULL )
    {
        if( aMinConn == NOD )    /* Nothing driving the net. */
        {
            if( aNetItemRef->m_Type == NET_PIN && aNetItemRef->m_Link )
                cmp_ref = aNetItemRef->GetComponentParent()->GetRef(
                    &aNetItemRef->m_SheetPath );

            msg.Printf( _( "Pin %s (%s) of component %s is not driven (Net %d)." ),
                        aNetItemRef->m_PinNum,
                        GetChars( GetText( ii ) ),
                        GetChars( cmp_ref ),
                        aNetItemRef->GetNet() );
            marker->SetData( ERCE_PIN_NOT_DRIVEN, aNetItemRef->m_Start, msg, aNetItemRef->m_Start );
            return;
        }
    }

    if( aNetItemTst )         /* Error between 2 pins */
    {
        jj = aNetItemTst->m_ElectricalPinType;
        int errortype = ERCE_PIN_TO_PIN_WARNING;

        if( aDiag == ERR )
        {
            marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_ERROR );
            errortype = ERCE_PIN_TO_PIN_ERROR;
        }

        wxString alt_cmp( "?" );

        if( aNetItemTst->m_Type == NET_PIN && aNetItemTst->m_Link )
            alt_cmp = aNetItemTst->GetComponentParent()->GetRef( &aNetItemTst->m_SheetPath );

        msg.Printf( _( "Pin %s (%s) of component %s is connected to " ),
                    aNetItemRef->m_PinNum,
                    GetChars( GetText( ii ) ),
                    GetChars( cmp_ref ) );
        marker->SetData( errortype, aNetItemRef->m_Start, msg, aNetItemRef->m_Start );
        msg.Printf( _( "pin %s (%s) of component %s (net %d)." ),
                    aNetItemTst->m_PinNum,
                    GetChars( GetText( jj ) ),
                    GetChars( alt_cmp ),
                    aNetItemRef->GetNet() );
        marker->SetAuxiliaryData( msg, aNetItemTst->m_Start );
    }
}


void TestOthersItems( NETLIST_OBJECT_LIST* aList, unsigned aNetItemRef, unsigned aNetStart,
                      int* aMinConnexion )
{
    unsigned netItemTst = aNetStart;
    ELECTRICAL_PINTYPE jj;
    int erc = OK;

    /* Analysis of the table of connections. */
    ELECTRICAL_PINTYPE ref_elect_type = aList->GetItem( aNetItemRef )->m_ElectricalPinType;
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
                     * search for another instance of this pin
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
                        // if the other pin is connected (i.e. if duplicate net has another
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
            jj = aList->GetItem( netItemTst )->m_ElectricalPinType;
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

int NETLIST_OBJECT_LIST::CountPinsInNet( unsigned aNetStart )
{
    int count = 0;
    int curr_net = GetItemNet( aNetStart );

    /* Test pins connected to NetItemRef */
    for( unsigned item = aNetStart; item < size(); item++ )
    {
        // We examine only a given net. We stop the search if the net changes
        if( curr_net != GetItemNet( item ) )   // End of net
            break;

        if( GetItemType( item ) == NET_PIN )
            count++;
    }

    return count;
}

bool WriteDiagnosticERC( EDA_UNITS_T aUnits, const wxString& aFullFileName )
{
    wxFFile file( aFullFileName, wxT( "wt" ) );

    if( !file.IsOpened() )
        return false;

    wxString msg = wxString::Format( _( "ERC report (%s, Encoding UTF8)\n" ), DateAndTime() );
    int err_count = 0;
    int warn_count = 0;
    int total_count = 0;
    SCH_SHEET_LIST sheetList( g_RootSheet );

    for( unsigned i = 0;  i < sheetList.size(); i++ )
    {
        msg << wxString::Format( _( "\n***** Sheet %s\n" ),
                                 GetChars( sheetList[i].PathHumanReadable() ) );

        for( SCH_ITEM* item = sheetList[i].LastDrawList(); item != NULL; item = item->Next() )
        {
            if( item->Type() != SCH_MARKER_T )
                continue;

            SCH_MARKER* marker = (SCH_MARKER*) item;

            if( marker->GetMarkerType() != MARKER_BASE::MARKER_ERC )
                continue;

            total_count++;

            if( marker->GetErrorLevel() == MARKER_BASE::MARKER_SEVERITY_ERROR )
                err_count++;

            if( marker->GetErrorLevel() == MARKER_BASE::MARKER_SEVERITY_WARNING )
                warn_count++;

            msg << marker->GetReporter().ShowReport( aUnits );
        }
    }

    msg << wxString::Format( _( "\n ** ERC messages: %d  Errors %d  Warnings %d\n" ),
                             total_count, err_count, warn_count );

    // Currently: write report using UTF8 (as usual in Kicad).
    // TODO: see if we can use the current encoding page (mainly for Windows users),
    // Or other format (HTML?)
    file.Write( msg );

    // wxFFile dtor will close the file.

    return true;
}


void NETLIST_OBJECT_LIST::TestforNonOrphanLabel( unsigned aNetItemRef, unsigned aStartNet )
{
    unsigned netItemTst = aStartNet;
    int      erc = 1;

    // Review the list of labels connected to NetItemRef:
    for( ; ; netItemTst++ )
    {
        if( netItemTst == aNetItemRef )
            continue;

        /* Is always in the same net? */
        if( ( netItemTst == size() )
          || ( GetItemNet( aNetItemRef ) != GetItemNet( netItemTst ) ) )
        {
            /* End Netcode found. */
            if( erc )
            {
                /* Glabel or SheetLabel orphaned. */
                Diagnose( GetItem( aNetItemRef ), NULL, -1, WAR );
            }

            return;
        }

        if( GetItem( aNetItemRef )->IsLabelConnected( GetItem( netItemTst ) ) )
            erc = 0;

        //same thing, different order.
        if( GetItem( netItemTst )->IsLabelConnected( GetItem( aNetItemRef ) ) )
            erc = 0;
    }
}


// this code try to detect similar labels, i.e. labels which are identical
// when they are compared using case insensitive coparisons.


// A helper struct to compare NETLIST_OBJECT items by sheetpath and label texts
// for a std::set<NETLIST_OBJECT*> container
// the full text is "sheetpath+label" for local labels and "label" for global labels
struct compare_labels
{
    bool operator() ( const NETLIST_OBJECT* lab1, const NETLIST_OBJECT* lab2 )
    {
        wxString str1 = lab1->m_SheetPath.Path() + lab1->m_Label;
        wxString str2 = lab2->m_SheetPath.Path() + lab2->m_Label;

        return str1.Cmp( str2 ) < 0;
    }
};

struct compare_label_names
{
    bool operator() ( const NETLIST_OBJECT* lab1, const NETLIST_OBJECT* lab2 )
    {
        return lab1->m_Label.Cmp( lab2->m_Label ) < 0;
    }
};

struct compare_paths
{
    bool operator() ( const NETLIST_OBJECT* lab1, const NETLIST_OBJECT* lab2 )
    {
        return lab1->m_SheetPath.Path().Cmp( lab2->m_SheetPath.Path() ) < 0;
    }
};

// Helper functions to build the warning messages about Similar Labels:
static int countIndenticalLabels( std::vector<NETLIST_OBJECT*>& aList, NETLIST_OBJECT* aRef );
static void SimilarLabelsDiagnose( NETLIST_OBJECT* aItemA, NETLIST_OBJECT* aItemB );


void NETLIST_OBJECT_LIST::TestforSimilarLabels()
{
    // Similar labels which are different when using case sensitive comparisons
    // but are equal when using case insensitive comparisons

    // list of all labels (used the better item to build diag messages)
    std::vector<NETLIST_OBJECT*> fullLabelList;
    // list of all labels , each label appears only once (used to to detect similar labels)
    std::set<NETLIST_OBJECT*, compare_labels> uniqueLabelList;
    wxString msg;

    // Build a list of differents labels. If inside a given sheet there are
    // more than one given label, only one label is stored.
    // not also the sheet labels are not taken in account for 2 reasons:
    //  * they are in the root sheet but they are seen only from the child sheet
    //  * any mismatch between child sheet hierarchical labels and the sheet label
    //    already detected by ERC
    for( unsigned netItem = 0; netItem < size(); ++netItem )
    {
        switch( GetItemType( netItem ) )
        {
        case NET_LABEL:
        case NET_BUSLABELMEMBER:
        case NET_PINLABEL:
        case NET_GLOBBUSLABELMEMBER:
        case NET_HIERLABEL:
        case NET_HIERBUSLABELMEMBER:
        case NET_GLOBLABEL:
            // add this label in lists
            uniqueLabelList.insert( GetItem( netItem ) );
            fullLabelList.push_back( GetItem( netItem ) );
            break;

        case NET_SHEETLABEL:
        case NET_SHEETBUSLABELMEMBER:
        default:
            break;
        }
    }

    // build global labels and compare
    std::set<NETLIST_OBJECT*, compare_label_names> loc_labelList;

    for( auto it = uniqueLabelList.begin(); it != uniqueLabelList.end(); ++it )
    {
        if( (*it)->IsLabelGlobal() )
            loc_labelList.insert( *it );
    }

    // compare global labels (same label names appears only once in list)
    for( auto it = loc_labelList.begin(); it != loc_labelList.end(); ++it )
    {
        auto it_aux = it;

        for( ++it_aux; it_aux != loc_labelList.end(); ++it_aux )
        {
            if( (*it)->m_Label.CmpNoCase( (*it_aux)->m_Label ) == 0 )
            {
                // Create new marker for ERC.
                int cntA = countIndenticalLabels( fullLabelList, *it );
                int cntB = countIndenticalLabels( fullLabelList, *it_aux );

                if( cntA <= cntB )
                    SimilarLabelsDiagnose( (*it), (*it_aux) );
                else
                    SimilarLabelsDiagnose( (*it_aux), (*it) );
            }
        }
    }

    // Build paths list
    std::set<NETLIST_OBJECT*, compare_paths> pathsList;

    for( auto it = uniqueLabelList.begin(); it != uniqueLabelList.end(); ++it )
        pathsList.insert( *it );

    // Examine each label inside a sheet path:
    for( auto it = pathsList.begin(); it != pathsList.end(); ++it )
    {
        loc_labelList.clear();

        auto it_uniq = uniqueLabelList.begin();

        for( ; it_uniq != uniqueLabelList.end(); ++it_uniq )
        {
            if( ( *it )->m_SheetPath.Path() == ( *it_uniq )->m_SheetPath.Path() )
                loc_labelList.insert( *it_uniq );
        }

        // at this point, loc_labelList contains labels of the current sheet path.
        // Detect similar labels (same label names appears only once in list)

        for( auto ref_it = loc_labelList.begin(); ref_it != loc_labelList.end(); ++ref_it )
        {
            NETLIST_OBJECT* ref_item = *ref_it;
            auto it_aux = ref_it;

            for( ++it_aux; it_aux != loc_labelList.end(); ++it_aux )
            {
                // global label versus global label was already examined.
                // here, at least one label must be local
                if( ref_item->IsLabelGlobal() && ( *it_aux )->IsLabelGlobal() )
                    continue;

                if( ref_item->m_Label.CmpNoCase( ( *it_aux )->m_Label ) == 0 )
                {
                    // Create new marker for ERC.
                    int cntA = countIndenticalLabels( fullLabelList, ref_item );
                    int cntB = countIndenticalLabels( fullLabelList, *it_aux );

                    if( cntA <= cntB )
                        SimilarLabelsDiagnose( ref_item, ( *it_aux ) );
                    else
                        SimilarLabelsDiagnose( ( *it_aux ), ref_item );
                }
            }
        }
    }
}

// Helper function: count the number of labels identical to aLabel
//  for global label: global labels in the full project
//  for local label: all labels in the current sheet
static int countIndenticalLabels( std::vector<NETLIST_OBJECT*>& aList, NETLIST_OBJECT* aRef )
{
    int count = 0;

    if( aRef->IsLabelGlobal() )
    {
        for( auto i : aList)
        {
            if( i->IsLabelGlobal() && i->m_Label == aRef->m_Label )
                count++;
        }
    }
    else
    {
        for( auto i : aList)
        {
            if( i->m_Label == aRef->m_Label && i->m_SheetPath.Path() == aRef->m_SheetPath.Path() )
                count++;
        }
    }

    return count;
}

// Helper function: creates a marker for similar labels ERC warning
static void SimilarLabelsDiagnose( NETLIST_OBJECT* aItemA, NETLIST_OBJECT* aItemB )
{
    // Create new marker for ERC.
    SCH_MARKER* marker = new SCH_MARKER();

    marker->SetTimeStamp( GetNewTimeStamp() );
    marker->SetMarkerType( MARKER_BASE::MARKER_ERC );
    marker->SetErrorLevel( MARKER_BASE::MARKER_SEVERITY_WARNING );
    SCH_SCREEN* screen = aItemA->m_SheetPath.LastScreen();
    screen->Append( marker );

    wxString fmt = aItemA->IsLabelGlobal() ? _( "Global label '%s' (sheet '%s') looks like:" ) :
                                             _( "Local label '%s' (sheet '%s') looks like:" );
    wxString msg;

    msg.Printf( fmt, aItemA->m_Label, aItemA->m_SheetPath.PathHumanReadable() );
    marker->SetData( aItemA->IsLabelGlobal() && aItemB->IsLabelGlobal() ?
                            ERCE_SIMILAR_GLBL_LABELS : ERCE_SIMILAR_LABELS,
                     aItemA->m_Start, msg, aItemA->m_Start );

    fmt = aItemB->IsLabelGlobal() ? _( "Global label \"%s\" (sheet \"%s\")" ) :
                                    _( "Local label \"%s\" (sheet \"%s\")" );
    msg.Printf( fmt, aItemB->m_Label, aItemB->m_SheetPath.PathHumanReadable() );
    marker->SetAuxiliaryData( msg, aItemB->m_Start );
}
