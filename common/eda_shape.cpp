/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <base_units.h>
#include <eda_draw_frame.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <eda_shape.h>


EDA_SHAPE::EDA_SHAPE( SHAPE_T aType, int aDefaultLineWidth ) :
    m_shape( aType ),
    m_width( aDefaultLineWidth )
{
    m_arcAngle = 0;
    m_filled = false;
}


EDA_SHAPE::~EDA_SHAPE()
{
}


wxString EDA_SHAPE::ShowShape() const
{
    switch( m_shape )
    {
    case SHAPE_T::SEGMENT: return _( "Line" );
    case SHAPE_T::RECT:    return _( "Rect" );
    case SHAPE_T::ARC:     return _( "Arc" );
    case SHAPE_T::CIRCLE:  return _( "Circle" );
    case SHAPE_T::BEZIER:  return _( "Bezier Curve" );
    case SHAPE_T::POLY:    return _( "Polygon" );
    default:               return wxT( "??" );
    }
}


wxString EDA_SHAPE::SHAPE_T_asString() const
{
    switch( m_shape )
    {
    case SHAPE_T::SEGMENT: return "S_SEGMENT";
    case SHAPE_T::RECT:    return "S_RECT";
    case SHAPE_T::ARC:     return "S_ARC";
    case SHAPE_T::CIRCLE:  return "S_CIRCLE";
    case SHAPE_T::POLY:    return "S_POLYGON";
    case SHAPE_T::BEZIER:  return "S_CURVE";
    case SHAPE_T::LAST:    return "!S_LAST!";  // Synthetic value, but if we come across it then
                                               // we're going to want to know.
    }

    return wxEmptyString;  // Just to quiet GCC.
}


void EDA_SHAPE::setPosition( const wxPoint& aPos )
{
    move( aPos - getPosition() );
}


wxPoint EDA_SHAPE::getPosition() const
{
    if( m_shape == SHAPE_T::ARC )
        return getCenter();
    else if( m_shape == SHAPE_T::POLY )
        return (wxPoint) m_poly.CVertex( 0 );
    else
        return m_start;
}


double EDA_SHAPE::GetLength() const
{
    double length = 0.0;

    switch( m_shape )
    {
    case SHAPE_T::BEZIER:
        for( size_t ii = 1; ii < m_bezierPoints.size(); ++ii )
            length += GetLineLength( m_bezierPoints[ ii - 1], m_bezierPoints[ii] );

        return length;

    case SHAPE_T::SEGMENT:
        return GetLineLength( GetStart(), GetEnd() );

    case SHAPE_T::POLY:
        for( int ii = 0; ii < m_poly.COutline( 0 ).SegmentCount(); ii++ )
            length += m_poly.COutline( 0 ).CSegment( ii ).Length();

        return length;

    case SHAPE_T::ARC:
        return 2 * M_PI * GetRadius() * ( GetArcAngle() / 3600.0 );

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return 0.0;
    }
}


void EDA_SHAPE::move( const wxPoint& aMoveVector )
{
    switch ( m_shape )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
        m_start += aMoveVector;
        m_end += aMoveVector;
        m_arcCenter += aMoveVector;
        break;

    case SHAPE_T::POLY:
        m_poly.Move( VECTOR2I( aMoveVector ) );
        break;

    case SHAPE_T::BEZIER:
        m_start += aMoveVector;
        m_end += aMoveVector;
        m_bezierC1 += aMoveVector;
        m_bezierC2 += aMoveVector;

        for( wxPoint& pt : m_bezierPoints)
            pt += aMoveVector;

        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::scale( double aScale )
{
    auto scalePt = [&]( wxPoint& pt )
                   {
                       pt.x = KiROUND( pt.x * aScale );
                       pt.y = KiROUND( pt.y * aScale );
                   };

    int radius = GetRadius();

    // specific parameters:
    switch( m_shape )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
        scalePt( m_start );
        scalePt( m_end );
        scalePt( m_arcCenter );
        break;

    case SHAPE_T::CIRCLE: //  ring or circle
        scalePt( m_start );
        m_end.x = m_start.x + KiROUND( radius * aScale );
        m_end.y = m_start.y;
        break;

    case SHAPE_T::POLY: // polygon
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

    case SHAPE_T::BEZIER:
        scalePt( m_start );
        scalePt( m_end );
        scalePt( m_bezierC1 );
        scalePt( m_bezierC2 );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::rotate( const wxPoint& aRotCentre, double aAngle )
{
    switch( m_shape )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
        // these can all be done by just rotating the constituent points
        RotatePoint( &m_start, aRotCentre, aAngle );
        RotatePoint( &m_end, aRotCentre, aAngle );
        RotatePoint( &m_arcCenter, aRotCentre, aAngle );
        break;

    case SHAPE_T::RECT:
        if( KiROUND( aAngle ) % 900 == 0 )
        {
            RotatePoint( &m_start, aRotCentre, aAngle );
            RotatePoint( &m_end, aRotCentre, aAngle );
            break;
        }

        // Convert non-cartesian-rotated rect to a diamond
        m_shape = SHAPE_T::POLY;
        m_poly.RemoveAllContours();
        m_poly.NewOutline();
        m_poly.Append( m_start );
        m_poly.Append( m_end.x, m_start.y );
        m_poly.Append( m_end );
        m_poly.Append( m_start.x, m_end.y );

        KI_FALLTHROUGH;

    case SHAPE_T::POLY:
        m_poly.Rotate( -DECIDEG2RAD( aAngle ), VECTOR2I( aRotCentre ) );
        break;

    case SHAPE_T::BEZIER:
        RotatePoint( &m_start, aRotCentre, aAngle);
        RotatePoint( &m_end, aRotCentre, aAngle);
        RotatePoint( &m_bezierC1, aRotCentre, aAngle);
        RotatePoint( &m_bezierC2, aRotCentre, aAngle);

        for( wxPoint& pt : m_bezierPoints )
            RotatePoint( &pt, aRotCentre, aAngle);

        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    switch ( m_shape )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::ARC:
        if( aFlipLeftRight )
        {
            m_start.x      = aCentre.x - ( m_start.x - aCentre.x );
            m_end.x        = aCentre.x - ( m_end.x - aCentre.x );
            m_arcCenter.x = aCentre.x - ( m_arcCenter.x - aCentre.x );
        }
        else
        {
            m_start.y      = aCentre.y - ( m_start.y - aCentre.y );
            m_end.y        = aCentre.y - ( m_end.y - aCentre.y );
            m_arcCenter.y = aCentre.y - ( m_arcCenter.y - aCentre.y );
        }
        break;

    case SHAPE_T::POLY:
        m_poly.Mirror( aFlipLeftRight, !aFlipLeftRight, VECTOR2I( aCentre ) );
        break;

    case SHAPE_T::BEZIER:
    {
        if( aFlipLeftRight )
        {
            m_start.x    = aCentre.x - ( m_start.x - aCentre.x );
            m_end.x      = aCentre.x - ( m_end.x - aCentre.x );
            m_bezierC1.x = aCentre.x - ( m_bezierC1.x - aCentre.x );
            m_bezierC2.x = aCentre.x - ( m_bezierC2.x - aCentre.x );
        }
        else
        {
            m_start.y    = aCentre.y - ( m_start.y - aCentre.y );
            m_end.y      = aCentre.y - ( m_end.y - aCentre.y );
            m_bezierC1.y = aCentre.y - ( m_bezierC1.y - aCentre.y );
            m_bezierC2.y = aCentre.y - ( m_bezierC2.y - aCentre.y );
        }

        // Rebuild the poly points shape
        std::vector<wxPoint> ctrlPoints = { m_start, m_bezierC1, m_bezierC2, m_end };
        BEZIER_POLY converter( ctrlPoints );
        converter.GetPoly( m_bezierPoints, m_width );
    }
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::RebuildBezierToSegmentsPointsList( int aMinSegLen )
{
    // Has meaning only for S_CURVE DRAW_SEGMENT shape
    if( m_shape != SHAPE_T::BEZIER )
    {
        m_bezierPoints.clear();
        return;
    }

    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    m_bezierPoints = buildBezierToSegmentsPointsList( aMinSegLen );
}


const std::vector<wxPoint> EDA_SHAPE::buildBezierToSegmentsPointsList( int aMinSegLen  ) const
{
    std::vector<wxPoint> bezierPoints;

    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    std::vector<wxPoint> ctrlPoints = { m_start, m_bezierC1, m_bezierC2, m_end };
    BEZIER_POLY converter( ctrlPoints );
    converter.GetPoly( bezierPoints, aMinSegLen );

    return bezierPoints;
}


wxPoint EDA_SHAPE::getCenter() const
{
    switch( m_shape )
    {
    case SHAPE_T::ARC:
        return m_arcCenter;

    case SHAPE_T::CIRCLE:
        return m_start;

    case SHAPE_T::SEGMENT:
        // Midpoint of the line
        return ( m_start + m_end ) / 2;

    case SHAPE_T::POLY:
    case SHAPE_T::RECT:
    case SHAPE_T::BEZIER:
        return getBoundingBox().Centre();

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return wxPoint();
    }
}


void EDA_SHAPE::SetCenter( const wxPoint& aCenter )
{
    switch( m_shape )
    {
    case SHAPE_T::ARC:
        m_arcCenter = aCenter;
        break;

    case SHAPE_T::CIRCLE:
        m_start = aCenter;
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


wxPoint EDA_SHAPE::GetArcMid() const
{
    wxPoint mid = m_start;
    RotatePoint( &mid, m_arcCenter, -m_arcAngle / 2.0 );
    return mid;
}


double EDA_SHAPE::GetArcAngleStart() const
{
    wxPoint arcStart = GetStart();
    wxPoint center = getCenter();
    double  angleStart = ArcTangente( arcStart.y - center.y, arcStart.x - center.x );

    // Normalize it to 0 ... 360 deg, to avoid discontinuity for angles near 180 deg
    // because 180 deg and -180 are very near angles when ampping betewwen -180 ... 180 deg.
    // and this is not easy to handle in calculations
    NORMALIZE_ANGLE_POS( angleStart );

    return angleStart;
}


double EDA_SHAPE::GetArcAngleEnd() const
{
    wxPoint arcEnd = GetEnd();
    wxPoint center = getCenter();
    double  angleStart = ArcTangente( arcEnd.y - center.y, arcEnd.x - center.x );

    // Normalize it to 0 ... 360 deg, to avoid discontinuity for angles near 180 deg
    // because 180 deg and -180 are very near angles when ampping betewwen -180 ... 180 deg.
    // and this is not easy to handle in calculations
    NORMALIZE_ANGLE_POS( angleStart );

    return angleStart;
}


int EDA_SHAPE::GetRadius() const
{
    double radius = 0.0;

    switch( m_shape )
    {
    case SHAPE_T::ARC:
        radius = GetLineLength( m_arcCenter, m_start );
        break;

    case SHAPE_T::CIRCLE:
        radius = GetLineLength( m_start, m_end );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    // don't allow degenerate circles/arcs
    return std::max( 1, KiROUND( radius ) );
}


void EDA_SHAPE::SetArcGeometry( const wxPoint& aStart, const wxPoint& aMid, const wxPoint& aEnd )
{
    m_start = aStart;
    m_end = aEnd;
    m_arcCenter = CalcArcCenter( aStart, aMid, aEnd );

    VECTOR2D startLine = m_start - m_arcCenter;
    VECTOR2D endLine   = aEnd - m_arcCenter;
    bool     clockwise = m_arcAngle > 0;

    m_arcAngle = RAD2DECIDEG( endLine.Angle() - startLine.Angle() );

    if( clockwise && m_arcAngle < 0.0 )
        m_arcAngle += 3600.0;
    else if( !clockwise && m_arcAngle > 0.0 )
        m_arcAngle -= 3600.0;
}


void EDA_SHAPE::SetArcAngle( double aAngle )
{
    // m_Angle must be >= -360 and <= +360 degrees
    m_arcAngle = NormalizeAngle360Max( aAngle );
}


void EDA_SHAPE::SetArcAngleAndEnd( double aAngle )
{
    // m_Angle must be >= -360 and <= +360 degrees
    m_arcAngle = NormalizeAngle360Max( aAngle );

    m_end = m_start;
    RotatePoint( &m_end, m_arcCenter, -m_arcAngle );
}


void EDA_SHAPE::ShapeGetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS         units = aFrame->GetUserUnits();
    ORIGIN_TRANSFORMS originTransforms = aFrame->GetOriginTransforms();
    wxString          msg;

    wxString shape = _( "Shape" );

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
        aList.emplace_back( shape, _( "Circle" ) );

        msg = MessageTextFromValue( units, GetLineLength( GetStart(), GetEnd() ) );
        aList.emplace_back( _( "Radius" ), msg );
        break;

    case SHAPE_T::ARC:
        aList.emplace_back( shape, _( "Arc" ) );

        msg.Printf( wxT( "%.1f" ), m_arcAngle / 10.0 );
        aList.emplace_back( _( "Angle" ), msg );

        msg = MessageTextFromValue( units, GetLineLength( getCenter(), GetStart() ) );
        aList.emplace_back( _( "Radius" ), msg );
        break;

    case SHAPE_T::BEZIER:
        aList.emplace_back( shape, _( "Curve" ) );

        msg = MessageTextFromValue( units, GetLength() );
        aList.emplace_back( _( "Length" ), msg );
        break;

    case SHAPE_T::POLY:
        aList.emplace_back( shape, _( "Polygon" ) );

        msg.Printf( "%d", GetPolyShape().Outline(0).PointCount() );
        aList.emplace_back( _( "Points" ), msg );
        break;

    case SHAPE_T::RECT:
        aList.emplace_back( shape, _( "Rectangle" ) );

        msg = MessageTextFromValue( units, std::abs( GetEnd().x - GetStart().x ) );
        aList.emplace_back( _( "Width" ), msg );

        msg = MessageTextFromValue( units, std::abs( GetEnd().y - GetStart().y ) );
        aList.emplace_back( _( "Height" ), msg );
        break;

    case SHAPE_T::SEGMENT:
    {
        aList.emplace_back( shape, _( "Segment" ) );

        msg = MessageTextFromValue( units, GetLineLength( GetStart(), GetEnd() ) );
        aList.emplace_back( _( "Length" ), msg );

        // angle counter-clockwise from 3'o-clock
        const double deg = RAD2DEG( atan2( (double)( GetStart().y - GetEnd().y ),
                                           (double)( GetEnd().x - GetStart().x ) ) );
        aList.emplace_back( _( "Angle" ), wxString::Format( "%.1f", deg ) );
    }
        break;

    default:
        aList.emplace_back( shape, _( "Unrecognized" ) );
        break;
    }

    aList.emplace_back( _( "Line width" ), MessageTextFromValue( units, m_width ) );
}


const EDA_RECT EDA_SHAPE::getBoundingBox() const
{
    EDA_RECT bbox;

    switch( m_shape )
    {
    case SHAPE_T::RECT:
        for( wxPoint& pt : GetRectCorners() )
            bbox.Merge( pt );

        break;

    case SHAPE_T::SEGMENT:
        bbox.SetOrigin( GetStart() );
        bbox.SetEnd( GetEnd() );
        break;

    case SHAPE_T::CIRCLE:
        bbox.SetOrigin( GetStart() );
        bbox.Inflate( GetRadius() );
        break;

    case SHAPE_T::ARC:
        computeArcBBox( bbox );
        break;

    case SHAPE_T::POLY:
        if( m_poly.IsEmpty() )
            break;

        for( auto iter = m_poly.CIterate(); iter; iter++ )
        {
            wxPoint pt( iter->x, iter->y );

            RotatePoint( &pt, getParentOrientation() );
            pt += getParentPosition();

            bbox.Merge( pt );
        }

        break;

    case SHAPE_T::BEZIER:
        bbox.SetOrigin( GetStart() );
        bbox.Merge( GetBezierC1() );
        bbox.Merge( GetBezierC2() );
        bbox.Merge( GetEnd() );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }

    bbox.Inflate( m_width / 2 );
    bbox.Normalize();

    return bbox;
}


bool EDA_SHAPE::hitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int maxdist = aAccuracy + ( m_width / 2 );

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
    {
        int radius = GetRadius();
        int dist   = KiROUND( EuclideanNorm( aPosition - getCenter() ) );

        if( IsFilled() )
            return dist <= radius + maxdist;          // Filled circle hit-test
        else
            return abs( radius - dist ) <= maxdist;   // Ring hit-test
    }

    case SHAPE_T::ARC:
    {
        wxPoint relPos = aPosition - getCenter();
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

            // Check angle: inside the arc angle when it is > 0 and outside the not drawn arc when
            // it is < 0
            if( GetArcAngle() >= 0.0 )
                return arc_hittest <= GetArcAngle();
            else
                return arc_hittest >= ( 3600.0 + GetArcAngle() );
        }
        else
        {
            return false;
        }
    }

    case SHAPE_T::BEZIER:
        const_cast<EDA_SHAPE*>( this )->RebuildBezierToSegmentsPointsList( m_width );

        for( unsigned int i= 1; i < m_bezierPoints.size(); i++)
        {
            if( TestSegmentHit( aPosition, m_bezierPoints[ i - 1], m_bezierPoints[i], maxdist ) )
                return true;
        }

        return false;

    case SHAPE_T::SEGMENT:
        return TestSegmentHit( aPosition, GetStart(), GetEnd(), maxdist );

    case SHAPE_T::RECT:
        if( IsFilled() )            // Filled rect hit-test
        {
            SHAPE_POLY_SET poly;
            poly.NewOutline();

            for( const wxPoint& pt : GetRectCorners() )
                poly.Append( pt );

            return poly.Collide( VECTOR2I( aPosition ), maxdist );
        }
        else                        // Open rect hit-test
        {
            std::vector<wxPoint> pts = GetRectCorners();

            return TestSegmentHit( aPosition, pts[0], pts[1], maxdist )
                    || TestSegmentHit( aPosition, pts[1], pts[2], maxdist )
                    || TestSegmentHit( aPosition, pts[2], pts[3], maxdist )
                    || TestSegmentHit( aPosition, pts[3], pts[0], maxdist );
        }

    case SHAPE_T::POLY:
        if( IsFilled() )
        {
            return m_poly.Collide( VECTOR2I( aPosition ), maxdist );
        }
        else
        {
            SHAPE_POLY_SET::VERTEX_INDEX dummy;
            return m_poly.CollideEdge( VECTOR2I( aPosition ), dummy, maxdist );
        }

    default:
        wxFAIL_MSG( "EDA_SHAPE::hitTest(point) not implemented for " + SHAPE_T_asString() );
        return false;
    }
}


bool EDA_SHAPE::hitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    EDA_RECT arcRect;
    EDA_RECT bb = getBoundingBox();

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
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
                return arect.IntersectsCircleEdge( getCenter(), GetRadius(), GetWidth() );
            }
        }

    case SHAPE_T::ARC:
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
                   arcRect.IntersectsCircleEdge( getCenter(), GetRadius(), GetWidth() );
        }

    case SHAPE_T::RECT:
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

    case SHAPE_T::SEGMENT:
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

    case SHAPE_T::POLY:
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

            // Polygons in footprints use coordinates relative to the footprint.
            // Therefore, instead of using m_poly, we make a copy which is translated
            // to the actual location in the board.
            double  orientation = 0.0;
            wxPoint offset = getParentPosition();

            if( getParentOrientation() )
                orientation = -DECIDEG2RAD( getParentOrientation() );

            SHAPE_POLY_SET poly{ m_poly };
            poly.Rotate( orientation );
            poly.Move( offset );

            int count = poly.TotalVertices();

            for( int ii = 0; ii < count; ii++ )
            {
                VECTOR2I vertex = poly.CVertex( ii );
                VECTOR2I vertexNext = poly.CVertex( ( ii + 1 ) % count );

                // Test if the point is within aRect
                if( arect.Contains( ( wxPoint ) vertex ) )
                    return true;

                // Test if this edge intersects aRect
                if( arect.Intersects( ( wxPoint ) vertex, ( wxPoint ) vertexNext ) )
                    return true;
            }

            return false;
        }

    case SHAPE_T::BEZIER:
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

            return false;
        }

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return false;
    }
}


std::vector<wxPoint> EDA_SHAPE::GetRectCorners() const
{
    std::vector<wxPoint> pts;
    wxPoint              topLeft = GetStart();
    wxPoint              botRight = GetEnd();

    // Un-rotate rect topLeft and botRight
    if( KiROUND( getParentOrientation() ) % 900 != 0 )
    {
        topLeft -= getParentPosition();
        RotatePoint( &topLeft, -getParentOrientation() );

        botRight -= getParentPosition();
        RotatePoint( &botRight, -getParentOrientation() );
    }

    // Set up the un-rotated 4 corners
    pts.emplace_back( topLeft );
    pts.emplace_back( botRight.x, topLeft.y );
    pts.emplace_back( botRight );
    pts.emplace_back( topLeft.x, botRight.y );

    // Now re-rotate the 4 corners to get a diamond
    if( KiROUND( getParentOrientation() ) % 900 != 0 )
    {
        for( wxPoint& pt : pts )
        {
            RotatePoint( &pt, getParentOrientation() );
            pt += getParentPosition();
        }
    }

    return pts;
}


void EDA_SHAPE::computeArcBBox( EDA_RECT& aBBox ) const
{
    // Do not include the center, which is not necessarily
    // inside the BB of a arc with a small angle
    aBBox.SetOrigin( m_start );
    aBBox.Merge( m_end );

    // Determine the starting quarter
    // 0 right-bottom
    // 1 left-bottom
    // 2 left-top
    // 3 right-top
    unsigned int quarter = 0;       // assume right-bottom

    if( m_start.x < m_arcCenter.x )
    {
        if( m_start.y <= m_arcCenter.y )
            quarter = 2;
        else // ( m_start.y > m_arcCenter.y )
            quarter = 1;
    }
    else if( m_start.x >= m_arcCenter.x )
    {
        if( m_start.y < m_arcCenter.y )
            quarter = 3;
        else if( m_start.x == m_arcCenter.x )
            quarter = 1;
    }

    int radius = GetRadius();
    int angle = (int) GetArcAngleStart() % 900 + m_arcAngle;
    bool directionCW = ( m_arcAngle > 0 );      // Is the direction of arc clockwise?

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
        case 0: aBBox.Merge( wxPoint( m_arcCenter.x,          m_arcCenter.y + radius ) ); break;  // down
        case 1: aBBox.Merge( wxPoint( m_arcCenter.x - radius, m_arcCenter.y          ) ); break;  // left
        case 2: aBBox.Merge( wxPoint( m_arcCenter.x,          m_arcCenter.y - radius ) ); break;  // up
        case 3: aBBox.Merge( wxPoint( m_arcCenter.x + radius, m_arcCenter.y          ) ); break;  // right
        }

        if( directionCW )
            ++quarter;
        else
            quarter += 3;       // -1 modulo arithmetic

        quarter %= 4;
        angle -= 900;
    }

    aBBox.Inflate( m_width );   // Technically m_width / 2, but it doesn't hurt to have the
                                // bounding box a bit large to account for drawing clearances,
                                // etc.
}


void EDA_SHAPE::SetPolyPoints( const std::vector<wxPoint>& aPoints )
{
    m_poly.RemoveAllContours();
    m_poly.NewOutline();

    for ( const wxPoint& p : aPoints )
        m_poly.Append( p.x, p.y );
}


std::vector<SHAPE*> EDA_SHAPE::MakeEffectiveShapes() const
{
    std::vector<SHAPE*> effectiveShapes;

    switch( m_shape )
    {
    case SHAPE_T::ARC:
        effectiveShapes.emplace_back( new SHAPE_ARC( m_arcCenter, m_start, m_arcAngle / 10.0,
                                                     m_width ) );
        break;

    case SHAPE_T::SEGMENT:
        effectiveShapes.emplace_back( new SHAPE_SEGMENT( m_start, m_end, m_width ) );
        break;

    case SHAPE_T::RECT:
    {
        std::vector<wxPoint> pts = GetRectCorners();

        if( IsFilled() )
            effectiveShapes.emplace_back( new SHAPE_SIMPLE( pts ) );

        if( m_width > 0 || !IsFilled() )
        {
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[0], pts[1], m_width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[1], pts[2], m_width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[2], pts[3], m_width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[3], pts[0], m_width ) );
        }
    }
        break;

    case SHAPE_T::CIRCLE:
    {
        if( IsFilled() )
            effectiveShapes.emplace_back( new SHAPE_CIRCLE( getCenter(), GetRadius() ) );

        if( m_width > 0 || !IsFilled() )
        {
            // SHAPE_CIRCLE has no ConvertToPolyline() method, so use a 360.0 SHAPE_ARC
            SHAPE_ARC        circle( getCenter(), GetEnd(), 360.0 );
            SHAPE_LINE_CHAIN l = circle.ConvertToPolyline();

            for( int i = 0; i < l.SegmentCount(); i++ )
            {
                effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ).A, l.Segment( i ).B,
                                                                 m_width ) );
            }
        }

        break;
    }

    case SHAPE_T::BEZIER:
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

    case SHAPE_T::POLY:
    {
        SHAPE_LINE_CHAIN l = GetPolyShape().COutline( 0 );

        l.Rotate( -DECIDEG2RAD( getParentOrientation() ) );
        l.Move( getParentPosition() );

        if( IsFilled() )
            effectiveShapes.emplace_back( new SHAPE_SIMPLE( l ) );

        if( m_width > 0 || !IsFilled() )
        {
            for( int i = 0; i < l.SegmentCount(); i++ )
                effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.Segment( i ), m_width ) );
        }
    }
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }

    return effectiveShapes;
}


void EDA_SHAPE::DupPolyPointsList( std::vector<wxPoint>& aBuffer ) const
{
    if( m_poly.OutlineCount() )
    {
        int pointCount = m_poly.COutline( 0 ).PointCount();

        if( pointCount )
        {
            aBuffer.reserve( pointCount );

            for ( auto iter = m_poly.CIterate(); iter; iter++ )
                aBuffer.emplace_back( iter->x, iter->y );
        }
    }
}


bool EDA_SHAPE::IsPolyShapeValid() const
{
    // return true if the polygonal shape is valid (has more than 2 points)
    if( GetPolyShape().OutlineCount() == 0 )
        return false;

    const SHAPE_LINE_CHAIN& outline = ( (SHAPE_POLY_SET&)GetPolyShape() ).Outline( 0 );

    return outline.PointCount() > 2;
}


int EDA_SHAPE::GetPointCount() const
{
    // return the number of corners of the polygonal shape
    // this shape is expected to be only one polygon without hole
    if( GetPolyShape().OutlineCount() )
        return GetPolyShape().VertexCount( 0 );

    return 0;
}


void EDA_SHAPE::SwapShape( EDA_SHAPE* aImage )
{
    EDA_SHAPE* image = dynamic_cast<EDA_SHAPE*>( aImage );
    assert( image );

    std::swap( m_width, image->m_width );
    std::swap( m_start, image->m_start );
    std::swap( m_end, image->m_end );
    std::swap( m_arcCenter, image->m_arcCenter );
    std::swap( m_shape, image->m_shape );
    std::swap( m_arcAngle, image->m_arcAngle );
    std::swap( m_bezierC1, image->m_bezierC1 );
    std::swap( m_bezierC2, image->m_bezierC2 );
    std::swap( m_bezierPoints, image->m_bezierPoints );
    std::swap( m_poly, image->m_poly );
}


int EDA_SHAPE::Compare( const EDA_SHAPE* aOther ) const
{
#define TEST( a, b ) { if( a != b ) return a < b; }
#define TEST_PT( a, b ) { TEST( a.x, b.x ); TEST( a.y, b.y ); }

    TEST_PT( m_start, aOther->m_start );
    TEST_PT( m_end, aOther->m_end );

    TEST( m_shape, aOther->m_shape );

    if( m_shape == SHAPE_T::ARC )
    {
        TEST_PT( m_arcCenter, aOther->m_arcCenter );
    }
    else if( m_shape == SHAPE_T::BEZIER )
    {
        TEST_PT( m_bezierC1, aOther->m_bezierC1 );
        TEST_PT( m_bezierC2, aOther->m_bezierC2 );
    }
    else if( m_shape == SHAPE_T::POLY )
    {
        TEST( m_poly.TotalVertices(), aOther->m_poly.TotalVertices() );
    }

    for( size_t ii = 0; ii < m_bezierPoints.size(); ++ii )
        TEST_PT( m_bezierPoints[ii], aOther->m_bezierPoints[ii] );

    for( int ii = 0; ii < m_poly.TotalVertices(); ++ii )
        TEST_PT( m_poly.CVertex( ii ), aOther->m_poly.CVertex( ii ) );

    TEST( m_width, aOther->m_width );

    return 0;
}