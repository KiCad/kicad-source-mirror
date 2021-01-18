/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bezier_curves.h>
#include <pcb_screen.h>
#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <base_units.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_compound.h>
#include <origin_transforms.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <i18n_utility.h>


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype )
{
    m_angle = 0;
    m_filled = false;
    m_flags = 0;
    m_shape = S_SEGMENT;
    m_width = Millimeter2iu( DEFAULT_LINE_WIDTH );
}


PCB_SHAPE::~PCB_SHAPE()
{
}


void PCB_SHAPE::SetPosition( const wxPoint& aPos )
{
    m_start = aPos;
}


wxPoint PCB_SHAPE::GetPosition() const
{
    if( m_shape == S_POLYGON )
        return (wxPoint) m_poly.CVertex( 0 );
    else
        return m_start;
}


double PCB_SHAPE::GetLength() const
{
    double length = 0.0;

    switch( m_shape )
    {
    case S_CURVE:
        for( size_t ii = 1; ii < m_bezierPoints.size(); ++ii )
            length += GetLineLength( m_bezierPoints[ ii - 1], m_bezierPoints[ii] );

        break;

    case S_SEGMENT:
        length = GetLineLength( GetStart(), GetEnd() );
        break;

    case S_POLYGON:
        for( int ii = 0; ii < m_poly.COutline( 0 ).SegmentCount(); ii++ )
            length += m_poly.COutline( 0 ).CSegment( ii ).Length();

        break;

    case S_ARC:
        length = 2 * M_PI * GetRadius() * ( GetAngle() / 3600.0 );
        break;

    default:
        wxASSERT_MSG( false, "PCB_SHAPE::GetLength not implemented for shape"
                + ShowShape( GetShape() ) );
        break;
    }

    return length;
}


void PCB_SHAPE::Move( const wxPoint& aMoveVector )
{
    // Move vector should not affect start/end for polygon since it will
    // be applied directly to polygon outline.
    if( m_shape != S_POLYGON )
    {
        m_start += aMoveVector;
        m_end += aMoveVector;
    }

    switch ( m_shape )
    {
    case S_POLYGON:
        m_poly.Move( VECTOR2I( aMoveVector ) );
        break;

    case S_ARC:
        m_thirdPoint += aMoveVector;
        break;

    case S_CURVE:
        m_bezierC1 += aMoveVector;
        m_bezierC2 += aMoveVector;

        for( wxPoint& pt : m_bezierPoints)
            pt += aMoveVector;

        break;

    default:
        break;
    }
}


void PCB_SHAPE::Scale( double aScale )
{
    auto scalePt = [&]( wxPoint& pt )
                   {
                       pt.x = KiROUND( pt.x * aScale );
                       pt.y = KiROUND( pt.y * aScale );
                   };

    int radius = GetRadius();

    scalePt( m_start );
    scalePt( m_end );

    // specific parameters:
    switch( m_shape )
    {
    case S_CURVE:
        scalePt( m_bezierC1 );
        scalePt( m_bezierC2 );
        break;

    case S_ARC:
        scalePt( m_thirdPoint );
        break;

    case S_CIRCLE:          //  ring or circle
        m_end.x = m_start.x + KiROUND( radius * aScale );
        m_end.y = m_start.y;
        break;

    case S_POLYGON:         // polygon
    {
        std::vector<wxPoint> pts;

        for( const VECTOR2I& pt : m_poly.Outline( 0 ).CPoints() )
        {
            pts.emplace_back( pt );
            scalePt( pts.back() );
        }

        SetPolyPoints( pts );
    }
        break;

    default:
        break;
    }
}


void PCB_SHAPE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    switch( m_shape )
    {
    case S_ARC:
    case S_SEGMENT:
    case S_CIRCLE:
        // these can all be done by just rotating the constituent points
        RotatePoint( &m_start, aRotCentre, aAngle );
        RotatePoint( &m_end, aRotCentre, aAngle );
        RotatePoint( &m_thirdPoint, aRotCentre, aAngle );
        break;

    case S_RECT:
        if( KiROUND( aAngle ) % 900 == 0 )
        {
            RotatePoint( &m_start, aRotCentre, aAngle );
            RotatePoint( &m_end, aRotCentre, aAngle );
            break;
        }

        // Convert non-cartesian-rotated rect to a diamond
        m_shape = S_POLYGON;
        m_poly.RemoveAllContours();
        m_poly.NewOutline();
        m_poly.Append( m_start );
        m_poly.Append( m_end.x, m_start.y );
        m_poly.Append( m_end );
        m_poly.Append( m_start.x, m_end.y );

        KI_FALLTHROUGH;

    case S_POLYGON:
        m_poly.Rotate( -DECIDEG2RAD( aAngle ), VECTOR2I( aRotCentre ) );
        break;

    case S_CURVE:
        RotatePoint( &m_start, aRotCentre, aAngle);
        RotatePoint( &m_end, aRotCentre, aAngle);
        RotatePoint( &m_bezierC1, aRotCentre, aAngle);
        RotatePoint( &m_bezierC2, aRotCentre, aAngle);

        for( wxPoint& pt : m_bezierPoints )
            RotatePoint( &pt, aRotCentre, aAngle);

        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::Rotate not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }
}


void PCB_SHAPE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_start.x = aCentre.x - ( m_start.x - aCentre.x );
        m_end.x   = aCentre.x - ( m_end.x - aCentre.x );
    }
    else
    {
        m_start.y = aCentre.y - ( m_start.y - aCentre.y );
        m_end.y   = aCentre.y - ( m_end.y - aCentre.y );
    }

    switch ( m_shape )
    {
    case S_ARC:
        if( aFlipLeftRight )
            m_thirdPoint.x   = aCentre.x - ( m_thirdPoint.x - aCentre.x );
        else
            m_thirdPoint.y   = aCentre.y - ( m_thirdPoint.y - aCentre.y );

        m_angle = -m_angle;
        break;

    case S_POLYGON:
        m_poly.Mirror( aFlipLeftRight, !aFlipLeftRight, VECTOR2I( aCentre ) );
        break;

    case S_CURVE:
        {
            if( aFlipLeftRight )
            {
                m_bezierC1.x = aCentre.x - ( m_bezierC1.x - aCentre.x );
                m_bezierC2.x = aCentre.x - ( m_bezierC2.x - aCentre.x );
            }
            else
            {
                m_bezierC1.y = aCentre.y - ( m_bezierC1.y - aCentre.y );
                m_bezierC2.y = aCentre.y - ( m_bezierC2.y - aCentre.y );
            }

            // Rebuild the poly points shape
            std::vector<wxPoint> ctrlPoints = { m_start, m_bezierC1, m_bezierC2, m_end };
            BEZIER_POLY converter( ctrlPoints );
            converter.GetPoly( m_bezierPoints, m_width );
        }
        break;

    case S_SEGMENT:
    case S_RECT:
    case S_CIRCLE:
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::Flip not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }

    // PCB_SHAPE items are not allowed on copper layers, so
    // copper layers count is not taken in account in Flip transform
    SetLayer( FlipLayer( GetLayer() ) );
}


void PCB_SHAPE::RebuildBezierToSegmentsPointsList( int aMinSegLen )
{
    // Has meaning only for S_CURVE DRAW_SEGMENT shape
    if( m_shape != S_CURVE )
    {
        m_bezierPoints.clear();
        return;
    }
    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    m_bezierPoints = buildBezierToSegmentsPointsList( aMinSegLen );
}


const std::vector<wxPoint> PCB_SHAPE::buildBezierToSegmentsPointsList( int aMinSegLen  ) const
{
    std::vector<wxPoint> bezierPoints;

    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    std::vector<wxPoint> ctrlPoints = { m_start, m_bezierC1, m_bezierC2, m_end };
    BEZIER_POLY converter( ctrlPoints );
    converter.GetPoly( bezierPoints, aMinSegLen );

    return bezierPoints;
}


wxPoint PCB_SHAPE::GetCenter() const
{
    wxPoint c;

    switch( m_shape )
    {
    case S_ARC:
    case S_CIRCLE:
        c = m_start;
        break;

    case S_SEGMENT:
        // Midpoint of the line
        c = ( GetStart() + GetEnd() ) / 2;
        break;

    case S_POLYGON:
    case S_RECT:
    case S_CURVE:
        c = GetBoundingBox().Centre();
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::GetCentre not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }

    return c;
}


wxPoint PCB_SHAPE::GetArcEnd() const
{
    wxPoint endPoint( m_end );         // start of arc

    switch( m_shape )
    {
    case S_ARC:
        endPoint = m_thirdPoint;
        break;

    default:
        break;
    }

    return endPoint;   // after rotation, the end of the arc.
}


wxPoint PCB_SHAPE::GetArcMid() const
{
    wxPoint endPoint( m_end );

    switch( m_shape )
    {
    case S_ARC:
        // rotate the starting point of the arc, given by m_End, through half
        // the angle m_Angle to get the middle of the arc.
        // m_Start is the arc centre
        endPoint  = m_end;         // m_End = start point of arc
        RotatePoint( &endPoint, m_start, -m_angle / 2.0 );
        break;

    default:
        break;
    }

    return endPoint;   // after rotation, the end of the arc.
}


double PCB_SHAPE::GetArcAngleStart() const
{
    // due to the Y axis orient atan2 needs - y value
    double angleStart = ArcTangente( GetArcStart().y - GetCenter().y,
                                     GetArcStart().x - GetCenter().x );

    // Normalize it to 0 ... 360 deg, to avoid discontinuity for angles near 180 deg
    // because 180 deg and -180 are very near angles when ampping betewwen -180 ... 180 deg.
    // and this is not easy to handle in calculations
    NORMALIZE_ANGLE_POS( angleStart );

    return angleStart;
}

double PCB_SHAPE::GetArcAngleEnd() const
{
    // due to the Y axis orient atan2 needs - y value
    double angleStart = ArcTangente( GetArcEnd().y - GetCenter().y,
                                     GetArcEnd().x - GetCenter().x );

    // Normalize it to 0 ... 360 deg, to avoid discontinuity for angles near 180 deg
    // because 180 deg and -180 are very near angles when ampping betewwen -180 ... 180 deg.
    // and this is not easy to handle in calculations
    NORMALIZE_ANGLE_POS( angleStart );

    return angleStart;
}


void PCB_SHAPE::SetAngle( double aAngle, bool aUpdateEnd )
{
    // m_Angle must be >= -360 and <= +360 degrees
    m_angle = NormalizeAngle360Max( aAngle );

    if( aUpdateEnd )
    {
        m_thirdPoint = m_end;
        RotatePoint( &m_thirdPoint, m_start, -m_angle );
    }
}


FOOTPRINT* PCB_SHAPE::GetParentFootprint() const
{
    if( !m_parent || m_parent->Type() != PCB_FOOTPRINT_T )
        return NULL;

    return (FOOTPRINT*) m_parent;
}


void PCB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS         units = aFrame->GetUserUnits();
    ORIGIN_TRANSFORMS originTransforms = aFrame->GetOriginTransforms();
    wxString          msg;

    aList.emplace_back( _( "Type" ), _( "Drawing" ) );

    if( IsLocked() )
        aList.emplace_back( _( "Status" ), _( "locked" ) );

    wxString shape = _( "Shape" );

    switch( m_shape )
    {
    case S_CIRCLE:
        aList.emplace_back( shape, _( "Circle" ) );

        msg = MessageTextFromValue( units, GetLineLength( m_start, m_end ) );
        aList.emplace_back( _( "Radius" ), msg );
        break;

    case S_ARC:
        aList.emplace_back( shape, _( "Arc" ) );

        msg.Printf( wxT( "%.1f" ), m_angle / 10.0 );
        aList.emplace_back( _( "Angle" ), msg );

        msg = MessageTextFromValue( units, GetLineLength( m_start, m_end ) );
        aList.emplace_back( _( "Radius" ), msg );
        break;

    case S_CURVE:
        aList.emplace_back( shape, _( "Curve" ) );

        msg = MessageTextFromValue( units, GetLength() );
        aList.emplace_back( _( "Length" ), msg );
        break;

    case S_POLYGON:
        aList.emplace_back( shape, _( "Polygon" ) );

        msg.Printf( "%d", GetPolyShape().Outline(0).PointCount() );
        aList.emplace_back( _( "Points" ), msg );
        break;

    case S_RECT:
        aList.emplace_back( shape, _( "Rectangle" ) );

        msg = MessageTextFromValue( units, std::abs( m_end.x - m_start.x ) );
        aList.emplace_back( _( "Width" ), msg );

        msg = MessageTextFromValue( units, std::abs( m_end.y - m_start.y ) );
        aList.emplace_back( _( "Height" ), msg );
        break;

    case S_SEGMENT:
    {
        aList.emplace_back( shape, _( "Segment" ) );

        msg = MessageTextFromValue( units, GetLineLength( m_start, m_end ) );
        aList.emplace_back( _( "Length" ), msg );

        // angle counter-clockwise from 3'o-clock
        const double deg = RAD2DEG( atan2( (double)( m_start.y - m_end.y ),
                                           (double)( m_end.x - m_start.x ) ) );
        aList.emplace_back( _( "Angle" ), wxString::Format( "%.1f", deg ) );
    }
        break;

    default:
        aList.emplace_back( shape, _( "Unrecognized" ) );
        break;
    }

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Width" ), MessageTextFromValue( units, m_width ) );
}


const EDA_RECT PCB_SHAPE::GetBoundingBox() const
{
    EDA_RECT bbox;

    bbox.SetOrigin( m_start );

    switch( m_shape )
    {
    case S_RECT:
    {
        std::vector<wxPoint> pts = GetRectCorners();

        bbox = EDA_RECT();  // re-init for merging

        for( wxPoint& pt : pts )
            bbox.Merge( pt );
    }
        break;

    case S_SEGMENT:
        bbox.SetEnd( m_end );
        break;

    case S_CIRCLE:
        bbox.Inflate( GetRadius() );
        break;

    case S_ARC:
        computeArcBBox( bbox );
        break;

    case S_POLYGON:
    {
        if( m_poly.IsEmpty() )
            break;

        FOOTPRINT* parentFootprint = GetParentFootprint();
        bbox = EDA_RECT();  // re-init for merging

        for( auto iter = m_poly.CIterate(); iter; iter++ )
        {
            wxPoint pt( iter->x, iter->y );

            if( parentFootprint ) // Transform, if we belong to a footprint
            {
                RotatePoint( &pt, parentFootprint->GetOrientation() );
                pt += parentFootprint->GetPosition();
            }

            bbox.Merge( pt );
        }
    }
        break;

    case S_CURVE:
        bbox.Merge( m_bezierC1 );
        bbox.Merge( m_bezierC2 );
        bbox.Merge( m_end );
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::GetBoundingBox not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }

    bbox.Inflate( m_width / 2 );
    bbox.Normalize();

    return bbox;
}


bool PCB_SHAPE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int maxdist = aAccuracy + ( m_width / 2 );

    switch( m_shape )
    {
    case S_CIRCLE:
    {
        int radius = GetRadius();
        int dist   = KiROUND( EuclideanNorm( aPosition - GetCenter() ) );

        if( IsFilled() )        // Filled circle hit-test
        {
            if( dist <= radius + maxdist )
                return true;
        }
        else                    // Ring hit-test
        {
            if( abs( radius - dist ) <= maxdist )
                return true;
        }
    }
        break;

    case S_ARC:
    {
        wxPoint relPos = aPosition - GetCenter();
        int radius = GetRadius();
        int dist   = KiROUND( EuclideanNorm( relPos ) );

        if( abs( radius - dist ) <= maxdist )
        {
            // For arcs, the test point angle must be >= arc angle start
            // and <= arc angle end
            // However angle values > 360 deg are not easy to handle
            // so we calculate the relative angle between arc start point and teast point
            // this relative arc should be < arc angle if arc angle > 0 (CW arc)
            // and > arc angle if arc angle < 0 (CCW arc)
            double arc_angle_start = GetArcAngleStart();    // Always 0.0 ... 360 deg, in 0.1 deg

            double arc_hittest = ArcTangente( relPos.y, relPos.x );

            // Calculate relative angle between the starting point of the arc, and the test point
            arc_hittest -= arc_angle_start;

            // Normalise arc_hittest between 0 ... 360 deg
            NORMALIZE_ANGLE_POS( arc_hittest );

            // Check angle: inside the arc angle when it is > 0
            // and outside the not drawn arc when it is < 0
            if( GetAngle() >= 0.0 )
            {
                if( arc_hittest <= GetAngle() )
                    return true;
            }
            else
            {
                if( arc_hittest >= ( 3600.0 + GetAngle() ) )
                    return true;
            }
        }
    }
        break;

    case S_CURVE:
        const_cast<PCB_SHAPE*>( this )->RebuildBezierToSegmentsPointsList( m_width );

        for( unsigned int i= 1; i < m_bezierPoints.size(); i++)
        {
            if( TestSegmentHit( aPosition, m_bezierPoints[ i - 1], m_bezierPoints[i], maxdist ) )
                return true;
        }

        break;

    case S_SEGMENT:
        if( TestSegmentHit( aPosition, m_start, m_end, maxdist ) )
            return true;

        break;

    case S_RECT:
    {
        std::vector<wxPoint> pts = GetRectCorners();

        if( IsFilled() )            // Filled rect hit-test
        {
            SHAPE_POLY_SET poly;
            poly.NewOutline();

            for( const wxPoint& pt : pts )
                poly.Append( pt );

            if( poly.Collide( VECTOR2I( aPosition ), maxdist ) )
                return true;
        }
        else                        // Open rect hit-test
        {
            if( TestSegmentHit( aPosition, pts[0], pts[1], maxdist )
                    || TestSegmentHit( aPosition, pts[1], pts[2], maxdist )
                    || TestSegmentHit( aPosition, pts[2], pts[3], maxdist )
                    || TestSegmentHit( aPosition, pts[3], pts[0], maxdist ) )
            {
                return true;
            }
        }
    }
        break;

    case S_POLYGON:
        if( IsFilled() )
        {
            return m_poly.Collide( VECTOR2I( aPosition ), maxdist );
        }
        else
        {
            SHAPE_POLY_SET::VERTEX_INDEX dummy;
            return m_poly.CollideEdge( VECTOR2I( aPosition ), dummy, maxdist );
        }

        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::HitTest (point) not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }

    return false;
}


bool PCB_SHAPE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    EDA_RECT arcRect;
    EDA_RECT bb = GetBoundingBox();

    switch( m_shape )
    {
    case S_CIRCLE:
        // Test if area intersects or contains the circle:
        if( aContained )
            return arect.Contains( bb );
        else
        {
            // If the rectangle does not intersect the bounding box, this is a much quicker test
            if( !aRect.Intersects( bb ) )
            {
                return false;
            }
            else
            {
                return arect.IntersectsCircleEdge( GetCenter(), GetRadius(), GetWidth() );
            }
        }
        break;

    case S_ARC:
        // Test for full containment of this arc in the rect
        if( aContained )
        {
            return arect.Contains( bb );
        }
        // Test if the rect crosses the arc
        else
        {
            arcRect = bb.Common( arect );

            /* All following tests must pass:
             * 1. Rectangle must intersect arc BoundingBox
             * 2. Rectangle must cross the outside of the arc
             */
            return arcRect.Intersects( arect ) &&
                   arcRect.IntersectsCircleEdge( GetCenter(), GetRadius(), GetWidth() );
        }
        break;

    case S_RECT:
        if( aContained )
        {
            return arect.Contains( bb );
        }
        else
        {
            std::vector<wxPoint> pts = GetRectCorners();

            // Account for the width of the lines
            arect.Inflate( GetWidth() / 2 );
            return ( arect.Intersects( pts[0], pts[1] )
                  || arect.Intersects( pts[1], pts[2] )
                  || arect.Intersects( pts[2], pts[3] )
                  || arect.Intersects( pts[3], pts[0] ) );
        }

        break;

    case S_SEGMENT:
        if( aContained )
        {
            return arect.Contains( GetStart() ) && aRect.Contains( GetEnd() );
        }
        else
        {
            // Account for the width of the line
            arect.Inflate( GetWidth() / 2 );
            return arect.Intersects( GetStart(), GetEnd() );
        }

        break;

    case S_POLYGON:
        if( aContained )
        {
            return arect.Contains( bb );
        }
        else
        {
            // Fast test: if aRect is outside the polygon bounding box,
            // rectangles cannot intersect
            if( !arect.Intersects( bb ) )
                return false;

            // Account for the width of the line
            arect.Inflate( GetWidth() / 2 );
            int count = m_poly.TotalVertices();

            for( int ii = 0; ii < count; ii++ )
            {
                auto vertex = m_poly.CVertex( ii );
                auto vertexNext = m_poly.CVertex(( ii + 1 ) % count );

                // Test if the point is within aRect
                if( arect.Contains( ( wxPoint ) vertex ) )
                    return true;

                // Test if this edge intersects aRect
                if( arect.Intersects( ( wxPoint ) vertex, ( wxPoint ) vertexNext ) )
                    return true;
            }
        }
        break;

    case S_CURVE:
        if( aContained )
        {
            return arect.Contains( bb );
        }
        else
        {
            // Fast test: if aRect is outside the polygon bounding box,
            // rectangles cannot intersect
            if( !arect.Intersects( bb ) )
                return false;

            // Account for the width of the line
            arect.Inflate( GetWidth() / 2 );
            unsigned count = m_bezierPoints.size();

            for( unsigned ii = 1; ii < count; ii++ )
            {
                wxPoint vertex = m_bezierPoints[ ii - 1];
                wxPoint vertexNext = m_bezierPoints[ii];

                // Test if the point is within aRect
                if( arect.Contains( ( wxPoint ) vertex ) )
                    return true;

                // Test if this edge intersects aRect
                if( arect.Intersects( vertex, vertexNext ) )
                    return true;
            }
        }
        break;


    default:
        wxFAIL_MSG( "PCB_SHAPE::HitTest (rect) not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }

    return false;
}


wxString PCB_SHAPE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "%s on %s" ),
                             ShowShape( m_shape ),
                             GetLayerName() );
}


BITMAP_DEF PCB_SHAPE::GetMenuImage() const
{
    return add_dashed_line_xpm;
}


EDA_ITEM* PCB_SHAPE::Clone() const
{
    return new PCB_SHAPE( *this );
}


const BOX2I PCB_SHAPE::ViewBBox() const
{
    // For arcs - do not include the center point in the bounding box,
    // it is redundant for displaying an arc
    if( m_shape == S_ARC )
    {
        EDA_RECT bbox;
        bbox.SetOrigin( m_end );
        computeArcBBox( bbox );
        return BOX2I( bbox.GetOrigin(), bbox.GetSize() );
    }

    return EDA_ITEM::ViewBBox();
}


std::vector<wxPoint> PCB_SHAPE::GetRectCorners() const
{
    std::vector<wxPoint> pts;
    FOOTPRINT* parentFootprint = GetParentFootprint();
    wxPoint    topLeft = GetStart();
    wxPoint    botRight = GetEnd();

    // Un-rotate rect topLeft and botRight
    if( parentFootprint && KiROUND( parentFootprint->GetOrientation() ) % 900 != 0 )
    {
        topLeft -= parentFootprint->GetPosition();
        RotatePoint( &topLeft, -parentFootprint->GetOrientation() );

        botRight -= parentFootprint->GetPosition();
        RotatePoint( &botRight, -parentFootprint->GetOrientation() );
    }

    // Set up the un-rotated 4 corners
    pts.emplace_back( topLeft );
    pts.emplace_back( botRight.x, topLeft.y );
    pts.emplace_back( botRight );
    pts.emplace_back( topLeft.x, botRight.y );

    // Now re-rotate the 4 corners to get a diamond
    if( parentFootprint && KiROUND( parentFootprint->GetOrientation() ) % 900 != 0 )
    {
        for( wxPoint& pt : pts )
        {
            RotatePoint( &pt, parentFootprint->GetOrientation() );
            pt += parentFootprint->GetPosition();
        }
    }

    return pts;
}


void PCB_SHAPE::computeArcBBox( EDA_RECT& aBBox ) const
{
    // Do not include the center, which is not necessarily
    // inside the BB of a arc with a small angle
    aBBox.SetOrigin( m_end );

    wxPoint end = m_end;
    RotatePoint( &end, m_start, -m_angle );
    aBBox.Merge( end );

    // Determine the starting quarter
    // 0 right-bottom
    // 1 left-bottom
    // 2 left-top
    // 3 right-top
    unsigned int quarter = 0;       // assume right-bottom

    if( m_end.x < m_start.x )
    {
        if( m_end.y <= m_start.y )
            quarter = 2;
        else // ( m_End.y > m_Start.y )
            quarter = 1;
    }
    else if( m_end.x >= m_start.x )
    {
        if( m_end.y < m_start.y )
            quarter = 3;
        else if( m_end.x == m_start.x )
            quarter = 1;
    }

    int radius = GetRadius();
    int angle = (int) GetArcAngleStart() % 900 + m_angle;
    bool directionCW = ( m_angle > 0 );      // Is the direction of arc clockwise?

    // Make the angle positive, so we go clockwise and merge points belonging to the arc
    if( !directionCW )
    {
        angle = 900 - angle;
        quarter = ( quarter + 3 ) % 4;       // -1 modulo arithmetic
    }

    while( angle > 900 )
    {
        switch( quarter )
        {
        case 0: aBBox.Merge( wxPoint( m_start.x, m_start.y + radius ) ); break;  // down
        case 1: aBBox.Merge( wxPoint( m_start.x - radius, m_start.y          ) ); break;  // left
        case 2: aBBox.Merge( wxPoint( m_start.x, m_start.y - radius ) ); break;  // up
        case 3: aBBox.Merge( wxPoint( m_start.x + radius, m_start.y          ) ); break;  // right
        }

        if( directionCW )
            ++quarter;
        else
            quarter += 3;       // -1 modulo arithmetic

        quarter %= 4;
        angle -= 900;
    }
}


void PCB_SHAPE::SetPolyPoints( const std::vector<wxPoint>& aPoints )
{
    m_poly.RemoveAllContours();
    m_poly.NewOutline();

    for ( const wxPoint& p : aPoints )
        m_poly.Append( p.x, p.y );
}


std::vector<SHAPE*> PCB_SHAPE::MakeEffectiveShapes() const
{
    std::vector<SHAPE*> effectiveShapes;

    switch( m_shape )
    {
    case S_ARC:
    {
        SHAPE_ARC        arc( GetCenter(), GetArcStart(), (double) GetAngle() / 10.0 );
        SHAPE_LINE_CHAIN l = arc.ConvertToPolyline();

        for( int i = 0; i < l.SegmentCount(); i++ )
        {
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ).A,
                                                             l.Segment( i ).B, m_width ) );
        }

        break;
    }

    case S_SEGMENT:
        effectiveShapes.emplace_back( new SHAPE_SEGMENT( GetStart(), GetEnd(), m_width ) );
        break;

    case S_RECT:
    {
        std::vector<wxPoint> pts = GetRectCorners();

        if( IsFilled() )
        {
            effectiveShapes.emplace_back( new SHAPE_SIMPLE( pts ) );
        }

        if( m_width > 0 || !IsFilled() )
        {
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[0], pts[1], m_width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[1], pts[2], m_width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[2], pts[3], m_width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[3], pts[0], m_width ) );
        }
    }
        break;

    case S_CIRCLE:
    {
        if( IsFilled() )
        {
            effectiveShapes.emplace_back( new SHAPE_CIRCLE( GetCenter(), GetRadius() ) );
        }

        if( m_width > 0 || !IsFilled() )
        {
            // SHAPE_CIRCLE has no ConvertToPolyline() method, so use a 360.0 SHAPE_ARC
            SHAPE_ARC        circle( GetCenter(), GetEnd(), 360.0 );
            SHAPE_LINE_CHAIN l = circle.ConvertToPolyline();

            for( int i = 0; i < l.SegmentCount(); i++ )
            {
                effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ).A,
                                                                 l.Segment( i ).B, m_width ) );
            }
        }

        break;
    }

    case S_CURVE:
    {
        auto bezierPoints = buildBezierToSegmentsPointsList( GetWidth() );
        wxPoint start_pt = bezierPoints[0];

        for( unsigned int jj = 1; jj < bezierPoints.size(); jj++ )
        {
            wxPoint end_pt = bezierPoints[jj];
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( start_pt, end_pt, m_width ) );
            start_pt = end_pt;
        }

        break;
    }

    case S_POLYGON:
    {
        SHAPE_LINE_CHAIN l = GetPolyShape().COutline( 0 );
        FOOTPRINT*       parentFootprint = dynamic_cast<FOOTPRINT*>( m_parent );

        if( parentFootprint )
        {
            l.Rotate( -parentFootprint->GetOrientationRadians() );
            l.Move( parentFootprint->GetPosition() );
        }

        if( IsFilled() )
        {
            effectiveShapes.emplace_back( new SHAPE_SIMPLE( l ) );
        }

        if( m_width > 0 || !IsFilled() )
        {
            for( int i = 0; i < l.SegmentCount(); i++ )
                effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ), m_width ) );
        }
    }
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::MakeEffectiveShapes unsupported PCB_SHAPE shape: "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }

    return effectiveShapes;
}


std::shared_ptr<SHAPE> PCB_SHAPE::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return std::make_shared<SHAPE_COMPOUND>( MakeEffectiveShapes() );
}


const std::vector<wxPoint> PCB_SHAPE::BuildPolyPointsList() const
{
    std::vector<wxPoint> rv;

    if( m_poly.OutlineCount() )
    {
        if( m_poly.COutline( 0 ).PointCount() )
        {
            for ( auto iter = m_poly.CIterate(); iter; iter++ )
                rv.emplace_back( iter->x, iter->y );
        }
    }

    return rv;
}


bool PCB_SHAPE::IsPolyShapeValid() const
{
    // return true if the polygonal shape is valid (has more than 2 points)
    if( GetPolyShape().OutlineCount() == 0 )
        return false;

    const SHAPE_LINE_CHAIN& outline = ( (SHAPE_POLY_SET&)GetPolyShape() ).Outline( 0 );

    return outline.PointCount() > 2;
}


int PCB_SHAPE::GetPointCount() const
{
    // return the number of corners of the polygonal shape
    // this shape is expected to be only one polygon without hole
    if( GetPolyShape().OutlineCount() )
        return GetPolyShape().VertexCount( 0 );

    return 0;
}


void PCB_SHAPE::SwapData( BOARD_ITEM* aImage )
{
    PCB_SHAPE* image = dynamic_cast<PCB_SHAPE*>( aImage );
    assert( image );

    std::swap( m_width, image->m_width );
    std::swap( m_start, image->m_start );
    std::swap( m_end, image->m_end );
    std::swap( m_thirdPoint, image->m_thirdPoint );
    std::swap( m_shape, image->m_shape );
    std::swap( m_angle, image->m_angle );
    std::swap( m_bezierC1, image->m_bezierC1 );
    std::swap( m_bezierC2, image->m_bezierC2 );
    std::swap( m_bezierPoints, image->m_bezierPoints );
    std::swap( m_poly, image->m_poly );
    std::swap( m_layer, image->m_layer );
    std::swap( m_flags, image->m_flags );
    std::swap( m_status, image->m_status );
    std::swap( m_parent, image->m_parent );
    std::swap( m_forceVisible, image->m_forceVisible );
}


bool PCB_SHAPE::cmp_drawings::operator()( const BOARD_ITEM* aFirst, const BOARD_ITEM* aSecond ) const
{
    if( aFirst->Type() != aSecond->Type() )
        return aFirst->Type() < aSecond->Type();

    if( aFirst->GetLayer() != aSecond->GetLayer() )
        return aFirst->GetLayer() < aSecond->GetLayer();

    if( aFirst->Type() == PCB_SHAPE_T )
    {
        const PCB_SHAPE* dwgA = static_cast<const PCB_SHAPE*>( aFirst );
        const PCB_SHAPE* dwgB = static_cast<const PCB_SHAPE*>( aSecond );

        if( dwgA->GetShape() != dwgB->GetShape() )
            return dwgA->GetShape() < dwgB->GetShape();
    }

    return aFirst->m_Uuid < aSecond->m_Uuid;
}


static struct DRAWSEGMENT_DESC
{
    DRAWSEGMENT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_SHAPE );
        propMgr.InheritsAfter( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ) );

        propMgr.AddProperty( new PROPERTY<PCB_SHAPE, int>( _HKI( "Thickness" ),
                    &PCB_SHAPE::SetWidth, &PCB_SHAPE::GetWidth, PROPERTY_DISPLAY::DISTANCE ) );
        // TODO show certain properties depending on the shape
        //propMgr.AddProperty( new PROPERTY<PCB_SHAPE, double>( _HKI( "Angle" ),
        //            &PCB_SHAPE::SetAngle, &PCB_SHAPE::GetAngle, PROPERTY_DISPLAY::DECIDEGREE ) );
        // TODO or may have different names (arcs)
        // TODO type?
        propMgr.AddProperty( new PROPERTY<PCB_SHAPE, int>( _HKI( "End X" ),
                    &PCB_SHAPE::SetEndX, &PCB_SHAPE::GetEndX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PCB_SHAPE, int>( _HKI( "End Y" ),
                    &PCB_SHAPE::SetEndY, &PCB_SHAPE::GetEndY, PROPERTY_DISPLAY::DISTANCE ) );
    }
} _DRAWSEGMENT_DESC;
