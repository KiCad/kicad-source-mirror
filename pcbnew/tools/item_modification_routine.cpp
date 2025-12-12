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

#include "item_modification_routine.h"

#include <geometry/geometry_utils.h>
#include <geometry/circle.h>
#include <geometry/oval.h>
#include <geometry/roundrect.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_utils.h>
#include <geometry/vector_utils.h>
#include <math.h>

#include <pad.h>
#include <pcb_track.h>
#include <tools/pcb_tool_utils.h>
#include <confirm.h>
#include <board.h>
#include <wx/log.h>

namespace
{

/**
 * Check if two segments share an endpoint (can be at either end of either segment)
 */
bool SegmentsShareEndpoint( const SEG& aSegA, const SEG& aSegB )
{
    return ( aSegA.A == aSegB.A || aSegA.A == aSegB.B || aSegA.B == aSegB.A || aSegA.B == aSegB.B );
}


std::pair<VECTOR2I*, VECTOR2I*> GetSharedEndpoints( SEG& aSegA, SEG& aSegB )
{
    std::pair<VECTOR2I*, VECTOR2I*> result = { nullptr, nullptr };

    if( aSegA.A == aSegB.A )
    {
        result = { &aSegA.A, &aSegB.A };
    }
    else if( aSegA.A == aSegB.B )
    {
        result = { &aSegA.A, &aSegB.B };
    }
    else if( aSegA.B == aSegB.A )
    {
        result = { &aSegA.B, &aSegB.A };
    }
    else if( aSegA.B == aSegB.B )
    {
        result = { &aSegA.B, &aSegB.B };
    }

    return result;
}

} // namespace


bool ITEM_MODIFICATION_ROUTINE::ModifyLineOrDeleteIfZeroLength( PCB_SHAPE&                aLine,
                                                                const std::optional<SEG>& aSeg )
{
    wxASSERT_MSG( aLine.GetShape() == SHAPE_T::SEGMENT, "Can only modify segments" );

    const bool removed = !aSeg.has_value() || aSeg->Length() == 0;

    if( !removed )
    {
        // Mark modified, then change it
        GetHandler().MarkItemModified( aLine );
        aLine.SetStart( aSeg->A );
        aLine.SetEnd( aSeg->B );
    }
    else
    {
        // The line has become zero length - delete it
        GetHandler().DeleteItem( aLine );
    }

    return removed;
}


wxString LINE_FILLET_ROUTINE::GetCommitDescription() const
{
    return _( "Fillet Lines" );
}


std::optional<wxString> LINE_FILLET_ROUTINE::GetStatusMessage( int aSegmentCount ) const
{
    if( GetSuccesses() == 0 )
        return _( "Unable to fillet the selected lines." );
    else if( GetFailures() > 0 || (int) GetSuccesses() < aSegmentCount - 1 )
        return _( "Some of the lines could not be filleted." );

    return std::nullopt;
}


void LINE_FILLET_ROUTINE::ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB )
{
    if( aLineA.GetLength() == 0.0 || aLineB.GetLength() == 0.0 )
        return;

    SEG       seg_a( aLineA.GetStart(), aLineA.GetEnd() );
    SEG       seg_b( aLineB.GetStart(), aLineB.GetEnd() );

    auto [a_pt, b_pt] = GetSharedEndpoints( seg_a, seg_b );

    if( !a_pt || !b_pt )
    {
        // The lines do not share an endpoint, so we can't fillet them
        return;
    }

    if( seg_a.Angle( seg_b ).IsHorizontal() )
        return;

    SHAPE_ARC sArc( seg_a, seg_b, m_filletRadiusIU );
    VECTOR2I  t1newPoint, t2newPoint;

    auto setIfPointOnSeg =
            []( VECTOR2I& aPointToSet, SEG aSegment, VECTOR2I aVecToTest )
            {
                VECTOR2I segToVec = aSegment.NearestPoint( aVecToTest ) - aVecToTest;

                // Find out if we are on the segment (minimum precision)
                if( segToVec.EuclideanNorm() < SHAPE_ARC::MIN_PRECISION_IU )
                {
                    aPointToSet.x = aVecToTest.x;
                    aPointToSet.y = aVecToTest.y;
                    return true;
                }

                return false;
            };

    //Do not draw a fillet if the end points of the arc are not within the track segments
    if( !setIfPointOnSeg( t1newPoint, seg_a, sArc.GetP0() )
        && !setIfPointOnSeg( t2newPoint, seg_b, sArc.GetP0() ) )
    {
        AddFailure();
        return;
    }

    if( !setIfPointOnSeg( t1newPoint, seg_a, sArc.GetP1() )
        && !setIfPointOnSeg( t2newPoint, seg_b, sArc.GetP1() ) )
    {
        AddFailure();
        return;
    }

    auto tArc = std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::ARC );

    tArc->SetArcGeometry( sArc.GetP0(), sArc.GetArcMid(), sArc.GetP1() );

    // Copy properties from one of the source lines
    tArc->SetWidth( aLineA.GetWidth() );
    tArc->SetLayer( aLineA.GetLayer() );
    tArc->SetLocked( aLineA.IsLocked() );

    CHANGE_HANDLER& handler = GetHandler();

    handler.AddNewItem( std::move( tArc ) );

    *a_pt = t1newPoint;
    *b_pt = t2newPoint;

    ModifyLineOrDeleteIfZeroLength( aLineA, seg_a );
    ModifyLineOrDeleteIfZeroLength( aLineB, seg_b );

    AddSuccess();
}

wxString LINE_CHAMFER_ROUTINE::GetCommitDescription() const
{
    return _( "Chamfer Lines" );
}


std::optional<wxString> LINE_CHAMFER_ROUTINE::GetStatusMessage( int aSegmentCount ) const
{
    if( GetSuccesses() == 0 )
        return _( "Unable to chamfer the selected lines." );
    else if( GetFailures() > 0 || (int) GetSuccesses() < aSegmentCount - 1 )
        return _( "Some of the lines could not be chamfered." );

    return std::nullopt;
}


void LINE_CHAMFER_ROUTINE::ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB )
{
    if( aLineA.GetLength() == 0.0 || aLineB.GetLength() == 0.0 )
        return;

    SEG seg_a( aLineA.GetStart(), aLineA.GetEnd() );
    SEG seg_b( aLineB.GetStart(), aLineB.GetEnd() );

    // If the segments share an endpoint, we won't try to chamfer them
    // (we could extend to the intersection point, but this gets complicated
    // and inconsistent when you select more than two lines)
    if( !SegmentsShareEndpoint( seg_a, seg_b ) )
    {
        // not an error, lots of lines in a 2+ line selection will not intersect
        return;
    }

    std::optional<CHAMFER_RESULT> chamfer_result =
            ComputeChamferPoints( seg_a, seg_b, m_chamferParams );

    if( !chamfer_result )
    {
        AddFailure();
        return;
    }

    auto tSegment = std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::SEGMENT );

    tSegment->SetStart( chamfer_result->m_chamfer.A );
    tSegment->SetEnd( chamfer_result->m_chamfer.B );

    // Copy properties from one of the source lines
    tSegment->SetWidth( aLineA.GetWidth() );
    tSegment->SetLayer( aLineA.GetLayer() );
    tSegment->SetLocked( aLineA.IsLocked() );

    CHANGE_HANDLER& handler = GetHandler();

    handler.AddNewItem( std::move( tSegment ) );

    ModifyLineOrDeleteIfZeroLength( aLineA, chamfer_result->m_updated_seg_a );
    ModifyLineOrDeleteIfZeroLength( aLineB, chamfer_result->m_updated_seg_b );

    AddSuccess();
}


wxString DOGBONE_CORNER_ROUTINE::GetCommitDescription() const
{
    return _( "Dogbone Corners" );
}


std::optional<wxString> DOGBONE_CORNER_ROUTINE::GetStatusMessage( int aSegmentCount ) const
{
    wxString msg;

    if( GetSuccesses() == 0 )
        msg += _( "Unable to add dogbone corners to the selected lines." );
    else if( GetFailures() > 0 || (int) GetSuccesses() < aSegmentCount - 1 )
        msg += _( "Some of the lines could not have dogbone corners added." );

    if( m_haveNarrowMouths )
    {
        if( !msg.empty() )
            msg += " ";

        msg += _( "Some of the dogbone corners are too narrow to fit a "
                  "cutter of the specified radius." );

        if( !m_params.AddSlots )
            msg += _( " Consider enabling the 'Add Slots' option." );
        else
            msg += _( " Slots were added." );
    }

    if( msg.empty() )
        return std::nullopt;

    return msg;
}


void DOGBONE_CORNER_ROUTINE::ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB )
{
    if( aLineA.GetLength() == 0.0 || aLineB.GetLength() == 0.0 )
    {
        wxLogTrace( "DOGBONE", "Skip: zero-length line(s) (A len=%f, B len=%f)",
                    aLineA.GetLength(), aLineB.GetLength() );
        return;
    }

    if( !EnsureBoardOutline() )
    {
        wxLogTrace( "DOGBONE", "Skip: board outline unavailable" );
        return;
    }

    SEG seg_a( aLineA.GetStart(), aLineA.GetEnd() );
    SEG seg_b( aLineB.GetStart(), aLineB.GetEnd() );

    auto [a_pt, b_pt] = GetSharedEndpoints( seg_a, seg_b );

    if( !a_pt || !b_pt )
    {
        wxLogTrace( "DOGBONE", "Skip: segments do not share endpoint" );
        return;
    }

    // Cannot handle parallel lines
    if( seg_a.Angle( seg_b ).IsHorizontal() )
    {
        wxLogTrace( "DOGBONE", "Skip: parallel segments" );
        AddFailure();
        return;
    }

    // Determine if this corner points into the board outline: we construct the bisector
    // vector (as done in ComputeDogbone) and test a point a small distance in the opposite
    // direction (i.e. exterior). If that opposite point is INSIDE the board outline, the
    // corner indentation points inward (needs dogbone). If the opposite test point is
    // outside, skip.

    const VECTOR2I corner = *a_pt; // shared endpoint
    // Build vectors from corner toward other ends
    const VECTOR2I vecA = ( seg_a.A == corner ? seg_a.B - corner : seg_a.A - corner );
    const VECTOR2I vecB = ( seg_b.A == corner ? seg_b.B - corner : seg_b.A - corner );
    // Normalize (resize) to common length to form bisector reliably
    int maxLen = std::max( vecA.EuclideanNorm(), vecB.EuclideanNorm() );
    if( maxLen == 0 )
    {
        wxLogTrace( "DOGBONE", "Skip: degenerate corner (maxLen==0)" );
        return;
    }
    VECTOR2I vecAn = vecA.Resize( maxLen );
    VECTOR2I vecBn = vecB.Resize( maxLen );
    VECTOR2I bisectorOutward = vecAn + vecBn; // direction inside angle region

    // If vectors are nearly opposite, no meaningful dogbone
    if( bisectorOutward.EuclideanNorm() == 0 )
    {
        wxLogTrace( "DOGBONE", "Skip: bisector zero (vectors opposite)" );
        return;
    }

    // Opposite direction of bisector (points "outside" if angle is convex, or further into
    // material if reflex). We'll sample a point a small distance along -bisectorOutward.
    VECTOR2I sampleDir = ( -bisectorOutward ).Resize( std::min( 1000, m_params.DogboneRadiusIU ) );
    VECTOR2I samplePoint = corner + sampleDir; // test point opposite bisector

    bool oppositeInside = m_boardOutline.Contains( samplePoint );

    if( !oppositeInside )
    {
        wxLogTrace( "DOGBONE", "Skip: corner not inward (sample outside polygon)" );
        return;
    }

    std::optional<DOGBONE_RESULT> dogbone_result =
            ComputeDogbone( seg_a, seg_b, m_params.DogboneRadiusIU, m_params.AddSlots );

    if( !dogbone_result )
    {
        wxLogTrace( "DOGBONE", "Skip: ComputeDogbone failed (radius=%d slots=%d)",
                    m_params.DogboneRadiusIU, (int) m_params.AddSlots );
        AddFailure();
        return;
    }

    if( dogbone_result->m_small_arc_mouth )
    {
        wxLogTrace( "DOGBONE", "Info: small arc mouth (slots %s)",
                    m_params.AddSlots ? "enabled" : "disabled" );
        // The arc is too small to fit the radius
        m_haveNarrowMouths = true;
    }

    CHANGE_HANDLER& handler = GetHandler();

    auto tArc = std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::ARC );

    const auto copyProps = [&]( PCB_SHAPE& aShape )
    {
        aShape.SetWidth( aLineA.GetWidth() );
        aShape.SetLayer( aLineA.GetLayer() );
        aShape.SetLocked( aLineA.IsLocked() );
    };

    const auto addSegment = [&]( const SEG& aSeg )
    {
        if( aSeg.Length() == 0 )
            return;

        auto tSegment = std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::SEGMENT );
        tSegment->SetStart( aSeg.A );
        tSegment->SetEnd( aSeg.B );

        copyProps( *tSegment );
        handler.AddNewItem( std::move( tSegment ) );
    };

    tArc->SetArcGeometry( dogbone_result->m_arc.GetP0(), dogbone_result->m_arc.GetArcMid(),
                          dogbone_result->m_arc.GetP1() );

    // Copy properties from one of the source lines
    copyProps( *tArc );

    addSegment( SEG{ dogbone_result->m_arc.GetP0(), dogbone_result->m_updated_seg_a->B } );
    addSegment( SEG{ dogbone_result->m_arc.GetP1(), dogbone_result->m_updated_seg_b->B } );

    handler.AddNewItem( std::move( tArc ) );

    ModifyLineOrDeleteIfZeroLength( aLineA, dogbone_result->m_updated_seg_a );
    ModifyLineOrDeleteIfZeroLength( aLineB, dogbone_result->m_updated_seg_b );

    wxLogTrace( "DOGBONE", "Success: dogbone added at (%d,%d)", corner.x, corner.y );
    AddSuccess();
}

bool DOGBONE_CORNER_ROUTINE::EnsureBoardOutline() const
{
    if( m_boardOutlineCached )
        return !m_boardOutline.IsEmpty();

    m_boardOutlineCached = true;

    BOARD* board = dynamic_cast<BOARD*>( GetBoard() );
    if( !board )
    {
        wxLogTrace( "DOGBONE", "EnsureBoardOutline: board cast failed" );
        return false;
    }

    // Build outlines; ignore errors and arcs for this classification
    if( !board->GetBoardPolygonOutlines( m_boardOutline, false ) )
    {
        wxLogTrace( "DOGBONE", "EnsureBoardOutline: GetBoardPolygonOutlines failed" );
        return false;
    }

    bool ok = !m_boardOutline.IsEmpty();
    wxLogTrace( "DOGBONE", "EnsureBoardOutline: outline %s", ok ? "ready" : "empty" );
    return ok;
}


wxString LINE_EXTENSION_ROUTINE::GetCommitDescription() const
{
    return _( "Extend Lines to Meet" );
}


std::optional<wxString> LINE_EXTENSION_ROUTINE::GetStatusMessage( int aSegmentCount ) const
{
    if( GetSuccesses() == 0 )
        return _( "Unable to extend the selected lines to meet." );
    else if( GetFailures() > 0 || (int) GetSuccesses() < aSegmentCount - 1 )
        return _( "Some of the lines could not be extended to meet." );

    return std::nullopt;
}


void LINE_EXTENSION_ROUTINE::ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB )
{
    if( aLineA.GetLength() == 0.0 || aLineB.GetLength() == 0.0 )
        return;

    SEG seg_a( aLineA.GetStart(), aLineA.GetEnd() );
    SEG seg_b( aLineB.GetStart(), aLineB.GetEnd() );

    if( seg_a.Intersects( seg_b ) )
    {
        // already intersecting, nothing to do
        return;
    }

    OPT_VECTOR2I intersection = seg_a.IntersectLines( seg_b );

    if( !intersection )
    {
        // This might be an error, but it's also possible that the lines are
        // parallel and don't intersect.  We'll just ignore this case.
        return;
    }

    CHANGE_HANDLER& handler = GetHandler();

    const auto line_extender = [&]( const SEG& aSeg, PCB_SHAPE& aLine )
    {
        // If the intersection point is not already n the line, we'll extend to it
        if( !aSeg.Contains( *intersection ) )
        {
            const int dist_start = ( *intersection - aSeg.A ).EuclideanNorm();
            const int dist_end = ( *intersection - aSeg.B ).EuclideanNorm();

            const VECTOR2I& furthest_pt = ( dist_start < dist_end ) ? aSeg.B : aSeg.A;
            // Note, the drawing tool has COORDS_PADDING of 20mm, but we need a larger buffer
            // or we are not able to select the generated segments
            unsigned int    edge_padding = static_cast<unsigned>( pcbIUScale.mmToIU( 200 ) );
            VECTOR2I        new_end = GetClampedCoords( *intersection, edge_padding );

            handler.MarkItemModified( aLine );
            aLine.SetStart( furthest_pt );
            aLine.SetEnd( new_end );
        }
    };

    line_extender( seg_a, aLineA );
    line_extender( seg_b, aLineB );

    AddSuccess();
}


void POLYGON_BOOLEAN_ROUTINE::ProcessShape( PCB_SHAPE& aPcbShape )
{
    std::unique_ptr<SHAPE_POLY_SET> poly;

    switch( aPcbShape.GetShape() )
    {
    case SHAPE_T::POLY:
    {
        poly = std::make_unique<SHAPE_POLY_SET>( aPcbShape.GetPolyShape() );
        // Arcs cannot be handled by polygon boolean transforms
        poly->ClearArcs();
        break;
    }
    case SHAPE_T::RECTANGLE:
    {
        poly = std::make_unique<SHAPE_POLY_SET>();

        const std::vector<VECTOR2I> rect_pts = aPcbShape.GetRectCorners();

        poly->NewOutline();

        for( const VECTOR2I& pt : rect_pts )
        {
            poly->Append( pt );
        }
        break;
    }
    case SHAPE_T::CIRCLE:
    {
        poly = std::make_unique<SHAPE_POLY_SET>();
        const SHAPE_ARC arc{ aPcbShape.GetCenter(), aPcbShape.GetCenter() + VECTOR2I{ aPcbShape.GetRadius(), 0 },
                             FULL_CIRCLE, 0 };

        poly->NewOutline();
        poly->Append( arc );
        break;
    }
    default:
    {
        break;
    }
    }

    if( !poly )
    {
        // Not a polygon or rectangle, nothing to do
        return;
    }

    if( m_firstPolygon )
    {
        m_width = aPcbShape.GetWidth();
        m_layer = aPcbShape.GetLayer();
        m_fillMode = aPcbShape.GetFillMode();
        m_workingPolygons = std::move( *poly );
        m_firstPolygon = false;

        // Boolean ops work, but assert on arcs
        m_workingPolygons.ClearArcs();

        GetHandler().DeleteItem( aPcbShape );
    }
    else
    {
        if( ProcessSubsequentPolygon( *poly ) )
        {
            // If we could process the polygon, delete the source
            GetHandler().DeleteItem( aPcbShape );
            AddSuccess();
        }
        else
        {
            AddFailure();
        }
    }
}


void POLYGON_BOOLEAN_ROUTINE::Finalize()
{
    if( m_workingPolygons.OutlineCount() == 0 || m_firstPolygon )
    {
        // Nothing to do (no polygons handled or nothing left?)
        return;
    }

    CHANGE_HANDLER& handler = GetHandler();

    // If we have disjoint polygons, we'll fix that now and create
    // new PCB_SHAPEs for each outline
    for( int i = 0; i < m_workingPolygons.OutlineCount(); ++i )
    {
        // If we handled any polygons to get any outline,
        // there must be a layer set by now.
        wxASSERT( m_layer >= 0 );

        std::unique_ptr<PCB_SHAPE> new_poly_shape =
                std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::POLY );

        SHAPE_POLY_SET poly_set = m_workingPolygons.UnitSet( i );

        new_poly_shape->SetPolyShape( poly_set );

        // Copy properties from the source polygon
        new_poly_shape->SetWidth( m_width );
        new_poly_shape->SetLayer( m_layer );
        new_poly_shape->SetFillMode( m_fillMode );

        handler.AddNewItem( std::move( new_poly_shape ) );
    }
}


wxString POLYGON_MERGE_ROUTINE::GetCommitDescription() const
{
    return _( "Merge Polygons" );
}


std::optional<wxString> POLYGON_MERGE_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
        return _( "Unable to merge the selected polygons." );
    else if( GetFailures() > 0 )
        return _( "Some of the polygons could not be merged." );

    return std::nullopt;
}


bool POLYGON_MERGE_ROUTINE::ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon )
{
    SHAPE_POLY_SET no_arcs_poly = aPolygon;
    no_arcs_poly.ClearArcs();

    GetWorkingPolygons().BooleanAdd( no_arcs_poly );
    return true;
}


wxString POLYGON_SUBTRACT_ROUTINE::GetCommitDescription() const
{
    return _( "Subtract Polygons" );
}


std::optional<wxString> POLYGON_SUBTRACT_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
        return _( "Unable to subtract the selected polygons." );
    else if( GetFailures() > 0 )
        return _( "Some of the polygons could not be subtracted." );

    return std::nullopt;
}


bool POLYGON_SUBTRACT_ROUTINE::ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon )
{
    SHAPE_POLY_SET& working_polygons = GetWorkingPolygons();
    SHAPE_POLY_SET  working_copy = working_polygons;

    SHAPE_POLY_SET no_arcs_poly = aPolygon;
    no_arcs_poly.ClearArcs();

    working_copy.BooleanSubtract( no_arcs_poly );

    working_polygons = std::move( working_copy );
    return true;
}


wxString POLYGON_INTERSECT_ROUTINE::GetCommitDescription() const
{
    return _( "Intersect Polygons" );
}


std::optional<wxString> POLYGON_INTERSECT_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
        return _( "Unable to intersect the selected polygons." );
    else if( GetFailures() > 0 )
        return _( "Some of the polygons could not be intersected." );

    return std::nullopt;
}


bool POLYGON_INTERSECT_ROUTINE::ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon )
{
    SHAPE_POLY_SET& working_polygons = GetWorkingPolygons();
    SHAPE_POLY_SET  working_copy = working_polygons;

    SHAPE_POLY_SET no_arcs_poly = aPolygon;
    no_arcs_poly.ClearArcs();

    working_copy.BooleanIntersection( no_arcs_poly );

    // Is there anything left?
    if( working_copy.OutlineCount() == 0 )
    {
        // There was no intersection. Rather than deleting the working polygon, we'll skip
        // and report a failure.
        return false;
    }

    working_polygons = std::move( working_copy );
    return true;
}


wxString OUTSET_ROUTINE::GetCommitDescription() const
{
    return _( "Outset Items" );
}

std::optional<wxString> OUTSET_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
        return _( "Unable to outset the selected items." );
    else if( GetFailures() > 0 )
        return _( "Some of the items could not be outset." );

    return std::nullopt;
}


static SHAPE_RECT GetRectRoundedToGridOutwards( const SHAPE_RECT& aRect, int aGridSize )
{
    const VECTOR2I newPos = KIGEOM::RoundNW( aRect.GetPosition(), aGridSize );
    const VECTOR2I newOpposite =
            KIGEOM::RoundSE( aRect.GetPosition() + aRect.GetSize(), aGridSize );
    return SHAPE_RECT( newPos, newOpposite );
}


void OUTSET_ROUTINE::ProcessItem( BOARD_ITEM& aItem )
{
    /*
     * This attempts to do exact outsetting, rather than punting to Clipper.
     * So it can't do all shapes, but it can do the most obvious ones, which are probably
     * the ones you want to outset anyway, most usually when making a courtyard for a footprint.
     */

    PCB_LAYER_ID layer = m_params.useSourceLayers ? aItem.GetLayer() : m_params.layer;

    // Not all items have a width, even if the parameters want to copy it
    // So fall back to the given width if we can't get one.
    int width = m_params.lineWidth;

    if( m_params.useSourceWidths )
    {
        std::optional<int> item_width = GetBoardItemWidth( aItem );

        if( item_width.has_value() )
            width = *item_width;
    }

    CHANGE_HANDLER& handler = GetHandler();

    const auto addPolygonalChain = [&]( const SHAPE_LINE_CHAIN& aChain )
    {
        SHAPE_POLY_SET new_poly( aChain );

        std::unique_ptr<PCB_SHAPE> new_shape =
                std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::POLY );

        new_shape->SetPolyShape( new_poly );
        new_shape->SetLayer( layer );
        new_shape->SetWidth( width );

        handler.AddNewItem( std::move( new_shape ) );
    };

    // Iterate the SHAPE_LINE_CHAIN in the polygon, pulling out
    // segments and arcs to create new PCB_SHAPE primitives.
    const auto addChain = [&]( const SHAPE_LINE_CHAIN& aChain )
    {
        // Prefer to add a polygonal chain if there are no arcs
        // as this permits boolean ops
        if( aChain.ArcCount() == 0 )
        {
            addPolygonalChain( aChain );
            return;
        }

        for( size_t si = 0; si < aChain.GetSegmentCount(); ++si )
        {
            const SEG seg = aChain.GetSegment( si );

            if( seg.Length() == 0 )
                continue;

            if( aChain.IsArcSegment( si ) )
                continue;

            std::unique_ptr<PCB_SHAPE> new_shape =
                    std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::SEGMENT );
            new_shape->SetStart( seg.A );
            new_shape->SetEnd( seg.B );
            new_shape->SetLayer( layer );
            new_shape->SetWidth( width );

            handler.AddNewItem( std::move( new_shape ) );
        }

        for( size_t ai = 0; ai < aChain.ArcCount(); ++ai )
        {
            const SHAPE_ARC& arc = aChain.Arc( ai );

            if( arc.GetRadius() == 0 || arc.GetP0() == arc.GetP1() )
                continue;

            std::unique_ptr<PCB_SHAPE> new_shape =
                    std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::ARC );
            new_shape->SetArcGeometry( arc.GetP0(), arc.GetArcMid(), arc.GetP1() );
            new_shape->SetLayer( layer );
            new_shape->SetWidth( width );

            handler.AddNewItem( std::move( new_shape ) );
        }
    };

    const auto addPoly = [&]( const SHAPE_POLY_SET& aPoly )
    {
        for( int oi = 0; oi < aPoly.OutlineCount(); ++oi )
        {
            addChain( aPoly.Outline( oi ) );
        }
    };

    const auto addRect = [&]( const SHAPE_RECT& aRect )
    {
        std::unique_ptr<PCB_SHAPE> new_shape =
                std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::RECTANGLE );

        if( !m_params.gridRounding.has_value() )
        {
            new_shape->SetPosition( aRect.GetPosition() );
            new_shape->SetRectangleWidth( aRect.GetWidth() );
            new_shape->SetRectangleHeight( aRect.GetHeight() );
        }
        else
        {
            const SHAPE_RECT grid_rect =
                    GetRectRoundedToGridOutwards( aRect, *m_params.gridRounding );
            new_shape->SetPosition( grid_rect.GetPosition() );
            new_shape->SetRectangleWidth( grid_rect.GetWidth() );
            new_shape->SetRectangleHeight( grid_rect.GetHeight() );
        }

        new_shape->SetLayer( layer );
        new_shape->SetWidth( width );

        handler.AddNewItem( std::move( new_shape ) );
    };

    const auto addCircle = [&]( const CIRCLE& aCircle )
    {
        std::unique_ptr<PCB_SHAPE> new_shape =
                std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::CIRCLE );
        new_shape->SetCenter( aCircle.Center );
        new_shape->SetRadius( aCircle.Radius );
        new_shape->SetLayer( layer );
        new_shape->SetWidth( width );

        handler.AddNewItem( std::move( new_shape ) );
    };

    const auto addCircleOrRect = [&]( const CIRCLE& aCircle )
    {
        if( m_params.roundCorners )
        {
            addCircle( aCircle );
        }
        else
        {
            const VECTOR2I   rVec{ aCircle.Radius, aCircle.Radius };
            const SHAPE_RECT rect{ aCircle.Center - rVec, aCircle.Center + rVec };
            addRect( rect );
        }
    };

    switch( aItem.Type() )
    {
    case PCB_PAD_T:
    {
        const PAD& pad = static_cast<const PAD&>( aItem );

        // TODO(JE) padstacks
        const PAD_SHAPE pad_shape = pad.GetShape( PADSTACK::ALL_LAYERS );

        switch( pad_shape )
        {
        case PAD_SHAPE::RECTANGLE:
        case PAD_SHAPE::ROUNDRECT:
        case PAD_SHAPE::OVAL:
        {
            const VECTOR2I pad_size = pad.GetSize( PADSTACK::ALL_LAYERS );

            BOX2I box{ pad.GetPosition() - pad_size / 2, pad_size };
            box.Inflate( m_params.outsetDistance );

            int radius = m_params.outsetDistance;
            if( pad_shape == PAD_SHAPE::ROUNDRECT )
            {
                radius += pad.GetRoundRectCornerRadius( PADSTACK::ALL_LAYERS );
            }
            else if( pad_shape == PAD_SHAPE::OVAL )
            {
                radius += std::min( pad_size.x, pad_size.y ) / 2;
            }

            radius = m_params.roundCorners ? radius : 0;

            // No point doing a SHAPE_RECT as we may need to rotate it
            ROUNDRECT      rrect( box, radius );
            SHAPE_POLY_SET poly;
            rrect.TransformToPolygon( poly, pad.GetMaxError() );

            poly.Rotate( pad.GetOrientation(), pad.GetPosition() );
            addPoly( poly );
            AddSuccess();
            break;
        }
        case PAD_SHAPE::CIRCLE:
        {
            const int radius = pad.GetSize( PADSTACK::ALL_LAYERS ).x / 2 + m_params.outsetDistance;
            const CIRCLE circle( pad.GetPosition(), radius );
            addCircleOrRect( circle );
            AddSuccess();
            break;
        }
        case PAD_SHAPE::TRAPEZOID:
        {
            // Not handled yet, but could use a generic convex polygon outset method.
            break;
        }
        default:
            // Other pad shapes are not supported with exact outsets
            break;
        }
        break;
    }
    case PCB_SHAPE_T:
    {
        const PCB_SHAPE& pcb_shape = static_cast<const PCB_SHAPE&>( aItem );

        switch( pcb_shape.GetShape() )
        {
        case SHAPE_T::RECTANGLE:
        {
            BOX2I box{ pcb_shape.GetPosition(),
                       VECTOR2I{ pcb_shape.GetRectangleWidth(), pcb_shape.GetRectangleHeight() } };
            box.Inflate( m_params.outsetDistance );

            box.Normalize();

            SHAPE_RECT rect( box );
            int        cornerRadius = pcb_shape.GetCornerRadius();

            if( m_params.roundCorners )
                cornerRadius += m_params.outsetDistance;

            if( m_params.gridRounding.has_value() )
                rect = GetRectRoundedToGridOutwards( rect, *m_params.gridRounding );

            if( cornerRadius > 0 )
            {
                ROUNDRECT rrect( rect, cornerRadius );
                SHAPE_POLY_SET poly;
                rrect.TransformToPolygon( poly, pcb_shape.GetMaxError() );
                addChain( poly.Outline( 0 ) );
            }
            else
            {
                addRect( rect );
            }

            AddSuccess();
            break;
        }
        case SHAPE_T::CIRCLE:
        {
            const CIRCLE circle( pcb_shape.GetCenter(),
                                 pcb_shape.GetRadius() + m_params.outsetDistance );
            addCircleOrRect( circle );
            AddSuccess();
            break;
        }
        case SHAPE_T::SEGMENT:
        {
            // For now just make the whole stadium shape and let the user delete the unwanted bits
            const SEG seg( pcb_shape.GetStart(), pcb_shape.GetEnd() );

            if( m_params.roundCorners )
            {
                const SHAPE_SEGMENT oval( seg, m_params.outsetDistance * 2 );
                addChain( KIGEOM::ConvertToChain( oval ) );
            }
            else
            {
                SHAPE_LINE_CHAIN chain;
                const VECTOR2I   ext = ( seg.B - seg.A ).Resize( m_params.outsetDistance );
                const VECTOR2I   perp = GetRotated( ext, ANGLE_90 );

                chain.Append( seg.A - ext + perp );
                chain.Append( seg.A - ext - perp );
                chain.Append( seg.B + ext - perp );
                chain.Append( seg.B + ext + perp );
                chain.SetClosed( true );
                addChain( chain );
            }

            AddSuccess();
            break;
        }
        case SHAPE_T::ARC:
        {
            // Not 100% sure what a sensible non-round outset of an arc is!
            // (not sure it's that important in practice)

            // Gets rather complicated if this isn't true
            if( pcb_shape.GetRadius() >= m_params.outsetDistance )
            {
                // Again, include the endcaps and let the user delete the unwanted bits
                const SHAPE_ARC arc{ pcb_shape.GetCenter(), pcb_shape.GetStart(),
                                     pcb_shape.GetArcAngle(), 0 };

                const VECTOR2I startNorm =
                        VECTOR2I( arc.GetP0() - arc.GetCenter() ).Resize( m_params.outsetDistance );

                const SHAPE_ARC inner{ arc.GetCenter(), arc.GetP0() - startNorm,
                                       arc.GetCentralAngle(), 0 };
                const SHAPE_ARC outer{ arc.GetCenter(), arc.GetP0() + startNorm,
                                       arc.GetCentralAngle(), 0 };

                SHAPE_LINE_CHAIN chain;
                chain.Append( outer );
                // End cap at the P1 end
                chain.Append( SHAPE_ARC{ arc.GetP1(), outer.GetP1(), ANGLE_180 } );

                if( inner.GetRadius() > 0 )
                {
                    chain.Append( inner.Reversed() );
                }

                // End cap at the P0 end back to the start
                chain.Append( SHAPE_ARC{ arc.GetP0(), inner.GetP0(), ANGLE_180 } );
                addChain( chain );
                AddSuccess();
            }

            break;
        }

        default:
            // Other shapes are not supported with exact outsets
            // (convex) POLY shouldn't be too traumatic and it would bring trapezoids for free.
            break;
        }

        break;
    }
    default:
        // Other item types are not supported with exact outsets
        break;
    }

    if( m_params.deleteSourceItems )
    {
        handler.DeleteItem( aItem );
    }
}
