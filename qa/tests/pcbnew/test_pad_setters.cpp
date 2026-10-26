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

#include <memory>

BOOST_AUTO_TEST_SUITE( PadSetters )

// Regression test for GitLab #24333: editing a circular pad's diameter via the
// Properties panel calls PAD::SetSizeX with a single dimension. SetSizeX must
// keep the orthogonal dimension in lock-step for CIRCLE pads, otherwise DRC
// (e.g. annular-width) reads a stale Y and reports incorrect measurements.
BOOST_AUTO_TEST_CASE( SetSizeXOnCirclePadUpdatesY )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( &board );
    std::unique_ptr<PAD>       pad = std::make_unique<PAD>( fp.get() );

    pad->SetPadstackMode( PADSTACK::MODE::NORMAL );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );

    pad->SetSizeXProp( pcbIUScale.mmToIU( 1.5 ) );

    BOOST_CHECK_EQUAL( pad->GetSizeXProp(), pcbIUScale.mmToIU( 1.5 ) );
    BOOST_CHECK_EQUAL( pad->GetSizeYProp(), pcbIUScale.mmToIU( 1.5 ) );
}


// Mirror of the above. SetSizeY is hidden from the panel for CIRCLE pads via
// SetAvailableFunc, but the public setter still needs to honour the invariant.
BOOST_AUTO_TEST_CASE( SetSizeYOnCirclePadUpdatesX )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( &board );
    std::unique_ptr<PAD>       pad = std::make_unique<PAD>( fp.get() );

    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );

    pad->SetSizeYProp( pcbIUScale.mmToIU( 1.5 ) );

    BOOST_CHECK_EQUAL( pad->GetSizeXProp(), pcbIUScale.mmToIU( 1.5 ) );
    BOOST_CHECK_EQUAL( pad->GetSizeYProp(), pcbIUScale.mmToIU( 1.5 ) );
}


// Guard against the fix over-applying: non-circular shapes have independently
// meaningful X and Y, so SetSizeX must not touch Y.
BOOST_AUTO_TEST_CASE( SetSizeXOnRectPadLeavesYAlone )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( &board );
    std::unique_ptr<PAD>       pad = std::make_unique<PAD>( fp.get() );

    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 0.5 ) ) );

    pad->SetSizeXProp( pcbIUScale.mmToIU( 1.5 ) );

    BOOST_CHECK_EQUAL( pad->GetSizeXProp(), pcbIUScale.mmToIU( 1.5 ) );
    BOOST_CHECK_EQUAL( pad->GetSizeYProp(), pcbIUScale.mmToIU( 0.5 ) );
}


// PAD::SetAttribute() must drop all drilling when the new type has no hole.
// This is used (for example) when parsing an SMD pad as the pad starts as PTH
// and then changes to SMD, which has no hole.
BOOST_AUTO_TEST_CASE( SetAttributeClearsAllDrilling )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( &board );
    std::unique_ptr<PAD>       pad = std::make_unique<PAD>( fp.get() );

    // Start with a plated through-hole pad with a drill and post-machining
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );

    // Set up "some" drilling and post-machining features
    pad->Padstack().SetBackdrillSize( true, pcbIUScale.mmToIU( 1.2 ) );
    pad->Padstack().SetBackdrillEndLayer( true, In1_Cu );
    pad->Padstack().FrontPostMachining().mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE;
    pad->Padstack().FrontPostMachining().size = pcbIUScale.mmToIU( 1.5 );

    // Switch to SMD, which has no hole
    pad->SetAttribute( PAD_ATTRIB::SMD );

    // All drilling and post-machining features should be cleared
    BOOST_CHECK_EQUAL( pad->GetDrillSize().x, 0 );
    BOOST_CHECK_EQUAL( pad->GetDrillSize().y, 0 );
    BOOST_CHECK_EQUAL( pad->Padstack().SecondaryDrill().size.x, 0 );
    BOOST_CHECK( !pad->Padstack().FrontPostMachining().mode.has_value() );
    BOOST_CHECK_EQUAL( pad->Padstack().FrontPostMachining().size, 0 );
}

BOOST_AUTO_TEST_SUITE_END()
