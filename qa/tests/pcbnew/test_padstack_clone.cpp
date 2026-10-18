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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>


BOOST_AUTO_TEST_SUITE( PadstackClone )


// A front/inner/back pad with no inner entry reads its inner layer through the base entry
// fails on revert, since cloning adds a blank inner entry that hides the inherited size
BOOST_AUTO_TEST_CASE( InnerLayerKeepsInheritedSize )
{
    BOARD     board;
    FOOTPRINT footprint( &board );
    PAD       pad( &footprint );

    pad.SetSize( F_Cu, VECTOR2I( 1500000, 1200000 ) );
    pad.SetShape( F_Cu, PAD_SHAPE::OVAL );
    pad.Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK );

    BOOST_REQUIRE_EQUAL( pad.GetSize( In1_Cu ).x, 1500000 );

    std::unique_ptr<PAD> clone( static_cast<PAD*>( pad.Clone() ) );

    BOOST_CHECK_EQUAL( clone->GetSize( In1_Cu ).x, 1500000 );
    BOOST_CHECK_EQUAL( clone->GetSize( In1_Cu ).y, 1200000 );
    BOOST_CHECK( clone->GetShape( In1_Cu ) == PAD_SHAPE::OVAL );
}


// A layer that starts from the base entry owns its primitives, or flipping one layer would flip the other too
BOOST_AUTO_TEST_CASE( NewLayerOwnsItsPrimitives )
{
    BOARD     board;
    FOOTPRINT footprint( &board );
    PAD       pad( &footprint );

    auto* shape = new PCB_SHAPE( &footprint, SHAPE_T::SEGMENT );
    pad.AddPrimitive( F_Cu, shape );
    pad.Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK );

    // writing the back layer seeds it from the base entry
    pad.SetSize( B_Cu, VECTOR2I( 1000000, 1000000 ) );

    BOOST_REQUIRE_EQUAL( pad.GetPrimitives( B_Cu ).size(), 1 );
    BOOST_CHECK( pad.GetPrimitives( B_Cu ).front().get() != pad.GetPrimitives( F_Cu ).front().get() );
}


// Editing a layer a custom padstack does not define starts from what the layer read through to
// fails on revert, since the new entry starts blank and the layer loses its size
BOOST_AUTO_TEST_CASE( CustomLayerEditKeepsInheritedSize )
{
    BOARD     board;
    FOOTPRINT footprint( &board );
    PAD       pad( &footprint );

    pad.SetSize( F_Cu, VECTOR2I( 1500000, 1200000 ) );
    pad.Padstack().SetMode( PADSTACK::MODE::CUSTOM );
    pad.Padstack().FlipLayers( nullptr );

    BOOST_REQUIRE( !pad.Padstack().HasExplicitDefinitionForLayer( F_Cu ) );

    pad.SetShape( F_Cu, PAD_SHAPE::OVAL );

    BOOST_CHECK_EQUAL( pad.GetSize( F_Cu ).x, 1500000 );
    BOOST_CHECK_EQUAL( pad.GetSize( F_Cu ).y, 1200000 );
}


// A half turned pad is the same pad only when it is symmetric about its centre
// fails on revert, since the library comparison then treats a turned pad of another padstack mode as unchanged
BOOST_AUTO_TEST_CASE( HalfTurnedPadComparison )
{
    BOARD board;

    auto makeFootprint = [&]( const VECTOR2I& aOffset, double aAngle )
    {
        FOOTPRINT* footprint = new FOOTPRINT( &board );
        PAD*       pad = new PAD( footprint );

        pad->SetNumber( wxS( "1" ) );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( 2000000, 1000000 ) );
        pad->SetOffset( PADSTACK::ALL_LAYERS, aOffset );
        pad->SetOrientation( EDA_ANGLE( aAngle, DEGREES_T ) );
        footprint->Add( pad );
        board.Add( footprint );
        return footprint;
    };

    FOOTPRINT* centred = makeFootprint( VECTOR2I( 0, 0 ), 0 );
    FOOTPRINT* centredTurned = makeFootprint( VECTOR2I( 0, 0 ), 180 );
    FOOTPRINT* shifted = makeFootprint( VECTOR2I( 500000, 0 ), 0 );
    FOOTPRINT* shiftedTurned = makeFootprint( VECTOR2I( 500000, 0 ), 180 );

    BOOST_CHECK( !centredTurned->FootprintNeedsUpdate( centred, BOARD_ITEM::COMPARE_FLAGS::DRC ) );
    BOOST_CHECK( shiftedTurned->FootprintNeedsUpdate( shifted, BOARD_ITEM::COMPARE_FLAGS::DRC ) );

    // a different padstack mode is a different pad, whatever the turn
    centredTurned->Pads().front()->Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK );

    BOOST_CHECK( centredTurned->FootprintNeedsUpdate( centred, BOARD_ITEM::COMPARE_FLAGS::DRC ) );
}


BOOST_AUTO_TEST_SUITE_END()
