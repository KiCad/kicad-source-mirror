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
 * @file test_drc_creepage_issue23576.cpp
 *
 * Regression test for issue #23576: creepage DRC path starts at incorrect point
 * when encountering NPTH slot.
 *
 * Uses the same board as issue #23389 (same NPTH slot geometry). Validates that
 * the creepage path start/end points lie on the physical edge of their respective
 * copper items, not at the centerline of a track.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <footprint.h>
#include <pcb_marker.h>
#include <pcb_track.h>
#include <pad.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_CREEPAGE_PATH_FIXTURE
{
    DRC_CREEPAGE_PATH_FIXTURE() = default;

    ~DRC_CREEPAGE_PATH_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( CreepagePathStartPointIssue23576, DRC_CREEPAGE_PATH_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23389/issue23389", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue23389" );

    struct ViolationInfo
    {
        std::shared_ptr<DRC_ITEM> item;
        VECTOR2I                  pos;
        std::vector<PCB_SHAPE>    pathShapes;
    };

    std::vector<ViolationInfo> violations;
    BOARD_DESIGN_SETTINGS&     bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) != SEVERITY::RPT_SEVERITY_ERROR )
                    return;

                ViolationInfo vi;
                vi.item = aItem;
                vi.pos = aPos;

                if( aPathGenerator )
                {
                    PCB_MARKER marker( aItem, aPos, aLayer );
                    aPathGenerator( &marker );
                    vi.pathShapes = marker.GetShapes();
                }

                violations.push_back( vi );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_REQUIRE_GE( violations.size(), 1u );

    // Build a lookup of board items by UUID for resolving DRC item references
    std::map<KIID, BOARD_ITEM*> itemMap;

    for( PCB_TRACK* track : m_board->Tracks() )
        itemMap[track->m_Uuid] = track;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        itemMap[fp->m_Uuid] = fp;

        for( PAD* pad : fp->Pads() )
            itemMap[pad->m_Uuid] = pad;
    }

    for( size_t i = 0; i < violations.size(); i++ )
    {
        const ViolationInfo& vi = violations[i];

        BOOST_TEST_MESSAGE( wxString::Format( "Violation %zu: %s", i,
                                              vi.item->GetErrorMessage( false ) ) );
        BOOST_TEST_MESSAGE( wxString::Format( "  Pos: (%.4f, %.4f) mm, shapes: %d",
                                              vi.pos.x / 1e6, vi.pos.y / 1e6,
                                              (int) vi.pathShapes.size() ) );

        for( size_t j = 0; j < vi.pathShapes.size(); j++ )
        {
            const PCB_SHAPE& s = vi.pathShapes[j];

            if( s.GetShape() == SHAPE_T::SEGMENT )
            {
                BOOST_TEST_MESSAGE( wxString::Format(
                        "  [%zu] SEG: (%.4f,%.4f)->(%.4f,%.4f) mm", j,
                        s.GetStart().x / 1e6, s.GetStart().y / 1e6,
                        s.GetEnd().x / 1e6, s.GetEnd().y / 1e6 ) );
            }
            else if( s.GetShape() == SHAPE_T::ARC )
            {
                BOOST_TEST_MESSAGE( wxString::Format(
                        "  [%zu] ARC: (%.4f,%.4f)->(%.4f,%.4f) c=(%.4f,%.4f) mm", j,
                        s.GetStart().x / 1e6, s.GetStart().y / 1e6,
                        s.GetEnd().x / 1e6, s.GetEnd().y / 1e6,
                        s.GetCenter().x / 1e6, s.GetCenter().y / 1e6 ) );
            }
        }

        if( vi.pathShapes.empty() )
            continue;

        // Resolve the items from the violation
        BOARD_ITEM* itemA = nullptr;
        BOARD_ITEM* itemB = nullptr;

        KIID idA = vi.item->GetMainItemID();
        KIID idB = vi.item->GetAuxItemID();

        auto itA = itemMap.find( idA );
        auto itB = itemMap.find( idB );

        if( itA != itemMap.end() )
            itemA = itA->second;

        if( itB != itemMap.end() )
            itemB = itB->second;

        if( itemA )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "  ItemA: %s at (%.4f, %.4f) mm",
                    itemA->GetClass(),
                    itemA->GetPosition().x / 1e6, itemA->GetPosition().y / 1e6 ) );
        }

        if( itemB )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "  ItemB: %s at (%.4f, %.4f) mm",
                    itemB->GetClass(),
                    itemB->GetPosition().x / 1e6, itemB->GetPosition().y / 1e6 ) );
        }

        // For tracks, verify that the path shape vertex nearest to the track lies on
        // its physical edge (at halfWidth from the centerline), not on the centerline.
        // Path shapes can have inconsistent direction, so check ALL vertices.
        for( const BOARD_ITEM* item : { itemA, itemB } )
        {
            if( !item || item->Type() != PCB_TRACE_T )
                continue;

            const PCB_TRACK* track = static_cast<const PCB_TRACK*>( item );
            SEG              trackSeg( track->GetStart(), track->GetEnd() );
            int              halfWidth = track->GetWidth() / 2;

            int minDist = std::numeric_limits<int>::max();

            for( const PCB_SHAPE& s : vi.pathShapes )
            {
                minDist = std::min( minDist, trackSeg.Distance( s.GetStart() ) );
                minDist = std::min( minDist, trackSeg.Distance( s.GetEnd() ) );
            }

            BOOST_TEST_MESSAGE( wxString::Format(
                    "  Track hw=%d closest_vertex=%d start=(%.4f,%.4f) end=(%.4f,%.4f)",
                    halfWidth, minDist,
                    track->GetStart().x / 1e6, track->GetStart().y / 1e6,
                    track->GetEnd().x / 1e6, track->GetEnd().y / 1e6 ) );

            int tolerance = 10000; // 10um

            BOOST_CHECK_MESSAGE(
                    std::abs( minDist - halfWidth ) <= tolerance,
                    wxString::Format(
                            "Violation %zu: closest path vertex is %d nm from track "
                            "centerline, expected %d nm (halfWidth +/- %d nm tolerance). "
                            "Path may be starting at track center instead of edge.",
                            i, minDist, halfWidth, tolerance ) );
        }
    }
}
