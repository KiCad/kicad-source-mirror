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
#include <constraints/constraint_builder.h>

#include <tools/constraint_overlay.h>

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

    // The badges store their (shared) anchor; the fan-out lives in the screen-space layout.
    BOOST_CHECK_EQUAL( overlay.Badges()[0].pos, overlay.Badges()[1].pos );

    // LayoutBadges separates same-anchor badges by at least the fan step at any zoom, and the
    // separation scales with world-per-pixel -- the regression against the old fixed 1.5 mm fan.
    for( double worldPerPx : { 200.0, 8000.0 } )
    {
        std::vector<VECTOR2D> layout = CONSTRAINT_OVERLAY::LayoutBadges( overlay.Badges(), worldPerPx );

        BOOST_REQUIRE_EQUAL( layout.size(), 2 );
        double dist = ( layout[0] - layout[1] ).EuclideanNorm();
        BOOST_TEST( dist >= 18.0 * worldPerPx - 1.0 );
    }

    // Selection state changes only on a real change.
    BOOST_TEST( overlay.SetSelected( c1->m_Uuid ) );
    BOOST_TEST( !overlay.SetSelected( c1->m_Uuid ) );
    BOOST_CHECK( overlay.GetSelected() == c1->m_Uuid );
    BOOST_TEST( overlay.SetSelected( niluuid ) );
}


// Badges on DIFFERENT anchors that project within a glyph of each other at far zoom-out are still
// pushed apart by the layout (the cross-anchor de-overlap the old world-space fan also did).
BOOST_AUTO_TEST_CASE( LayoutDeOverlapsNearbyAnchors )
{
    constexpr int MM = 1000000;

    BOARD       board;
    KIGFX::VIEW view;

    PCB_SHAPE* seg = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    seg->SetStart( VECTOR2I( 0, 0 ) );
    seg->SetEnd( VECTOR2I( MM / 10, 0 ) );   // endpoints only 0.1 mm apart
    board.Add( seg );

    auto* c1 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    c1->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c1 );

    auto* c2 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    c2->AddMember( seg->m_Uuid, CONSTRAINT_ANCHOR::END );
    board.Add( c2 );

    CONSTRAINT_OVERLAY overlay( &board, &view );
    overlay.Update( DiagnoseBoardConstraints( &board ) );
    BOOST_REQUIRE_EQUAL( overlay.Badges().size(), 2 );

    const double          worldPerPx = 8000.0;
    std::vector<VECTOR2D> layout = CONSTRAINT_OVERLAY::LayoutBadges( overlay.Badges(), worldPerPx );

    BOOST_TEST( ( layout[0] - layout[1] ).EuclideanNorm() >= 18.0 * worldPerPx - 1.0 );
}


// The visibility mode and hover filter are orthogonal: ALWAYS shows every badge, HOVER with nothing
// hovered shows none, HOVER with a shape shows only its constraints, and a panel isolation wins.
BOOST_AUTO_TEST_CASE( VisibilityModeAndHoverFilter )
{
    constexpr int MM = 1000000;

    BOARD       board;
    KIGFX::VIEW view;

    PCB_SHAPE* seg1 = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    seg1->SetStart( VECTOR2I( 0, 0 ) );
    seg1->SetEnd( VECTOR2I( 10 * MM, 0 ) );
    board.Add( seg1 );

    PCB_SHAPE* seg2 = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    seg2->SetStart( VECTOR2I( 0, 10 * MM ) );
    seg2->SetEnd( VECTOR2I( 10 * MM, 10 * MM ) );
    board.Add( seg2 );

    auto* c1 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    c1->AddMember( seg1->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c1 );

    auto* c2 = new PCB_CONSTRAINT( &board, PCB_CONSTRAINT_TYPE::FIXED_POSITION );
    c2->AddMember( seg2->m_Uuid, CONSTRAINT_ANCHOR::START );
    board.Add( c2 );

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( &board );
    CONSTRAINT_OVERLAY           overlay( &board, &view );

    overlay.SetVisibilityMode( OVERLAY_MODE::ALWAYS );
    overlay.Update( diag );
    BOOST_CHECK_EQUAL( overlay.Badges().size(), 2 );

    overlay.SetVisibilityMode( OVERLAY_MODE::HOVER );
    overlay.Update( diag );
    BOOST_CHECK_EQUAL( overlay.Badges().size(), 0 );   // nothing hovered -> hidden

    overlay.SetHoverShape( seg1->m_Uuid );
    overlay.Update( diag );
    BOOST_REQUIRE_EQUAL( overlay.Badges().size(), 1 );
    BOOST_CHECK( overlay.Badges()[0].constraint == c1->m_Uuid );

    overlay.SetIsolated( c2->m_Uuid );   // panel focus wins over hover
    overlay.Update( diag );
    BOOST_REQUIRE_EQUAL( overlay.Badges().size(), 1 );
    BOOST_CHECK( overlay.Badges()[0].constraint == c2->m_Uuid );
}


BOOST_AUTO_TEST_CASE( NearestConstrainedShapeUsesCandidatesOnly )
{
    constexpr int MM = 1000000;

    BOARD      board;
    PCB_SHAPE* seg = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    seg->SetStart( VECTOR2I( 0, 0 ) );
    seg->SetEnd( VECTOR2I( 10 * MM, 0 ) );
    board.Add( seg );

    std::vector<PCB_SHAPE*> candidates = { seg };

    std::optional<KIID> hit = NearestConstrainedShape( candidates, VECTOR2I( 5 * MM, 1000 ), 5000 );
    BOOST_REQUIRE( hit.has_value() );
    BOOST_CHECK( *hit == seg->m_Uuid );

    BOOST_CHECK( !NearestConstrainedShape( candidates, VECTOR2I( 5 * MM, 5 * MM ), 5000 ).has_value() );
    BOOST_CHECK( !NearestConstrainedShape( {}, VECTOR2I( 0, 0 ), 5000 ).has_value() );
}


BOOST_AUTO_TEST_SUITE_END()
