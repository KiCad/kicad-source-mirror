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

#include <padstack.h>
#include <layer_ids.h>
#include <base_units.h>

// Regression tests for GitLab #23901. A backdrill's side is identified by its start layer
// (F_Cu = top, B_Cu = bottom), not by which drill slot it occupies. The accessors must read both
// the current layout and the one written by KiCad 10.0 (top backdrill in the tertiary slot), and
// must write start layers that older KiCad reads back correctly.
BOOST_AUTO_TEST_SUITE( BackdrillProperties )


// Top backdrill -> secondary drill, start F_Cu.
BOOST_AUTO_TEST_CASE( TopBackdrillUsesSecondaryDrill )
{
    PADSTACK stack( nullptr );
    stack.Drill().size = { pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.4 ) };

    stack.SetBackdrillMode( BACKDRILL_MODE::BACKDRILL_TOP );
    stack.SetBackdrillSize( true, pcbIUScale.mmToIU( 0.7 ) );
    stack.SetBackdrillEndLayer( true, In1_Cu );

    BOOST_CHECK( stack.SecondaryDrill().size.x > 0 );
    BOOST_CHECK_EQUAL( stack.SecondaryDrill().start, F_Cu );
    BOOST_CHECK_EQUAL( stack.SecondaryDrill().end, In1_Cu );
    BOOST_CHECK_EQUAL( stack.TertiaryDrill().size.x, 0 );

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::BACKDRILL_TOP );
    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( true ).value_or( 0 ), pcbIUScale.mmToIU( 0.7 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( true ), In1_Cu );
}


// Bottom backdrill -> tertiary drill, start B_Cu.
BOOST_AUTO_TEST_CASE( BottomBackdrillUsesTertiaryDrill )
{
    PADSTACK stack( nullptr );
    stack.Drill().size = { pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.4 ) };

    stack.SetBackdrillMode( BACKDRILL_MODE::BACKDRILL_BOTTOM );
    stack.SetBackdrillSize( false, pcbIUScale.mmToIU( 0.7 ) );
    stack.SetBackdrillEndLayer( false, In2_Cu );

    BOOST_CHECK( stack.TertiaryDrill().size.x > 0 );
    BOOST_CHECK_EQUAL( stack.TertiaryDrill().start, B_Cu );
    BOOST_CHECK_EQUAL( stack.TertiaryDrill().end, In2_Cu );
    BOOST_CHECK_EQUAL( stack.SecondaryDrill().size.x, 0 );

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::BACKDRILL_BOTTOM );
    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( false ).value_or( 0 ), pcbIUScale.mmToIU( 0.7 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( false ), In2_Cu );
}


// Getters must read dialog-written slots back unchanged (no swap).
BOOST_AUTO_TEST_CASE( DialogToPanelRoundTrip )
{
    PADSTACK stack( nullptr );

    // What the dialog writes for a top + bottom backdrill.
    PADSTACK::DRILL_PROPS& secondary = stack.SecondaryDrill();
    secondary.size = { pcbIUScale.mmToIU( 0.9 ), pcbIUScale.mmToIU( 0.9 ) };
    secondary.start = F_Cu;
    secondary.end = In1_Cu;
    secondary.shape = PAD_DRILL_SHAPE::CIRCLE;

    PADSTACK::DRILL_PROPS& tertiary = stack.TertiaryDrill();
    tertiary.size = { pcbIUScale.mmToIU( 0.7 ), pcbIUScale.mmToIU( 0.7 ) };
    tertiary.start = B_Cu;
    tertiary.end = In2_Cu;
    tertiary.shape = PAD_DRILL_SHAPE::CIRCLE;

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::BACKDRILL_BOTH );

    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( true ).value_or( 0 ), pcbIUScale.mmToIU( 0.9 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( true ), In1_Cu );

    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( false ).value_or( 0 ), pcbIUScale.mmToIU( 0.7 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( false ), In2_Cu );
}


// Re-editing a size must not jump slots or zero out.
BOOST_AUTO_TEST_CASE( EditingSizePreservesSlotAndValue )
{
    PADSTACK stack( nullptr );
    stack.Drill().size = { pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.4 ) };

    stack.SetBackdrillMode( BACKDRILL_MODE::BACKDRILL_BOTTOM );
    stack.SetBackdrillSize( false, pcbIUScale.mmToIU( 0.7 ) );
    stack.SetBackdrillEndLayer( false, In2_Cu );

    stack.SetBackdrillSize( false, pcbIUScale.mmToIU( 0.8 ) );

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::BACKDRILL_BOTTOM );
    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( false ).value_or( 0 ), pcbIUScale.mmToIU( 0.8 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( false ), In2_Cu );
    BOOST_CHECK_EQUAL( stack.SecondaryDrill().size.x, 0 );
}


// NO_BACKDRILL clears both slots.
BOOST_AUTO_TEST_CASE( ClearingModeRemovesBackdrill )
{
    PADSTACK stack( nullptr );
    stack.Drill().size = { pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.4 ) };

    stack.SetBackdrillMode( BACKDRILL_MODE::BACKDRILL_TOP );
    stack.SetBackdrillSize( true, pcbIUScale.mmToIU( 0.7 ) );

    stack.SetBackdrillMode( BACKDRILL_MODE::NO_BACKDRILL );

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::NO_BACKDRILL );
    BOOST_CHECK_EQUAL( stack.SecondaryDrill().size.x, 0 );
    BOOST_CHECK_EQUAL( stack.TertiaryDrill().size.x, 0 );
}


// Seed a padstack the way KiCad 10.0 wrote it: top backdrill in the tertiary slot (start F_Cu),
// bottom backdrill in the secondary slot (start B_Cu). The accessors must read each side from the
// slot carrying the matching start layer, not from a fixed slot position.
BOOST_AUTO_TEST_CASE( ReadsLegacyV10SlotLayout )
{
    PADSTACK stack( nullptr );

    PADSTACK::DRILL_PROPS& secondary = stack.SecondaryDrill();
    secondary.size = { pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.8 ) };
    secondary.start = B_Cu;
    secondary.end = In2_Cu;
    secondary.shape = PAD_DRILL_SHAPE::CIRCLE;

    PADSTACK::DRILL_PROPS& tertiary = stack.TertiaryDrill();
    tertiary.size = { pcbIUScale.mmToIU( 0.6 ), pcbIUScale.mmToIU( 0.6 ) };
    tertiary.start = F_Cu;
    tertiary.end = In1_Cu;
    tertiary.shape = PAD_DRILL_SHAPE::CIRCLE;

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::BACKDRILL_BOTH );

    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( true ).value_or( 0 ), pcbIUScale.mmToIU( 0.6 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( true ), In1_Cu );

    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( false ).value_or( 0 ), pcbIUScale.mmToIU( 0.8 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( false ), In2_Cu );
}


// A 10.0 top-only backdrill lives in the tertiary slot; it must read as a top backdrill, not a
// bottom one (the slot-position reading regressed exactly this case).
BOOST_AUTO_TEST_CASE( ReadsLegacyV10TopOnly )
{
    PADSTACK stack( nullptr );

    PADSTACK::DRILL_PROPS& tertiary = stack.TertiaryDrill();
    tertiary.size = { pcbIUScale.mmToIU( 0.6 ), pcbIUScale.mmToIU( 0.6 ) };
    tertiary.start = F_Cu;
    tertiary.end = In1_Cu;
    tertiary.shape = PAD_DRILL_SHAPE::CIRCLE;

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::BACKDRILL_TOP );
    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( true ).value_or( 0 ), pcbIUScale.mmToIU( 0.6 ) );
    BOOST_CHECK_EQUAL( stack.GetBackdrillEndLayer( true ), In1_Cu );
    BOOST_CHECK( !stack.GetBackdrillSize( false ).has_value() );
}


// Editing a 10.0 top-only backdrill must update it in place (in the tertiary slot it already
// occupies) and keep the F_Cu start layer, so the result stays readable by older KiCad and no
// duplicate top backdrill appears in the secondary slot.
BOOST_AUTO_TEST_CASE( EditingLegacyBackdrillStaysInPlaceAndReadable )
{
    PADSTACK stack( nullptr );

    PADSTACK::DRILL_PROPS& tertiary = stack.TertiaryDrill();
    tertiary.size = { pcbIUScale.mmToIU( 0.6 ), pcbIUScale.mmToIU( 0.6 ) };
    tertiary.start = F_Cu;
    tertiary.end = In1_Cu;
    tertiary.shape = PAD_DRILL_SHAPE::CIRCLE;

    stack.SetBackdrillSize( true, pcbIUScale.mmToIU( 0.75 ) );

    BOOST_CHECK_EQUAL( stack.TertiaryDrill().size.x, pcbIUScale.mmToIU( 0.75 ) );
    BOOST_CHECK_EQUAL( stack.TertiaryDrill().start, F_Cu );
    BOOST_CHECK_EQUAL( stack.SecondaryDrill().size.x, 0 );

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::BACKDRILL_TOP );
    BOOST_CHECK_EQUAL( stack.GetBackdrillSize( true ).value_or( 0 ), pcbIUScale.mmToIU( 0.75 ) );
}


// Clearing the must-cut layer removes the backdrill: a sized drill with no must-cut does not
// exist (the via layer sanitizer enforces the same rule).
BOOST_AUTO_TEST_CASE( ClearingMustCutRemovesBackdrill )
{
    PADSTACK stack( nullptr );
    stack.Drill().size = { pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.4 ) };

    stack.SetBackdrillMode( BACKDRILL_MODE::BACKDRILL_TOP );
    stack.SetBackdrillSize( true, pcbIUScale.mmToIU( 0.7 ) );
    stack.SetBackdrillEndLayer( true, In1_Cu );

    stack.SetBackdrillEndLayer( true, UNDEFINED_LAYER );

    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::NO_BACKDRILL );
    BOOST_CHECK( !stack.GetBackdrillSize( true ).has_value() );
    BOOST_CHECK_EQUAL( stack.SecondaryDrill().size.x, 0 );
}


// A malformed padstack with the same side duplicated across both slots must be fully cleared, not
// left with a surviving duplicate that re-asserts the backdrill.
BOOST_AUTO_TEST_CASE( ClearingCollapsesDuplicateSideSlots )
{
    PADSTACK stack( nullptr );

    PADSTACK::DRILL_PROPS& secondary = stack.SecondaryDrill();
    secondary.size = { pcbIUScale.mmToIU( 0.7 ), pcbIUScale.mmToIU( 0.7 ) };
    secondary.start = F_Cu;
    secondary.end = In1_Cu;
    secondary.shape = PAD_DRILL_SHAPE::CIRCLE;

    PADSTACK::DRILL_PROPS& tertiary = stack.TertiaryDrill();
    tertiary.size = { pcbIUScale.mmToIU( 0.6 ), pcbIUScale.mmToIU( 0.6 ) };
    tertiary.start = F_Cu;
    tertiary.end = In1_Cu;
    tertiary.shape = PAD_DRILL_SHAPE::CIRCLE;

    stack.SetBackdrillSize( true, std::nullopt );

    BOOST_CHECK_EQUAL( stack.SecondaryDrill().size.x, 0 );
    BOOST_CHECK_EQUAL( stack.TertiaryDrill().size.x, 0 );
    BOOST_CHECK( stack.GetBackdrillMode() == BACKDRILL_MODE::NO_BACKDRILL );
}


BOOST_AUTO_TEST_SUITE_END()
