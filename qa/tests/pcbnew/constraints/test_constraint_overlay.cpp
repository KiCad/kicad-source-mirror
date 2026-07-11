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

/**
 * @file
 * Regression test for the SIGSEGV in KIGFX::VIEW_RTREE::Query reached from a GAL repaint after
 * toggling the constraint overlay off. VIEW::MakeOverlay() already registers the overlay, so the
 * extra VIEW::Add() in CONSTRAINT_OVERLAY's constructor inserted a second R-tree entry that the
 * destructor's single VIEW::Remove() could not reclaim. The leftover entry dangled on the freed
 * VIEW_OVERLAY and was dereferenced on the next redraw.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <set>

#include <board.h>
#include <math/box2.h>
#include <pcb_shape.h>
#include <view/view.h>

#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>

#include <tools/constraint_overlay.h>
#include <tools/constraint_endpoint_overlay.h>

BOOST_AUTO_TEST_SUITE( ConstraintOverlay )


BOOST_AUTO_TEST_CASE( OverlayRegistersOncePerLayerAndUnregistersFully )
{
    BOARD       board;
    KIGFX::VIEW view;

    BOX2I maxBox;
    maxBox.SetMaximum();

    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> baseline;
    view.Query( maxBox, baseline );

    {
        CONSTRAINT_OVERLAY overlay( &board, &view );

        std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> withOverlay;
        view.Query( maxBox, withOverlay );

        // Each item may appear once per layer it occupies. A duplicate (item, layer) pair means
        // the same VIEW_ITEM was added to the view twice and can only be removed once.
        std::set<KIGFX::VIEW::LAYER_ITEM_PAIR> unique( withOverlay.begin(), withOverlay.end() );
        BOOST_CHECK_EQUAL( unique.size(), withOverlay.size() );

        // The overlay registers two items: the world-space tint overlay and the screen-constant
        // badge item.
        BOOST_CHECK_EQUAL( withOverlay.size(), baseline.size() + 2 );
    }

    // After destruction the view must hold no leftover (dangling) reference to the overlay. The
    // pair query only collects pointers, so this is safe even when the overlay has been freed.
    std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> afterDestruction;
    view.Query( maxBox, afterDestruction );

    BOOST_CHECK_EQUAL( afterDestruction.size(), baseline.size() );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( ConstraintEndpointOverlay )


BOOST_AUTO_TEST_CASE( PointSetTogglesAndPrunes )
{
    BOARD       board;
    KIGFX::VIEW view;

    // The overlay resolves shapes by KIID against the board, so the shapes must live on it.
    PCB_SHAPE* a = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    a->SetStart( VECTOR2I( 0, 0 ) );
    a->SetEnd( VECTOR2I( 1000000, 0 ) );
    board.Add( a );

    PCB_SHAPE* b = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    b->SetStart( VECTOR2I( 0, 1000000 ) );
    b->SetEnd( VECTOR2I( 1000000, 1000000 ) );
    board.Add( b );

    const double tol = 100;

    CONSTRAINT_ENDPOINT_OVERLAY overlay( &board, &view );
    overlay.SetShapes( { a, b } );

    // Clicking an anchor adds it; clicking again removes it.
    BOOST_TEST( overlay.ToggleNearest( VECTOR2I( 0, 0 ), tol ) );
    BOOST_CHECK_EQUAL( overlay.PointSet().size(), 1 );
    BOOST_CHECK( overlay.PointSet()[0] == CONSTRAINT_MEMBER( a->m_Uuid, CONSTRAINT_ANCHOR::START ) );

    BOOST_TEST( overlay.ToggleNearest( VECTOR2I( 0, 0 ), tol ) );
    BOOST_TEST( overlay.PointSet().empty() );

    // A click far from any anchor binds nothing.
    BOOST_TEST( !overlay.ToggleNearest( VECTOR2I( 500000, 500000 ), tol ) );

    // Two anchors across two shapes.
    overlay.ToggleNearest( VECTOR2I( 0, 0 ), tol );                // a START
    overlay.ToggleNearest( VECTOR2I( 1000000, 1000000 ), tol );    // b END
    BOOST_CHECK_EQUAL( overlay.PointSet().size(), 2 );

    // Dropping shape b from the offered set prunes its picked anchor.
    overlay.SetShapes( { a } );
    BOOST_REQUIRE_EQUAL( overlay.PointSet().size(), 1 );
    BOOST_CHECK( overlay.PointSet()[0] == CONSTRAINT_MEMBER( a->m_Uuid, CONSTRAINT_ANCHOR::START ) );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( ConstraintBadges )


BOOST_AUTO_TEST_CASE( BadgesFanOutAndSelectionToggles )
{
    constexpr int MM = 1000000;

    BOARD       board;
    KIGFX::VIEW view;

    PCB_SHAPE* seg = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    seg->SetStart( VECTOR2I( 0, 0 ) );
    seg->SetEnd( VECTOR2I( 10 * MM, 0 ) );
    board.Add( seg );

    // Two constraints whose first member resolves to the same point: their badges must not stack.
    auto* c1 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    c1->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c1 );

    auto* c2 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    c2->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c2 );

    CONSTRAINT_OVERLAY overlay( &board, &view );
    overlay.Update( DiagnoseBoardConstraints( &board ) );

    BOOST_REQUIRE_EQUAL( overlay.Badges().size(), 2 );

    // Both anchor on the segment's start, so fan-out must separate them by at least two hit-radii
    // (one click can then never fall inside both badges' targets).
    double dist = ( overlay.Badges()[0].pos - overlay.Badges()[1].pos ).EuclideanNorm();
    BOOST_TEST( dist >= 2.0 * CONSTRAINT_OVERLAY::BadgeHitRadius() - 1.0 );

    // Selection state changes only on a real change.
    BOOST_TEST( overlay.SetSelected( c1->m_Uuid ) );
    BOOST_TEST( !overlay.SetSelected( c1->m_Uuid ) );
    BOOST_CHECK( overlay.GetSelected() == c1->m_Uuid );
    BOOST_TEST( overlay.SetSelected( niluuid ) );
}


BOOST_AUTO_TEST_SUITE_END()
