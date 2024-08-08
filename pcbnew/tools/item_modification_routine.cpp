/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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


std::optional<wxString> LINE_FILLET_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
    {
        return _( "Unable to fillet the selected lines." );
    }
    else if( GetFailures() > 0 )
    {
        return _( "Some of the lines could not be filleted." );
    }
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
        AddFailure();
        return;
    }

    if( seg_a.Angle( seg_b ).IsHorizontal() )
        return;

    SHAPE_ARC sArc( seg_a, seg_b, m_filletRadiusIU );
    VECTOR2I  t1newPoint, t2newPoint;

    auto setIfPointOnSeg = []( VECTOR2I& aPointToSet, SEG aSegment, VECTOR2I aVecToTest )
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


std::optional<wxString> LINE_CHAMFER_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
    {
        return _( "Unable to chamfer the selected lines." );
    }
    else if( GetFailures() > 0 )
    {
        return _( "Some of the lines could not be chamfered." );
    }
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

    ModifyLineOrDeleteIfZeroLength( aLineA, *chamfer_result->m_updated_seg_a );
    ModifyLineOrDeleteIfZeroLength( aLineB, *chamfer_result->m_updated_seg_b );

    AddSuccess();
}


wxString DOGBONE_CORNER_ROUTINE::GetCommitDescription() const
{
    return _( "Dogbone Corners" );
}


std::optional<wxString> DOGBONE_CORNER_ROUTINE::GetStatusMessage() const
{
    wxString msg;

    if( GetSuccesses() == 0 )
    {
        msg += _( "Unable to add dogbone corners to the selected lines." );
    }
    else if( GetFailures() > 0 )
    {
        msg += _( "Some of the lines could not have dogbone corners added." );
    }

    if( m_haveNarrowMouths )
    {
        if( !msg.empty() )
            msg += " ";

        msg += _( "Some of the dogbone corners are too narrow to fit a "
                  "cutter of the specified radius." );
    }

    if( msg.empty() )
        return std::nullopt;

    return msg;
}


void DOGBONE_CORNER_ROUTINE::ProcessLinePair( PCB_SHAPE& aLineA, PCB_SHAPE& aLineB )
{
    if( aLineA.GetLength() == 0.0 || aLineB.GetLength() == 0.0 )
        return;

    SEG seg_a( aLineA.GetStart(), aLineA.GetEnd() );
    SEG seg_b( aLineB.GetStart(), aLineB.GetEnd() );

    auto [a_pt, b_pt] = GetSharedEndpoints( seg_a, seg_b );

    if( !a_pt || !b_pt )
    {
        return;
    }

    // Cannot handle parallel lines
    if( seg_a.Angle( seg_b ).IsHorizontal() )
    {
        AddFailure();
        return;
    }

    std::optional<DOGBONE_RESULT> dogbone_result =
            ComputeDogbone( seg_a, seg_b, m_dogboneRadiusIU );

    if( !dogbone_result )
    {
        AddFailure();
        return;
    }

    if( dogbone_result->m_small_arc_mouth )
    {
        // The arc is too small to fit the radius
        m_haveNarrowMouths = true;
    }

    auto tArc = std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::ARC );

    tArc->SetArcGeometry( dogbone_result->m_arc_start, dogbone_result->m_arc_mid,
                          dogbone_result->m_arc_end );

    // Copy properties from one of the source lines
    tArc->SetWidth( aLineA.GetWidth() );
    tArc->SetLayer( aLineA.GetLayer() );
    tArc->SetLocked( aLineA.IsLocked() );

    CHANGE_HANDLER& handler = GetHandler();
    handler.AddNewItem( std::move( tArc ) );

    ModifyLineOrDeleteIfZeroLength( aLineA, dogbone_result->m_updated_seg_a );
    ModifyLineOrDeleteIfZeroLength( aLineB, dogbone_result->m_updated_seg_b );

    AddSuccess();
}


wxString LINE_EXTENSION_ROUTINE::GetCommitDescription() const
{
    return _( "Extend Lines to Meet" );
}


std::optional<wxString> LINE_EXTENSION_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
    {
        return _( "Unable to extend the selected lines to meet." );
    }
    else if( GetFailures() > 0 )
    {
        return _( "Some of the lines could not be extended to meet." );
    }
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
        break;
    }
    case SHAPE_T::RECTANGLE:
    {
        SHAPE_POLY_SET rect_poly;

        const std::vector<VECTOR2I> rect_pts = aPcbShape.GetRectCorners();

        rect_poly.NewOutline();

        for( const VECTOR2I& pt : rect_pts )
        {
            rect_poly.Append( pt );
        }

        poly = std::make_unique<SHAPE_POLY_SET>( std::move( rect_poly ) );
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

    if( !m_workingPolygon )
    {
        auto initial = std::make_unique<PCB_SHAPE>( GetBoard(), SHAPE_T::POLY );
        initial->SetPolyShape( *poly );

        // Copy properties
        initial->SetLayer( aPcbShape.GetLayer() );
        initial->SetWidth( aPcbShape.GetWidth() );

        // Keep the pointer
        m_workingPolygon = initial.get();
        // Hand over ownership
        GetHandler().AddNewItem( std::move( initial ) );

        // And remove the shape
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


wxString POLYGON_MERGE_ROUTINE::GetCommitDescription() const
{
    return _( "Merge polygons." );
}


std::optional<wxString> POLYGON_MERGE_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
    {
        return _( "Unable to merge the selected polygons." );
    }
    else if( GetFailures() > 0 )
    {
        return _( "Some of the polygons could not be merged." );
    }
    return std::nullopt;
}


bool POLYGON_MERGE_ROUTINE::ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon )
{
    const SHAPE_POLY_SET::POLYGON_MODE poly_mode = SHAPE_POLY_SET::POLYGON_MODE::PM_FAST;

    SHAPE_POLY_SET working_copy = GetWorkingPolygon()->GetPolyShape();
    working_copy.BooleanAdd( aPolygon, poly_mode );

    // Check it's not disjoint - this doesn't work well in the UI
    if( working_copy.OutlineCount() != 1 )
    {
        return false;
    }

    GetWorkingPolygon()->SetPolyShape( working_copy );
    return true;
}


wxString POLYGON_SUBTRACT_ROUTINE::GetCommitDescription() const
{
    return _( "Subtract polygons." );
}


std::optional<wxString> POLYGON_SUBTRACT_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
    {
        return _( "Unable to subtract the selected polygons." );
    }
    else if( GetFailures() > 0 )
    {
        return _( "Some of the polygons could not be subtracted." );
    }
    return std::nullopt;
}


bool POLYGON_SUBTRACT_ROUTINE::ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon )
{
    const SHAPE_POLY_SET::POLYGON_MODE poly_mode = SHAPE_POLY_SET::POLYGON_MODE::PM_FAST;

    SHAPE_POLY_SET working_copy = GetWorkingPolygon()->GetPolyShape();
    working_copy.BooleanSubtract( aPolygon, poly_mode );

    // Subtraction can create holes or delete the polygon
    // In theory we can allow holes as the EDA_SHAPE will fracture for us, but that's
    // probably not what the user has in mind (?)
    if( working_copy.OutlineCount() != 1 || working_copy.HoleCount( 0 ) > 0
        || working_copy.VertexCount( 0 ) == 0 )
    {
        // If that happens, just skip the operation
        return false;
    }

    GetWorkingPolygon()->SetPolyShape( working_copy );
    return true;
}

wxString POLYGON_INTERSECT_ROUTINE::GetCommitDescription() const
{
    return _( "Intersect polygons." );
}


std::optional<wxString> POLYGON_INTERSECT_ROUTINE::GetStatusMessage() const
{
    if( GetSuccesses() == 0 )
    {
        return _( "Unable to intersect the selected polygons." );
    }
    else if( GetFailures() > 0 )
    {
        return _( "Some of the polygons could not be intersected." );
    }
    return std::nullopt;
}


bool POLYGON_INTERSECT_ROUTINE::ProcessSubsequentPolygon( const SHAPE_POLY_SET& aPolygon )
{
    const SHAPE_POLY_SET::POLYGON_MODE poly_mode = SHAPE_POLY_SET::POLYGON_MODE::PM_FAST;

    SHAPE_POLY_SET working_copy = GetWorkingPolygon()->GetPolyShape();
    working_copy.BooleanIntersection( aPolygon, poly_mode );

    // Is there anything left?
    if( working_copy.OutlineCount() == 0 )
    {
        // There was no intersection. Rather than deleting the working polygon, we'll skip
        // and report a failure.
        return false;
    }

    GetWorkingPolygon()->SetPolyShape( working_copy );
    return true;
}
