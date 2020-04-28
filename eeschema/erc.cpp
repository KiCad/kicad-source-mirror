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
 *  Minimal ERC requirements:
 *  All pins *must* be connected (except ELECTRICAL_PINTYPE::PT_NC).
 *  When a pin is not connected in schematic, the user must place a "non
 * connected" symbol to this pin.
 *  This ensures a forgotten connection will be detected.
 */

/* Messages for conflicts :
 *  ELECTRICAL_PINTYPE::PT_INPUT, ELECTRICAL_PINTYPE::PT_OUTPUT, ELECTRICAL_PINTYPE:PT_:BIDI, ELECTRICAL_PINTYPE::PT_TRISTATE, ELECTRICAL_PINTYPE::PT_PASSIVE,
 *  ELECTRICAL_PINTYPE::PT_UNSPECIFIED, ELECTRICAL_PINTYPE::PT_POWER_IN, ELECTRICAL_PINTYPE::PT_POWER_OUT, ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR,
 *  ELECTRICAL_PINTYPE::PT_OPENEMITTER, ELECTRICAL_PINTYPE::PT_NC
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
int PinMap[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL];

/**
 * Default Look up table which gives the ERC error level for a pair of connected pins
 * Same as DiagErc, but cannot be modified.
 *  Used to init or reset DiagErc
 *  note also, to avoid inconsistancy:
 *    DefaultDiagErc[i][j] = DefaultDiagErc[j][i]
 */
int DefaultPinMap[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL] =
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
static int MinimalReq[ELECTRICAL_PINTYPES_TOTAL][ELECTRICAL_PINTYPES_TOTAL] =
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
    int         err_count = 0;
    SCH_SCREENS screenList;      // Created the list of screen

    for( screen = screenList.GetFirst(); screen != NULL; screen = screenList.GetNext() )
    {
        std::vector<SCH_SHEET*> list;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
            list.push_back( static_cast<SCH_SHEET*>( item ) );

        for( size_t i = 0; i < list.size(); i++ )
        {
            SCH_SHEET* item = list[i];

            for( size_t j = i + 1; j < list.size(); j++ )
            {
                SCH_SHEET* test_item = list[j];

                // We have found a second sheet: compare names
                // we are using case insensitive comparison to avoid mistakes between
                // similar names like Mysheet and mysheet
                if( item->GetName().CmpNoCase( test_item->GetName() ) == 0 )
                {
                    if( aCreateMarker )
                    {
                        ERC_ITEM* ercItem = new ERC_ITEM( ERCE_DUPLICATE_SHEET_NAME );
                        ercItem->SetItems( item, test_item );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, item->GetPosition() );
                        screen->Append( marker );
                    }

                    err_count++;
                }
            }
        }
    }

    return err_count;
}


void TestTextVars()
{
    SCH_SCREENS screens;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_LOCATE_ANY_T ) )
        {
            if( item->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

                for( SCH_FIELD& field : component->GetFields() )
                {
                    if( field.GetShownText().Matches( wxT( "*${*}*" ) ) )
                    {
                        wxPoint pos = field.GetPosition() - component->GetPosition();
                        pos = component->GetTransform().TransformCoordinate( pos );
                        pos += component->GetPosition();

                        ERC_ITEM* ercItem = new ERC_ITEM( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( &field );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, pos );
                        screen->Append( marker );
                    }
                }
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

                for( SCH_FIELD& field : sheet->GetFields() )
                {
                    if( field.GetShownText().Matches( wxT( "*${*}*" ) ) )
                    {
                        ERC_ITEM* ercItem = new ERC_ITEM( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( &field );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, field.GetPosition() );
                        screen->Append( marker );
                    }
                }

                for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
                {
                    if( pin->GetShownText().Matches( wxT( "*${*}*" ) ) )
                    {
                        ERC_ITEM* ercItem = new ERC_ITEM( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( pin );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetPosition() );
                        screen->Append( marker );
                    }
                }
            }
            else if( SCH_TEXT* text = dynamic_cast<SCH_TEXT*>( item ) )
            {
                if( text->GetShownText().Matches( wxT( "*${*}*" ) ) )
                {
                    ERC_ITEM* ercItem = new ERC_ITEM( ERCE_UNRESOLVED_VARIABLE );
                    ercItem->SetItems( text );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, text->GetPosition() );
                    screen->Append( marker );
                }
            }
        }
    }
}


int TestConflictingBusAliases()
{
    wxString    msg;
    int         err_count = 0;
    SCH_SCREENS screens;
    std::vector< std::shared_ptr<BUS_ALIAS> > aliases;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != NULL; screen = screens.GetNext() )
    {
        std::unordered_set< std::shared_ptr<BUS_ALIAS> > screen_aliases = screen->GetBusAliases();

        for( const std::shared_ptr<BUS_ALIAS>& alias : screen_aliases )
        {
            for( const std::shared_ptr<BUS_ALIAS>& test : aliases )
            {
                if( alias->GetName() == test->GetName() && alias->Members() != test->Members() )
                {
                    msg.Printf( _( "Bus alias %s has conflicting definitions on %s and %s" ),
                                alias->GetName(),
                                alias->GetParent()->GetFileName(),
                                test->GetParent()->GetFileName() );

                    ERC_ITEM* ercItem = new ERC_ITEM( ERCE_BUS_ALIAS_CONFLICT );
                    ercItem->SetErrorMessage( msg );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, wxPoint() );
                    test->GetParent()->Append( marker );

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
        SCH_COMPONENT* unit = nullptr;
        wxString       unitName;
        wxString       unitFP;

        for( unsigned i = 0; i < component.second.GetCount(); ++i )
        {
            SCH_SHEET_PATH sheetPath = refList.GetItem( i ).GetSheetPath();
            unitFP = refList.GetItem( i ).GetComp()->GetField( FOOTPRINT )->GetText();

            if( !unitFP.IsEmpty() )
            {
                unit = refList.GetItem( i ).GetComp();
                unitName = unit->GetRef( &sheetPath, true );
                break;
            }
        }

        for( unsigned i = 0; i < component.second.GetCount(); ++i )
        {
            SCH_REFERENCE& secondRef = refList.GetItem( i );
            SCH_COMPONENT* secondUnit = secondRef.GetComp();
            wxString       secondName = secondUnit->GetRef( &secondRef.GetSheetPath(), true );
            const wxString secondFp = secondUnit->GetField( FOOTPRINT )->GetText();
            wxString       msg;

            if( !secondFp.IsEmpty() && unitFP != secondFp )
            {
                msg.Printf( _( "Different footprints assigned to %s and %s" ),
                            unitName, secondName );

                ERC_ITEM* ercItem = new ERC_ITEM( ERCE_DIFFERENT_UNIT_FP );
                ercItem->SetErrorMessage( msg );
                ercItem->SetItems( unit, secondUnit );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, secondUnit->GetPosition() );
                secondRef.GetSheetPath().LastScreen()->Append( marker );

                ++errors;
            }
        }
    }

    return errors;
}


void Diagnose( NETLIST_OBJECT* aNetItemRef, NETLIST_OBJECT* aNetItemTst, int aMinConn, int aDiag )
{
    if( aDiag == OK || aMinConn < 1 || aNetItemRef->m_Type != NETLIST_ITEM::PIN )
        return;

    SCH_PIN* pin = static_cast<SCH_PIN*>( aNetItemRef->m_Comp );

    if( aNetItemTst == NULL)
    {
        if( aMinConn == NOD )    /* Nothing driving the net. */
        {
            ERC_ITEM* ercItem = new ERC_ITEM( ERCE_PIN_NOT_DRIVEN );
            ercItem->SetItems( pin );

            SCH_MARKER* marker = new SCH_MARKER( ercItem, aNetItemRef->m_Start );
            aNetItemRef->m_SheetPath.LastScreen()->Append( marker );
            return;
        }
    }

    if( aNetItemTst && aNetItemTst->m_Type == NETLIST_ITEM::PIN )  /* Error between 2 pins */
    {
        ERC_ITEM* ercItem = new ERC_ITEM( aDiag == ERR ? ERCE_PIN_TO_PIN_ERROR
                                                       : ERCE_PIN_TO_PIN_WARNING );
        ercItem->SetItems( pin, static_cast<SCH_PIN*>( aNetItemTst->m_Comp ) );

        SCH_MARKER* marker = new SCH_MARKER( ercItem, aNetItemRef->m_Start );
        aNetItemRef->m_SheetPath.LastScreen()->Append( marker );
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

    if( ref_elect_type == ELECTRICAL_PINTYPE::PT_NC )
        local_minconn = NPI;

    /* Test pins connected to NetItemRef */
    for( ; ; netItemTst++ )
    {
        if( aNetItemRef == netItemTst )
            continue;

        if( netItemTst < aList->size() )
        {
            ELECTRICAL_PINTYPE test_elect_type = aList->GetItem( netItemTst )->m_ElectricalPinType;
            erc = PinMap[static_cast<int>( ref_elect_type )][static_cast<int>(test_elect_type )];
        }

        if( erc != OK )
            Diagnose( aList->GetItem( aNetItemRef ), aList->GetItem( netItemTst ), 1, erc );

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

                if( local_minconn == NOC && aList->GetItemType( aNetItemRef ) == NETLIST_ITEM::PIN )
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
                        if( aList->GetItemType( duplicate ) != NETLIST_ITEM::PIN )
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
        case NETLIST_ITEM::ITEM_UNSPECIFIED:
        case NETLIST_ITEM::SEGMENT:
        case NETLIST_ITEM::BUS:
        case NETLIST_ITEM::JUNCTION:
        case NETLIST_ITEM::LABEL:
        case NETLIST_ITEM::HIERLABEL:
        case NETLIST_ITEM::BUSLABELMEMBER:
        case NETLIST_ITEM::HIERBUSLABELMEMBER:
        case NETLIST_ITEM::SHEETBUSLABELMEMBER:
        case NETLIST_ITEM::SHEETLABEL:
        case NETLIST_ITEM::GLOBLABEL:
        case NETLIST_ITEM::GLOBBUSLABELMEMBER:
        case NETLIST_ITEM::PINLABEL:
            break;

        case NETLIST_ITEM::NOCONNECT:
            local_minconn = std::max( NET_NC, local_minconn );
            break;

        case NETLIST_ITEM::PIN:
            jj            = aList->GetItem( netItemTst )->m_ElectricalPinType;
            local_minconn = std::max(
                    MinimalReq[static_cast<int>( ref_elect_type )][static_cast<int>( jj )],
                    local_minconn );

            if( netItemTst <= aNetItemRef )
                break;

            if( erc == OK )
            {
                erc = PinMap[static_cast<int>( ref_elect_type )][static_cast<int>( jj )];

                if( erc != OK )
                {
                    if( aList->GetConnectionType( netItemTst ) == NET_CONNECTION::UNCONNECTED )
                    {
                        aList->SetConnectionType( netItemTst,
                                                  NET_CONNECTION::NOCONNECT_SYMBOL_PRESENT );
                    }
                }
            }

            break;
        }
    }
}

// this code try to detect similar labels, i.e. labels which are identical
// when they are compared using case insensitive coparisons.


// A helper struct to compare NETLIST_OBJECT items by sheetpath and label texts
// for a std::set<NETLIST_OBJECT*> container
// the full text is "sheetpath+label" for local labels and "label" for global labels
struct compare_labels
{
    bool operator() ( const NETLIST_OBJECT* lab1, const NETLIST_OBJECT* lab2 ) const
    {
        wxString str1 = lab1->m_SheetPath.PathAsString() + lab1->m_Label;
        wxString str2 = lab2->m_SheetPath.PathAsString() + lab2->m_Label;

        return str1.Cmp( str2 ) < 0;
    }
};

struct compare_label_names
{
    bool operator() ( const NETLIST_OBJECT* lab1, const NETLIST_OBJECT* lab2 ) const
    {
        return lab1->m_Label.Cmp( lab2->m_Label ) < 0;
    }
};

struct compare_paths
{
    bool operator() ( const NETLIST_OBJECT* lab1, const NETLIST_OBJECT* lab2 ) const
    {
        return lab1->m_SheetPath.Path() < lab2->m_SheetPath.Path();
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
        case NETLIST_ITEM::LABEL:
        case NETLIST_ITEM::BUSLABELMEMBER:
        case NETLIST_ITEM::PINLABEL:
        case NETLIST_ITEM::GLOBBUSLABELMEMBER:
        case NETLIST_ITEM::HIERLABEL:
        case NETLIST_ITEM::HIERBUSLABELMEMBER:
        case NETLIST_ITEM::GLOBLABEL:
            // add this label in lists
            uniqueLabelList.insert( GetItem( netItem ) );
            fullLabelList.push_back( GetItem( netItem ) );
            break;

        case NETLIST_ITEM::SHEETLABEL:
        case NETLIST_ITEM::SHEETBUSLABELMEMBER:
        default:
            break;
        }
    }

    // build global labels and compare
    std::set<NETLIST_OBJECT*, compare_label_names> loc_labelList;

    for( NETLIST_OBJECT* label : uniqueLabelList )
    {
        if( label->IsLabelGlobal() )
            loc_labelList.insert( label );
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

    for( NETLIST_OBJECT* label : uniqueLabelList )
        pathsList.insert( label );

    // Examine each label inside a sheet path:
    for( NETLIST_OBJECT* candidate : pathsList )
    {
        loc_labelList.clear();

        for( NETLIST_OBJECT* uniqueLabel : uniqueLabelList)
        {
            if( candidate->m_SheetPath.Path() == uniqueLabel->m_SheetPath.Path() )
                loc_labelList.insert( uniqueLabel );
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
        for( NETLIST_OBJECT* i : aList )
        {
            if( i->IsLabelGlobal() && i->m_Label == aRef->m_Label )
                count++;
        }
    }
    else
    {
        for( NETLIST_OBJECT* i : aList )
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
    ERC_ITEM* ercItem = new ERC_ITEM( ERCE_SIMILAR_LABELS );
    ercItem->SetItems( aItemA->m_Comp, aItemB->m_Comp );

    SCH_MARKER* marker = new SCH_MARKER( ercItem, aItemA->m_Start );
    aItemA->m_SheetPath.LastScreen()->Append( marker );
}
