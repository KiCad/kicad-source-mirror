/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>     // for ExpandEnvVarSubstitutions
#include <erc.h>
#include <erc_sch_pin_context.h>
#include <string_utils.h>
#include <lib_pin.h>
#include <sch_edit_frame.h>
#include <sch_marker.h>
#include <sch_reference_list.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_textbox.h>
#include <sch_line.h>
#include <schematic.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <wx/ffile.h>
#include <sim/sim_lib_mgr.h>


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

int ERC_TESTER::TestDuplicateSheetNames( bool aCreateMarker )
{
    SCH_SCREEN* screen;
    int         err_count = 0;

    SCH_SCREENS screenList( m_schematic->Root() );

    for( screen = screenList.GetFirst(); screen != nullptr; screen = screenList.GetNext() )
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
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_DUPLICATE_SHEET_NAME );
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
        return str.Matches( wxT( "*${*}*" ) );
    };

    if( aDrawingSheet )
    {
        wsItems.SetPageNumber( wxS( "1" ) );
        wsItems.SetSheetCount( 1 );
        wsItems.SetFileName( wxS( "dummyFilename" ) );
        wsItems.SetSheetName( wxS( "dummySheet" ) );
        wsItems.SetSheetLayer( wxS( "dummyLayer" ) );
        wsItems.SetProject( &m_schematic->Prj() );
        wsItems.BuildDrawItemsList( aDrawingSheet->GetPageInfo(), aDrawingSheet->GetTitleBlock());
    }

    SCH_SHEET_PATH  savedCurrentSheet = m_schematic->CurrentSheet();
    SCH_SHEET_LIST  sheets = m_schematic->GetSheets();

    for( SCH_SHEET_PATH& sheet : sheets )
    {
        m_schematic->SetCurrentSheet( sheet );
        SCH_SCREEN* screen = sheet.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LOCATE_ANY_T ) )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                for( SCH_FIELD& field : symbol->GetFields() )
                {
                    if( unresolved( field.GetShownText( true ) ) )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem =
                                ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( &field );

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
                    if( unresolved( field.GetShownText( true ) ) )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem =
                                ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( &field );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, field.GetPosition() );
                        screen->Append( marker );
                    }
                }

                for( SCH_SHEET_PIN* pin : static_cast<SCH_SHEET*>( item )->GetPins() )
                {
                    if( pin->GetShownText( true ).Matches( wxT( "*${*}*" ) ) )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem =
                                ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                        ercItem->SetItems( pin );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, pin->GetPosition() );
                        screen->Append( marker );
                    }
                }
            }
            else if( SCH_TEXT* text = dynamic_cast<SCH_TEXT*>( item ) )
            {
                if( text->GetShownText( true ).Matches( wxT( "*${*}*" ) ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem =
                            ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    ercItem->SetItems( text );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, text->GetPosition() );
                    screen->Append( marker );
                }
            }
            else if( SCH_TEXTBOX* textBox = dynamic_cast<SCH_TEXTBOX*>( item ) )
            {
                if( textBox->GetShownText( true ).Matches( wxT( "*${*}*" ) ) )
                {
                    std::shared_ptr<ERC_ITEM> ercItem =
                            ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    ercItem->SetItems( textBox );

                    SCH_MARKER* marker = new SCH_MARKER( ercItem, textBox->GetPosition() );
                    screen->Append( marker );
                }
            }
        }

        for( DS_DRAW_ITEM_BASE* item = wsItems.GetFirst(); item; item = wsItems.GetNext() )
        {
            if( DS_DRAW_ITEM_TEXT* text = dynamic_cast<DS_DRAW_ITEM_TEXT*>( item ) )
            {
                if( text->GetShownText( true ).Matches( wxT( "*${*}*" ) ) )
                {
                    std::shared_ptr<ERC_ITEM> erc = ERC_ITEM::Create( ERCE_UNRESOLVED_VARIABLE );
                    erc->SetErrorMessage( _( "Unresolved text variable in drawing sheet" ) );

                    SCH_MARKER* marker = new SCH_MARKER( erc, text->GetPosition() );
                    screen->Append( marker );
                }
            }
        }
    }

    m_schematic->SetCurrentSheet( savedCurrentSheet );
}


int ERC_TESTER::TestConflictingBusAliases()
{
    wxString    msg;
    int         err_count = 0;

    SCH_SCREENS screens( m_schematic->Root() );
    std::vector< std::shared_ptr<BUS_ALIAS> > aliases;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != nullptr; screen = screens.GetNext() )
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
    SCH_SHEET_LIST sheets = m_schematic->GetSheets();

    int errors = 0;
    SCH_MULTI_UNIT_REFERENCE_MAP refMap;
    sheets.GetMultiUnitSymbols( refMap, true );

    for( std::pair<const wxString, SCH_REFERENCE_LIST>& symbol : refMap )
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
    ERC_SETTINGS&  settings = m_schematic->ErcSettings();
    SCH_SHEET_LIST sheets = m_schematic->GetSheets();

    int errors = 0;
    SCH_MULTI_UNIT_REFERENCE_MAP refMap;
    sheets.GetMultiUnitSymbols( refMap, true );

    for( std::pair<const wxString, SCH_REFERENCE_LIST>& symbol : refMap )
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

        auto report_missing = [&]( std::set<int>& aMissingUnits, wxString aErrorMsg, int aErrorCode )
        {
            wxString msg;
            wxString missing_pin_units = wxT( "[ " );
            int ii = 0;

            for( int missing_unit : aMissingUnits )
            {
                if( ii++ == 3 )
                {
                    missing_pin_units += wxT( "....." );
                    break;
                }

                missing_pin_units += libSymbol->GetUnitDisplayName( missing_unit ) + ", " ;
            }

            missing_pin_units.Truncate( missing_pin_units.length() - 2 );
            missing_pin_units += wxT( " ]" );

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

        if( !missing_units.empty() && settings.IsTestEnabled( ERCE_MISSING_UNIT ) )
        {
            report_missing( missing_units, _( "Symbol %s has unplaced units %s" ),
                    ERCE_MISSING_UNIT );
        }

        std::set<int> missing_power;
        std::set<int> missing_input;
        std::set<int> missing_bidi;

        for( int missing_unit : missing_units )
        {
            LIB_PINS pins;
            int convert = 0;

            for( size_t ii = 0; ii < refList.GetCount(); ++ii )
            {
                if( refList.GetItem( ii ).GetUnit() == missing_unit )
                {
                    convert = refList.GetItem( ii ).GetSymbol()->GetConvert();
                    break;
                }
            }

            libSymbol->GetPins( pins, missing_unit, convert );

            for( auto pin : pins )
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

        if( !missing_power.empty() && settings.IsTestEnabled( ERCE_MISSING_POWER_INPUT_PIN ) )
        {
            report_missing( missing_power, _( "Symbol %s has input power pins in units %s that are not placed." ),
                     ERCE_MISSING_POWER_INPUT_PIN );
        }

        if( !missing_input.empty() && settings.IsTestEnabled( ERCE_MISSING_INPUT_PIN ) )
        {
           report_missing( missing_input, _( "Symbol %s has input pins in units %s that are not placed." ),
                   ERCE_MISSING_INPUT_PIN );
        }

        if( !missing_bidi.empty() && settings.IsTestEnabled( ERCE_MISSING_BIDI_PIN ) )
        {
            report_missing( missing_bidi, _( "Symbol %s has bidirectional pins in units %s that are not placed." ),
                    ERCE_MISSING_BIDI_PIN );
        }
    }

    return errors;
}


int ERC_TESTER::TestNoConnectPins()
{
    int err_count = 0;

    for( const SCH_SHEET_PATH& sheet : m_schematic->GetSheets() )
    {
        std::map<VECTOR2I, std::vector<SCH_PIN*>> pinMap;

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                if( pin->GetLibPin()->GetType() == ELECTRICAL_PINTYPE::PT_NC )
                    pinMap[pin->GetPosition()].emplace_back( pin );
            }
        }

        for( const std::pair<const VECTOR2I, std::vector<SCH_PIN*>>& pair : pinMap )
        {
            if( pair.second.size() > 1 )
            {
                err_count++;

                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_NOCONNECT_CONNECTED );

                ercItem->SetItems( pair.second[0], pair.second[1],
                                   pair.second.size() > 2 ? pair.second[2] : nullptr,
                                   pair.second.size() > 3 ? pair.second[3] : nullptr );
                ercItem->SetErrorMessage( _( "Pins with 'no connection' type are connected" ) );

                SCH_MARKER* marker = new SCH_MARKER( ercItem, pair.first );
                sheet.LastScreen()->Append( marker );
            }
        }
    }

    return err_count;
}


int ERC_TESTER::TestPinToPin()
{
    ERC_SETTINGS&  settings = m_schematic->ErcSettings();
    const NET_MAP& nets     = m_schematic->ConnectionGraph()->GetNetMap();

    int errors = 0;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : nets )
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

        std::set<std::pair<ERC_SCH_PIN_CONTEXT, ERC_SCH_PIN_CONTEXT>> tested;

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

        for( ERC_SCH_PIN_CONTEXT& refPin : pins )
        {
            ELECTRICAL_PINTYPE refType = refPin.Pin()->GetType();

            if( DrivenPinTypes.count( refType ) )
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

            for( ERC_SCH_PIN_CONTEXT& testPin : pins )
            {
                if( testPin == refPin )
                    continue;

                ERC_SCH_PIN_CONTEXT first_pin = refPin;
                ERC_SCH_PIN_CONTEXT second_pin = testPin;

                if( second_pin < first_pin )
                    std::swap( first_pin, second_pin );

                std::pair<ERC_SCH_PIN_CONTEXT, ERC_SCH_PIN_CONTEXT> pair =
                        std::make_pair( first_pin, second_pin );

                if( auto [ins_pin, inserted ] = tested.insert( pair ); !inserted )
                    continue;

                // Multiple pins in the same symbol that share a type,
                // name and position are considered
                // "stacked" and shouldn't trigger ERC errors
                if( refPin.Pin()->IsStacked( testPin.Pin() ) && refPin.Sheet() == testPin.Sheet() )
                    continue;

                ELECTRICAL_PINTYPE testType = testPin.Pin()->GetType();

                if( ispowerNet )
                    hasDriver |= ( DrivingPowerPinTypes.count( testType ) != 0 );
                else
                    hasDriver |= ( DrivingPinTypes.count( testType ) != 0 );

                PIN_ERROR erc = settings.GetPinMapValue( refType, testType );

                if( erc != PIN_ERROR::OK && settings.IsTestEnabled( ERCE_PIN_TO_PIN_WARNING ) )
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

                    SCH_MARKER* marker =
                            new SCH_MARKER( ercItem, refPin.Pin()->GetTransformedPosition() );
                    pinToScreenMap[refPin.Pin()]->Append( marker );
                    errors++;
                }
            }
        }

        if( needsDriver.Pin() && !hasDriver && !has_noconnect )
        {
            int err_code = ispowerNet ? ERCE_POWERPIN_NOT_DRIVEN : ERCE_PIN_NOT_DRIVEN;

            if( settings.IsTestEnabled( err_code ) )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( err_code );

                ercItem->SetItems( needsDriver.Pin() );
                ercItem->SetSheetSpecificPath( needsDriver.Sheet() );
                ercItem->SetItemsSheetPaths( needsDriver.Sheet() );

                SCH_MARKER* marker =
                        new SCH_MARKER( ercItem, needsDriver.Pin()->GetTransformedPosition() );
                pinToScreenMap[needsDriver.Pin()]->Append( marker );
                errors++;
            }
        }
    }

    return errors;
}


int ERC_TESTER::TestMultUnitPinConflicts()
{
    const NET_MAP& nets = m_schematic->ConnectionGraph()->GetNetMap();

    int errors = 0;

    std::unordered_map<wxString, std::pair<wxString, SCH_PIN*>> pinToNetMap;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : nets )
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

                    if( !pin->GetLibPin()->GetParent()->IsMulti() )
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

                        SCH_MARKER* marker = new SCH_MARKER( ercItem,
                                                             pin->GetTransformedPosition() );
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
    const NET_MAP& nets = m_schematic->ConnectionGraph()->GetNetMap();

    int errors = 0;

    std::unordered_map<wxString, std::pair<SCH_LABEL_BASE*, SCH_SHEET_PATH>> labelMap;

    for( const std::pair<NET_NAME_CODE_CACHE_KEY, std::vector<CONNECTION_SUBGRAPH*>> net : nets )
    {
        for( CONNECTION_SUBGRAPH* subgraph : net.second )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                switch( item->Type() )
                {
                case SCH_LABEL_T:
                case SCH_HIER_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                {
                    SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

                    wxString normalized = label->GetShownText( false ).Lower();

                    if( !labelMap.count( normalized ) )
                    {
                        labelMap[normalized] = std::make_pair( label, subgraph->GetSheet() );
                    }
                    else if( labelMap.at( normalized ).first->GetShownText( false )
                             != label->GetShownText( false ) )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_SIMILAR_LABELS );
                        ercItem->SetItems( label, labelMap.at( normalized ).first );
                        ercItem->SetSheetSpecificPath( subgraph->GetSheet() );
                        ercItem->SetItemsSheetPaths( subgraph->GetSheet(),
                                                     labelMap.at( normalized ).second );

                        SCH_MARKER* marker = new SCH_MARKER( ercItem, label->GetPosition() );
                        subgraph->GetSheet().LastScreen()->Append( marker );
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

    SYMBOL_LIB_TABLE* libTable = m_schematic->Prj().SchSymbolLibTable();
    wxString          msg;
    int               err_count = 0;

    SCH_SCREENS screens( m_schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != nullptr; screen = screens.GetNext() )
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
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_ISSUES );
                ercItem->SetItems( symbol );
                msg.Printf( _( "The current configuration does not include the library '%s'" ),
                            UnescapeString( libName ) );
                ercItem->SetErrorMessage( msg );

                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                continue;
            }
            else if( !libTable->HasLibrary( libName, true ) )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_ISSUES );
                ercItem->SetItems( symbol );
                msg.Printf( _( "The library '%s' is not enabled in the current configuration" ),
                            UnescapeString( libName ) );
                ercItem->SetErrorMessage( msg );

                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                continue;
            }

            wxString    symbolName = symbol->GetLibId().GetLibItemName();
            LIB_SYMBOL* libSymbol = SchGetLibSymbol( symbol->GetLibId(), libTable );

            if( libSymbol == nullptr )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_ISSUES );
                ercItem->SetItems( symbol );
                msg.Printf( _( "Symbol '%s' not found in symbol library '%s'" ),
                            UnescapeString( symbolName ),
                            UnescapeString( libName ) );
                ercItem->SetErrorMessage( msg );

                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
                continue;
            }

            std::unique_ptr<LIB_SYMBOL> flattenedSymbol = libSymbol->Flatten();
            constexpr int flags = LIB_ITEM::COMPARE_FLAGS::EQUALITY | LIB_ITEM::COMPARE_FLAGS::ERC;

            if( flattenedSymbol->Compare( *libSymbolInSchematic, flags ) != 0 )
            {
                std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_LIB_SYMBOL_ISSUES );
                ercItem->SetItems( symbol );
                msg.Printf( _( "Symbol '%s' has been modified in library '%s'" ),
                            UnescapeString( symbolName ),
                            UnescapeString( libName ) );
                ercItem->SetErrorMessage( msg );

                markers.emplace_back( new SCH_MARKER( ercItem, symbol->GetPosition() ) );
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


int ERC_TESTER::TestOffGridEndpoints( int aGridSize )
{
    // The minimal grid size allowed to place a pin is 25 mils
    // the best grid size is 50 mils, but 25 mils is still usable
    // this is because all symbols are using a 50 mils grid to place pins, and therefore
    // the wires must be on the 50 mils grid
    // So raise an error if a pin is not on a 25 mil (or bigger: 50 mil or 100 mil) grid
    const int min_grid_size = schIUScale.MilsToIU( 25 );
    const int clamped_grid_size = ( aGridSize < min_grid_size ) ? min_grid_size : aGridSize;

    SCH_SCREENS screens( m_schematic->Root() );
    int         err_count = 0;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen != nullptr; screen = screens.GetNext() )
    {
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : screen->Items() )
        {
            if( item->Type() == SCH_LINE_T && item->IsConnectable() )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( item );

                if( ( line->GetStartPoint().x % clamped_grid_size ) != 0
                        || ( line->GetStartPoint().y % clamped_grid_size) != 0 )
                {
                    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
                    ercItem->SetItems( line );

                    markers.emplace_back( new SCH_MARKER( ercItem, line->GetStartPoint() ) );
                }
                else if( ( line->GetEndPoint().x % clamped_grid_size ) != 0
                            || ( line->GetEndPoint().y % clamped_grid_size) != 0 )
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
                    VECTOR2I pinPos = pin->GetTransformedPosition();

                    if( ( pinPos.x % clamped_grid_size ) != 0
                            || ( pinPos.y % clamped_grid_size) != 0 )
                    {
                        std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( ERCE_ENDPOINT_OFF_GRID );
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
    SCH_SHEET_LIST     sheets = m_schematic->GetSheets();
    int                err_count = 0;
    SIM_LIB_MGR        libMgr( &m_schematic->Prj(), &reporter );

    for( SCH_SHEET_PATH& sheet : sheets )
    {
        std::vector<SCH_MARKER*> markers;

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Power symbols and other symbols which have the reference starting with "#" are
            // not included in simulation
            if( symbol->GetRef( &sheet ).StartsWith( '#' ) || symbol->GetExcludeFromSim() )
                continue;

            // Reset for each symbol
            msg.Clear();

            SIM_LIBRARY::MODEL model = libMgr.CreateModel( &sheet, *symbol );

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
