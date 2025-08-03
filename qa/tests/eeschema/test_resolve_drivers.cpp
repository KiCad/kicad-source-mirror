/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <sch_label.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <lib_symbol.h>
#include <vector>

class RESOLVE_DRIVERS_FIXTURE
{
public:
    RESOLVE_DRIVERS_FIXTURE() :
            subgraph( &graph )
    {}

    ~RESOLVE_DRIVERS_FIXTURE()
    {
        for( SCH_ITEM* item : looseItems )
            delete item;

        for( SCH_SYMBOL* sym : symbols )
            delete sym;

        for( LIB_SYMBOL* lib : libs )
            delete lib;
    }

    SCH_LABEL* MakeLabel( const wxString& aText )
    {
        SCH_LABEL* label = new SCH_LABEL( VECTOR2I( 0, 0 ), aText );
        SCH_CONNECTION* conn = label->GetOrInitConnection( sheetPath, &graph );
        conn->ConfigureFromLabel( aText );
        looseItems.push_back( label );
        return label;
    }

    SCH_PIN* MakePin( LIB_SYMBOL* aLib, ELECTRICAL_PINTYPE aType, const wxString& aRef )
    {
        SCH_PIN* libPin = new SCH_PIN( aLib );
        libPin->SetNumber( "1" );
        libPin->SetName( "P" );
        libPin->SetType( aType );
        libPin->SetPosition( VECTOR2I( 0, 0 ) );
        aLib->AddDrawItem( libPin );

        SCH_SYMBOL* symbol = new SCH_SYMBOL( *aLib, aLib->GetLibId(), &sheetPath, 0, 0, VECTOR2I( 0, 0 ) );
        symbol->SetRef( &sheetPath, aRef );
        symbol->UpdatePins();
        symbols.push_back( symbol );

        SCH_PIN* pin = symbol->GetPins( &sheetPath )[0];
        SCH_CONNECTION* conn = pin->GetOrInitConnection( sheetPath, &graph );
        conn->ConfigureFromLabel( "NET" );
        return pin;
    }

    SCH_PIN* MakeGlobalPowerPin()
    {
        LIB_SYMBOL* lib = new LIB_SYMBOL( wxS( "G_PWR" ), nullptr );
        lib->SetGlobalPower();
        libs.push_back( lib );
        return MakePin( lib, ELECTRICAL_PINTYPE::PT_POWER_IN, wxS( "PWR1" ) );
    }

    SCH_PIN* MakeLocalPowerPin()
    {
        LIB_SYMBOL* lib = new LIB_SYMBOL( wxS( "L_PWR" ), nullptr );
        lib->SetLocalPower();
        libs.push_back( lib );
        return MakePin( lib, ELECTRICAL_PINTYPE::PT_POWER_IN, wxS( "PWR2" ) );
    }

    SCH_PIN* MakeRegularPin()
    {
        LIB_SYMBOL* lib = new LIB_SYMBOL( wxS( "REG" ), nullptr );
        lib->SetNormal();
        libs.push_back( lib );
        return MakePin( lib, ELECTRICAL_PINTYPE::PT_OUTPUT, wxS( "U1" ) );
    }

    SCH_SHEET_PIN* MakeSheetPin( const wxString& aText, LABEL_FLAG_SHAPE aShape )
    {
        SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( &sheet );
        pin->SetText( aText );
        pin->SetShape( aShape );
        SCH_SHEET_PATH sheetPath;
        SCH_CONNECTION* conn = pin->GetOrInitConnection( sheetPath, &graph );
        conn->ConfigureFromLabel( aText );
        looseItems.push_back( pin );
        return pin;
    }

    CONNECTION_GRAPH     graph;
    SCH_SHEET            sheet;
    SCH_SHEET_PATH       sheetPath;
    CONNECTION_SUBGRAPH  subgraph;

    std::vector<LIB_SYMBOL*> libs;
    std::vector<SCH_SYMBOL*> symbols;
    std::vector<SCH_ITEM*>   looseItems;
};

BOOST_FIXTURE_TEST_CASE( BusSupersetPreference, RESOLVE_DRIVERS_FIXTURE )
{
    SCH_LABEL* subset = MakeLabel( wxS( "BUS[1..3]" ) );
    SCH_LABEL* superset = MakeLabel( wxS( "BUS[1..4]" ) );

    subgraph.AddItem( subset );
    subgraph.AddItem( superset );

    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), superset );
    subgraph.RemoveItem( subset );

    // Check that the superset is still the driver after removing the subset
    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), superset );
    subgraph.RemoveItem( superset );

    // Check group labels as well
    subset = MakeLabel( wxS( "BUS{ ONE TWO THREE }" ) );
    superset = MakeLabel( wxS( "BUS{ ONE TWO THREE FOUR }" ) );

    subgraph.AddItem( subset );
    subgraph.AddItem( superset );

    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), superset );
}

BOOST_FIXTURE_TEST_CASE( PowerSymbolPinPrecedence, RESOLVE_DRIVERS_FIXTURE )
{
    SCH_PIN* regular = MakeRegularPin();
    subgraph.AddItem( regular );
    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), regular );

    SCH_PIN* local = MakeLocalPowerPin();
    subgraph.AddItem( local );
    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), local );

    SCH_PIN* global = MakeGlobalPowerPin();
    subgraph.AddItem( global );
    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), global );
}

BOOST_FIXTURE_TEST_CASE( NameQualityHeuristic, RESOLVE_DRIVERS_FIXTURE )
{
    SCH_LABEL* good = MakeLabel( wxS( "VCC" ) );
    SCH_LABEL* bad = MakeLabel( wxS( "Net-Pad1" ) );

    subgraph.AddItem( good );
    subgraph.AddItem( bad );

    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), good );
}

BOOST_FIXTURE_TEST_CASE( AlphabeticalTiebreaker, RESOLVE_DRIVERS_FIXTURE )
{
    SCH_LABEL* aaa = MakeLabel( wxS( "AAA" ) );
    SCH_LABEL* bbb = MakeLabel( wxS( "BBB" ) );

    subgraph.AddItem( aaa );
    subgraph.AddItem( bbb );

    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), aaa );
}

BOOST_FIXTURE_TEST_CASE( SheetPinOutputBias, RESOLVE_DRIVERS_FIXTURE )
{
    SCH_SHEET_PIN* input = MakeSheetPin( wxS( "IN" ), LABEL_FLAG_SHAPE::L_INPUT );
    SCH_SHEET_PIN* output = MakeSheetPin( wxS( "OUT" ), LABEL_FLAG_SHAPE::L_OUTPUT );

    subgraph.AddItem( input );
    subgraph.AddItem( output );

    subgraph.ResolveDrivers( false );
    BOOST_CHECK_EQUAL( subgraph.GetDriver(), output );
}

