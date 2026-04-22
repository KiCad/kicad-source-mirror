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

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <padstack.h>

#include <memory>

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23225
// When two pads are swapped in the footprint editor, the user expects the visible copper
// shapes to exchange positions.  When a pad has a shape offset relative to its hole, the
// previous implementation swapped hole/anchor positions instead, producing incorrect results.

BOOST_AUTO_TEST_SUITE( PadSwapPositions )

static std::unique_ptr<PAD> makeRoundPad( FOOTPRINT* aParent, const VECTOR2I& aPos,
                                          const VECTOR2I& aOffset,
                                          const EDA_ANGLE& aOrient = ANGLE_0 )
{
    auto pad = std::make_unique<PAD>( aParent );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, { pcbIUScale.mmToIU( 2.0 ), pcbIUScale.mmToIU( 2.0 ) } );
    pad->SetDrillSize( { pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.8 ) } );
    pad->SetOrientation( aOrient );
    pad->SetPosition( aPos );
    pad->SetOffset( PADSTACK::ALL_LAYERS, aOffset );
    return pad;
}


BOOST_AUTO_TEST_CASE( SwapPadsWithoutOffset )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );
    auto footprint = std::make_unique<FOOTPRINT>( &board );

    const VECTOR2I posA{ pcbIUScale.mmToIU( -1.5 ), 0 };
    const VECTOR2I posB{ pcbIUScale.mmToIU( 0.0 ),  0 };

    auto padA = makeRoundPad( footprint.get(), posA, { 0, 0 } );
    auto padB = makeRoundPad( footprint.get(), posB, { 0, 0 } );

    VECTOR2I originalShapeA = padA->ShapePos( PADSTACK::ALL_LAYERS );
    VECTOR2I originalShapeB = padB->ShapePos( PADSTACK::ALL_LAYERS );

    PAD::SwapShapePositions( padA.get(), padB.get() );

    BOOST_CHECK_EQUAL( padA->ShapePos( PADSTACK::ALL_LAYERS ), originalShapeB );
    BOOST_CHECK_EQUAL( padB->ShapePos( PADSTACK::ALL_LAYERS ), originalShapeA );
}


BOOST_AUTO_TEST_CASE( SwapPadsWhenOneHasShapeOffset )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );
    auto footprint = std::make_unique<FOOTPRINT>( &board );

    // Pad A holds a shape offset from its drill/anchor; pad B does not.
    const VECTOR2I posA{ pcbIUScale.mmToIU( -1.5 ), pcbIUScale.mmToIU( 0.6 ) };
    const VECTOR2I offsetA{ pcbIUScale.mmToIU( 0.6 ), 0 };
    const VECTOR2I posB{ pcbIUScale.mmToIU( 0.0 ), 0 };

    auto padA = makeRoundPad( footprint.get(), posA, offsetA, EDA_ANGLE( 90, DEGREES_T ) );
    auto padB = makeRoundPad( footprint.get(), posB, { 0, 0 } );

    VECTOR2I originalShapeA = padA->ShapePos( PADSTACK::ALL_LAYERS );
    VECTOR2I originalShapeB = padB->ShapePos( PADSTACK::ALL_LAYERS );

    PAD::SwapShapePositions( padA.get(), padB.get() );

    // Visible shape centers must be exchanged; each pad keeps its own offset/orientation.
    BOOST_CHECK_EQUAL( padA->ShapePos( PADSTACK::ALL_LAYERS ), originalShapeB );
    BOOST_CHECK_EQUAL( padB->ShapePos( PADSTACK::ALL_LAYERS ), originalShapeA );
    BOOST_CHECK_EQUAL( padA->GetOffset( PADSTACK::ALL_LAYERS ), offsetA );
    BOOST_CHECK_EQUAL( padB->GetOffset( PADSTACK::ALL_LAYERS ), VECTOR2I( 0, 0 ) );
}


BOOST_AUTO_TEST_CASE( SwapPadsWithDifferentOffsets )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );
    auto footprint = std::make_unique<FOOTPRINT>( &board );

    const VECTOR2I posA{ pcbIUScale.mmToIU( -2.0 ), pcbIUScale.mmToIU( 0.5 ) };
    const VECTOR2I offsetA{ pcbIUScale.mmToIU( 0.3 ), pcbIUScale.mmToIU( 0.1 ) };
    const VECTOR2I posB{ pcbIUScale.mmToIU( 2.0 ), pcbIUScale.mmToIU( -0.5 ) };
    const VECTOR2I offsetB{ pcbIUScale.mmToIU( -0.2 ), pcbIUScale.mmToIU( 0.4 ) };

    auto padA = makeRoundPad( footprint.get(), posA, offsetA, EDA_ANGLE( 45, DEGREES_T ) );
    auto padB = makeRoundPad( footprint.get(), posB, offsetB, EDA_ANGLE( -30, DEGREES_T ) );

    VECTOR2I originalShapeA = padA->ShapePos( PADSTACK::ALL_LAYERS );
    VECTOR2I originalShapeB = padB->ShapePos( PADSTACK::ALL_LAYERS );

    PAD::SwapShapePositions( padA.get(), padB.get() );

    BOOST_CHECK_EQUAL( padA->ShapePos( PADSTACK::ALL_LAYERS ), originalShapeB );
    BOOST_CHECK_EQUAL( padB->ShapePos( PADSTACK::ALL_LAYERS ), originalShapeA );
    // Offsets and orientations must remain with each pad.
    BOOST_CHECK_EQUAL( padA->GetOffset( PADSTACK::ALL_LAYERS ), offsetA );
    BOOST_CHECK_EQUAL( padB->GetOffset( PADSTACK::ALL_LAYERS ), offsetB );
}


BOOST_AUTO_TEST_CASE( SwapIsInvolutiveForTwoPads )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );
    auto footprint = std::make_unique<FOOTPRINT>( &board );

    const VECTOR2I posA{ pcbIUScale.mmToIU( -1.5 ), pcbIUScale.mmToIU( 0.6 ) };
    const VECTOR2I offsetA{ pcbIUScale.mmToIU( 0.6 ), 0 };
    const VECTOR2I posB{ pcbIUScale.mmToIU( 0.0 ), 0 };

    auto padA = makeRoundPad( footprint.get(), posA, offsetA, EDA_ANGLE( 90, DEGREES_T ) );
    auto padB = makeRoundPad( footprint.get(), posB, { 0, 0 } );

    VECTOR2I originalPosA = padA->GetPosition();
    VECTOR2I originalPosB = padB->GetPosition();

    PAD::SwapShapePositions( padA.get(), padB.get() );
    PAD::SwapShapePositions( padA.get(), padB.get() );

    BOOST_CHECK_EQUAL( padA->GetPosition(), originalPosA );
    BOOST_CHECK_EQUAL( padB->GetPosition(), originalPosB );
}

BOOST_AUTO_TEST_SUITE_END()
