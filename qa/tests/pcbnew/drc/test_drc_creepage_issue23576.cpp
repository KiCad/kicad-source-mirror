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

        // The reported violation position (vi.pos) is m_pathStart — the arrow tip the user
        // sees. For a track-to-track or pad-to-track violation, this point must lie on the
        // outer copper boundary of the net, not inside overlapping copper from an adjacent
        // track/pad that shares an endpoint. We validate two conditions:
        //
        //   (a) The point is at least halfWidth from every track's centerline of the same
        //       net as itemA (or itemB respectively). Less than halfWidth would mean the
        //       point is strictly inside that track's copper body.
        //   (b) For the track that the path originates from, the point is at halfWidth
        //       from its centerline (on the copper edge).

        auto validateOnOuterBoundary =
                [&]( const BOARD_ITEM* anchor, const VECTOR2I& pt, const char* label )
                {
                    if( !anchor || anchor->Type() != PCB_TRACE_T )
                        return;

                    const PCB_TRACK* anchorTrack = static_cast<const PCB_TRACK*>( anchor );
                    int              anchorHalfWidth = anchorTrack->GetWidth() / 2;
                    int              netCode = anchorTrack->GetNetCode();
                    int              tolerance = 10000; // 10um

                    SEG anchorSeg( anchorTrack->GetStart(), anchorTrack->GetEnd() );
                    int distToAnchor = anchorSeg.Distance( pt );

                    BOOST_TEST_MESSAGE( wxString::Format(
                            "  %s at (%.4f, %.4f): dist to anchor centerline=%d (hw=%d)",
                            label, pt.x / 1e6, pt.y / 1e6, distToAnchor, anchorHalfWidth ) );

                    BOOST_CHECK_MESSAGE(
                            std::abs( distToAnchor - anchorHalfWidth ) <= tolerance,
                            wxString::Format( "Violation %zu %s: path endpoint is %d nm from "
                                              "anchor track centerline, expected %d nm (halfWidth)",
                                              i, label, distToAnchor, anchorHalfWidth ) );

                    // Ensure the point is not inside any other same-net track's copper body.
                    for( PCB_TRACK* other : m_board->Tracks() )
                    {
                        if( other == anchorTrack )
                            continue;

                        if( other->GetNetCode() != netCode )
                            continue;

                        if( !other->IsOnLayer( anchorTrack->GetLayer() ) )
                            continue;

                        SEG otherSeg( other->GetStart(), other->GetEnd() );
                        int otherHalfWidth = other->GetWidth() / 2;
                        int distToOther = otherSeg.Distance( pt );

                        // A point strictly inside another same-net track's copper body
                        // (distance less than halfWidth by more than tolerance) means the
                        // creepage path starts at an "interior" point that isn't a physical
                        // copper edge once adjacent segments are accounted for.
                        BOOST_CHECK_MESSAGE(
                                distToOther >= otherHalfWidth - tolerance,
                                wxString::Format(
                                        "Violation %zu %s: path endpoint (%.4f, %.4f) is "
                                        "%d nm inside adjacent same-net track "
                                        "(%.4f,%.4f)->(%.4f,%.4f) hw=%d. The path starts "
                                        "at an interior point, not a physical copper edge.",
                                        i, label,
                                        pt.x / 1e6, pt.y / 1e6,
                                        otherHalfWidth - distToOther,
                                        other->GetStart().x / 1e6,
                                        other->GetStart().y / 1e6,
                                        other->GetEnd().x / 1e6,
                                        other->GetEnd().y / 1e6,
                                        otherHalfWidth ) );
                    }
                };

        validateOnOuterBoundary( itemA, vi.pos, "startPoint" );

        if( itemB && itemB->Type() == PCB_TRACE_T )
        {
            const PCB_TRACK* trackB = static_cast<const PCB_TRACK*>( itemB );
            SEG              segB( trackB->GetStart(), trackB->GetEnd() );

            // The path vertex closest to trackB is taken as the path end point.
            int      bestDist = std::numeric_limits<int>::max();
            VECTOR2I endPt;

            for( const PCB_SHAPE& s : vi.pathShapes )
            {
                int d = segB.Distance( s.GetStart() );

                if( d < bestDist )
                {
                    bestDist = d;
                    endPt = s.GetStart();
                }

                d = segB.Distance( s.GetEnd() );

                if( d < bestDist )
                {
                    bestDist = d;
                    endPt = s.GetEnd();
                }
            }

            validateOnOuterBoundary( itemB, endPt, "endPoint" );
        }
    }
}
