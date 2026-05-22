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

#include <memory>

BOOST_AUTO_TEST_SUITE( PadSizeSetters )

// Regression test for GitLab #24333: editing a circular pad's diameter via the
// Properties panel calls PAD::SetSizeX with a single dimension. SetSizeX must
// keep the orthogonal dimension in lock-step for CIRCLE pads, otherwise DRC
// (e.g. annular-width) reads a stale Y and reports incorrect measurements.
BOOST_AUTO_TEST_CASE( SetSizeXOnCirclePadUpdatesY )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto fp = std::make_unique<FOOTPRINT>( &board );
    auto pad = std::make_unique<PAD>( fp.get() );

    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );

    pad->SetSizeX( pcbIUScale.mmToIU( 1.5 ) );

    BOOST_CHECK_EQUAL( pad->GetSizeX(), pcbIUScale.mmToIU( 1.5 ) );
    BOOST_CHECK_EQUAL( pad->GetSizeY(), pcbIUScale.mmToIU( 1.5 ) );
}


// Mirror of the above. SetSizeY is hidden from the panel for CIRCLE pads via
// SetAvailableFunc, but the public setter still needs to honour the invariant.
BOOST_AUTO_TEST_CASE( SetSizeYOnCirclePadUpdatesX )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto fp = std::make_unique<FOOTPRINT>( &board );
    auto pad = std::make_unique<PAD>( fp.get() );

    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( PAD::PTHMask() );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );

    pad->SetSizeY( pcbIUScale.mmToIU( 1.5 ) );

    BOOST_CHECK_EQUAL( pad->GetSizeX(), pcbIUScale.mmToIU( 1.5 ) );
    BOOST_CHECK_EQUAL( pad->GetSizeY(), pcbIUScale.mmToIU( 1.5 ) );
}


// Guard against the fix over-applying: non-circular shapes have independently
// meaningful X and Y, so SetSizeX must not touch Y.
BOOST_AUTO_TEST_CASE( SetSizeXOnRectPadLeavesYAlone )
{
    BOARD board;
    board.SetBoardUse( BOARD_USE::FPHOLDER );

    auto fp = std::make_unique<FOOTPRINT>( &board );
    auto pad = std::make_unique<PAD>( fp.get() );

    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetLayerSet( LSET( { F_Cu } ) );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 0.5 ) ) );

    pad->SetSizeX( pcbIUScale.mmToIU( 1.5 ) );

    BOOST_CHECK_EQUAL( pad->GetSizeX(), pcbIUScale.mmToIU( 1.5 ) );
    BOOST_CHECK_EQUAL( pad->GetSizeY(), pcbIUScale.mmToIU( 0.5 ) );
}

BOOST_AUTO_TEST_SUITE_END()
