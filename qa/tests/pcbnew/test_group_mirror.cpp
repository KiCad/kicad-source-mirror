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
#include <board_item.h>
#include <footprint.h>
#include <pcb_dimension.h>
#include <pcb_group.h>
#include <pcb_shape.h>

// Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/23731
//
// A footprint has no mirror (only flip). Mirroring a group that contains one used to call
// BOARD_ITEM::Mirror on the footprint, spawning a warning dialog per footprint and mirroring
// the other members while the footprint stayed put, tearing the group apart. A group is now
// left untouched unless every member can be mirrored.

BOOST_AUTO_TEST_SUITE( GroupMirror )


BOOST_AUTO_TEST_CASE( GroupWithFootprintIsLeftIntact )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_SHAPE* shape = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    shape->SetStart( VECTOR2I( 1000000, 2000000 ) );
    shape->SetEnd( VECTOR2I( 3000000, 4000000 ) );
    board->Add( shape );

    FOOTPRINT* footprint = new FOOTPRINT( board.get() );
    footprint->SetPosition( VECTOR2I( 5000000, 6000000 ) );
    board->Add( footprint );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->AddItem( shape );
    group->AddItem( footprint );
    board->Add( group );

    const VECTOR2I shapeStart = shape->GetStart();
    const VECTOR2I shapeEnd = shape->GetEnd();
    const VECTOR2I fpPos = footprint->GetPosition();

    group->Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    // Nothing moved: the group is untouchable because it holds a footprint.
    BOOST_CHECK_EQUAL( shape->GetStart(), shapeStart );
    BOOST_CHECK_EQUAL( shape->GetEnd(), shapeEnd );
    BOOST_CHECK_EQUAL( footprint->GetPosition(), fpPos );
}


BOOST_AUTO_TEST_CASE( GroupWithoutFootprintStillMirrors )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_SHAPE* shape = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    shape->SetStart( VECTOR2I( 1000000, 2000000 ) );
    shape->SetEnd( VECTOR2I( 3000000, 4000000 ) );
    board->Add( shape );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->AddItem( shape );
    board->Add( group );

    group->Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    // Mirror across x = 0 flips the x coordinate, y is unchanged.
    BOOST_CHECK_EQUAL( shape->GetStart(), VECTOR2I( -1000000, 2000000 ) );
    BOOST_CHECK_EQUAL( shape->GetEnd(), VECTOR2I( -3000000, 4000000 ) );
}


BOOST_AUTO_TEST_CASE( GroupWithDimensionStillMirrors )
{
    // A dimension has a Mirror(), so a group holding one must stay mirrorable.
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_SHAPE* shape = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    shape->SetStart( VECTOR2I( 1000000, 2000000 ) );
    shape->SetEnd( VECTOR2I( 3000000, 4000000 ) );
    board->Add( shape );

    PCB_DIM_ALIGNED* dim = new PCB_DIM_ALIGNED( board.get(), PCB_DIM_ALIGNED_T );
    dim->SetStart( VECTOR2I( 1000000, 0 ) );
    dim->SetEnd( VECTOR2I( 5000000, 0 ) );
    board->Add( dim );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->AddItem( shape );
    group->AddItem( dim );
    board->Add( group );

    group->Mirror( VECTOR2I( 0, 0 ), FLIP_DIRECTION::LEFT_RIGHT );

    // The group was mirrored, both members flipped across x = 0.
    BOOST_CHECK_EQUAL( shape->GetStart(), VECTOR2I( -1000000, 2000000 ) );
    BOOST_CHECK_EQUAL( dim->GetStart(), VECTOR2I( -1000000, 0 ) );
    BOOST_CHECK_EQUAL( dim->GetEnd(), VECTOR2I( -5000000, 0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
