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
 * @file test_drc_creepage_issue24544.cpp
 *
 * Regression test for issue #24544: the creepage DRC path around a partially
 * rounded-rectangle slot on Edge.Cuts (a gr_rect with a corner radius) that
 * separates the two C4 pads of a footprint rotated to a non-orthogonal angle.
 *
 * #24544 was reported as #24543 "plus one extra problem": the path both started
 * at the pad hole and did not respect the slot's curved ends. The root cause of
 * BOTH symptoms is the same dropped rotated-pad copper outline fixed in #24543
 * (CREEPAGE_GRAPH::Addshape() had no SH_SIMPLE case). With the pad copper anchor
 * restored, the shortest creepage path correctly wraps a rounded end of the
 * slot, hugging the arc rather than cutting across the slot interior; the slot's
 * own rounded-rectangle modelling (commit "Model rounded-rect Edge.Cuts slots in
 * creepage") was already correct. There is no separate curve-handling defect.
 *
 * This test locks in the corrected behaviour: the reported creepage path between
 * the two pads must (1) never pass through the slot interior and (2) run flush
 * against a rounded end of the slot. It fails without the #24543 fix (the pad
 * pair reports no usable path) and passes with it.
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
#include <pcb_shape.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_arc.h>


struct DRC_CREEPAGE_ROUNDED_SLOT_FIXTURE
{
    DRC_CREEPAGE_ROUNDED_SLOT_FIXTURE() = default;

    ~DRC_CREEPAGE_ROUNDED_SLOT_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageRoundedSlotIssue24544, DRC_CREEPAGE_ROUNDED_SLOT_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24544/issue24544", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24544" );

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

    // Recover the rounded-rectangle slot geometry from Edge.Cuts. The repro draws the slot
    // as a gr_rect with a positive corner radius. We use the geometry directly (rather than a
    // hard-coded constant) so the test tracks the data file.
    VECTOR2I slotStart, slotEnd;
    int      slotRadius = 0;
    bool     slotFound = false;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        PCB_SHAPE* s = dynamic_cast<PCB_SHAPE*>( item );

        if( !s || !s->IsOnLayer( Edge_Cuts ) )
            continue;

        if( s->GetShape() != SHAPE_T::RECTANGLE || s->GetCornerRadius() <= 0 )
            continue;

        slotStart = s->GetStart();
        slotEnd = s->GetEnd();
        slotRadius = s->GetCornerRadius();
        slotFound = true;
        break;
    }

    BOOST_REQUIRE_MESSAGE( slotFound, "Rounded-rectangle slot not found on Edge.Cuts" );

    const int x1 = std::min( slotStart.x, slotEnd.x );
    const int y1 = std::min( slotStart.y, slotEnd.y );
    const int x2 = std::max( slotStart.x, slotEnd.x );
    const int y2 = std::max( slotStart.y, slotEnd.y );
    const int r = slotRadius;

    BOOST_TEST_MESSAGE( wxString::Format(
            "Rounded slot box (%.4f,%.4f)-(%.4f,%.4f) mm, corner radius %.4f mm",
            x1 / 1e6, y1 / 1e6, x2 / 1e6, y2 / 1e6, r / 1e6 ) );

    // Build the exact rounded-rectangle slot outline as a polygon so we can test whether a
    // creepage path segment ever crosses into the slot interior. A correct path that respects
    // the rounded ends wraps the curved boundary and stays out of the slot region entirely.
    SHAPE_POLY_SET slotPoly;
    slotPoly.NewOutline();

    const int ERR = 1000;   // arc approximation error, 1 um

    // Top edge (left arc end -> right arc end), then right arc, bottom edge, left arc.
    slotPoly.Append( x1 + r, y1 );
    slotPoly.Append( x2 - r, y1 );

    auto appendArc = [&]( const VECTOR2I& aCenter, const EDA_ANGLE& aStart, const EDA_ANGLE& aEnd )
    {
        VECTOR2I  startPt( aCenter.x + KiROUND( r * aStart.Cos() ),
                           aCenter.y + KiROUND( r * aStart.Sin() ) );
        SHAPE_ARC realArc( aCenter, startPt, aEnd - aStart, 0 );
        SHAPE_LINE_CHAIN chain = realArc.ConvertToPolyline( ERR );

        for( int i = 0; i < chain.PointCount(); ++i )
            slotPoly.Append( chain.CPoint( i ) );
    };

    // Right-side end cap: from (x2-r, y1) sweeping to (x2-r, y2) through (x2, y1+r)..(x2, y2-r).
    appendArc( { x2 - r, y1 + r }, EDA_ANGLE( -90.0, DEGREES_T ), EDA_ANGLE( 0.0, DEGREES_T ) );

    if( ( y2 - y1 ) > 2 * r )
        appendArc( { x2 - r, y2 - r }, EDA_ANGLE( 0.0, DEGREES_T ), EDA_ANGLE( 90.0, DEGREES_T ) );

    slotPoly.Append( x2 - r, y2 );
    slotPoly.Append( x1 + r, y2 );

    // Left-side end cap.
    appendArc( { x1 + r, y2 - r }, EDA_ANGLE( 90.0, DEGREES_T ), EDA_ANGLE( 180.0, DEGREES_T ) );

    if( ( y2 - y1 ) > 2 * r )
        appendArc( { x1 + r, y1 + r }, EDA_ANGLE( 180.0, DEGREES_T ),
                   EDA_ANGLE( 270.0, DEGREES_T ) );

    slotPoly.Outline( 0 ).SetClosed( true );

    // Identify the creepage violation whose path wraps the slot. We accept any F.Cu creepage
    // violation reported between two C4 pads (the pair separated by the slot).
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

    // The #24543 root cause only triggers when a pad is rotated off-axis (so GetEffectiveShape()
    // emits a SHAPE_SIMPLE rather than an axis-aligned SHAPE_RECT). Confirm the precondition so a
    // future data drift to orthogonal pads cannot silently neuter this regression.
    auto isOrthogonal =
            []( const PAD* aPad )
            {
                double deg = aPad->GetOrientation().Normalize().AsDegrees();
                double mod = std::fmod( deg, 90.0 );
                return mod < 0.01 || mod > 89.99;
            };

    BOOST_REQUIRE_MESSAGE( !isOrthogonal( pad1 ) || !isOrthogonal( pad2 ),
                           "Expected at least one C4 pad to be rotated off-axis" );

    const ViolationInfo* slotViolation = nullptr;

    for( const ViolationInfo& vi : violations )
    {
        if( vi.layer != F_Cu )
            continue;

        const KIID idA = vi.item->GetMainItemID();
        const KIID idB = vi.item->GetAuxItemID();
        const bool matchA = ( idA == pad1->m_Uuid || idA == pad2->m_Uuid );
        const bool matchB = ( idB == pad1->m_Uuid || idB == pad2->m_Uuid );

        if( matchA && matchB && idA != idB && !vi.pathShapes.empty() )
        {
            slotViolation = &vi;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( slotViolation,
                           "No creepage violation reported between C4 pad1 and pad2" );

    // Densify the path into a point list. ARC path shapes are expanded along their true arc so
    // that a path correctly hugging the slot's rounded end is NOT misread as cutting the chord.
    std::vector<VECTOR2I> pathPts;

    auto pushPt = [&]( const VECTOR2I& aPt )
    {
        if( pathPts.empty() || pathPts.back() != aPt )
            pathPts.push_back( aPt );
    };

    for( const PCB_SHAPE& s : slotViolation->pathShapes )
    {
        BOOST_TEST_MESSAGE( wxString::Format(
                "  path shape type=%d start(%.4f,%.4f) end(%.4f,%.4f) mid(%.4f,%.4f)",
                (int) s.GetShape(), s.GetStart().x / 1e6, s.GetStart().y / 1e6,
                s.GetEnd().x / 1e6, s.GetEnd().y / 1e6,
                s.GetArcMid().x / 1e6, s.GetArcMid().y / 1e6 ) );

        if( s.GetShape() == SHAPE_T::ARC )
        {
            SHAPE_ARC        arc( s.GetStart(), s.GetArcMid(), s.GetEnd(), 0 );
            SHAPE_LINE_CHAIN chain = arc.ConvertToPolyline( 1000 );

            for( int i = 0; i < chain.PointCount(); ++i )
                pushPt( chain.CPoint( i ) );
        }
        else
        {
            pushPt( s.GetStart() );
            pushPt( s.GetEnd() );
        }
    }

    BOOST_REQUIRE_MESSAGE( pathPts.size() >= 2, "Reported creepage path has no usable geometry" );

    // (1) The path must respect the slot boundary - no point may fall inside the slot interior.
    // Sample each path span and the densified arc points against the true rounded-slot polygon.
    double   maxInsideMM = 0.0;
    VECTOR2I worstInside;

    for( size_t i = 0; i + 1 < pathPts.size(); ++i )
    {
        const VECTOR2I& a = pathPts[i];
        const VECTOR2I& b = pathPts[i + 1];
        const int       steps = 32;

        for( int k = 0; k <= steps; ++k )
        {
            VECTOR2I pt = a + ( b - a ) * k / steps;

            if( slotPoly.Contains( pt ) )
            {
                double depth = std::sqrt( (double) slotPoly.COutline( 0 ).SquaredDistance( pt ) )
                               / 1e6;

                if( depth > maxInsideMM )
                {
                    maxInsideMM = depth;
                    worstInside = pt;
                }
            }
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format(
            "Path has %d shapes; deepest excursion into the slot interior is %.4f mm at "
            "(%.4f,%.4f)",
            (int) slotViolation->pathShapes.size(), maxInsideMM, worstInside.x / 1e6,
            worstInside.y / 1e6 ) );

    BOOST_CHECK_MESSAGE( maxInsideMM < 0.02,
            wxString::Format( "Creepage path passes through the slot interior by %.4f mm at "
                              "(%.4f,%.4f); it must route around the slot.",
                              maxInsideMM, worstInside.x / 1e6, worstInside.y / 1e6 ) );

    // (2) The path must actually follow the CURVED part of a rounded end, not merely the straight
    // side between the two corners. The four rounded corners occupy the quadrants where x is within
    // r of a short end AND y is within r of a long side; only there does the boundary curve. A
    // point that is flush against the outline inside one of those quadrants proves the path hugged
    // an arc. A path that cuts a chord across the corner never touches the rounded boundary there.
    auto onRoundedCorner =
            [&]( const VECTOR2I& aPt ) -> bool
            {
                bool inEndBand = ( aPt.x < x1 + r ) || ( aPt.x > x2 - r );
                bool inSideBand = ( aPt.y < y1 + r ) || ( aPt.y > y2 - r );

                if( !inEndBand || !inSideBand )
                    return false;

                double distToOutline =
                        std::sqrt( (double) slotPoly.COutline( 0 ).SquaredDistance( aPt ) ) / 1e6;

                return distToOutline < 0.02;
            };

    bool wrapsRoundedCorner = false;

    for( const VECTOR2I& pt : pathPts )
    {
        if( onRoundedCorner( pt ) )
        {
            wrapsRoundedCorner = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( wrapsRoundedCorner,
            "Creepage path does not run flush against a rounded corner of the slot; it is not "
            "respecting the slot's curved boundary." );
}
