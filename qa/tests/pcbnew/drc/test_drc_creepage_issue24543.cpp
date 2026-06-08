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
 * @file test_drc_creepage_issue24543.cpp
 *
 * Regression test for issue #24543: creepage DRC calculates an incorrect path
 * from rectangular, chamfered-rectangular and trapezoidal pads placed at an
 * arbitrary (non-orthogonal) rotation. Rounded-rectangle pads are not affected.
 *
 * The repro board has a capacitor footprint C4 with two THT rectangular pads
 * straddling an NPTH oval slot. One pad is rotated by a non-orthogonal angle.
 * For such a pad PAD::GetEffectiveShape() returns a SHAPE_SIMPLE (an arbitrary
 * polygon) rather than the axis-aligned SHAPE_RECT used at orthogonal angles.
 * CREEPAGE_GRAPH::Addshape() had no SH_SIMPLE case, so the rotated pad's copper
 * outline was silently dropped from the creepage graph. With no copper anchor
 * the path search snapped the violation to the nearest remaining feature - the
 * pad's NPTH hole - so the reported creepage path started at the hole instead
 * of the copper edge.
 *
 * The geometric assertion below verifies that both endpoints of the reported
 * creepage path lie on (or very near) the pad copper outlines, and crucially
 * that neither endpoint snaps to the interior of an NPTH hole.
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

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_segment.h>


struct DRC_CREEPAGE_ROTATED_PAD_FIXTURE
{
    DRC_CREEPAGE_ROTATED_PAD_FIXTURE() = default;

    ~DRC_CREEPAGE_ROTATED_PAD_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageRotatedRectPadIssue24543, DRC_CREEPAGE_ROTATED_PAD_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24543/issue24543", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24543" );

    struct ViolationInfo
    {
        std::shared_ptr<DRC_ITEM> item;
        VECTOR2I                  pos;
        std::vector<PCB_SHAPE>    pathShapes;
        int                       layer = 0;
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

    // The bug is specific to a non-orthogonal rotation, so confirm at least one C4 pad
    // is rotated off-axis. If neither were, GetEffectiveShape() would emit a SHAPE_RECT
    // and the original (axis-aligned) path would already be correct.
    auto isOrthogonal =
            []( const PAD* aPad )
            {
                double deg = aPad->GetOrientation().Normalize().AsDegrees();
                double mod = std::fmod( deg, 90.0 );
                return mod < 0.01 || mod > 89.99;
            };

    BOOST_REQUIRE_MESSAGE( !isOrthogonal( pad1 ) || !isOrthogonal( pad2 ),
                           "Expected at least one C4 pad to be rotated off-axis" );

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

    BOOST_REQUIRE_GE( c4Violation->pathShapes.size(), 1u );

    // Collect every endpoint of the reported creepage path.
    std::vector<VECTOR2I> endpoints;

    for( const PCB_SHAPE& s : c4Violation->pathShapes )
    {
        endpoints.push_back( s.GetStart() );
        endpoints.push_back( s.GetEnd() );
    }

    const std::shared_ptr<SHAPE_POLY_SET>& poly1 = pad1->GetEffectivePolygon( F_Cu );
    const std::shared_ptr<SHAPE_POLY_SET>& poly2 = pad2->GetEffectivePolygon( F_Cu );

    BOOST_REQUIRE( poly1 && poly2 );
    BOOST_REQUIRE( poly1->OutlineCount() > 0 && poly2->OutlineCount() > 0 );

    // Distance from a point to the copper edge (outline), not the solid interior. A creepage
    // anchor sits on the copper boundary, so the outline distance is ~0 there. We measure
    // against the outline (rather than SHAPE_POLY_SET::SquaredDistance with aOutlineOnly,
    // which is unimplemented) so a point inside the copper annulus does not score as 0.
    const SHAPE_LINE_CHAIN& edge1 = poly1->COutline( 0 );
    const SHAPE_LINE_CHAIN& edge2 = poly2->COutline( 0 );

    // Distance of a point from the nearest NPTH hole centre. The footprint slot is an oval
    // hole centred between the two pads; a path endpoint that snapped to "the hole" sits
    // near the hole boundary, well inside the pad-to-pad gap and far from any pad copper.
    auto distToNearestHoleMM =
            [&]( const VECTOR2I& aPt ) -> double
            {
                double best = std::numeric_limits<double>::max();

                for( const PAD* p : m_board->GetPads() )
                {
                    if( p->GetAttribute() != PAD_ATTRIB::NPTH )
                        continue;

                    std::shared_ptr<SHAPE_SEGMENT> hole = p->GetEffectiveHoleShape();

                    if( !hole )
                        continue;

                    int      r = hole->GetWidth() / 2;
                    SEG::ecoord d = hole->GetSeg().SquaredDistance( aPt );
                    double      edgeDist = std::abs( std::sqrt( (double) d ) - r ) / 1e6;
                    best = std::min( best, edgeDist );
                }

                return best;
            };

    // Examine the two extreme endpoints (closest to each pad). The reported creepage path
    // should anchor on the pad copper edges. Allow a generous tolerance for the rotated-pad
    // outline approximation but flag the failure mode where an endpoint sits at the hole.
    double closestToPad1 = std::numeric_limits<double>::max();
    double closestToPad2 = std::numeric_limits<double>::max();
    VECTOR2I anchor1, anchor2;

    for( const VECTOR2I& pt : endpoints )
    {
        double d1 = std::sqrt( (double) edge1.SquaredDistance( pt ) ) / 1e6;
        double d2 = std::sqrt( (double) edge2.SquaredDistance( pt ) ) / 1e6;

        if( d1 < closestToPad1 )
        {
            closestToPad1 = d1;
            anchor1 = pt;
        }

        if( d2 < closestToPad2 )
        {
            closestToPad2 = d2;
            anchor2 = pt;
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format(
            "Path anchor nearest pad1 at (%.4f,%.4f) mm, dist-to-copper %.4f mm, "
            "dist-to-hole %.4f mm",
            anchor1.x / 1e6, anchor1.y / 1e6, closestToPad1,
            distToNearestHoleMM( anchor1 ) ) );
    BOOST_TEST_MESSAGE( wxString::Format(
            "Path anchor nearest pad2 at (%.4f,%.4f) mm, dist-to-copper %.4f mm, "
            "dist-to-hole %.4f mm",
            anchor2.x / 1e6, anchor2.y / 1e6, closestToPad2,
            distToNearestHoleMM( anchor2 ) ) );

    // Core assertion: the creepage path must terminate on each pad's copper outline.
    // Before the fix the rotated pad contributes no copper anchor, so the path endpoint
    // nearest that pad lands on the NPTH hole boundary instead - tens of mm of copper
    // distance away, but right on the hole.
    BOOST_CHECK_MESSAGE( closestToPad1 < 0.05,
            wxString::Format( "Creepage path endpoint nearest C4 pad1 is %.4f mm from the pad "
                              "copper outline; it should anchor on the copper. dist-to-hole=%.4f mm",
                              closestToPad1, distToNearestHoleMM( anchor1 ) ) );

    BOOST_CHECK_MESSAGE( closestToPad2 < 0.05,
            wxString::Format( "Creepage path endpoint nearest C4 pad2 is %.4f mm from the pad "
                              "copper outline; it should anchor on the copper. dist-to-hole=%.4f mm",
                              closestToPad2, distToNearestHoleMM( anchor2 ) ) );

    // The path must reach the copper of BOTH pads. The #24543 signature is that the rotated
    // pad contributes no copper anchor at all, so its endpoint snaps onto the NPTH hole edge
    // (right on the hole, far from any copper). Anchoring around the hole on intermediate
    // vertices is legitimate; what is not legitimate is an anchor sitting on the hole instead
    // of the copper. Verify each pad anchor is on copper and not pinned to the hole.
    BOOST_CHECK_MESSAGE( distToNearestHoleMM( anchor1 ) > 0.1 || closestToPad1 < 0.05,
            wxString::Format( "Creepage path anchor for C4 pad1 is pinned to the NPTH hole edge "
                              "(%.4f mm) instead of the pad copper (%.4f mm).",
                              distToNearestHoleMM( anchor1 ), closestToPad1 ) );

    BOOST_CHECK_MESSAGE( distToNearestHoleMM( anchor2 ) > 0.1 || closestToPad2 < 0.05,
            wxString::Format( "Creepage path anchor for C4 pad2 is pinned to the NPTH hole edge "
                              "(%.4f mm) instead of the pad copper (%.4f mm).",
                              distToNearestHoleMM( anchor2 ), closestToPad2 ) );
}
