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
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <drc/drc_engine.h>
#include <core/profile.h>
#include <settings/settings_manager.h>


struct DRC_SOLDER_MASK_EXPANSION_TEST_FIXTURE
{
    DRC_SOLDER_MASK_EXPANSION_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( FootprintLevelSolderMaskExpansion, DRC_SOLDER_MASK_EXPANSION_TEST_FIXTURE )
{
    // This test verifies that footprint-level solder mask expansion override is correctly
    // applied to pads that don't have their own local override.
    // Regression test for https://gitlab.com/kicad/code/kicad/-/issues/22751

    KI_TEST::LoadBoard( m_settingsManager, "issue22751/issue22751", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.m_DRCEngine->InitEngine( wxFileName() );

    // Board default solder mask expansion is 0.05mm (50000 IU)
    BOOST_CHECK_EQUAL( bds.m_SolderMaskExpansion, 50000 );

    // Find footprints and their pads
    PAD* padWithFootprintMargin = nullptr;
    PAD* padWithNoMargin = nullptr;
    PAD* padWithOwnMargin = nullptr;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        if( fp->GetFPIDAsString() == "TestFootprint_WithSolderMaskMargin" )
        {
            // Footprint has solder mask margin of 0.1mm (100000 IU)
            BOOST_CHECK( fp->GetLocalSolderMaskMargin().has_value() );
            BOOST_CHECK_EQUAL( fp->GetLocalSolderMaskMargin().value(), 100000 );

            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetNumber() == "1" )
                {
                    padWithFootprintMargin = pad;

                    // Pad should NOT have its own local margin
                    BOOST_CHECK( !pad->GetLocalSolderMaskMargin().has_value() );
                }
            }
        }
        else if( fp->GetFPIDAsString() == "TestFootprint_NoSolderMaskMargin" )
        {
            // Footprint has no solder mask margin
            BOOST_CHECK( !fp->GetLocalSolderMaskMargin().has_value() );

            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetNumber() == "1" )
                    padWithNoMargin = pad;
            }
        }
        else if( fp->GetFPIDAsString() == "TestFootprint_PadOverridesSolderMaskMargin" )
        {
            // Footprint has solder mask margin of 0.1mm
            BOOST_CHECK( fp->GetLocalSolderMaskMargin().has_value() );
            BOOST_CHECK_EQUAL( fp->GetLocalSolderMaskMargin().value(), 100000 );

            for( PAD* pad : fp->Pads() )
            {
                if( pad->GetNumber() == "1" )
                {
                    padWithOwnMargin = pad;

                    // Pad has its own margin of 0.2mm (200000 IU)
                    BOOST_CHECK( pad->GetLocalSolderMaskMargin().has_value() );
                    BOOST_CHECK_EQUAL( pad->GetLocalSolderMaskMargin().value(), 200000 );
                }
            }
        }
    }

    BOOST_REQUIRE( padWithFootprintMargin != nullptr );
    BOOST_REQUIRE( padWithNoMargin != nullptr );
    BOOST_REQUIRE( padWithOwnMargin != nullptr );

    // Test GetSolderMaskExpansion which uses the DRC engine to evaluate the constraint

    // Pad in footprint with solder mask margin should use footprint's margin (0.1mm = 100000 IU)
    int expansionWithFpMargin = padWithFootprintMargin->GetSolderMaskExpansion( F_Mask );
    BOOST_CHECK_EQUAL( expansionWithFpMargin, 100000 );

    // Pad in footprint without solder mask margin should use board default (0.05mm = 50000 IU)
    int expansionNoMargin = padWithNoMargin->GetSolderMaskExpansion( F_Mask );
    BOOST_CHECK_EQUAL( expansionNoMargin, 50000 );

    // Pad with its own solder mask margin should use pad's margin (0.2mm = 200000 IU),
    // overriding the footprint's margin
    int expansionWithPadMargin = padWithOwnMargin->GetSolderMaskExpansion( F_Mask );
    BOOST_CHECK_EQUAL( expansionWithPadMargin, 200000 );
}


BOOST_FIXTURE_TEST_CASE( SolderMaskExpansionPerformance, DRC_SOLDER_MASK_EXPANSION_TEST_FIXTURE )
{
    // Verify that GetSolderMaskExpansion is fast when no custom DRC rules exist.
    // Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23213
    //
    // Before the fix, GetSolderMaskExpansion called EvalRules for every item even when no
    // custom rules existed, causing a 10-14x DRC slowdown vs KiCad 9.

    KI_TEST::LoadBoard( m_settingsManager, "stonehenge", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.m_DRCEngine->InitEngine( wxFileName() );

    BOOST_REQUIRE( !bds.m_DRCEngine->HasRulesForConstraintType( SOLDER_MASK_EXPANSION_CONSTRAINT ) );

    // Collect all items that would be queried during the solder mask test
    std::vector<PAD*>       pads;
    std::vector<PCB_TRACK*> tracks;
    std::vector<PCB_SHAPE*> shapes;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
            pads.push_back( pad );

        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
                shapes.push_back( static_cast<PCB_SHAPE*>( item ) );
        }
    }

    for( PCB_TRACK* track : m_board->Tracks() )
        tracks.push_back( track );

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() == PCB_SHAPE_T )
            shapes.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    // Simulate the access pattern from the solder mask bridge test: call GetSolderMaskExpansion
    // many times for each item (once per potential collision pair).
    const int iterations = 1000;

    PROF_TIMER timer;

    for( int i = 0; i < iterations; ++i )
    {
        for( PAD* pad : pads )
            pad->GetSolderMaskExpansion( F_Mask );

        for( PCB_TRACK* track : tracks )
            track->GetSolderMaskExpansion();

        for( PCB_SHAPE* shape : shapes )
            shape->GetSolderMaskExpansion();
    }

    timer.Stop();

    int totalCalls = iterations * ( pads.size() + tracks.size() + shapes.size() );

    BOOST_TEST_MESSAGE( wxString::Format( "%d calls to GetSolderMaskExpansion took %0.1f ms "
                                          "(%0.0f ns/call)",
                                          totalCalls, timer.msecs(),
                                          timer.msecs() * 1e6 / totalCalls ) );

    // With the fix (direct property lookup), this should be well under 100ms.
    // Without the fix (EvalRules for every call), this was ~500ms+ even in debug builds.
    // Use a generous threshold to avoid flakiness on slow CI machines.
    BOOST_CHECK_MESSAGE( timer.msecs() < 500.0,
                         wxString::Format( "GetSolderMaskExpansion too slow: %0.1f ms for %d calls",
                                           timer.msecs(), totalCalls ) );
}
