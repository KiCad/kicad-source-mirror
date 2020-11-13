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
    m_Type  = 0;
    m_Angle = 0;
    m_Flags = 0;
    m_Shape = S_SEGMENT;
    m_Width = Millimeter2iu( DEFAULT_LINE_WIDTH );
}


PCB_SHAPE::~PCB_SHAPE()
{
}


void PCB_SHAPE::SetPosition( const wxPoint& aPos )
{
    m_Start = aPos;
}


wxPoint PCB_SHAPE::GetPosition() const
{
    if( m_Shape == S_POLYGON )
        return (wxPoint) m_Poly.CVertex( 0 );
    else
        return m_Start;
}


double PCB_SHAPE::GetLength() const
{
    double length = 0.0;

    switch( m_Shape )
    {
    case S_CURVE:
        for( size_t ii = 1; ii < m_BezierPoints.size(); ++ii )
            length += GetLineLength( m_BezierPoints[ii - 1], m_BezierPoints[ii] );

        break;

    case S_SEGMENT:
        length = GetLineLength( GetStart(), GetEnd() );
        break;

    case S_POLYGON:
        for( int ii = 0; ii < m_Poly.COutline( 0 ).SegmentCount(); ii++ )
            length += m_Poly.COutline( 0 ).CSegment( ii ).Length();

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
    if( m_Shape != S_POLYGON )
    {
        m_Start += aMoveVector;
        m_End += aMoveVector;
    }

    switch ( m_Shape )
    {
    case S_POLYGON:
        m_Poly.Move( VECTOR2I( aMoveVector ) );
        break;

    case S_ARC:
        m_ThirdPoint += aMoveVector;
        break;

    case S_CURVE:
        m_BezierC1 += aMoveVector;
        m_BezierC2 += aMoveVector;

        for( wxPoint& pt : m_BezierPoints)
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

    scalePt( m_Start );
    scalePt( m_End );

    // specific parameters:
    switch( m_Shape )
    {
    case S_CURVE:
        scalePt( m_BezierC1 );
        scalePt( m_BezierC2 );
        break;

    case S_ARC:
        scalePt( m_ThirdPoint );
        break;

    case S_CIRCLE:          //  ring or circle
        m_End.x = m_Start.x + KiROUND( radius * aScale );
        m_End.y = m_Start.y;
        break;

    case S_POLYGON:         // polygon
    {
        std::vector<wxPoint> pts;

        for( const VECTOR2I& pt : m_Poly.Outline( 0 ).CPoints() )
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
    switch( m_Shape )
    {
    case S_ARC:
    case S_SEGMENT:
    case S_CIRCLE:
        // these can all be done by just rotating the constituent points
        RotatePoint( &m_Start, aRotCentre, aAngle );
        RotatePoint( &m_End, aRotCentre, aAngle );
        RotatePoint( &m_ThirdPoint, aRotCentre, aAngle );
        break;

    case S_RECT:
        if( KiROUND( aAngle ) % 900 == 0 )
        {
            RotatePoint( &m_Start, aRotCentre, aAngle );
            RotatePoint( &m_End, aRotCentre, aAngle );
            break;
        }

        // Convert non-cartesian-rotated rect to a diamond
        m_Shape = S_POLYGON;
        m_Poly.RemoveAllContours();
        m_Poly.NewOutline();
        m_Poly.Append( m_Start );
        m_Poly.Append( m_End.x, m_Start.y );
        m_Poly.Append( m_End );
        m_Poly.Append( m_Start.x, m_End.y );

        KI_FALLTHROUGH;

    case S_POLYGON:
        m_Poly.Rotate( -DECIDEG2RAD( aAngle ), VECTOR2I( aRotCentre ) );
        break;

    case S_CURVE:
        RotatePoint( &m_Start, aRotCentre, aAngle);
        RotatePoint( &m_End, aRotCentre, aAngle);
        RotatePoint( &m_BezierC1, aRotCentre, aAngle);
        RotatePoint( &m_BezierC2, aRotCentre, aAngle);

        for( wxPoint& pt : m_BezierPoints )
            RotatePoint( &pt, aRotCentre, aAngle);

        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::Rotate not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_Shape ) );
        break;
    }
}


void PCB_SHAPE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
    }

    switch ( m_Shape )
    {
    case S_ARC:
        if( aFlipLeftRight )
            m_ThirdPoint.x   = aCentre.x - ( m_ThirdPoint.x - aCentre.x );
        else
            m_ThirdPoint.y   = aCentre.y - ( m_ThirdPoint.y - aCentre.y );

        m_Angle = -m_Angle;
        break;

    case S_POLYGON:
        m_Poly.Mirror( aFlipLeftRight, !aFlipLeftRight, VECTOR2I( aCentre ) );
        break;

    case S_CURVE:
        {
            if( aFlipLeftRight )
            {
                m_BezierC1.x = aCentre.x - ( m_BezierC1.x - aCentre.x );
                m_BezierC2.x = aCentre.x - ( m_BezierC2.x - aCentre.x );
            }
            else
            {
                m_BezierC1.y = aCentre.y - ( m_BezierC1.y - aCentre.y );
                m_BezierC2.y = aCentre.y - ( m_BezierC2.y - aCentre.y );
            }

            // Rebuild the poly points shape
            std::vector<wxPoint> ctrlPoints = { m_Start, m_BezierC1, m_BezierC2, m_End };
            BEZIER_POLY converter( ctrlPoints );
            converter.GetPoly( m_BezierPoints, m_Width );
        }
        break;

    case S_SEGMENT:
    case S_RECT:
    case S_CIRCLE:
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::Flip not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_Shape ) );
        break;
    }

    // PCB_SHAPE items are not allowed on copper layers, so
    // copper layers count is not taken in account in Flip transform
    SetLayer( FlipLayer( GetLayer() ) );
}


void PCB_SHAPE::RebuildBezierToSegmentsPointsList( int aMinSegLen )
{
    // Has meaning only for S_CURVE DRAW_SEGMENT shape
    if( m_Shape != S_CURVE )
    {
        m_BezierPoints.clear();
        return;
    }
    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    m_BezierPoints = buildBezierToSegmentsPointsList( aMinSegLen );
}


const std::vector<wxPoint> PCB_SHAPE::buildBezierToSegmentsPointsList( int aMinSegLen  ) const
{
    std::vector<wxPoint> bezierPoints;

    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    std::vector<wxPoint> ctrlPoints = { m_Start, m_BezierC1, m_BezierC2, m_End };
    BEZIER_POLY converter( ctrlPoints );
    converter.GetPoly( bezierPoints, aMinSegLen );

    return bezierPoints;
}


wxPoint PCB_SHAPE::GetCenter() const
{
    wxPoint c;

    switch( m_Shape )
    {
    case S_ARC:
    case S_CIRCLE:
        c = m_Start;
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
                    + PCB_SHAPE_TYPE_T_asString( m_Shape ) );
        break;
    }

    return c;
}


wxPoint PCB_SHAPE::GetArcEnd() const
{
    wxPoint endPoint( m_End );         // start of arc

    switch( m_Shape )
    {
    case S_ARC:
        endPoint = m_ThirdPoint;
        break;

    default:
        break;
    }

    return endPoint;   // after rotation, the end of the arc.
}


wxPoint PCB_SHAPE::GetArcMid() const
{
    wxPoint endPoint( m_End );

    switch( m_Shape )
    {
    case S_ARC:
        // rotate the starting point of the arc, given by m_End, through half
        // the angle m_Angle to get the middle of the arc.
        // m_Start is the arc centre
        endPoint  = m_End;         // m_End = start point of arc
        RotatePoint( &endPoint, m_Start, -m_Angle / 2.0 );
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
    m_Angle = NormalizeAngle360Max( aAngle );

    if( aUpdateEnd )
    {
        m_ThirdPoint = m_End;
        RotatePoint( &m_ThirdPoint, m_Start, -m_Angle );
    }
}


MODULE* PCB_SHAPE::GetParentFootprint() const
{
    if( !m_Parent || m_Parent->Type() != PCB_MODULE_T )
        return NULL;

    return (MODULE*) m_Parent;
}


void PCB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();
    ORIGIN_TRANSFORMS originTransforms = aFrame->GetOriginTransforms();
    wxString  msg;

    msg = _( "Drawing" );

    aList.emplace_back( _( "Type" ), msg, DARKCYAN );

    wxString    shape = _( "Shape" );

    switch( m_Shape )
    {
    case S_CIRCLE:
        aList.emplace_back( shape, _( "Circle" ), RED );

        msg = MessageTextFromValue( units, GetLineLength( m_Start, m_End ) );
        aList.emplace_back( _( "Radius" ), msg, RED );
        break;

    case S_ARC:
        aList.emplace_back( shape, _( "Arc" ), RED );
        msg.Printf( wxT( "%.1f" ), m_Angle / 10.0 );
        aList.emplace_back( _( "Angle" ), msg, RED );

        msg = MessageTextFromValue( units, GetLineLength( m_Start, m_End ) );
        aList.emplace_back( _( "Radius" ), msg, RED );
        break;

    case S_CURVE:
        aList.emplace_back( shape, _( "Curve" ), RED );

        msg = MessageTextFromValue( units, GetLength() );
        aList.emplace_back( _( "Length" ), msg, DARKGREEN );
        break;

    case S_POLYGON:
        aList.emplace_back( shape, _( "Polygon" ), RED );

        msg.Printf( "%d", GetPolyShape().Outline(0).PointCount() );
        aList.emplace_back( _( "Points" ), msg, DARKGREEN );
        break;

    case S_RECT:
        aList.emplace_back( shape, _( "Rectangle" ), RED );

        msg = MessageTextFromValue( units, std::abs( m_End.x - m_Start.x ) );
        aList.emplace_back( _( "Width" ), msg, DARKGREEN );

        msg = MessageTextFromValue( units, std::abs( m_End.y - m_Start.y ) );
        aList.emplace_back( _( "Height" ), msg, DARKGREEN );
        break;

    case S_SEGMENT:
    {
        aList.emplace_back( shape, _( "Segment" ), RED );

        msg = MessageTextFromValue( units, GetLineLength( m_Start, m_End ) );
        aList.emplace_back( _( "Length" ), msg, DARKGREEN );

        // angle counter-clockwise from 3'o-clock
        const double deg = RAD2DEG( atan2( (double)( m_Start.y - m_End.y ),
                                           (double)( m_End.x - m_Start.x ) ) );
        msg.Printf( wxT( "%.1f" ), deg );
        aList.emplace_back( _( "Angle" ), msg, DARKGREEN );
    }
        break;

    default:
        aList.emplace_back( shape, _( "Unrecognized" ), RED );
        break;
    }

    aList.emplace_back( _( "Layer" ), GetLayerName(), DARKBROWN );

    msg = MessageTextFromValue( units, m_Width );
    aList.emplace_back( _( "Width" ), msg, DARKCYAN );
}


const EDA_RECT PCB_SHAPE::GetBoundingBox() const
{
    EDA_RECT bbox;

    bbox.SetOrigin( m_Start );

    switch( m_Shape )
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
        bbox.SetEnd( m_End );
        break;

    case S_CIRCLE:
        bbox.Inflate( GetRadius() );
        break;

    case S_ARC:
        computeArcBBox( bbox );
        break;

    case S_POLYGON:
    {
        if( m_Poly.IsEmpty() )
            break;

        MODULE* parentFootprint = GetParentFootprint();
        bbox = EDA_RECT();  // re-init for merging

        for( auto iter = m_Poly.CIterate(); iter; iter++ )
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
        bbox.Merge( m_BezierC1 );
        bbox.Merge( m_BezierC2 );
        bbox.Merge( m_End );
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::GetBoundingBox not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_Shape ) );
        break;
    }

    bbox.Inflate( ((m_Width+1) / 2) + 1 );
    bbox.Normalize();

    return bbox;
}


bool PCB_SHAPE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int maxdist = aAccuracy + ( m_Width / 2 );

    switch( m_Shape )
    {
    case S_CIRCLE:
    {
        int radius = GetRadius();
        int dist   = KiROUND( EuclideanNorm( aPosition - GetCenter() ) );

        if( m_Width == 0 )      // Filled circle hit-test
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
                if( arc_hittest >= (3600.0 + GetAngle()) )
                    return true;
            }
        }
    }
        break;

    case S_CURVE:
        ( (PCB_SHAPE*) this)->RebuildBezierToSegmentsPointsList( m_Width );

        for( unsigned int i= 1; i < m_BezierPoints.size(); i++)
        {
            if( TestSegmentHit( aPosition, m_BezierPoints[i-1], m_BezierPoints[i], maxdist ) )
                return true;
        }
        break;

    case S_SEGMENT:
        if( TestSegmentHit( aPosition, m_Start, m_End, maxdist ) )
            return true;
        break;

    case S_RECT:
    {
        std::vector<wxPoint> pts = GetRectCorners();

        if( m_Width == 0 )          // Filled rect hit-test
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
        {
            if( !IsPolygonFilled() )
            {
                SHAPE_POLY_SET::VERTEX_INDEX dummy;
                return m_Poly.CollideEdge( VECTOR2I( aPosition ), dummy, maxdist );
            }
            else
                return m_Poly.Collide( VECTOR2I( aPosition ), maxdist );
        }
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::HitTest (point) not implemented for "
                    + PCB_SHAPE_TYPE_T_asString( m_Shape ) );
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

    switch( m_Shape )
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
            int count = m_Poly.TotalVertices();

            for( int ii = 0; ii < count; ii++ )
            {
                auto vertex = m_Poly.CVertex( ii );
                auto vertexNext = m_Poly.CVertex( ( ii + 1 ) % count );

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
            unsigned count = m_BezierPoints.size();

            for( unsigned ii = 1; ii < count; ii++ )
            {
                wxPoint vertex = m_BezierPoints[ii-1];
                wxPoint vertexNext = m_BezierPoints[ii];

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
                    + PCB_SHAPE_TYPE_T_asString( m_Shape ) );
        break;
    }

    return false;
}


wxString PCB_SHAPE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "%s on %s" ),
                             ShowShape( m_Shape ),
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
    if( m_Shape == S_ARC )
    {
        EDA_RECT bbox;
        bbox.SetOrigin( m_End );
        computeArcBBox( bbox );
        return BOX2I( bbox.GetOrigin(), bbox.GetSize() );
    }

    return EDA_ITEM::ViewBBox();
}


std::vector<wxPoint> PCB_SHAPE::GetRectCorners() const
{
    std::vector<wxPoint> pts;
    MODULE* parentFootprint = GetParentFootprint();
    wxPoint topLeft = GetStart();
    wxPoint botRight = GetEnd();

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
    aBBox.SetOrigin( m_End );

    wxPoint end = m_End;
    RotatePoint( &end, m_Start, -m_Angle );
    aBBox.Merge( end );

    // Determine the starting quarter
    // 0 right-bottom
    // 1 left-bottom
    // 2 left-top
    // 3 right-top
    unsigned int quarter = 0;       // assume right-bottom

    if( m_End.x < m_Start.x )
    {
        if( m_End.y <= m_Start.y )
            quarter = 2;
        else // ( m_End.y > m_Start.y )
            quarter = 1;
    }
    else if( m_End.x >= m_Start.x )
    {
        if( m_End.y < m_Start.y )
            quarter = 3;
        else if( m_End.x == m_Start.x )
            quarter = 1;
    }

    int radius = GetRadius();
    int angle = (int) GetArcAngleStart() % 900 + m_Angle;
    bool directionCW = ( m_Angle > 0 );      // Is the direction of arc clockwise?

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
        case 0: aBBox.Merge( wxPoint( m_Start.x,          m_Start.y + radius ) ); break;  // down
        case 1: aBBox.Merge( wxPoint( m_Start.x - radius, m_Start.y          ) ); break;  // left
        case 2: aBBox.Merge( wxPoint( m_Start.x,          m_Start.y - radius ) ); break;  // up
        case 3: aBBox.Merge( wxPoint( m_Start.x + radius, m_Start.y          ) ); break;  // right
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
    m_Poly.RemoveAllContours();
    m_Poly.NewOutline();

    for ( const wxPoint& p : aPoints )
        m_Poly.Append( p.x, p.y );
}


std::vector<SHAPE*> PCB_SHAPE::MakeEffectiveShapes() const
{
    std::vector<SHAPE*> effectiveShapes;

    switch( m_Shape )
    {
    case S_ARC:
    {
        SHAPE_ARC        arc( GetCenter(), GetArcStart(), (double) GetAngle() / 10.0 );
        SHAPE_LINE_CHAIN l = arc.ConvertToPolyline();

        for( int i = 0; i < l.SegmentCount(); i++ )
        {
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ).A,
                                                             l.Segment( i ).B, m_Width ) );
        }

        break;
    }

    case S_SEGMENT:
        effectiveShapes.emplace_back( new SHAPE_SEGMENT( GetStart(), GetEnd(), m_Width ) );
        break;

    case S_RECT:
    {
        std::vector<wxPoint> pts = GetRectCorners();

        if( m_Width == 0 )
        {
            effectiveShapes.emplace_back( new SHAPE_SIMPLE( pts ) );
        }
        else
        {
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[0], pts[1], m_Width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[1], pts[2], m_Width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[2], pts[3], m_Width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[3], pts[0], m_Width ) );
        }
    }
        break;

    case S_CIRCLE:
    {
        if( m_Width == 0 )
        {
            effectiveShapes.emplace_back( new SHAPE_CIRCLE( GetCenter(), GetRadius() ) );
        }
        else
        {
            // SHAPE_CIRCLE has no ConvertToPolyline() method, so use a 360.0 SHAPE_ARC
            SHAPE_ARC        circle( GetCenter(), GetEnd(), 360.0 );
            SHAPE_LINE_CHAIN l = circle.ConvertToPolyline();

            for( int i = 0; i < l.SegmentCount(); i++ )
            {
                effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ).A,
                                                                 l.Segment( i ).B, m_Width ) );
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
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( start_pt, end_pt, m_Width ) );
            start_pt = end_pt;
        }

        break;
    }

    case S_POLYGON:
    {
        SHAPE_LINE_CHAIN l = GetPolyShape().COutline( 0 );

        if( IsPolygonFilled() )
        {
            effectiveShapes.emplace_back( new SHAPE_SIMPLE( l ) );
        }

        if( !IsPolygonFilled() || m_Width > 0 )
        {
            for( int i = 0; i < l.SegmentCount(); i++ )
                effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ), m_Width ) );
        }
    }
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::MakeEffectiveShapes unsupported PCB_SHAPE shape: "
                    + PCB_SHAPE_TYPE_T_asString( m_Shape ) );
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

    if( m_Poly.OutlineCount() )
    {
        if( m_Poly.COutline( 0 ).PointCount() )
        {
            for ( auto iter = m_Poly.CIterate(); iter; iter++ )
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

    const SHAPE_LINE_CHAIN& outline = ((SHAPE_POLY_SET&)GetPolyShape()).Outline( 0 );

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

    std::swap( m_Width, image->m_Width );
    std::swap( m_Start, image->m_Start );
    std::swap( m_End, image->m_End );
    std::swap( m_ThirdPoint, image->m_ThirdPoint );
    std::swap( m_Shape, image->m_Shape );
    std::swap( m_Type, image->m_Type );
    std::swap( m_Angle, image->m_Angle );
    std::swap( m_BezierC1, image->m_BezierC1 );
    std::swap( m_BezierC2, image->m_BezierC2 );
    std::swap( m_BezierPoints, image->m_BezierPoints );
    std::swap( m_Poly, image->m_Poly );
    std::swap( m_Layer, image->m_Layer );
    std::swap( m_Flags, image->m_Flags );
    std::swap( m_Status, image->m_Status );
    std::swap( m_Parent, image->m_Parent );
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
