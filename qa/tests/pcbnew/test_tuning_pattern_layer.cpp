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
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/17971
 *
 * The reproduction board carries a differential-pair length-tuning pattern on B.Cu.  Moving it to
 * another copper layer and then deleting it must restore the straight baseline on the new layer.
 * Baseline recovery fed the pattern's PCB_LAYER_ID straight into the PNS router, where a
 * differently-encoded PNS layer index is required, so a pattern moved off F.Cu recovered its
 * baseline on the wrong copper layer.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <pcb_track.h>
#include <pcb_generator.h>
#include <generators/pcb_tuning_pattern.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>

#include <router/pns_router.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_item.h>


struct TUNING_PATTERN_LAYER_FIXTURE
{
    PCB_TUNING_PATTERN& LoadPattern()
    {
        KI_TEST::LoadBoard( m_settingsManager, "issue17971/issue17971", m_board );
        BOOST_REQUIRE( m_board );

        KIID        patternUuid( "24d674f0-f4fd-411b-8bf7-cc6e9297d826" );
        BOARD_ITEM& item = KI_TEST::RequireBoardItemWithTypeAndId( *m_board, PCB_GENERATOR_T, patternUuid );

        return static_cast<PCB_TUNING_PATTERN&>( item );
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// PCB_TUNING_PATTERN::recoverBaseline() is protected; a derived class can form a pointer to it and
// invoke that on the real (base) object, driving the exact code path that #17971 corrected without
// an ill-formed downcast of the loaded pattern.
struct BASELINE_ACCESSOR : public PCB_TUNING_PATTERN
{
    static bool Recover( PCB_TUNING_PATTERN& aPattern, PNS::ROUTER* aRouter, int aPNSLayer )
    {
        return ( aPattern.*( &BASELINE_ACCESSOR::recoverBaseline ) )( aRouter, aPNSLayer );
    }
};


// Captures the PNS layer of every item the router hands back so the test can observe which copper
// layer the recovered baseline actually landed on.  The stock base interface swallows AddItem().
struct RECOVERED_LAYER_IFACE : public PNS_KICAD_IFACE_BASE
{
    void AddItem( PNS::ITEM* aItem ) override
    {
        if( aItem->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T | PNS::ITEM::LINE_T ) )
            m_recoveredLayers.insert( aItem->Layer() );
    }

    std::set<int> m_recoveredLayers;
};


BOOST_FIXTURE_TEST_CASE( SetLayerPropagatesToMemberTracks, TUNING_PATTERN_LAYER_FIXTURE )
{
    PCB_TUNING_PATTERN& pattern = LoadPattern();

    // The reproduction board has the pattern on B.Cu.
    BOOST_CHECK_EQUAL( pattern.GetLayer(), B_Cu );

    // Step 4 of the report: move the pattern (and its member tracks) to another layer.
    pattern.SetLayer( In2_Cu );
    BOOST_CHECK_EQUAL( pattern.GetLayer(), In2_Cu );

    for( BOARD_ITEM* member : pattern.GetBoardItems() )
    {
        if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( member ) )
            BOOST_CHECK_EQUAL( track->GetLayer(), In2_Cu );
    }
}


BOOST_FIXTURE_TEST_CASE( RecoverBaselineFollowsMovedLayer, TUNING_PATTERN_LAYER_FIXTURE )
{
    PCB_TUNING_PATTERN& pattern = LoadPattern();

    // Reproduce the report: move the pattern off its saved layer before deleting it.
    pattern.SetLayer( In2_Cu );

    RECOVERED_LAYER_IFACE iface;
    iface.SetBoard( m_board.get() );

    PNS::ROUTER router;
    router.SetInterface( &iface );
    router.ClearWorld();
    router.SyncWorld();

    // Remove() converts the current board layer to a PNS layer before recovery; do the same here.
    const int pnsLayer = iface.GetPNSLayerFromBoardLayer( pattern.GetLayer() );

    bool ok = BASELINE_ACCESSOR::Recover( pattern, &router, pnsLayer );
    BOOST_CHECK( ok );

    BOOST_REQUIRE( !iface.m_recoveredLayers.empty() );

    // Every recovered baseline segment must sit on the pattern's current layer.  Feeding the raw
    // PCB_LAYER_ID (In2.Cu == 6) as a PNS layer, as the bug did, recovers onto an out-of-range PNS
    // layer that maps back to no board copper layer at all.
    for( int recoveredLayer : iface.m_recoveredLayers )
        BOOST_CHECK_EQUAL( iface.GetBoardLayerFromPNSLayer( recoveredLayer ), In2_Cu );
}
