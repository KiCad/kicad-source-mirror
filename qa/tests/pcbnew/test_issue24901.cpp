/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <pcb_field.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <tools/pcb_selection_tool.h>

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24901
//
// A footprint whose reference field carried (locked yes) in the file could not be moved even
// though every piece of footprint-level UI reported it as unlocked. Locked items inside a
// footprint move rigidly with it, so they must not pin the footprint. Locked group members
// still pin their group (issue 6841).

BOOST_AUTO_TEST_SUITE( Issue24901 )


BOOST_AUTO_TEST_CASE( LockedFieldDoesNotPinFootprint )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    FOOTPRINT* footprint = new FOOTPRINT( board.get() );
    board->Add( footprint );

    footprint->Reference().SetLocked( true );

    BOOST_CHECK( !footprint->IsLocked() );
    BOOST_CHECK( footprint->Reference().IsLocked() );
    BOOST_CHECK( !PCB_SELECTION_TOOL::HasLockedDescendant( footprint ) );
}


BOOST_AUTO_TEST_CASE( LockedGraphicDoesNotPinFootprint )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    FOOTPRINT* footprint = new FOOTPRINT( board.get() );
    board->Add( footprint );

    PCB_SHAPE* shape = new PCB_SHAPE( footprint, SHAPE_T::SEGMENT );
    shape->SetLocked( true );
    footprint->Add( shape );

    BOOST_CHECK( !PCB_SELECTION_TOOL::HasLockedDescendant( footprint ) );
}


BOOST_AUTO_TEST_CASE( LockedMemberPinsGroup )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    PCB_SHAPE* shape = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    shape->SetLocked( true );
    board->Add( shape );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->AddItem( shape );
    board->Add( group );

    BOOST_CHECK( PCB_SELECTION_TOOL::HasLockedDescendant( group ) );
}


BOOST_AUTO_TEST_CASE( LockedFootprintPinsGroup )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    FOOTPRINT* footprint = new FOOTPRINT( board.get() );
    footprint->SetLocked( true );
    board->Add( footprint );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->AddItem( footprint );
    board->Add( group );

    BOOST_CHECK( PCB_SELECTION_TOOL::HasLockedDescendant( group ) );
}


BOOST_AUTO_TEST_CASE( LockedFieldDoesNotPinGroupHoldingFootprint )
{
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    FOOTPRINT* footprint = new FOOTPRINT( board.get() );
    board->Add( footprint );

    footprint->Reference().SetLocked( true );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->AddItem( footprint );
    board->Add( group );

    BOOST_CHECK( !PCB_SELECTION_TOOL::HasLockedDescendant( group ) );
}


BOOST_AUTO_TEST_SUITE_END()
