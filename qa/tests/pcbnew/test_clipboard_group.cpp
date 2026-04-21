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

/**
 * @file test_clipboard_group.cpp
 *
 * Regression tests for clipboard copy/paste of PCB_GROUPs.
 *
 * Specifically guards against https://gitlab.com/kicad/code/kicad/-/issues/24000 where
 * groups appeared ungrouped after paste because PCB_IO_KICAD_SEXPR::format(PCB_GROUP*)
 * validated cloned member pointers against the source board's pointer cache and dropped
 * every member as "invalid", causing the (group ...) sexpr to be omitted entirely.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <kicad_clipboard.h>
#include <lset.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <tools/pcb_selection.h>


namespace
{

/**
 * Round-trips a selection through CLIPBOARD_IO and returns the parsed clipboard board.
 * The source board is not modified.
 */
std::unique_ptr<BOARD> roundTripSelection( BOARD* aSourceBoard, const PCB_SELECTION& aSelection )
{
    wxString clipboardData;

    CLIPBOARD_IO io;
    io.SetBoard( aSourceBoard );
    io.SetWriter( [&]( const wxString& aData ) { clipboardData = aData; } );
    io.SetReader( [&]() { return clipboardData; } );

    io.SaveSelection( aSelection, false );

    BOARD_ITEM* parsed = io.Parse();
    BOOST_REQUIRE( parsed );
    BOOST_REQUIRE_EQUAL( parsed->Type(), PCB_T );

    return std::unique_ptr<BOARD>( static_cast<BOARD*>( parsed ) );
}

} // anonymous namespace


BOOST_AUTO_TEST_SUITE( ClipboardGroup )


/**
 * Copy a group containing two PCB_SHAPE objects to the clipboard, parse it back, and
 * verify the round-tripped board still contains the group with both members associated.
 */
BOOST_AUTO_TEST_CASE( CopyPasteGroupPreservesMembership )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    PCB_SHAPE* line1 = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    line1->SetStart( VECTOR2I( 0, 0 ) );
    line1->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ) );
    line1->SetLayer( F_SilkS );
    line1->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    board->Add( line1 );

    PCB_SHAPE* rect = new PCB_SHAPE( board.get(), SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 0, 0 ) );
    rect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ) );
    rect->SetLayer( F_SilkS );
    rect->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    board->Add( rect );

    PCB_GROUP* group = new PCB_GROUP( board.get() );
    group->SetName( wxT( "TestGroup" ) );
    group->AddItem( line1 );
    group->AddItem( rect );
    board->Add( group );

    PCB_SELECTION selection;
    selection.Add( group );

    std::unique_ptr<BOARD> clipBoard = roundTripSelection( board.get(), selection );

    BOOST_REQUIRE_EQUAL( clipBoard->Groups().size(), 1u );

    PCB_GROUP* pastedGroup = clipBoard->Groups().front();
    BOOST_CHECK_EQUAL( pastedGroup->GetName(), wxT( "TestGroup" ) );
    BOOST_CHECK_EQUAL( pastedGroup->GetItems().size(), 2u );

    // Each pasted child must point back at the pasted group, not be orphaned.
    for( EDA_ITEM* member : pastedGroup->GetItems() )
    {
        BOOST_REQUIRE( member );
        BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( member );
        BOOST_REQUIRE( boardItem );
        BOOST_CHECK_EQUAL( boardItem->GetParentGroup(), pastedGroup );
    }
}


/**
 * Copy a nested group structure (group containing a group) and verify both levels
 * survive the clipboard round-trip with intact membership.
 */
BOOST_AUTO_TEST_CASE( CopyPasteNestedGroupPreservesMembership )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    PCB_SHAPE* innerShape1 = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    innerShape1->SetStart( VECTOR2I( 0, 0 ) );
    innerShape1->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 1 ), 0 ) );
    innerShape1->SetLayer( F_SilkS );
    innerShape1->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::SOLID ) );
    board->Add( innerShape1 );

    PCB_SHAPE* innerShape2 = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    innerShape2->SetStart( VECTOR2I( 0, 0 ) );
    innerShape2->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 2 ), 0 ) );
    innerShape2->SetLayer( F_SilkS );
    innerShape2->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::SOLID ) );
    board->Add( innerShape2 );

    PCB_GROUP* innerGroup = new PCB_GROUP( board.get() );
    innerGroup->SetName( wxT( "InnerGroup" ) );
    innerGroup->AddItem( innerShape1 );
    innerGroup->AddItem( innerShape2 );
    board->Add( innerGroup );

    PCB_SHAPE* outerShape = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
    outerShape->SetStart( VECTOR2I( 0, 0 ) );
    outerShape->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 3 ), 0 ) );
    outerShape->SetLayer( F_SilkS );
    outerShape->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::SOLID ) );
    board->Add( outerShape );

    PCB_GROUP* outerGroup = new PCB_GROUP( board.get() );
    outerGroup->SetName( wxT( "OuterGroup" ) );
    outerGroup->AddItem( innerGroup );
    outerGroup->AddItem( outerShape );
    board->Add( outerGroup );

    PCB_SELECTION selection;
    selection.Add( outerGroup );

    std::unique_ptr<BOARD> clipBoard = roundTripSelection( board.get(), selection );

    BOOST_REQUIRE_EQUAL( clipBoard->Groups().size(), 2u );

    // Find the outer group by name (round-trip should preserve names).
    PCB_GROUP* pastedOuter = nullptr;
    PCB_GROUP* pastedInner = nullptr;

    for( PCB_GROUP* g : clipBoard->Groups() )
    {
        if( g->GetName() == wxT( "OuterGroup" ) )
            pastedOuter = g;
        else if( g->GetName() == wxT( "InnerGroup" ) )
            pastedInner = g;
    }

    BOOST_REQUIRE( pastedOuter );
    BOOST_REQUIRE( pastedInner );

    BOOST_CHECK_EQUAL( pastedOuter->GetItems().size(), 2u );
    BOOST_CHECK_EQUAL( pastedInner->GetItems().size(), 2u );
    BOOST_CHECK_EQUAL( pastedInner->GetParentGroup(), pastedOuter );
}


BOOST_AUTO_TEST_SUITE_END()
