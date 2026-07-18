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
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_rule.h>
#include <settings/json_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_test_utils.h>

#include <nlohmann/json.hpp>


namespace
{
struct BDS_TEST_FIXTURE
{
    BDS_TEST_FIXTURE() :
            m_board( new BOARD() )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// A caller-owned parent so a BOARD_DESIGN_SETTINGS can nest under it without a full project.
class BDS_TEST_PARENT : public JSON_SETTINGS
{
public:
    BDS_TEST_PARENT() :
            JSON_SETTINGS( "bds_test_parent", SETTINGS_LOC::NONE, 0, false, false, false )
    {
    }
};
} // namespace


BOOST_FIXTURE_TEST_SUITE( BoardDesignSettings, BDS_TEST_FIXTURE )


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23327
 *
 * Negative silk clearance values must survive a save/load round-trip through
 * the project settings JSON. Previously the PARAM_SCALED lower bound was 0,
 * causing negative values to be reset to the default on load.
 */
BOOST_AUTO_TEST_CASE( NegativeSilkClearanceRoundTrip )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    int negativeValue = pcbIUScale.mmToIU( -0.1 );
    bds.m_SilkClearance = negativeValue;

    // Store all params into the JSON backing store
    bds.Store();

    // Reset to default
    bds.m_SilkClearance = 0;

    // Load back from JSON
    bds.Load();

    BOOST_CHECK_EQUAL( bds.m_SilkClearance, negativeValue );
}


/**
 * Regression test: physical_hole_clearance must count toward the worst-case clearance.
 *
 * GetBiggestClearanceValue() seeds the interactive router's broad-phase search radius.
 * It previously omitted PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, so a physical_hole_clearance
 * rule did not widen that radius. The router then discovered the hole only once a track
 * was already well inside the rule distance, reacting far too late. The worst-case value
 * must be at least the rule's minimum.
 */
BOOST_AUTO_TEST_CASE( BiggestClearanceIncludesPhysicalHoleClearance )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Larger than any default clearance, so it can only come from the rule below.
    const int ruleClearance = pcbIUScale.mmToIU( 4.0 );

    auto rule = std::make_shared<DRC_RULE>( wxT( "NPTH Hole to Track Clearance" ) );

    DRC_CONSTRAINT constraint( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT );
    constraint.Value().SetMin( ruleClearance );
    rule->AddConstraint( constraint );

    auto engine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );
    engine->InitEngine( rule );
    bds.m_DRCEngine = engine;

    BOOST_CHECK_GE( bds.GetBiggestClearanceValue(), ruleClearance );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24646
// Cycling predefined sizes must skip the index-0 netclass placeholder on roll-over and stay
// monotonic.
BOOST_AUTO_TEST_CASE( PredefinedTrackWidthCyclingMonotonic )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Index 0 is the netclass placeholder; indices 1..5 are sorted predefined sizes.
    bds.m_TrackWidthList = { 0,
                             pcbIUScale.mmToIU( 0.5 ),
                             pcbIUScale.mmToIU( 0.6 ),
                             pcbIUScale.mmToIU( 0.8 ),
                             pcbIUScale.mmToIU( 1.0 ),
                             pcbIUScale.mmToIU( 1.2 ) };

    const int lastReal = (int) bds.m_TrackWidthList.size() - 1;

    // Increment: netclass(0) -> smallest(1) -> ... -> largest, then roll over to smallest, never 0.
    BOOST_CHECK_EQUAL( bds.GetNextTrackWidthIndex( 0, true ), 1 );

    int idx = 1;

    for( int step = 2; step <= lastReal; ++step )
    {
        idx = bds.GetNextTrackWidthIndex( idx, true );
        BOOST_CHECK_EQUAL( idx, step );
    }

    // Roll-over from the largest must skip the placeholder and land on the smallest real size.
    BOOST_CHECK_EQUAL( bds.GetNextTrackWidthIndex( lastReal, true ), 1 );

    // Decrement: smallest(1) rolls over to the largest, never to the placeholder.
    BOOST_CHECK_EQUAL( bds.GetNextTrackWidthIndex( 1, false ), lastReal );

    idx = lastReal;

    for( int step = lastReal - 1; step >= 1; --step )
    {
        idx = bds.GetNextTrackWidthIndex( idx, false );
        BOOST_CHECK_EQUAL( idx, step );
    }

    // A full forward cycle must never report the netclass placeholder once stepping has started.
    idx = bds.GetNextTrackWidthIndex( 0, true );

    for( int step = 0; step < lastReal * 3; ++step )
    {
        BOOST_CHECK_GE( idx, 1 );
        idx = bds.GetNextTrackWidthIndex( idx, true );
    }

    // An empty list (no placeholder, no predefined sizes) must stay at index 0 and never store a
    // negative index through SetTrackWidthIndex.
    bds.m_TrackWidthList.clear();
    BOOST_CHECK_EQUAL( bds.GetNextTrackWidthIndex( 0, true ), 0 );
    BOOST_CHECK_EQUAL( bds.GetNextTrackWidthIndex( 0, false ), 0 );
    bds.SetTrackWidthIndex( bds.GetNextTrackWidthIndex( 0, false ) );
    BOOST_CHECK_EQUAL( bds.GetTrackWidthIndex(), 0 );
}


BOOST_AUTO_TEST_CASE( PredefinedViaSizeCyclingMonotonic )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Index 0 is the netclass placeholder; indices 1..3 are sorted predefined via sizes.
    bds.m_ViasDimensionsList = { { 0, 0 },
                                 { pcbIUScale.mmToIU( 0.6 ), pcbIUScale.mmToIU( 0.3 ) },
                                 { pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ) },
                                 { pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 0.5 ) } };

    const int lastReal = (int) bds.m_ViasDimensionsList.size() - 1;

    BOOST_CHECK_EQUAL( bds.GetNextViaSizeIndex( 0, true ), 1 );
    BOOST_CHECK_EQUAL( bds.GetNextViaSizeIndex( lastReal, true ), 1 );
    BOOST_CHECK_EQUAL( bds.GetNextViaSizeIndex( 1, false ), lastReal );

    // A list holding only the netclass placeholder must stay put in both directions.
    bds.m_ViasDimensionsList = { { 0, 0 } };
    BOOST_CHECK_EQUAL( bds.GetNextViaSizeIndex( 0, true ), 0 );
    BOOST_CHECK_EQUAL( bds.GetNextViaSizeIndex( 0, false ), 0 );

    // An empty list must stay at index 0 and never store a negative index.
    bds.m_ViasDimensionsList.clear();
    BOOST_CHECK_EQUAL( bds.GetNextViaSizeIndex( 0, true ), 0 );
    BOOST_CHECK_EQUAL( bds.GetNextViaSizeIndex( 0, false ), 0 );
    bds.SetViaSizeIndex( bds.GetNextViaSizeIndex( 0, false ) );
    BOOST_CHECK_EQUAL( bds.GetViaSizeIndex(), 0 );
}


BOOST_AUTO_TEST_CASE( PredefinedDiffPairCyclingMonotonic )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Index 0 is the netclass placeholder; indices 1..3 are sorted predefined diff-pair dimensions.
    bds.m_DiffPairDimensionsList = { { 0, 0, 0 },
                                     { pcbIUScale.mmToIU( 0.2 ), pcbIUScale.mmToIU( 0.2 ), 0 },
                                     { pcbIUScale.mmToIU( 0.25 ), pcbIUScale.mmToIU( 0.25 ), 0 },
                                     { pcbIUScale.mmToIU( 0.3 ), pcbIUScale.mmToIU( 0.3 ), 0 } };

    const int lastReal = (int) bds.m_DiffPairDimensionsList.size() - 1;

    BOOST_CHECK_EQUAL( bds.GetNextDiffPairIndex( 0, true ), 1 );
    BOOST_CHECK_EQUAL( bds.GetNextDiffPairIndex( lastReal, true ), 1 );
    BOOST_CHECK_EQUAL( bds.GetNextDiffPairIndex( 1, false ), lastReal );

    // An empty list must stay at index 0 and never store a negative index.
    bds.m_DiffPairDimensionsList.clear();
    BOOST_CHECK_EQUAL( bds.GetNextDiffPairIndex( 0, true ), 0 );
    BOOST_CHECK_EQUAL( bds.GetNextDiffPairIndex( 0, false ), 0 );
    bds.SetDiffPairIndex( bds.GetNextDiffPairIndex( 0, false ) );
    BOOST_CHECK_EQUAL( bds.GetDiffPairIndex(), 0 );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24644
// Switching track width from the connected-width placeholder must advance to a real size on the
// first press. The router gating is GUI bound, so this drives the same GetNextTrackWidthIndex
// stepping the hotkeys call.
BOOST_AUTO_TEST_CASE( TrackWidthSwitchAdvancesFromConnectedWidth )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    const int size1 = pcbIUScale.mmToIU( 0.25 );
    const int size2 = pcbIUScale.mmToIU( 0.50 );
    const int size3 = pcbIUScale.mmToIU( 1.00 );

    // Index 0 is the netclass/connected-width placeholder; the rest are real predefined sizes.
    bds.m_TrackWidthList = { 0, size1, size2, size3 };
    bds.UseCustomTrackViaSize( false );

    // Start on the connected-width placeholder, as the router does before any override.
    bds.SetTrackWidthIndex( 0 );
    BOOST_CHECK_EQUAL( bds.GetTrackWidthIndex(), 0 );

    auto incIndex = [&]() { bds.SetTrackWidthIndex( bds.GetNextTrackWidthIndex( bds.GetTrackWidthIndex(), true ) ); };
    auto decIndex = [&]() { bds.SetTrackWidthIndex( bds.GetNextTrackWidthIndex( bds.GetTrackWidthIndex(), false ) ); };

    // First "Next" press must move to the first real predefined size, not no-op.
    incIndex();
    BOOST_CHECK_EQUAL( bds.GetTrackWidthIndex(), 1 );
    BOOST_CHECK_EQUAL( bds.GetCurrentTrackWidth(), size1 );

    // Subsequent presses step through the list, then roll over to the smallest real size, skipping
    // the netclass placeholder at index 0.
    incIndex();
    BOOST_CHECK_EQUAL( bds.GetCurrentTrackWidth(), size2 );
    incIndex();
    BOOST_CHECK_EQUAL( bds.GetCurrentTrackWidth(), size3 );
    incIndex();
    BOOST_CHECK_EQUAL( bds.GetTrackWidthIndex(), 1 );
    BOOST_CHECK_EQUAL( bds.GetCurrentTrackWidth(), size1 );

    // First "Previous" press from the placeholder must roll over to the last real size.
    bds.SetTrackWidthIndex( 0 );
    decIndex();
    BOOST_CHECK_EQUAL( bds.GetTrackWidthIndex(), 3 );
    BOOST_CHECK_EQUAL( bds.GetCurrentTrackWidth(), size3 );
}


/**
 * Highest-risk path of the #24402 fix. BOARD_DESIGN_SETTINGS sets m_resetParamsIfMissing = false
 * because its params are seeded from the .kicad_pcb parser before the project JSON loads. A
 * non-default value absent from the project JSON must not be reset on load and must survive a
 * save/reload round-trip through the parent -- the "absent-but-default counts as a match" logic
 * must never drop a genuine board value.
 */
BOOST_AUTO_TEST_CASE( SeededBoardValueSurvivesRoundTrip )
{
    BDS_TEST_PARENT parent;

    ( *parent.Internals() )["/board/design_settings"_json_pointer] =
            nlohmann::json{ { "meta", { { "version", 2 } } } };

    const int negativeValue = pcbIUScale.mmToIU( -0.1 );

    {
        BOARD_DESIGN_SETTINGS bds( &parent, "board.design_settings" );

        bds.m_SilkClearance = negativeValue;
        bds.LoadFromFile();
        BOOST_CHECK_EQUAL( bds.m_SilkClearance, negativeValue );

        bds.SaveToFile();
    }

    BOARD_DESIGN_SETTINGS reloaded( &parent, "board.design_settings" );
    reloaded.LoadFromFile();
    BOOST_CHECK_EQUAL( reloaded.m_SilkClearance, negativeValue );
}


BOOST_AUTO_TEST_SUITE_END()
