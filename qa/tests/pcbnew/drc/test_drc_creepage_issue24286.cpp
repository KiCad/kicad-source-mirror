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
 * @file test_drc_creepage_issue24286.cpp
 *
 * Regression test for issue #24286: creepage DRC calculates incorrect path
 * between two THT pads when an NPTH oval slot lies between them.
 *
 * The repro board has a polarized capacitor footprint C4 with two THT pads
 * straddling an NPTH oval slot. Both pads are on the 'Sitove' netclass with a
 * 5 mm creepage rule. The expected behaviour is that the creepage path between
 * the pads must travel around the NPTH slot boundary, not straight through the
 * slot interior. Before the fix the candidate-path midpoint test used a board
 * outline polygon that did not have NPTH holes subtracted, so the direct
 * (interior) path was accepted as valid and the reported violation distance
 * was understated.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <layer_ids.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_CREEPAGE_NPTH_PADS_FIXTURE
{
    DRC_CREEPAGE_NPTH_PADS_FIXTURE() = default;

    ~DRC_CREEPAGE_NPTH_PADS_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageNPTHBetweenPadsIssue24286, DRC_CREEPAGE_NPTH_PADS_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24286/issue24286", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24286" );

    struct ViolationInfo
    {
        std::shared_ptr<DRC_ITEM> item;
        VECTOR2I                  pos;
        std::vector<PCB_SHAPE>    pathShapes;
        int                       layer;
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
                vi.layer = aLayer;

                if( aPathGenerator )
                {
                    PCB_MARKER marker( aItem, aPos, aLayer );
                    aPathGenerator( &marker );
                    vi.pathShapes = marker.GetPath();
                }

                violations.push_back( vi );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations",
                                          (int) violations.size() ) );

    for( const ViolationInfo& vi : violations )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "  layer=%d arrow=(%.4f,%.4f) shapes=%d msg=%s",
                                              vi.layer,
                                              vi.pos.x / 1e6, vi.pos.y / 1e6,
                                              (int) vi.pathShapes.size(),
                                              vi.item->GetErrorMessage( false ) ) );

        for( size_t j = 0; j < vi.pathShapes.size(); j++ )
        {
            const PCB_SHAPE& s = vi.pathShapes[j];

            if( s.GetShape() == SHAPE_T::SEGMENT )
            {
                BOOST_TEST_MESSAGE( wxString::Format(
                        "    [%zu] SEG: (%.4f,%.4f)->(%.4f,%.4f)", j,
                        s.GetStart().x / 1e6, s.GetStart().y / 1e6,
                        s.GetEnd().x / 1e6, s.GetEnd().y / 1e6 ) );
            }
            else if( s.GetShape() == SHAPE_T::ARC )
            {
                BOOST_TEST_MESSAGE( wxString::Format(
                        "    [%zu] ARC: (%.4f,%.4f)->(%.4f,%.4f) c=(%.4f,%.4f)", j,
                        s.GetStart().x / 1e6, s.GetStart().y / 1e6,
                        s.GetEnd().x / 1e6, s.GetEnd().y / 1e6,
                        s.GetCenter().x / 1e6, s.GetCenter().y / 1e6 ) );
            }
        }
    }

    // Locate the C4 pads to drive the geometric assertions.
    PAD* pad1 = nullptr;
    PAD* pad2 = nullptr;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        if( fp->GetReference() != wxT( "C4" ) )
            continue;

        for( PAD* p : fp->Pads() )
        {
            if( p->GetNumber() == wxT( "1" ) )
                pad1 = p;
            else if( p->GetNumber() == wxT( "2" ) )
                pad2 = p;
        }
    }

    BOOST_REQUIRE_MESSAGE( pad1 && pad2, "C4 pads 1 and 2 not found in board" );

    const VECTOR2I p1Pos = pad1->GetPosition();
    const VECTOR2I p2Pos = pad2->GetPosition();
    const SEG      directSeg( p1Pos, p2Pos );
    const double   directDist = ( p2Pos - p1Pos ).EuclideanNorm() / 1e6;

    BOOST_TEST_MESSAGE( wxString::Format(
            "C4 pad1 at (%.4f, %.4f) mm, pad2 at (%.4f, %.4f) mm, center-to-center %.4f mm",
            p1Pos.x / 1e6, p1Pos.y / 1e6, p2Pos.x / 1e6, p2Pos.y / 1e6, directDist ) );

    // Find the violation reported between pad1 and pad2 on the F.Cu layer. The bug
    // surfaces specifically on F.Cu (the user-reported "incorrect" path); B.Cu is
    // separately reported and not the regression target here.
    const ViolationInfo* c4Violation = nullptr;

    for( const ViolationInfo& vi : violations )
    {
        if( vi.layer != F_Cu )
            continue;

        const KIID idA = vi.item->GetMainItemID();
        const KIID idB = vi.item->GetAuxItemID();
        const bool matchA = ( idA == pad1->m_Uuid || idA == pad2->m_Uuid );
        const bool matchB = ( idB == pad1->m_Uuid || idB == pad2->m_Uuid );

        if( matchA && matchB && idA != idB )
        {
            c4Violation = &vi;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( c4Violation,
                           "No F.Cu creepage violation reported between C4 pad1 and pad2" );

    BOOST_TEST_MESSAGE( wxString::Format(
            "C4 violation: layer=%d shapes=%d arrow=(%.4f, %.4f) mm",
            c4Violation->layer,
            (int) c4Violation->pathShapes.size(),
            c4Violation->pos.x / 1e6, c4Violation->pos.y / 1e6 ) );

    BOOST_REQUIRE_GE( c4Violation->pathShapes.size(), 1u );

    // Sum the geometric length of the reported path.
    double pathLen = 0.0;

    for( const PCB_SHAPE& s : c4Violation->pathShapes )
    {
        if( s.GetShape() == SHAPE_T::SEGMENT )
        {
            pathLen += ( s.GetEnd() - s.GetStart() ).EuclideanNorm() / 1e6;
        }
        else if( s.GetShape() == SHAPE_T::ARC )
        {
            EDA_SHAPE arc( SHAPE_T::ARC, 0, FILL_T::NO_FILL );
            arc.SetArcGeometry( s.GetStart(), s.GetArcMid(), s.GetEnd() );
            pathLen += arc.GetLength() / 1e6;
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "C4 path total length: %.4f mm (direct: %.4f mm)",
                                          pathLen, directDist ) );

    // Path must not cut through the NPTH slot interior. The capacitor is centred on the
    // NPTH oval so the direct centre-to-centre segment runs straight through the slot.
    // A correct creepage path must wrap around the slot, so every segment of the reported
    // path must lie at least 0.4 mm (half the slot's short axis, minus a 100 um tolerance)
    // off the direct line.
    bool pathLeavesDirectLine = false;

    for( const PCB_SHAPE& s : c4Violation->pathShapes )
    {
        const int distStart = directSeg.Distance( s.GetStart() );
        const int distEnd   = directSeg.Distance( s.GetEnd() );

        if( distStart > 400000 || distEnd > 400000 )
        {
            pathLeavesDirectLine = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( pathLeavesDirectLine,
            "Creepage path between C4 pads stays on the centre line of the NPTH slot, "
            "indicating it cuts through the slot interior instead of going around it." );

    // The actual surface creepage between two THT pads centred on a 4mm x 1mm NPTH slot
    // must be appreciably longer than the centre-to-centre distance. A direct-through-slot
    // path would report ~ directDist - 1.6 mm (subtracting the two pad radii). The correct
    // path wraps around the slot end caps, adding at least the short-axis radius twice
    // (~1 mm) plus an arc length. Require the reported distance to exceed the direct
    // centre-to-centre distance (with a small margin) as a coarse sanity check on the
    // routing geometry.
    BOOST_CHECK_MESSAGE( pathLen >= directDist - 0.1,
            wxString::Format( "Reported creepage path length %.4f mm is shorter than the "
                              "pad-centre to pad-centre distance %.4f mm, which is impossible "
                              "for a path that routes around the NPTH slot.",
                              pathLen, directDist ) );

    // Parse the violation message's reported "actual N.NNNN mm" and verify it lies in the
    // expected range. Before the fix the creepage validator rejected legitimate tangent
    // paths to BE_SHAPE_ARC obstacles (it failed to ignore the obstacle's own parent for
    // arc shapes), forcing Dijkstra onto a longer fallback that connected through arc
    // endpoints. The reported distance landed around 4.5 mm. With the fix the path uses
    // proper tangent points and the reported distance drops below 4 mm.
    wxString errMsg = c4Violation->item->GetErrorMessage( false );
    double   reportedActual = 0.0;
    int      actualPos = errMsg.Find( wxT( "actual " ) );

    BOOST_REQUIRE_MESSAGE( actualPos != wxNOT_FOUND,
                           wxString::Format( "Could not find 'actual' in error message: %s",
                                             errMsg ) );

    wxString tail = errMsg.Mid( actualPos + 7 );
    int      spacePos = tail.Find( ' ' );

    if( spacePos != wxNOT_FOUND )
        tail = tail.Left( spacePos );

    BOOST_REQUIRE_MESSAGE( tail.ToDouble( &reportedActual ),
                           wxString::Format( "Could not parse reported actual from '%s'", tail ) );

    BOOST_CHECK_MESSAGE( reportedActual < 4.0,
            wxString::Format( "Reported creepage actual %.4f mm exceeds 4 mm; this indicates "
                              "the validator is still falling back to arc-endpoint connections "
                              "instead of tangent-on-arc paths.",
                              reportedActual ) );
}
