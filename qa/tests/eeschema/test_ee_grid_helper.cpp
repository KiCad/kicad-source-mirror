/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <tools/ee_grid_helper.h>
#include <lib_symbol.h>
#include <sch_text.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <sch_junction.h>
#include <sch_symbol.h>
#include <layer_ids.h>

class EEGridHelperTestFixture
{
public:
    static BOX2I LayoutBounds( const SCH_ITEM& aItem ) { return EE_GRID_HELPER::layoutBounds( aItem ); }

    static std::vector<std::pair<const SCH_PIN*, VECTOR2I>> LayoutPins( SCH_ITEM& aItem )
    {
        return EE_GRID_HELPER::layoutPins( aItem );
    }
};

BOOST_AUTO_TEST_SUITE( EEGridHelperTest )

BOOST_AUTO_TEST_CASE( ItemGridClassification )
{
    EE_GRID_HELPER helper;

    SCH_TEXT text;
    BOOST_CHECK_EQUAL( helper.GetItemGrid( &text ), GRID_TEXT );

    SCH_LINE wire( VECTOR2I( 0, 0 ), LAYER_WIRE );
    BOOST_CHECK_EQUAL( helper.GetItemGrid( &wire ), GRID_WIRES );

    SCH_LINE graphic( VECTOR2I( 0, 0 ), LAYER_NOTES );
    BOOST_CHECK_EQUAL( helper.GetItemGrid( &graphic ), GRID_GRAPHICS );

    SCH_JUNCTION junc;
    BOOST_CHECK_EQUAL( helper.GetItemGrid( &junc ), GRID_WIRES );
}


namespace
{
std::unique_ptr<LIB_SYMBOL> MakeTwoPinSymbol()
{
    auto libSymbol = std::make_unique<LIB_SYMBOL>( "R", nullptr );

    for( const auto& [number, position] : { std::pair( "1", VECTOR2I( -508000, 0 ) ),
                                            std::pair( "2", VECTOR2I( 508000, 0 ) ) } )
    {
        SCH_PIN* pin = new SCH_PIN( libSymbol.get() );
        pin->SetNumber( number );
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        pin->SetPosition( position );
        libSymbol->AddDrawItem( pin );
    }

    return libSymbol;
}
} // namespace


BOOST_AUTO_TEST_CASE( SymbolLayoutBoundsIgnoreFields )
{
    std::unique_ptr<LIB_SYMBOL> libSymbol = MakeTwoPinSymbol();
    SCH_SYMBOL symbol( *libSymbol, libSymbol->GetLibId(), nullptr, 0, 0, VECTOR2I( 0, 0 ) );

    symbol.GetField( FIELD_T::REFERENCE )->SetPosition( VECTOR2I( 0, 50000000 ) );

    BOOST_CHECK( EEGridHelperTestFixture::LayoutBounds( symbol ) == symbol.GetBodyAndPinsBoundingBox() );
    BOOST_CHECK( EEGridHelperTestFixture::LayoutBounds( symbol ) != symbol.GetBoundingBox() );
}


BOOST_AUTO_TEST_CASE( SymbolLayoutPinsReportEveryPin )
{
    std::unique_ptr<LIB_SYMBOL> libSymbol = MakeTwoPinSymbol();
    SCH_SYMBOL symbol( *libSymbol, libSymbol->GetLibId(), nullptr, 0, 0, VECTOR2I( 2540000, 1270000 ) );
    symbol.UpdatePins();

    std::vector<std::pair<const SCH_PIN*, VECTOR2I>> pins = EEGridHelperTestFixture::LayoutPins( symbol );
    BOOST_REQUIRE_EQUAL( pins.size(), 2 );

    for( const auto& [pin, position] : pins )
        BOOST_CHECK_EQUAL( position, pin->GetPosition() );

    // A bare pin is its own alignment anchor in the symbol editor.
    SCH_PIN standalone( libSymbol.get() );
    standalone.SetPosition( VECTOR2I( 1000, 2000 ) );
    BOOST_REQUIRE_EQUAL( EEGridHelperTestFixture::LayoutPins( standalone ).size(), 1 );
    BOOST_CHECK_EQUAL( EEGridHelperTestFixture::LayoutPins( standalone ).front().second, VECTOR2I( 1000, 2000 ) );
}

BOOST_AUTO_TEST_SUITE_END()
