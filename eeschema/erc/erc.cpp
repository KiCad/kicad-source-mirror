/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <numeric>

#include "connection_graph.h"
#include "kiface_ids.h"
#include <advanced_config.h>
#include <common.h>     // for ExpandEnvVarSubstitutions
#include <erc/erc.h>
#include <erc/erc_sch_pin_context.h>
#include <gal/graphics_abstraction_layer.h>
#include <string_utils.h>
#include <sch_pin.h>
#include <project_sch.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <sch_edit_frame.h>
#include <sch_marker.h>
#include <sch_reference_list.h>
#include <sch_rule_area.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_textbox.h>
#include <sch_line.h>
#include <schematic.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <wx/ffile.h>
#include <sim/sim_lib_mgr.h>
#include <progress_reporter.h>
#include <kiway.h>


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

// Messages for matrix rows:
const wxString CommentERC_H[] =
{
    _( "Input Pin" ),
    _( "Output Pin" ),
    _( "Bidirectional Pin" ),
    _( "Tri-State Pin" ),
    _( "Passive Pin" ),
    _( "Free Pin" ),
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
    _( "Free Pin" ),
    _( "Unspecified Pin" ),
    _( "Power Input Pin" ),
    _( "Power Output Pin" ),
    _( "Open Collector" ),
    _( "Open Emitter" ),
    _( "No Connection" )
};


// List of pin types that are considered drivers for usual input pins
// i.e. pin type = ELECTRICAL_PINTYPE::PT_INPUT, but not PT_POWER_IN
// that need only a PT_POWER_OUT pin type to be driven
const std::set<ELECTRICAL_PINTYPE> DrivingPinTypes =
        {
            ELECTRICAL_PINTYPE::PT_OUTPUT,
            ELECTRICAL_PINTYPE::PT_POWER_OUT,
            ELECTRICAL_PINTYPE::PT_PASSIVE,
            ELECTRICAL_PINTYPE::PT_TRISTATE,
            ELECTRICAL_PINTYPE::PT_BIDI
        };

// List of pin types that are considered drivers for power pins
// In fact only a ELECTRICAL_PINTYPE::PT_POWER_OUT pin type can drive
// power input pins
const std::set<ELECTRICAL_PINTYPE> DrivingPowerPinTypes =
        {
            ELECTRICAL_PINTYPE::PT_POWER_OUT
        };

// List of pin types that require a driver elsewhere on the net
const std::set<ELECTRICAL_PINTYPE> DrivenPinTypes =
        {
            ELECTRICAL_PINTYPE::PT_INPUT,
            ELECTRICAL_PINTYPE::PT_POWER_IN
        };

extern void CheckDuplicatePins( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                                UNITS_PROVIDER* aUnitsProvider );

int ERC_TESTER::TestDuplicateSheetNames( bool aCreateMarker )
{
    int err_count = 0;

    for( SCH_SCREEN* screen = m_screens.GetFirst(); screen; screen = m_screens.GetNext() )
    {
        std::vector<SCH_SHEET*> list;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
            list.push_back( static_cast<SCH_SHEET*>( item ) );

        for( size_t i = 0; i < list.size(); i++ )
        {
            SCH_SHEET* sheet = list[i];

            for( size_t j = i + 1; j < list.size(); j++ )
            {
                SCH_SHEET* test_item = list[j];

                // We have found a second sheet: compare names
                // we are using case insensitive comparison to avoid mistakes between
                // similar names like Mysheet and mysheet
                if( sheet->GetShownName( false ).CmpNoCase( test_item->GetShownName( false ) ) == 0 )
                {
                    if( aCreateMarker )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem =
                                ERC_ITEM::Create( ERCE_DUPLICATE_SHEET_NAME );
                        ercItem->SetItems( sheet, test_item );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, sheet->GetPosition() );
                        screen->Append( marker );
                    }

                    err_count++;
                }
            }
        }
    }

    return err_count;
}


void ERC_TESTER::TestTextVars( DS_PROXY_VIEW_ITEM* aDrawingSheet )
{
    DS_DRAW_ITEM_LIST wsItems( schIUScale );

    auto unresolved = [this]( wxString str )
    {
        str = ExpandEnvVarSubstitutions( str, &m_schematic->Prj() );
        return str.Matches( wxS( "*${*}*" ) );
    };

    if( aDrawingSheet )
    {
        wsItems.SetPageNumber( wxS( "1" ) );
        wsItems.SetSheetCount( 1 );
        wsItems.SetFileName( wxS( "dummyFilename" ) );
        wsItems.SetSheetName( wxS( "dummySheet" ) );
        wsItems.SetSheetLayer( wxS( "dummyLayer" ) );
        wsItems.SetProject( &m_schematic->Prj() );
        wsItems.BuildDrawItemsList( aDrawingSheet->GetPageInfo(), aDrawingSheet->GetTitleBlock() );
    }

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LOCATE_ANY_T ) )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                for( SCH_FIELD& field : symbol->GetFields() )
                {
                    if( unresolved( field.GetShownText( &sheet, true ) ) )
                    {
                        auto ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( &field );
                        ercItem->SetSheetSpecificPath( sheet );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, field.GetPosition() );
                        screen->Append( marker );
                    }
                }
            }
            else if( item->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* subSheet = static_cast<SCH_SHEET*>( item );

                for( SCH_FIELD& field : subSheet->GetFields() )
                {
                    if( unresolved( field.GetShownText( &sheet, true ) ) )
                    {
                        auto ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( &field );
                        ercItem->SetSheetSpecificPath( sheet );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, field.GetPosition() );
                        screen->Append( marker );
                    }
                }

                SCH_SHEET_PATH subSheetPath = sheet;
                subSheetPath.push_back( subSheet );

                for( SCH_SHEET_PIN* pin : subSheet->GetPins() )
                {
                    if( pin->GetShownText( &subSheetPath, true ).Matches( wxS( "*${*}*" ) ) )
                    {
                        auto ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( pin );
                        ercItem->SetSheetSpecificPath( sheet );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetPosition() );
                        screen->Append( marker );
                    }
                }
            }
            else if( SCH_TEXT* text = dynamic_cast<SCH_TEXT*>( item ) )
            {
                if( text->GetShownText( &sheet, true ).Matches( wxS( "*${*}*" ) ) )
                {
                    auto ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    ercItem->SetItems( text );
                    ercItem->SetSheetSpecificPath( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, text->GetPosition() );
                    screen->Append( marker );
                }
            }
            else if( SCH_TEXTBOX* textBox = dynamic_cast<SCH_TEXTBOX*>( item ) )
            {
                if( textBox->GetShownText( &sheet, true ).Matches( wxS( "*${*}*" ) ) )
                {
                    auto ercItem = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    ercItem->SetItems( textBox );
                    ercItem->SetSheetSpecificPath( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, textBox->GetPosition() );
                    screen->Append( marker );
                }
            }
        }

        for( DS_DRAW_ITEM_BASE* item = wsItems.GetFirst(); item; item = wsItems.GetNext() )
        {
            if( DS_DRAW_ITEM_TEXT* text = dynamic_cast<DS_DRAW_ITEM_TEXT*>( item ) )
            {
                if( text->GetShownText( true ).Matches( wxS( "*${*}*" ) ) )
                {
                    std::shared_ptr<ERC_ITEM> erc = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    erc->SetErrorMessage( _( "Unresolved text variable in drawing sheet" ) );
                    erc->SetSheetSpecificPath( sheet );

                    SCH_MARKER* marker = new SCH_MARKER( erc, text->GetPosition() );
                    screen->Append( marker );
                }
            }
        }
    }
}


int ERC_TESTER::TestConflictingBusAliases()
{
    wxString                                msg;
    int                                     err_count = 0;
    std::vector<std::shared_ptr<BUS_ALIAS>> aliases;

    for( SCH_SCREEN* screen = m_screens.GetFirst(); screen; screen = m_screens.GetNext() )
    {
        const auto& screen_aliases = screen->GetBusAliases();

        for( const std::shared_ptr<BUS_ALIAS>& alias : screen_aliases )
        {
            std::vector<wxString> aliasMembers = alias->Members();
            std::sort( aliasMembers.begin(), aliasMembers.end() );

            for( const std::shared_ptr<BUS_ALIAS>& test : aliases )
            {
                std::vector<wxString> testMembers = test->Members();
                std::sort( testMembers.begin(), testMembers.end() );

                if( alias->GetName() == test->GetName() && aliasMembers != testMembers )
                {
                    msg.Printf( _( "Bus alias %s has conflicting definitions on %s and %s" ),
                                alias->GetName(),
                                alias->GetParent()->GetFileName(),
                                test->GetParent()->GetFileName() );

                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_BUS_ALIAS_CONFLICT );
                    ercItem->SetErrorMessage( msg );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, VECTOR2I() );
                    test->GetParent()->Append( marker );

                    ++err_count;
                }
            }
        }

        aliases.insert( aliases.end(), screen_aliases.begin(), screen_aliases.end() );
    }

    return err_count;
}


int ERC_TESTER::TestMultiunitFootprints()
{
    int errors = 0;

    for( std::pair<const wxString, SCH_REFERENCE_LIST>& symbol : m_refMap )
    {
        SCH_REFERENCE_LIST& refList = symbol.second;

        if( refList.GetCount() == 0 )
        {
            wxFAIL;   // it should not happen
            continue;
        }

        // Reference footprint
        SCH_SYMBOL* unit = nullptr;
        wxString    unitName;
        wxString    unitFP;

        for( unsigned i = 0; i < refList.GetCount(); ++i )
        {
            SCH_SHEET_PATH sheetPath = refList.GetItem( i ).GetSheetPath();
            unitFP = refList.GetItem( i ).GetFootprint();

            if( !unitFP.IsEmpty() )
            {
                unit = refList.GetItem( i ).GetSymbol();
                unitName = unit->GetRef( &sheetPath, true );
                break;
            }
        }

        for( unsigned i = 0; i < refList.GetCount(); ++i )
        {
            SCH_REFERENCE& secondRef = refList.GetItem( i );
            SCH_SYMBOL*    secondUnit = secondRef.GetSymbol();
            wxString       secondName = secondUnit->GetRef( &secondRef.GetSheetPath(), true );
            const wxString secondFp = secondRef.GetFootprint();
            wxString       msg;

            if( unit && !secondFp.IsEmpty() && unitFP != secondFp )
            {
                msg.Printf( _( "Different footprints assigned to %s and %s" ),
                            unitName, secondName );

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DIFFERENT_UNIT_FP );
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


int ERC_TESTER::TestMissingUnits()
{
    int errors = 0;

    for( std::pair<const wxString, SCH_REFERENCE_LIST>& symbol : m_refMap )
    {
        SCH_REFERENCE_LIST& refList = symbol.second;

        wxCHECK2( refList.GetCount(), continue );

        // Reference unit
        SCH_REFERENCE& base_ref = refList.GetItem( 0 );
        SCH_SYMBOL* unit = base_ref.GetSymbol();
        LIB_SYMBOL* libSymbol = base_ref.GetLibPart();

        if( static_cast<ssize_t>( refList.GetCount() ) == libSymbol->GetUnitCount() )
            continue;

        std::set<int> lib_units;
        std::set<int> instance_units;
        std::set<int> missing_units;

        auto report_missing =
                [&]( std::set<int>& aMissingUnits, wxString aErrorMsg, int aErrorCode )
                {
                    wxString msg;
                    wxString missing_pin_units = wxS( "[ " );
                    int ii = 0;

                    for( int missing_unit : aMissingUnits )
                    {
                        if( ii++ == 3 )
                        {
                            missing_pin_units += wxS( "....." );
                            break;
                        }

                        missing_pin_units += libSymbol->GetUnitDisplayName( missing_unit ) + ", " ;
                    }

                    missing_pin_units.Truncate( missing_pin_units.length() - 2 );
                    missing_pin_units += wxS( " ]" );

                    msg.Printf( aErrorMsg, symbol.first, missing_pin_units );

                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( aErrorCode );
                    ercItem->SetErrorMessage( msg );
                    ercItem->SetItems( unit );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, unit->GetPosition() );
                    base_ref.GetSheetPath().LastScreen()->Append( marker );

                    ++errors;
                };

        for( int ii = 1; ii <= libSymbol->GetUnitCount(); ++ii )
            lib_units.insert( lib_units.end(), ii );

        for( size_t ii = 0; ii < refList.GetCount(); ++ii )
            instance_units.insert( instance_units.end(), refList.GetItem( ii ).GetUnit() );

        std::set_difference( lib_units.begin(), lib_units.end(),
                             instance_units.begin(), instance_units.end(),
                             std::inserter( missing_units, missing_units.begin() ) );

        if( !missing_units.empty() && m_settings.IsTestEnabled( ERCE_MISSING_UNIT ) )
        {
            report_missing( missing_units, _( "Symbol %s has unplaced units %s" ),
                            ERCE_MISSING_UNIT );
        }

        std::set<int> missing_power;
        std::set<int> missing_input;
        std::set<int> missing_bidi;

        for( int missing_unit : missing_units )
        {
            int bodyStyle = 0;

            for( size_t ii = 0; ii < refList.GetCount(); ++ii )
            {
                if( refList.GetItem( ii ).GetUnit() == missing_unit )
                {
                    bodyStyle = refList.GetItem( ii ).GetSymbol()->GetBodyStyle();
                    break;
                }
            }

            for( SCH_PIN* pin : libSymbol->GetPins( missing_unit, bodyStyle ) )
            {
                switch( pin->GetType() )
                {
                case ELECTRICAL_PINTYPE::PT_POWER_IN:
                    missing_power.insert( missing_unit );
                    break;

                case ELECTRICAL_PINTYPE::PT_BIDI:
                    missing_bidi.insert( missing_unit );
                    break;

                case ELECTRICAL_PINTYPE::PT_INPUT:
                    missing_input.insert( missing_unit );
                    break;

                default:
                    break;
                }
            }
        }

        if( !missing_power.empty() && m_settings.IsTestEnabled( ERCE_MISSING_POWER_INPUT_PIN ) )
        {
            report_missing( missing_power,
                            _( "Symbol %s has input power pins in units %s that are not placed." ),
                            ERCE_MISSING_POWER_INPUT_PIN );
        }

        if( !missing_input.empty() && m_settings.IsTestEnabled( ERCE_MISSING_INPUT_PIN ) )
        {
           report_missing( missing_input,
                           _( "Symbol %s has input pins in units %s that are not placed." ),
                           ERCE_MISSING_INPUT_PIN );
        }

        if( !missing_bidi.empty() && m_settings.IsTestEnabled( ERCE_MISSING_BIDI_PIN ) )
        {
            report_missing( missing_bidi,
                            _( "Symbol %s has bidirectional pins in units %s that are not "
                               "placed." ),
                            ERCE_MISSING_BIDI_PIN );
        }
    }

    return errors;
}


int ERC_TESTER::TestMissingNetclasses()
{
    int                            err_count = 0;
    std::shared_ptr<NET_SETTINGS>& settings = m_schematic->Prj().GetProjectFile().NetSettings();
    wxString                       defaultNetclass = settings->m_DefaultNetClass->GetName();

    auto logError =
            [&]( const SCH_SHEET_PATH& sheet, SCH_ITEM* item, const wxString& netclass )
            {
                err_count++;

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_UNDEFINED_NETCLASS );

                ercItem->SetItems( item );
                ercItem->SetErrorMessage( wxString::Format( _( "Netclass %s is not defined" ),
                                                            netclass ) );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, item->GetPosition() );
                sheet.LastScreen()->Append( marker );
            };

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            item->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        if( aChild->Type() == SCH_FIELD_T )
                        {
                            SCH_FIELD* field = static_cast<SCH_FIELD*>( aChild );

                            if( field->GetCanonicalName() == wxT( "Netclass" ) )
                            {
                                wxString netclass = field->GetShownText( &sheet, false );

                                if( !netclass.IsSameAs( defaultNetclass )
                                        && settings->m_NetClasses.count( netclass ) == 0 )
                                {
                                    logError( sheet, item, netclass );
                                }
                            }
                        }

                        return true;
                    } );
        }
    }

    return err_count;
}


int ERC_TESTER::TestFourWayJunction()
{
    int err_count = 0;

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        std::map<VECTOR2I, std::vector<SCH_ITEM*>> connMap;
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
                connMap[pin->GetPosition()].emplace_back( pin );
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->IsGraphicLine() )
                continue;

            for( const VECTOR2I& pt : line->GetConnectionPoints() )
                connMap[pt].emplace_back( line );
        }

        for( const std::pair<const VECTOR2I, std::vector<SCH_ITEM*>>& pair : connMap )
        {
            if( pair.second.size() >= 4 )
            {
                err_count++;

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_FOUR_WAY_JUNCTION );

                ercItem->SetItems( pair.second[0], pair.second[1], pair.second[2], pair.second[3] );

                wxString msg = wxString::Format( _( "Four items connected at %d, %d" ),
                                                 pair.first.x, pair.first.y );
                ercItem->SetErrorMessage( msg );

                ercItem->SetSheetSpecificPath( sheet );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, pair.first );
                sheet.LastScreen()->Append( marker );
            }
        }
    }

    return err_count;
}


int ERC_TESTER::TestNoConnectPins()
{
    int err_count = 0;

    for( const SCH_SHEET_PATH& sheet : m_sheetList )
    {
        std::map<VECTOR2I, std::vector<SCH_ITEM*>> pinMap;

        auto addOther =
                [&]( const VECTOR2I& pt, SCH_ITEM* aOther )
                {
                    if( pinMap.count( pt ) )
                        pinMap[pt].emplace_back( aOther );
                };

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                if( pin->GetLibPin()->GetType() == ELECTRICAL_PINTYPE::PT_NC )
                    pinMap[pin->GetPosition()].emplace_back( pin );
            }
        }

        for( SCH_ITEM* item : sheet.LastScreen()->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
                {
                    if( pin->GetLibPin()->GetType() != ELECTRICAL_PINTYPE::PT_NC )
                        addOther( pin->GetPosition(), pin );
                }
            }
            else if( item->IsConnectable() )
            {
                for( const VECTOR2I& pt : item->GetConnectionPoints() )
                    addOther( pt, item );
            }
        }

        for( const std::pair<const VECTOR2I, std::vector<SCH_ITEM*>>& pair : pinMap )
        {
            if( pair.second.size() > 1 )
            {
                err_count++;

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_CONNECTED );

                ercItem->SetItems( pair.second[0], pair.second[1],
                                   pair.second.size() > 2 ? pair.second[2] : nullptr,
                                   pair.second.size() > 3 ? pair.second[3] : nullptr );
                ercItem->SetErrorMessage( _( "Pin with 'no connection' type is connected" ) );
                ercItem->SetSheetSpecificPath( sheet );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, pair.first );
                sheet.LastScreen()->Append( marker );
            }
        }
    }

    return err_count;
}


int ERC_TESTER::TestPinToPin()
{
    int errors = 0;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : m_nets )
    {
        std::vector<ERC_SCH_PIN_CONTEXT>           pins;
        std::unordered_map<EDA_ITEM*, SCH_SCREEN*> pinToScreenMap;
        bool has_noconnect = false;

        for( CONNECTION_SUBGRAPH* subgraph: net.second )
        {
            if( subgraph->GetNoConnect() )
                has_noconnect = true;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    pins.emplace_back( static_cast<SCH_PIN*>( item ), subgraph->GetSheet() );
                    pinToScreenMap[item] = subgraph->GetSheet().LastScreen();
                }
            }
        }

        std::sort( pins.begin(), pins.end(),
                   []( const ERC_SCH_PIN_CONTEXT& lhs, const ERC_SCH_PIN_CONTEXT& rhs )
                   {
                       int ret = StrNumCmp( lhs.Pin()->GetParentSymbol()->GetRef( &lhs.Sheet() ),
                                            rhs.Pin()->GetParentSymbol()->GetRef( &rhs.Sheet() ) );

                       if( ret == 0 )
                           ret = StrNumCmp( lhs.Pin()->GetNumber(), rhs.Pin()->GetNumber() );

                       if( ret == 0 )
                           ret = lhs < rhs; // Fallback to hash to guarantee deterministic sort

                       return ret < 0;
                   } );

        ERC_SCH_PIN_CONTEXT needsDriver;
        bool                hasDriver = false;

        // We need different drivers for power nets and normal nets.
        // A power net has at least one pin having the ELECTRICAL_PINTYPE::PT_POWER_IN
        // and power nets can be driven only by ELECTRICAL_PINTYPE::PT_POWER_OUT pins
        bool     ispowerNet  = false;

        for( ERC_SCH_PIN_CONTEXT& refPin : pins )
        {
            if( refPin.Pin()->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN )
            {
                ispowerNet = true;
                break;
            }
        }

        for( auto refIt = pins.begin(); refIt != pins.end(); ++refIt )
        {
            ERC_SCH_PIN_CONTEXT& refPin = *refIt;
            ELECTRICAL_PINTYPE refType = refPin.Pin()->GetType();

            if( DrivenPinTypes.contains( refType ) )
            {
                // needsDriver will be the pin shown in the error report eventually, so try to
                // upgrade to a "better" pin if possible: something visible and only a power symbol
                // if this net needs a power driver
                if( !needsDriver.Pin()
                    || ( !needsDriver.Pin()->IsVisible() && refPin.Pin()->IsVisible() )
                    || ( ispowerNet
                                 != ( needsDriver.Pin()->GetType()
                                      == ELECTRICAL_PINTYPE::PT_POWER_IN )
                         && ispowerNet == ( refType == ELECTRICAL_PINTYPE::PT_POWER_IN ) ) )
                {
                    needsDriver = refPin;
                }
            }

            if( ispowerNet )
                hasDriver |= ( DrivingPowerPinTypes.count( refType ) != 0 );
            else
                hasDriver |= ( DrivingPinTypes.count( refType ) != 0 );

            for( auto testIt = refIt + 1; testIt != pins.end(); ++testIt )
            {
                ERC_SCH_PIN_CONTEXT& testPin = *testIt;

                // Multiple pins in the same symbol that share a type,
                // name and position are considered
                // "stacked" and shouldn't trigger ERC errors
                if( refPin.Pin()->IsStacked( testPin.Pin() ) && refPin.Sheet() == testPin.Sheet() )
                    continue;

                ELECTRICAL_PINTYPE testType = testPin.Pin()->GetType();

                if( ispowerNet )
                    hasDriver |= DrivingPowerPinTypes.contains( testType );
                else
                    hasDriver |= DrivingPinTypes.contains( testType );

                PIN_ERROR erc = m_settings.GetPinMapValue( refType, testType );

                if( erc != PIN_ERROR::OK && m_settings.IsTestEnabled( ERCE_PIN_TO_PIN_WARNING ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem =
                            ERC_ITEM::Create( erc == PIN_ERROR::WARNING ? ERCE_PIN_TO_PIN_WARNING :
                                                                          ERCE_PIN_TO_PIN_ERROR );
                    ercItem->SetItems( refPin.Pin(), testPin.Pin() );
                    ercItem->SetSheetSpecificPath( refPin.Sheet() );
                    ercItem->SetItemsSheetPaths( refPin.Sheet(), testPin.Sheet() );

                    ercItem->SetErrorMessage(
                            wxString::Format( _( "Pins of type %s and %s are connected" ),
                                              ElectricalPinTypeGetText( refType ),
                                              ElectricalPinTypeGetText( testType ) ) );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, refPin.Pin()->GetPosition() );
                    pinToScreenMap[refPin.Pin()]->Append( marker );
                    errors++;
                }
            }
        }

        if( needsDriver.Pin() && !hasDriver && !has_noconnect )
        {
            int err_code = ispowerNet ? ERCE_POWERPIN_NOT_DRIVEN : ERCE_PIN_NOT_DRIVEN;

            if( m_settings.IsTestEnabled( err_code ) )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( err_code );

                ercItem->SetItems( needsDriver.Pin() );
                ercItem->SetSheetSpecificPath( needsDriver.Sheet() );
                ercItem->SetItemsSheetPaths( needsDriver.Sheet() );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, needsDriver.Pin()->GetPosition() );
                pinToScreenMap[needsDriver.Pin()]->Append( marker );
                errors++;
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestMultUnitPinConflicts()
{
    int errors = 0;

    std::unordered_map<wxString, std::pair<wxString, SCH_PIN*>> pinToNetMap;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : m_nets )
    {
        const wxString& netName = net.first.Name;

        for( CONNECTION_SUBGRAPH* subgraph : net.second )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN* pin = static_cast<SCH_PIN*>( item );
                    const SCH_SHEET_PATH& sheet = subgraph->GetSheet();

                    if( !pin->GetLibPin()->GetParentSymbol()->IsMulti() )
                        continue;

                    wxString name = pin->GetParentSymbol()->GetRef( &sheet ) +
                                      + ":" + pin->GetShownNumber();

                    if( !pinToNetMap.count( name ) )
                    {
                        pinToNetMap[name] = std::make_pair( netName, pin );
                    }
                    else if( pinToNetMap[name].first != netName )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem =
                                ERC_ITEM::Create( ERCE_DIFFERENT_UNIT_NET );

                        ercItem->SetErrorMessage( wxString::Format(
                                _( "Pin %s is connected to both %s and %s" ),
                                pin->GetShownNumber(),
                                netName,
                                pinToNetMap[name].first ) );

                        ercItem->SetItems( pin, pinToNetMap[name].second );
                        ercItem->SetSheetSpecificPath( sheet );
                        ercItem->SetItemsSheetPaths( sheet, sheet );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetPosition() );
                        sheet.LastScreen()->Append( marker );
                        errors += 1;
                    }
                }
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestSimilarLabels()
{
    int errors = 0;

    std::unordered_map<wxString, std::pair<SCH_LABEL_BASE*, SCH_SHEET_PATH>> labelMap;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : m_nets )
    {
        for( CONNECTION_SUBGRAPH* subgraph : net.second )
        {
            const SCH_SHEET_PATH& sheet = subgraph->GetSheet();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                switch( item->Type() )
                {
                case SCH_LABEL_T:
                case SCH_HIER_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                {
                    SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

                    wxString normalized = label->GetShownText( &sheet, false ).Lower();

                    if( !labelMap.count( normalized ) )
                    {
                        labelMap[normalized] = std::make_pair( label, sheet );
                        break;
                    }

                    auto& [ otherLabel, otherSheet ] = labelMap.at( normalized );

                    if( otherLabel->GetShownText( &otherSheet, false )
                            != label->GetShownText( &sheet, false ) )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_SIMILAR_LABELS );
                        ercItem->SetItems( label, labelMap.at( normalized ).first );
                        ercItem->SetSheetSpecificPath( sheet );
                        ercItem->SetItemsSheetPaths( sheet, labelMap.at( normalized ).second );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, label->GetPosition() );
                        sheet.LastScreen()->Append( marker );
                        errors += 1;
                    }

                    break;
                }

                default:
                    break;
                }
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestLibSymbolIssues()
{
    wxCHECK( m_schematic, 0 );

    SYMBOL_LIB_TABLE* libTable = PROJECT_SCH::SchSymbolLibTable( &m_schematic->Prj() );
    wxString          msg;
    int               err_count = 0;

    for( SCH_SCREEN* screen = m_screens.GetFirst(); screen; screen = m_screens.GetNext() )
    {
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            LIB_SYMBOL* libSymbolInSchematic = symbol->GetLibSymbolRef().get();

            wxCHECK2( libSymbolInSchematic, continue );

            wxString       libName = symbol->GetLibId().GetLibNickname();
            LIB_TABLE_ROW* libTableRow = libTable->FindRow( libName, true );

            if( !libTableRow )
            {
                if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_ISSUES );
                    ercItem->SetItems( symbol );
                    msg.Printf( _( "The current configuration does not include the symbol library '%s'" ),
                                UnescapeString( libName ) );
                    ercItem->SetErrorMessage( msg );

                    markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                }

                continue;
            }
            else if( !libTable->HasLibrary( libName, true ) )
            {
                if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_ISSUES );
                    ercItem->SetItems( symbol );
                    msg.Printf( _( "The library '%s' is not enabled in the current configuration" ),
                                UnescapeString( libName ) );
                    ercItem->SetErrorMessage( msg );

                    markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                }

                continue;
            }

            wxString    symbolName = symbol->GetLibId().GetLibItemName();
            LIB_SYMBOL* libSymbol = SchGetLibSymbol( symbol->GetLibId(), libTable );

            if( libSymbol == nullptr )
            {
                if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_ISSUES );
                    ercItem->SetItems( symbol );
                    msg.Printf( _( "Symbol '%s' not found in symbol library '%s'" ),
                                UnescapeString( symbolName ),
                                UnescapeString( libName ) );
                    ercItem->SetErrorMessage( msg );

                    markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                }

                continue;
            }

            std::unique_ptr<LIB_SYMBOL> flattenedSymbol = libSymbol->Flatten();
            constexpr int flags = SCH_ITEM::COMPARE_FLAGS::EQUALITY | SCH_ITEM::COMPARE_FLAGS::ERC;

            if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_MISMATCH ) )
            {
                // We have to check for duplicate pins first as they will cause Compare() to fail.
                std::vector<wxString> messages;
                UNITS_PROVIDER        unitsProvider( schIUScale, EDA_UNITS::MILS );
                CheckDuplicatePins( libSymbolInSchematic, messages, &unitsProvider );

                if( !messages.empty() )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DUPLICATE_PIN_ERROR );
                    ercItem->SetItems( symbol );
                    msg.Printf( _( "Symbol '%s' has multiple pins with the same pin number" ),
                                UnescapeString( symbolName ) );
                    ercItem->SetErrorMessage( msg );

                    markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                }
                else if( flattenedSymbol->Compare( *libSymbolInSchematic, flags ) != 0 )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_MISMATCH );
                    ercItem->SetItems( symbol );
                    msg.Printf( _( "Symbol '%s' doesn't match copy in library '%s'" ),
                                UnescapeString( symbolName ),
                                UnescapeString( libName ) );
                    ercItem->SetErrorMessage( msg );

                    markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                }
            }
        }

        for( SCH_MARKER* marker : markers )
        {
            screen->Append( marker );
            err_count += 1;
        }
    }

    return err_count;
}


int ERC_TESTER::TestFootprintLinkIssues( KIFACE* aCvPcb, PROJECT* aProject )
{
    wxCHECK( m_schematic, 0 );

    wxString msg;
    int      err_count = 0;

    typedef int (*TESTER_FN_PTR)( const wxString&, PROJECT* );

    TESTER_FN_PTR linkTester = (TESTER_FN_PTR) aCvPcb->IfaceOrAddress( KIFACE_TEST_FOOTPRINT_LINK );

    for( SCH_SHEET_PATH& sheet : m_sheetList )
    {
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    footprint = symbol->GetFootprintFieldText( true, &sheet, false );

            if( footprint.IsEmpty() )
                continue;

            LIB_ID fpID;

            if( fpID.Parse( footprint, true ) >= 0 )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_FOOTPRINT_LINK_ISSUES );
                msg.Printf( _( "'%s' is not a valid footprint identifier." ), footprint );
                ercItem->SetErrorMessage( msg );
                ercItem->SetItems( symbol );
                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                continue;
            }

            wxString libName = fpID.GetLibNickname();
            wxString fpName = fpID.GetLibItemName();
            int      ret = (linkTester)( footprint, aProject );

            if( ret == KIFACE_TEST_FOOTPRINT_LINK_NO_LIBRARY )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_FOOTPRINT_LINK_ISSUES );
                msg.Printf( _( "The current configuration does not include the footprint library '%s'." ),
                            libName );
                ercItem->SetErrorMessage( msg );
                ercItem->SetItems( symbol );
                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
            }
            else if( ret == KIFACE_TEST_FOOTPRINT_LINK_LIBRARY_NOT_ENABLED )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_FOOTPRINT_LINK_ISSUES );
                msg.Printf( _( "The footprint library '%s' is not enabled in the current configuration." ),
                            libName );
                ercItem->SetErrorMessage( msg );
                ercItem->SetItems( symbol );
                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
            }
            else if( ret == KIFACE_TEST_FOOTPRINT_LINK_NO_FOOTPRINT )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_FOOTPRINT_LINK_ISSUES );
                msg.Printf( _( "Footprint '%s' not found in library '%s'." ),
                            fpName,
                            libName );
                ercItem->SetErrorMessage( msg );
                ercItem->SetItems( symbol );
                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
            }
        }

        for( SCH_MARKER* marker : markers )
        {
            sheet.LastScreen()->Append( marker );
            err_count += 1;
        }
    }

    return err_count;
}


int ERC_TESTER::TestOffGridEndpoints()
{
    const int gridSize = m_schematic->Settings().m_ConnectionGridSize;
    int       err_count = 0;

    for( SCH_SCREEN* screen = m_screens.GetFirst(); screen; screen = m_screens.GetNext() )
    {
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_LINE_T && item->IsConnectable() )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( item );

                if( ( line->GetStartPoint().x % gridSize ) != 0
                        || ( line->GetStartPoint().y % gridSize ) != 0 )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                    ercItem->SetItems( line );

                    markers.emplace_back( new SCH_MARKER( ercItem, line->GetStartPoint() ) );
                }
                else if( ( line->GetEndPoint().x % gridSize ) != 0
                            || ( line->GetEndPoint().y % gridSize ) != 0 )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                    ercItem->SetItems( line );

                    markers.emplace_back( new SCH_MARKER( ercItem, line->GetEndPoint() ) );
                }
            }
            else if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                for( SCH_PIN* pin : symbol->GetPins( nullptr ) )
                {
                    VECTOR2I pinPos = pin->GetPosition();

                    if( ( pinPos.x % gridSize ) != 0 || ( pinPos.y % gridSize ) != 0 )
                    {
                        auto ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                        ercItem->SetItems( pin );

                        markers.emplace_back( new SCH_MARKER( ercItem, pinPos ) );
                        break;
                    }
                }
            }
        }

        for( SCH_MARKER* marker : markers )
        {
            screen->Append( marker );
            err_count += 1;
        }
    }

    return err_count;
}


int ERC_TESTER::TestSimModelIssues()
{
    wxString           msg;
    WX_STRING_REPORTER reporter( &msg );
    int                err_count = 0;
    SIM_LIB_MGR        libMgr( &m_schematic->Prj() );

    for( SCH_SHEET_PATH& sheet : m_sheetList )
    {
        if( sheet.GetExcludedFromSim() )
            continue;

        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Power symbols and other symbols which have the reference starting with "#" are
            // not included in simulation
            if( symbol->GetRef( &sheet ).StartsWith( '#' ) || symbol->GetExcludedFromSim() )
                continue;

            // Reset for each symbol
            msg.Clear();

            SIM_LIBRARY::MODEL model = libMgr.CreateModel( &sheet, *symbol, reporter );

            if( !msg.IsEmpty() )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_SIMULATION_MODEL );

                //Remove \n and \r at e.o.l if any:
                msg.Trim();

                ercItem->SetErrorMessage( msg );
                ercItem->SetItems( symbol );

                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
            }
        }

        for( SCH_MARKER* marker : markers )
        {
            sheet.LastScreen()->Append( marker );
            err_count += 1;
        }
    }

    return err_count;
}


int ERC_TESTER::RunRuleAreaERC()
{
    int numErrors = 0;

    if( !m_settings.IsTestEnabled( ERCE_OVERLAPPING_RULE_AREAS ) )
        return 0;

    std::map<SCH_SCREEN*, std::vector<SCH_RULE_AREA*>> allScreenRuleAreas;

    for( SCH_SCREEN* screen = m_screens.GetFirst(); screen; screen = m_screens.GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_RULE_AREA_T ) )
        {
            allScreenRuleAreas[screen].push_back( static_cast<SCH_RULE_AREA*>( item ) );
        }
    }

    if( m_settings.IsTestEnabled( ERCE_OVERLAPPING_RULE_AREAS ) )
        numErrors += TestRuleAreaOverlappingRuleAreasERC( allScreenRuleAreas );

    return numErrors;
}


int ERC_TESTER::TestRuleAreaOverlappingRuleAreasERC(
        std::map<SCH_SCREEN*, std::vector<SCH_RULE_AREA*>>& allScreenRuleAreas )
{
    int numErrors = 0;

    for( auto screenRuleAreas : allScreenRuleAreas )
    {
        std::vector<SCH_RULE_AREA*>& ruleAreas = screenRuleAreas.second;

        for( std::size_t i = 0; i < ruleAreas.size(); ++i )
        {
            SHAPE_POLY_SET& polyFirst = ruleAreas[i]->GetPolyShape();

            for( std::size_t j = i + 1; j < ruleAreas.size(); ++j )
            {
                SHAPE_POLY_SET polySecond = ruleAreas[j]->GetPolyShape();

                if( polyFirst.Collide( &polySecond ) )
                {
                    numErrors++;

                    SCH_SCREEN*    screen = screenRuleAreas.first;
                    SCH_SHEET_PATH firstSheet = screen->GetClientSheetPaths()[0];

                    std::shared_ptr<ERC_ITEM> ercItem =
                            ERC_ITEM::Create( ERCE_OVERLAPPING_RULE_AREAS );
                    ercItem->SetItems( ruleAreas[i], ruleAreas[j] );
                    ercItem->SetSheetSpecificPath( firstSheet );
                    ercItem->SetItemsSheetPaths( firstSheet, firstSheet );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, ruleAreas[i]->GetPosition() );
                    screen->Append( marker );
                }
            }
        }
    }

    return numErrors;
}


void ERC_TESTER::RunTests( DS_PROXY_VIEW_ITEM* aDrawingSheet, SCH_EDIT_FRAME* aEditFrame,
                           KIFACE* aCvPcb, PROJECT* aProject, PROGRESS_REPORTER* aProgressReporter )
{
    m_sheetList.AnnotatePowerSymbols();

    // Test duplicate sheet names inside a given sheet.  While one can have multiple references
    // to the same file, each must have a unique name.
    if( m_settings.IsTestEnabled( ERCE_DUPLICATE_SHEET_NAME ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking sheet names..." ) );

        TestDuplicateSheetNames( true );
    }

    if( m_settings.IsTestEnabled( ERCE_BUS_ALIAS_CONFLICT ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking bus conflicts..." ) );

        TestConflictingBusAliases();
    }

    // The connection graph has a whole set of ERC checks it can run
    if( aProgressReporter )
        aProgressReporter->AdvancePhase( _( "Checking conflicts..." ) );

    // If we are using the new connectivity, make sure that we do a full-rebuild
    if( aEditFrame )
    {
        if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
            aEditFrame->RecalculateConnections( nullptr, GLOBAL_CLEANUP );
        else
            aEditFrame->RecalculateConnections( nullptr, NO_CLEANUP );
    }

    m_schematic->ConnectionGraph()->RunERC();

    if( aProgressReporter )
        aProgressReporter->AdvancePhase( _( "Checking rule areas..." ) );

    if( m_settings.IsTestEnabled( ERCE_OVERLAPPING_RULE_AREAS ) )
    {
        RunRuleAreaERC();
    }

    if( aProgressReporter )
        aProgressReporter->AdvancePhase( _( "Checking units..." ) );

    // Test is all units of each multiunit symbol have the same footprint assigned.
    if( m_settings.IsTestEnabled( ERCE_DIFFERENT_UNIT_FP ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking footprints..." ) );

        TestMultiunitFootprints();
    }

    if( m_settings.IsTestEnabled( ERCE_MISSING_UNIT )
        || m_settings.IsTestEnabled( ERCE_MISSING_INPUT_PIN )
        || m_settings.IsTestEnabled( ERCE_MISSING_POWER_INPUT_PIN )
        || m_settings.IsTestEnabled( ERCE_MISSING_BIDI_PIN ) )
    {
        TestMissingUnits();
    }

    if( aProgressReporter )
        aProgressReporter->AdvancePhase( _( "Checking pins..." ) );

    if( m_settings.IsTestEnabled( ERCE_DIFFERENT_UNIT_NET ) )
        TestMultUnitPinConflicts();

    // Test pins on each net against the pin connection table
    if( m_settings.IsTestEnabled( ERCE_PIN_TO_PIN_ERROR )
        || m_settings.IsTestEnabled( ERCE_POWERPIN_NOT_DRIVEN )
        || m_settings.IsTestEnabled( ERCE_PIN_NOT_DRIVEN ) )
    {
        TestPinToPin();
    }

    // Test similar labels (i;e. labels which are identical when
    // using case insensitive comparisons)
    if( m_settings.IsTestEnabled( ERCE_SIMILAR_LABELS ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking labels..." ) );

        TestSimilarLabels();
    }

    if( m_settings.IsTestEnabled( ERCE_UNRESOLVED_VARIABLE ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for unresolved variables..." ) );

        TestTextVars( aDrawingSheet );
    }

    if( m_settings.IsTestEnabled( ERCE_SIMULATION_MODEL ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking SPICE models..." ) );

        TestSimModelIssues();
    }

    if( m_settings.IsTestEnabled( ERCE_NOCONNECT_CONNECTED ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking no connect pins for connections..." ) );

        TestNoConnectPins();
    }

    if( m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_ISSUES )
        || m_settings.IsTestEnabled( ERCE_LIB_SYMBOL_MISMATCH ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for library symbol issues..." ) );

        TestLibSymbolIssues();
    }

    if( m_settings.IsTestEnabled( ERCE_FOOTPRINT_LINK_ISSUES ) && aCvPcb )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for footprint link issues..." ) );

        TestFootprintLinkIssues( aCvPcb, aProject );
    }

    if( m_settings.IsTestEnabled( ERCE_ENDPOINT_OFF_GRID ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for off grid pins and wires..." ) );

        TestOffGridEndpoints();
    }

    if( m_settings.IsTestEnabled( ERCE_FOUR_WAY_JUNCTION ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for four way junctions..." ) );

        TestFourWayJunction();
    }

    if( m_settings.IsTestEnabled( ERCE_UNDEFINED_NETCLASS ) )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Checking for undefined netclasses..." ) );

        TestMissingNetclasses();
    }

    m_schematic->ResolveERCExclusionsPostUpdate();
}
