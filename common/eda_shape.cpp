/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_shape.h>

#include <bezier_curves.h>
#include <convert_basic_shapes_to_polygon.h>
#include <eda_draw_frame.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND
#include <eda_item.h>
#include <plotters/plotter.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/common/types/base_types.pb.h>


EDA_SHAPE::EDA_SHAPE( SHAPE_T aType, int aLineWidth, FILL_T aFill ) :
        m_endsSwapped( false ),
        m_shape( aType ),
        m_stroke( aLineWidth, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_fill( aFill ),
        m_fillColor( COLOR4D::UNSPECIFIED ),
        m_rectangleHeight( 0 ),
        m_rectangleWidth( 0 ),
        m_segmentLength( 0 ),
        m_editState( 0 ),
        m_proxyItem( false )
{
}


EDA_SHAPE::~EDA_SHAPE()
{
}


EDA_SHAPE::EDA_SHAPE( const SHAPE& aShape ) :
        m_endsSwapped( false ),
        m_stroke( 0, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_fill(),
        m_rectangleHeight( 0 ),
        m_rectangleWidth( 0 ),
        m_segmentLength( 0 ),
        m_editState( 0 ),
        m_proxyItem( false )
{
    switch( aShape.Type() )
    {
    case SH_RECT:
    {
        auto rect = static_cast<const SHAPE_RECT&>( aShape );
        m_shape = SHAPE_T::RECTANGLE;
        SetStart( rect.GetPosition() );
        SetEnd( rect.GetPosition() + rect.GetSize() );
        break;
    }

    case SH_SEGMENT:
    {
        auto seg = static_cast<const SHAPE_SEGMENT&>( aShape );
        m_shape = SHAPE_T::SEGMENT;
        SetStart( seg.GetSeg().A );
        SetEnd( seg.GetSeg().B );
        SetWidth( seg.GetWidth() );
        break;
    }

    case SH_LINE_CHAIN:
    {
        auto line = static_cast<const SHAPE_LINE_CHAIN&>( aShape );
        m_shape = SHAPE_T::POLY;
        m_poly = SHAPE_POLY_SET();
        m_poly.AddOutline( line );
        SetWidth( line.Width() );
        break;
    }

    case SH_CIRCLE:
    {
        auto circle = static_cast<const SHAPE_CIRCLE&>( aShape );
        m_shape = SHAPE_T::CIRCLE;
        SetStart( circle.GetCenter() );
        SetEnd( circle.GetCenter() + circle.GetRadius() );
        break;
    }

    case SH_ARC:
    {
        auto arc = static_cast<const SHAPE_ARC&>( aShape );
        m_shape = SHAPE_T::ARC;
        SetArcGeometry( arc.GetP0(), arc.GetArcMid(), arc.GetP1() );
        SetWidth( arc.GetWidth() );
        break;
    }

    case SH_SIMPLE:
    {
        auto poly = static_cast<const SHAPE_SIMPLE&>( aShape );
        m_shape = SHAPE_T::POLY;
        poly.TransformToPolygon( m_poly, 0, ERROR_INSIDE );
        break;
    }

    // currently unhandled
    case SH_POLY_SET:
    case SH_COMPOUND:
    case SH_NULL:
    case SH_POLY_SET_TRIANGLE:
    default:
        m_shape = SHAPE_T::UNDEFINED;
        break;
    }
}


void EDA_SHAPE::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    types::GraphicShape shape;

    types::StrokeAttributes* stroke = shape.mutable_attributes()->mutable_stroke();
    types::GraphicFillAttributes* fill = shape.mutable_attributes()->mutable_fill();

    stroke->mutable_width()->set_value_nm( GetWidth() );

    switch( GetLineStyle() )
    {
    case LINE_STYLE::DEFAULT:    stroke->set_style( types::SLS_DEFAULT ); break;
    case LINE_STYLE::SOLID:      stroke->set_style( types::SLS_SOLID ); break;
    case LINE_STYLE::DASH:       stroke->set_style( types::SLS_DASH ); break;
    case LINE_STYLE::DOT:        stroke->set_style( types::SLS_DOT ); break;
    case LINE_STYLE::DASHDOT:    stroke->set_style( types::SLS_DASHDOT ); break;
    case LINE_STYLE::DASHDOTDOT: stroke->set_style( types::SLS_DASHDOTDOT ); break;
    default: break;
    }

    switch( GetFillMode() )
    {
    case FILL_T::FILLED_SHAPE: fill->set_fill_type( types::GFT_FILLED ); break;
    default:                   fill->set_fill_type( types::GFT_UNFILLED ); break;
    }

    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:
    {
        types::GraphicSegmentAttributes* segment = shape.mutable_segment();
        PackVector2( *segment->mutable_start(), GetStart() );
        PackVector2( *segment->mutable_end(), GetEnd() );
        break;
    }

    case SHAPE_T::RECTANGLE:
    {
        types::GraphicRectangleAttributes* rectangle = shape.mutable_rectangle();
        PackVector2( *rectangle->mutable_top_left(), GetStart() );
        PackVector2( *rectangle->mutable_bottom_right(), GetEnd() );
        break;
    }

    case SHAPE_T::ARC:
    {
        types::GraphicArcAttributes* arc = shape.mutable_arc();
        PackVector2( *arc->mutable_start(), GetStart() );
        PackVector2( *arc->mutable_mid(), GetArcMid() );
        PackVector2( *arc->mutable_end(), GetEnd() );
        break;
    }

    case SHAPE_T::CIRCLE:
    {
        types::GraphicCircleAttributes* circle = shape.mutable_circle();
        PackVector2( *circle->mutable_center(), GetStart() );
        PackVector2( *circle->mutable_radius_point(), GetEnd() );
        break;
    }

    case SHAPE_T::POLY:
    {
        PackPolySet( *shape.mutable_polygon(), GetPolyShape() );
        break;
    }

    case SHAPE_T::BEZIER:
    {
        types::GraphicBezierAttributes* bezier = shape.mutable_bezier();
        PackVector2( *bezier->mutable_start(), GetStart() );
        PackVector2( *bezier->mutable_control1(), GetBezierC1() );
        PackVector2( *bezier->mutable_control2(), GetBezierC2() );
        PackVector2( *bezier->mutable_end(), GetEnd() );
        break;
    }

    default:
        wxASSERT_MSG( false, "Unhandled shape in PCB_SHAPE::Serialize" );
    }

    // TODO m_hasSolderMask and m_solderMaskMargin

    aContainer.PackFrom( shape );
}


bool EDA_SHAPE::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;

    types::GraphicShape shape;

    if( !aContainer.UnpackTo( &shape ) )
        return false;

    // Initialize everything to a known state that doesn't get touched by every
    // codepath below, to make sure the equality operator is consistent
    m_start = {};
    m_end = {};
    m_arcCenter = {};
    m_arcMidData = {};
    m_bezierC1 = {};
    m_bezierC2 = {};
    m_editState = 0;
    m_proxyItem = false;
    m_endsSwapped = false;

    SetFilled( shape.attributes().fill().fill_type() == types::GFT_FILLED );
    SetWidth( shape.attributes().stroke().width().value_nm() );

    switch( shape.attributes().stroke().style() )
    {
    case types::SLS_DEFAULT:    SetLineStyle( LINE_STYLE::DEFAULT );    break;
    case types::SLS_SOLID:      SetLineStyle( LINE_STYLE::SOLID );      break;
    case types::SLS_DASH:       SetLineStyle( LINE_STYLE::DASH );       break;
    case types::SLS_DOT:        SetLineStyle( LINE_STYLE::DOT );        break;
    case types::SLS_DASHDOT:    SetLineStyle( LINE_STYLE::DASHDOT );    break;
    case types::SLS_DASHDOTDOT: SetLineStyle( LINE_STYLE::DASHDOTDOT ); break;
    default: break;
    }

    if( shape.has_segment() )
    {
        SetShape( SHAPE_T::SEGMENT );
        SetStart( UnpackVector2( shape.segment().start() ) );
        SetEnd( UnpackVector2( shape.segment().end() ) );
    }
    else if( shape.has_rectangle() )
    {
        SetShape( SHAPE_T::RECTANGLE );
        SetStart( UnpackVector2( shape.rectangle().top_left() ) );
        SetEnd( UnpackVector2( shape.rectangle().bottom_right() ) );
    }
    else if( shape.has_arc() )
    {
        SetShape( SHAPE_T::ARC );
        SetArcGeometry( UnpackVector2( shape.arc().start() ),
                        UnpackVector2( shape.arc().mid() ),
                        UnpackVector2( shape.arc().end() ) );
    }
    else if( shape.has_circle() )
    {
        SetShape( SHAPE_T::CIRCLE );
        SetStart( UnpackVector2( shape.circle().center() ) );
        SetEnd( UnpackVector2( shape.circle().radius_point() ) );
    }
    else if( shape.has_polygon() )
    {
        SetShape( SHAPE_T::POLY );
        SetPolyShape( UnpackPolySet( shape.polygon() ) );
    }
    else if( shape.has_bezier() )
    {
        SetShape( SHAPE_T::BEZIER );
        SetStart( UnpackVector2( shape.bezier().start() ) );
        SetBezierC1( UnpackVector2( shape.bezier().control1() ) );
        SetBezierC2( UnpackVector2( shape.bezier().control2() ) );
        SetEnd( UnpackVector2( shape.bezier().end() ) );
        RebuildBezierToSegmentsPointsList( ARC_HIGH_DEF );
    }

    return true;
}


wxString EDA_SHAPE::ShowShape() const
{
    if( IsProxyItem() )
    {
        switch( m_shape )
        {
        case SHAPE_T::SEGMENT:   return _( "Thermal Spoke" );
        case SHAPE_T::RECTANGLE: return _( "Number Box" );
        default:                 return wxT( "??" );
        }
    }
    else
    {
        switch( m_shape )
        {
        case SHAPE_T::SEGMENT:   return _( "Line" );
        case SHAPE_T::RECTANGLE: return _( "Rect" );
        case SHAPE_T::ARC:       return _( "Arc" );
        case SHAPE_T::CIRCLE:    return _( "Circle" );
        case SHAPE_T::BEZIER:    return _( "Bezier Curve" );
        case SHAPE_T::POLY:      return _( "Polygon" );
        default:                 return wxT( "??" );
        }
    }
}


wxString EDA_SHAPE::SHAPE_T_asString() const
{
    switch( m_shape )
    {
    case SHAPE_T::SEGMENT:   return wxS( "S_SEGMENT" );
    case SHAPE_T::RECTANGLE: return wxS( "S_RECT" );
    case SHAPE_T::ARC:       return wxS( "S_ARC" );
    case SHAPE_T::CIRCLE:    return wxS( "S_CIRCLE" );
    case SHAPE_T::POLY:      return wxS( "S_POLYGON" );
    case SHAPE_T::BEZIER:    return wxS( "S_CURVE" );
    case SHAPE_T::UNDEFINED: return wxS( "UNDEFINED" );
    }

    return wxEmptyString;  // Just to quiet GCC.
}


void EDA_SHAPE::setPosition( const VECTOR2I& aPos )
{
    move( aPos - getPosition() );
}


VECTOR2I EDA_SHAPE::getPosition() const
{
    if( m_shape == SHAPE_T::ARC )
        return getCenter();
    else if( m_shape == SHAPE_T::POLY )
        return m_poly.CVertex( 0 );
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
            length += m_bezierPoints[ ii - 1].Distance( m_bezierPoints[ii] );

        return length;

    case SHAPE_T::SEGMENT:
        return GetStart().Distance( GetEnd() );

    case SHAPE_T::POLY:
        for( int ii = 0; ii < m_poly.COutline( 0 ).SegmentCount(); ii++ )
            length += m_poly.COutline( 0 ).CSegment( ii ).Length();

        return length;

    case SHAPE_T::ARC:
        return GetRadius() * GetArcAngle().AsRadians();

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return 0.0;
    }
}

int EDA_SHAPE::GetRectangleHeight() const
{
    switch( m_shape )
    {
    case SHAPE_T::RECTANGLE:
        return GetEndY() - GetStartY();

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return 0;
    }
}

int EDA_SHAPE::GetRectangleWidth() const
{
    switch( m_shape )
    {
    case SHAPE_T::RECTANGLE:
        return GetEndX() - GetStartX();

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return 0;
    }
}

void EDA_SHAPE::SetLength( const double& aLength )
{
    switch( m_shape )
    {
    case SHAPE_T::SEGMENT:
        m_segmentLength = aLength; break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}

void EDA_SHAPE::SetRectangleHeight( const int& aHeight )
{
    switch ( m_shape )
    {
    case SHAPE_T::RECTANGLE:
        m_rectangleHeight = aHeight;
        SetEndY( GetStartY() + m_rectangleHeight );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}

void EDA_SHAPE::SetRectangleWidth( const int& aWidth )
{
    switch ( m_shape )
    {
    case SHAPE_T::RECTANGLE:
        m_rectangleWidth = aWidth;
        SetEndX( GetStartX() + m_rectangleWidth );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}

void EDA_SHAPE::SetRectangle( const long long int& aHeight, const long long int& aWidth )
{
    switch ( m_shape )
    {
    case SHAPE_T::RECTANGLE:
        m_rectangleHeight = aHeight;
        m_rectangleWidth = aWidth;
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


void EDA_SHAPE::SetSegmentAngle( const EDA_ANGLE& aAngle )
{
    switch( m_shape )
    {
    case SHAPE_T::SEGMENT:
        m_segmentAngle = aAngle;
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}

bool EDA_SHAPE::IsClosed() const
{
    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECTANGLE:
        return true;

    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
        return false;

    case SHAPE_T::POLY:
        if( m_poly.IsEmpty() )
            return false;
        else
            return m_poly.Outline( 0 ).IsClosed();

    case SHAPE_T::BEZIER:
        if( m_bezierPoints.size() < 3 )
            return false;
        else
            return m_bezierPoints[0] == m_bezierPoints[ m_bezierPoints.size() - 1 ];

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return false;
    }
}



void EDA_SHAPE::move( const VECTOR2I& aMoveVector )
{
    switch ( m_shape )
    {
    case SHAPE_T::ARC:
        m_arcCenter += aMoveVector;
        KI_FALLTHROUGH;

    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECTANGLE:
    case SHAPE_T::CIRCLE:
        m_start += aMoveVector;
        m_end += aMoveVector;
        break;

    case SHAPE_T::POLY:
        m_poly.Move( aMoveVector );
        break;

    case SHAPE_T::BEZIER:
        m_start += aMoveVector;
        m_end += aMoveVector;
        m_bezierC1 += aMoveVector;
        m_bezierC2 += aMoveVector;

        for( VECTOR2I& pt : m_bezierPoints )
            pt += aMoveVector;

        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::scale( double aScale )
{
    auto scalePt = [&]( VECTOR2I& pt )
                   {
                       pt.x = KiROUND( pt.x * aScale );
                       pt.y = KiROUND( pt.y * aScale );
                   };

    switch( m_shape )
    {
    case SHAPE_T::ARC:
        scalePt( m_arcCenter );
        KI_FALLTHROUGH;

    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECTANGLE:
        scalePt( m_start );
        scalePt( m_end );
        break;

    case SHAPE_T::CIRCLE: //  ring or circle
        scalePt( m_start );
        m_end.x = m_start.x + KiROUND( GetRadius() * aScale );
        m_end.y = m_start.y;
        break;

    case SHAPE_T::POLY: // polygon
    {
        std::vector<VECTOR2I> pts;

        for( int ii = 0; ii < m_poly.OutlineCount(); ++ ii )
        {
            for( const VECTOR2I& pt : m_poly.Outline( ii ).CPoints() )
            {
                pts.emplace_back( pt );
                scalePt( pts.back() );
            }
        }

        SetPolyPoints( pts );
    }
        break;

    case SHAPE_T::BEZIER:
        scalePt( m_start );
        scalePt( m_end );
        scalePt( m_bezierC1 );
        scalePt( m_bezierC2 );
        RebuildBezierToSegmentsPointsList( m_stroke.GetWidth() / 2 );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    switch( m_shape )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
        RotatePoint( m_start, aRotCentre, aAngle );
        RotatePoint( m_end, aRotCentre, aAngle );
        break;

    case SHAPE_T::ARC:
        RotatePoint( m_start, aRotCentre, aAngle );
        RotatePoint( m_end, aRotCentre, aAngle );
        RotatePoint( m_arcCenter, aRotCentre, aAngle );
        break;

    case SHAPE_T::RECTANGLE:
        if( aAngle.IsCardinal() )
        {
            RotatePoint( m_start, aRotCentre, aAngle );
            RotatePoint( m_end, aRotCentre, aAngle );
            break;
        }

        // Convert non-cardinally-rotated rect to a diamond
        m_shape = SHAPE_T::POLY;
        m_poly.RemoveAllContours();
        m_poly.NewOutline();
        m_poly.Append( m_start );
        m_poly.Append( m_end.x, m_start.y );
        m_poly.Append( m_end );
        m_poly.Append( m_start.x, m_end.y );

        KI_FALLTHROUGH;

    case SHAPE_T::POLY:
        m_poly.Rotate( aAngle, aRotCentre );
        break;

    case SHAPE_T::BEZIER:
        RotatePoint( m_start, aRotCentre, aAngle );
        RotatePoint( m_end, aRotCentre, aAngle );
        RotatePoint( m_bezierC1, aRotCentre, aAngle );
        RotatePoint( m_bezierC2, aRotCentre, aAngle );

        for( VECTOR2I& pt : m_bezierPoints )
            RotatePoint( pt, aRotCentre, aAngle);

        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    switch ( m_shape )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::RECTANGLE:
        MIRROR( m_start, aCentre, aFlipDirection );
        MIRROR( m_end, aCentre, aFlipDirection );
        break;

    case SHAPE_T::CIRCLE:
        MIRROR( m_start, aCentre, aFlipDirection );
        MIRROR( m_end, aCentre, aFlipDirection );
        break;

    case SHAPE_T::ARC:
        MIRROR( m_start, aCentre, aFlipDirection );
        MIRROR( m_end, aCentre, aFlipDirection );
        MIRROR( m_arcCenter, aCentre, aFlipDirection );

        std::swap( m_start, m_end );
        break;

    case SHAPE_T::POLY:
        m_poly.Mirror( aCentre, aFlipDirection );
        break;

    case SHAPE_T::BEZIER:
        MIRROR( m_start, aCentre, aFlipDirection );
        MIRROR( m_end, aCentre, aFlipDirection );
        MIRROR( m_bezierC1, aCentre, aFlipDirection );
        MIRROR( m_bezierC2, aCentre, aFlipDirection );

        RebuildBezierToSegmentsPointsList( m_stroke.GetWidth() / 2 );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::RebuildBezierToSegmentsPointsList( int aMaxError )
{
    // Has meaning only for SHAPE_T::BEZIER
    if( m_shape != SHAPE_T::BEZIER )
    {
        m_bezierPoints.clear();
        return;
    }

    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    m_bezierPoints = buildBezierToSegmentsPointsList( aMaxError );
}


const std::vector<VECTOR2I> EDA_SHAPE::buildBezierToSegmentsPointsList( int aMaxError ) const
{
    std::vector<VECTOR2I> bezierPoints;

    // Rebuild the m_BezierPoints vertex list that approximate the Bezier curve
    std::vector<VECTOR2I> ctrlPoints = { m_start, m_bezierC1, m_bezierC2, m_end };
    BEZIER_POLY converter( ctrlPoints );
    converter.GetPoly( bezierPoints, aMaxError );

    return bezierPoints;
}


VECTOR2I EDA_SHAPE::getCenter() const
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
    case SHAPE_T::RECTANGLE:
    case SHAPE_T::BEZIER:
        return getBoundingBox().Centre();

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return VECTOR2I();
    }
}


void EDA_SHAPE::SetCenter( const VECTOR2I& aCenter )
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


VECTOR2I EDA_SHAPE::GetArcMid() const
{
    // If none of the input data have changed since we loaded the arc,
    // keep the original mid point data to minimize churn
    if( m_arcMidData.start == m_start && m_arcMidData.end == m_end
            && m_arcMidData.center == m_arcCenter )
        return m_arcMidData.mid;

    VECTOR2I mid = m_start;
    RotatePoint( mid, m_arcCenter, -GetArcAngle() / 2.0 );
    return mid;
}


void EDA_SHAPE::CalcArcAngles( EDA_ANGLE& aStartAngle, EDA_ANGLE& aEndAngle ) const
{
    VECTOR2D startRadial( GetStart() - getCenter() );
    VECTOR2D endRadial( GetEnd() - getCenter() );

    aStartAngle = EDA_ANGLE( startRadial );
    aEndAngle = EDA_ANGLE( endRadial );

    if( aEndAngle == aStartAngle )
        aEndAngle = aStartAngle + ANGLE_360; // ring, not null

    while( aEndAngle < aStartAngle )
        aEndAngle += ANGLE_360;
}


int EDA_SHAPE::GetRadius() const
{
    double radius = 0.0;

    switch( m_shape )
    {
    case SHAPE_T::ARC:
        radius = m_arcCenter.Distance( m_start );
        break;

    case SHAPE_T::CIRCLE:
        radius = m_start.Distance( m_end );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    // don't allow degenerate circles/arcs
    return std::max( 1, KiROUND( radius ) );
}


void EDA_SHAPE::SetCachedArcData( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd, const VECTOR2I& aCenter )
{
    m_arcMidData.start = aStart;
    m_arcMidData.end = aEnd;
    m_arcMidData.center = aCenter;
    m_arcMidData.mid = aMid;
}


void EDA_SHAPE::SetArcGeometry( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    m_arcMidData = {};
    m_start = aStart;
    m_end = aEnd;
    m_arcCenter = CalcArcCenter( aStart, aMid, aEnd );
    VECTOR2I new_mid = GetArcMid();

    m_endsSwapped = false;

    // Watch the ordering here.  GetArcMid above needs to be called prior to initializing the
    // m_arcMidData structure in order to ensure we get the calculated variant, not the cached
    SetCachedArcData( aStart, aMid, aEnd, m_arcCenter );

    /*
     * If the input winding doesn't match our internal winding, the calculated midpoint will end
     * up on the other side of the arc.  In this case, we need to flip the start/end points and
     * flag this change for the system.
     */
    VECTOR2D dist( new_mid - aMid );
    VECTOR2D dist2( new_mid - m_arcCenter );

    if( dist.SquaredEuclideanNorm() > dist2.SquaredEuclideanNorm() )
    {
        std::swap( m_start, m_end );
        m_endsSwapped = true;
    }
}

EDA_ANGLE EDA_SHAPE::GetSegmentAngle() const
{
    EDA_ANGLE angle( atan2( static_cast<double>( GetStart().y - GetEnd().y ),
                            static_cast<double>( GetEnd().x - GetStart().x ) ), RADIANS_T );

    return angle;
}


EDA_ANGLE EDA_SHAPE::GetArcAngle() const
{
    EDA_ANGLE startAngle;
    EDA_ANGLE endAngle;

    CalcArcAngles( startAngle, endAngle );

    return endAngle - startAngle;
}


bool EDA_SHAPE::IsClockwiseArc() const
{
    if( m_shape == SHAPE_T::ARC )
    {
        VECTOR2D mid = GetArcMid();

        double orient = ( mid.x - m_start.x ) * ( m_end.y - m_start.y )
                        - ( mid.y - m_start.y ) * ( m_end.x - m_start.x );

        return orient < 0;
    }

    UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    return false;
}


void EDA_SHAPE::SetArcAngleAndEnd( const EDA_ANGLE& aAngle, bool aCheckNegativeAngle )
{
    EDA_ANGLE angle( aAngle );

    m_end = m_start;
    RotatePoint( m_end, m_arcCenter, -angle.Normalize720() );

    if( aCheckNegativeAngle && aAngle < ANGLE_0 )
    {
        std::swap( m_start, m_end );
        m_endsSwapped = true;
    }
}


wxString EDA_SHAPE::getFriendlyName() const
{
    if( IsProxyItem() )
    {
        switch( m_shape )
        {
        case SHAPE_T::RECTANGLE: return _( "Pad Number Box" );
        case SHAPE_T::SEGMENT:   return _( "Thermal Spoke Template" );
        default:                 return _( "Unrecognized" );
        }
    }
    else
    {
        switch( m_shape )
        {
        case SHAPE_T::CIRCLE:    return _( "Circle" );
        case SHAPE_T::ARC:       return _( "Arc" );
        case SHAPE_T::BEZIER:    return _( "Curve" );
        case SHAPE_T::POLY:      return _( "Polygon" );
        case SHAPE_T::RECTANGLE: return _( "Rectangle" );
        case SHAPE_T::SEGMENT:   return _( "Segment" );
        default:                 return _( "Unrecognized" );
        }
    }
}


void EDA_SHAPE::ShapeGetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    wxString shape = _( "Shape" );
    aList.emplace_back( shape, getFriendlyName() );

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
        aList.emplace_back( _( "Radius" ), aFrame->MessageTextFromValue( GetRadius() ) );
        break;

    case SHAPE_T::ARC:
        msg = EDA_UNIT_UTILS::UI::MessageTextFromValue( GetArcAngle() );
        aList.emplace_back( _( "Angle" ), msg );

        aList.emplace_back( _( "Radius" ), aFrame->MessageTextFromValue( GetRadius() ) );
        break;

    case SHAPE_T::BEZIER:
        aList.emplace_back( _( "Length" ), aFrame->MessageTextFromValue( GetLength() ) );
        break;

    case SHAPE_T::POLY:
        msg.Printf( wxS( "%d" ), GetPolyShape().Outline(0).PointCount() );
        aList.emplace_back( _( "Points" ), msg );
        break;

    case SHAPE_T::RECTANGLE:
        aList.emplace_back( _( "Width" ),
                            aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );

        aList.emplace_back( _( "Height" ),
                            aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ) );
        break;

    case SHAPE_T::SEGMENT:
    {
        aList.emplace_back( _( "Length" ),
                            aFrame->MessageTextFromValue( GetStart().Distance( GetEnd() ) ));

        // angle counter-clockwise from 3'o-clock
        EDA_ANGLE angle( atan2( (double)( GetStart().y - GetEnd().y ),
                                (double)( GetEnd().x - GetStart().x ) ), RADIANS_T );
        aList.emplace_back( _( "Angle" ), EDA_UNIT_UTILS::UI::MessageTextFromValue( angle ) );
        break;
    }

    default:
        break;
    }

    m_stroke.GetMsgPanelInfo( aFrame, aList );
}


const BOX2I EDA_SHAPE::getBoundingBox() const
{
    BOX2I bbox;

    switch( m_shape )
    {
    case SHAPE_T::RECTANGLE:
        for( VECTOR2I& pt : GetRectCorners() )
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
            bbox.Merge( *iter );

        break;

    case SHAPE_T::BEZIER:
        // Bezier BBoxes are not trivial to compute, so we approximate it by
        // using the bounding box of the curve (not control!) points.
        for( const VECTOR2I& pt : m_bezierPoints )
            bbox.Merge( pt );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }

    bbox.Inflate( std::max( 0, GetWidth() ) / 2 );
    bbox.Normalize();

    return bbox;
}


bool EDA_SHAPE::hitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    double maxdist = aAccuracy;

    if( GetWidth() > 0 )
        maxdist += GetWidth() / 2.0;

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
    {
        double radius = GetRadius();
        double dist = aPosition.Distance( getCenter() );

        if( IsFilledForHitTesting() )
            return dist <= radius + maxdist;          // Filled circle hit-test
        else
            return abs( radius - dist ) <= maxdist;   // Ring hit-test
    }

    case SHAPE_T::ARC:
    {
        if( aPosition.Distance( m_start ) <= maxdist )
            return true;

        if( aPosition.Distance( m_end ) <= maxdist )
            return true;

        double radius = GetRadius();
        VECTOR2D relPos( VECTOR2D( aPosition ) - getCenter() );
        double dist = relPos.EuclideanNorm();

        if( IsFilledForHitTesting() )
        {
            // Check distance from arc center
            if( dist > radius + maxdist )
                return false;
        }
        else
        {
            // Check distance from arc circumference
            if( abs( radius - dist ) > maxdist )
                return false;
        }

        // Finally, check to see if it's within arc's swept angle.
        EDA_ANGLE startAngle;
        EDA_ANGLE endAngle;
        CalcArcAngles( startAngle, endAngle );

        EDA_ANGLE relPosAngle( relPos );

        startAngle.Normalize();
        endAngle.Normalize();
        relPosAngle.Normalize();

        if( endAngle > startAngle )
            return relPosAngle >= startAngle && relPosAngle <= endAngle;
        else
            return relPosAngle >= startAngle || relPosAngle <= endAngle;
    }

    case SHAPE_T::BEZIER:
    {
        const std::vector<VECTOR2I>* pts = &m_bezierPoints;
        std::vector<VECTOR2I> updatedBezierPoints;

        if( m_bezierPoints.empty() )
        {
            BEZIER_POLY converter( m_start, m_bezierC1, m_bezierC2, m_end );
            converter.GetPoly( updatedBezierPoints, aAccuracy / 2 );
            pts = &updatedBezierPoints;
        }

        for( unsigned int i = 1; i < pts->size(); i++ )
        {
            if( TestSegmentHit( aPosition, ( *pts )[i - 1], ( *pts )[i], maxdist ) )
                return true;
        }

        return false;
    }
    case SHAPE_T::SEGMENT:
        return TestSegmentHit( aPosition, GetStart(), GetEnd(), maxdist );

    case SHAPE_T::RECTANGLE:
        if( IsProxyItem() || IsFilledForHitTesting() )   // Filled rect hit-test
        {
            SHAPE_POLY_SET poly;
            poly.NewOutline();

            for( const VECTOR2I& pt : GetRectCorners() )
                poly.Append( pt );

            return poly.Collide( aPosition, maxdist );
        }
        else                                            // Open rect hit-test
        {
            std::vector<VECTOR2I> pts = GetRectCorners();

            return TestSegmentHit( aPosition, pts[0], pts[1], maxdist )
                    || TestSegmentHit( aPosition, pts[1], pts[2], maxdist )
                    || TestSegmentHit( aPosition, pts[2], pts[3], maxdist )
                    || TestSegmentHit( aPosition, pts[3], pts[0], maxdist );
        }

    case SHAPE_T::POLY:
        if( IsFilledForHitTesting() )
        {
            if( !m_poly.COutline( 0 ).IsClosed() )
            {
                // Only one outline is expected
                SHAPE_LINE_CHAIN copy( m_poly.COutline( 0 ) );
                copy.SetClosed( true );
                return copy.Collide( aPosition, maxdist );
            }
            else
            {
                return m_poly.Collide( aPosition, maxdist );
            }
        }
        else
        {
            return m_poly.CollideEdge( aPosition, nullptr, maxdist );
        }

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return false;
    }
}


bool EDA_SHAPE::hitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Normalize();
    arect.Inflate( aAccuracy );

    BOX2I bbox = getBoundingBox();

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
        // Test if area intersects or contains the circle:
        if( aContained )
        {
            return arect.Contains( bbox );
        }
        else
        {
            // If the rectangle does not intersect the bounding box, this is a much quicker test
            if( !arect.Intersects( bbox ) )
                return false;
            else
                return arect.IntersectsCircleEdge( getCenter(), GetRadius(), GetWidth() );
        }

    case SHAPE_T::ARC:
        // Test for full containment of this arc in the rect
        if( aContained )
        {
            return arect.Contains( bbox );
        }
        // Test if the rect crosses the arc
        else
        {
            if( !arect.Intersects( bbox ) )
                return false;

            if( IsFilled() )
            {
                return ( arect.Intersects( getCenter(), GetStart() )
                      || arect.Intersects( getCenter(), GetEnd() )
                      || arect.IntersectsCircleEdge( getCenter(), GetRadius(), GetWidth() ) );
            }
            else
            {
                return arect.IntersectsCircleEdge( getCenter(), GetRadius(), GetWidth() );
            }
        }

    case SHAPE_T::RECTANGLE:
        if( aContained )
        {
            return arect.Contains( bbox );
        }
        else
        {
            std::vector<VECTOR2I> pts = GetRectCorners();

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
            return arect.Contains( bbox );
        }
        else
        {
            // Fast test: if aRect is outside the polygon bounding box,
            // rectangles cannot intersect
            if( !arect.Intersects( bbox ) )
                return false;

            // Account for the width of the line
            arect.Inflate( GetWidth() / 2 );

            for( int ii = 0; ii < m_poly.OutlineCount(); ++ii )
            {
                const SHAPE_LINE_CHAIN& poly = m_poly.Outline( ii );
                int                     count = poly.GetPointCount();

                for( int jj = 0; jj < count; jj++ )
                {
                    VECTOR2I vertex = poly.GetPoint( jj );

                    // Test if the point is within aRect
                    if( arect.Contains( vertex ) )
                        return true;

                    if( jj + 1 < count )
                    {
                        VECTOR2I vertexNext = poly.GetPoint( jj + 1 );

                        // Test if this edge intersects aRect
                        if( arect.Intersects( vertex, vertexNext ) )
                            return true;
                    }
                    else if( poly.IsClosed() )
                    {
                        VECTOR2I vertexNext = poly.GetPoint( 0 );

                        // Test if this edge intersects aRect
                        if( arect.Intersects( vertex, vertexNext ) )
                            return true;
                    }
                }
            }

            return false;
        }

    case SHAPE_T::BEZIER:
        if( aContained )
        {
            return arect.Contains( bbox );
        }
        else
        {
            // Fast test: if aRect is outside the polygon bounding box,
            // rectangles cannot intersect
            if( !arect.Intersects( bbox ) )
                return false;

            // Account for the width of the line
            arect.Inflate( GetWidth() / 2 );
            const std::vector<VECTOR2I>* pts = &m_bezierPoints;
            std::vector<VECTOR2I> updatedBezierPoints;

            if( m_bezierPoints.empty() )
            {
                BEZIER_POLY converter( m_start, m_bezierC1, m_bezierC2, m_end );
                converter.GetPoly( updatedBezierPoints, aAccuracy / 2 );
                pts = &updatedBezierPoints;
            }

            for( unsigned ii = 1; ii < pts->size(); ii++ )
            {
                VECTOR2I vertex = ( *pts )[ii - 1];
                VECTOR2I vertexNext = ( *pts )[ii];

                // Test if the point is within aRect
                if( arect.Contains( vertex ) )
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


std::vector<VECTOR2I> EDA_SHAPE::GetRectCorners() const
{
    std::vector<VECTOR2I> pts;
    VECTOR2I              topLeft = GetStart();
    VECTOR2I              botRight = GetEnd();

    pts.emplace_back( topLeft );
    pts.emplace_back( botRight.x, topLeft.y );
    pts.emplace_back( botRight );
    pts.emplace_back( topLeft.x, botRight.y );

    return pts;
}


void EDA_SHAPE::computeArcBBox( BOX2I& aBBox ) const
{
    // Start, end, and each inflection point the arc crosses will enclose the entire arc.
    // Only include the center when filled; it's not necessarily inside the BB of an unfilled
    // arc with a small included angle.
    aBBox.SetOrigin( m_start );
    aBBox.Merge( m_end );

    if( IsFilled() )
        aBBox.Merge( m_arcCenter );

    int       radius = GetRadius();
    EDA_ANGLE t1, t2;

    CalcArcAngles( t1, t2 );

    t1.Normalize();
    t2.Normalize();

    if( t2 > t1 )
    {
        if( t1 < ANGLE_0 && t2 > ANGLE_0 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x + radius, m_arcCenter.y ) ); // right

        if( t1 < ANGLE_90 && t2 > ANGLE_90 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x, m_arcCenter.y + radius ) ); // down

        if( t1 < ANGLE_180 && t2 > ANGLE_180 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x - radius, m_arcCenter.y ) ); // left

        if( t1 < ANGLE_270 && t2 > ANGLE_270 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x, m_arcCenter.y - radius ) ); // up
    }
    else
    {
        if( t1 < ANGLE_0 || t2 > ANGLE_0 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x + radius, m_arcCenter.y ) ); // right

        if( t1 < ANGLE_90 || t2 > ANGLE_90 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x, m_arcCenter.y + radius ) ); // down

        if( t1 < ANGLE_180 || t2 > ANGLE_180 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x - radius, m_arcCenter.y ) ); // left

        if( t1 < ANGLE_270 || t2 > ANGLE_270 )
            aBBox.Merge( VECTOR2I( m_arcCenter.x, m_arcCenter.y - radius ) ); // up
    }
}


void EDA_SHAPE::SetPolyPoints( const std::vector<VECTOR2I>& aPoints )
{
    m_poly.RemoveAllContours();
    m_poly.NewOutline();

    for( const VECTOR2I& p : aPoints )
        m_poly.Append( p.x, p.y );
}


std::vector<SHAPE*> EDA_SHAPE::makeEffectiveShapes( bool aEdgeOnly, bool aLineChainOnly ) const
{
    std::vector<SHAPE*> effectiveShapes;
    int                 width = GetEffectiveWidth();

    switch( m_shape )
    {
    case SHAPE_T::ARC:
        effectiveShapes.emplace_back( new SHAPE_ARC( m_arcCenter, m_start, GetArcAngle(), width ) );
        break;

    case SHAPE_T::SEGMENT:
        effectiveShapes.emplace_back( new SHAPE_SEGMENT( m_start, m_end, width ) );
        break;

    case SHAPE_T::RECTANGLE:
    {
        std::vector<VECTOR2I> pts = GetRectCorners();

        if( ( IsFilled() || IsProxyItem() ) && !aEdgeOnly )
            effectiveShapes.emplace_back( new SHAPE_SIMPLE( pts ) );

        if( width > 0 || !IsFilled() || aEdgeOnly )
        {
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[0], pts[1], width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[1], pts[2], width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[2], pts[3], width ) );
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( pts[3], pts[0], width ) );
        }
    }
        break;

    case SHAPE_T::CIRCLE:
    {
        if( IsFilled() && !aEdgeOnly )
            effectiveShapes.emplace_back( new SHAPE_CIRCLE( getCenter(), GetRadius() ) );

        if( width > 0 || !IsFilled() || aEdgeOnly )
            effectiveShapes.emplace_back( new SHAPE_ARC( getCenter(), GetEnd(), ANGLE_360, width ) );

        break;
    }

    case SHAPE_T::BEZIER:
    {
        std::vector<VECTOR2I> bezierPoints = buildBezierToSegmentsPointsList( width / 2 );
        VECTOR2I              start_pt = bezierPoints[0];

        for( unsigned int jj = 1; jj < bezierPoints.size(); jj++ )
        {
            VECTOR2I end_pt = bezierPoints[jj];
            effectiveShapes.emplace_back( new SHAPE_SEGMENT( start_pt, end_pt, width ) );
            start_pt = end_pt;
        }

        break;
    }

    case SHAPE_T::POLY:
    {
        if( GetPolyShape().OutlineCount() == 0 )    // malformed/empty polygon
            break;

        for( int ii = 0; ii < GetPolyShape().OutlineCount(); ++ii )
        {
            const SHAPE_LINE_CHAIN& l = GetPolyShape().COutline( ii );

            if( IsFilled() && !aEdgeOnly )
                effectiveShapes.emplace_back( new SHAPE_SIMPLE( l ) );

            if( width > 0 || !IsFilled() || aEdgeOnly )
            {
                int segCount = l.SegmentCount();

                if( aLineChainOnly && l.IsClosed() )
                    segCount--; // Treat closed chain as open

                for( int jj = 0; jj < segCount; jj++ )
                    effectiveShapes.emplace_back( new SHAPE_SEGMENT( l.CSegment( jj ), width ) );
            }
        }
    }
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }

    return effectiveShapes;
}


void EDA_SHAPE::DupPolyPointsList( std::vector<VECTOR2I>& aBuffer ) const
{
    for( int ii = 0; ii < m_poly.OutlineCount(); ++ii )
    {
        int pointCount = m_poly.COutline( ii ).PointCount();

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
    return GetPolyShape().OutlineCount() > 0 && GetPolyShape().Outline( 0 ).PointCount() > 2;
}


int EDA_SHAPE::GetPointCount() const
{
    // return the number of corners of the polygonal shape
    // this shape is expected to be only one polygon without hole
    return GetPolyShape().OutlineCount() ? GetPolyShape().VertexCount( 0 ) : 0;
}


void EDA_SHAPE::beginEdit( const VECTOR2I& aPosition )
{
    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECTANGLE:
        SetStart( aPosition );
        SetEnd( aPosition );
        break;

    case SHAPE_T::ARC:
        SetArcGeometry( aPosition, aPosition, aPosition );
        m_editState = 1;
        break;

    case SHAPE_T::BEZIER:
        SetStart( aPosition );
        SetEnd( aPosition );
        SetBezierC1( aPosition );
        SetBezierC2( aPosition );
        m_editState = 1;

        RebuildBezierToSegmentsPointsList( GetWidth() / 2 );
        break;

    case SHAPE_T::POLY:
        m_poly.NewOutline();
        m_poly.Outline( 0 ).SetClosed( false );

        // Start and end of the first segment (co-located for now)
        m_poly.Outline( 0 ).Append( aPosition );
        m_poly.Outline( 0 ).Append( aPosition, true );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


bool EDA_SHAPE::continueEdit( const VECTOR2I& aPosition )
{
    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECTANGLE:
        return false;

    case SHAPE_T::BEZIER:
        if( m_editState == 3 )
            return false;

        m_editState++;
        return true;

    case SHAPE_T::POLY:
    {
        SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        // do not add zero-length segments
        if( poly.CPoint( poly.GetPointCount() - 2 ) != poly.CLastPoint() )
            poly.Append( aPosition, true );
    }
        return true;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        return false;
    }
}


void EDA_SHAPE::calcEdit( const VECTOR2I& aPosition )
{
#define sq( x ) pow( x, 2 )

    switch( GetShape() )
    {
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECTANGLE:
        SetEnd( aPosition );
        break;

    case SHAPE_T::BEZIER:
    {
        switch( m_editState )
        {
        case 0:
            SetStart( aPosition );
            SetEnd( aPosition );
            SetBezierC1( aPosition );
            SetBezierC2( aPosition );
            break;
        case 1:
            SetBezierC2( aPosition );
            SetEnd( aPosition );
            break;
        case 2: SetBezierC1( aPosition ); break;
        case 3: SetBezierC2( aPosition ); break;
        }

        RebuildBezierToSegmentsPointsList( GetWidth() / 2 );
    }
    break;

    case SHAPE_T::ARC:
    {
        double    radius = GetRadius();
        EDA_ANGLE lastAngle = GetArcAngle();

        // Edit state 0: drawing: place start
        // Edit state 1: drawing: place end (center calculated for 90-degree subtended angle)
        // Edit state 2: point edit: move start (center calculated for invariant subtended angle)
        // Edit state 3: point edit: move end (center calculated for invariant subtended angle)
        // Edit state 4: point edit: move center
        // Edit state 5: point edit: move arc-mid-point

        switch( m_editState )
        {
        case 0:
            SetArcGeometry( aPosition, aPosition, aPosition );
            return;

        case 1:
            m_end = aPosition;
            radius = m_start.Distance( m_end ) * M_SQRT1_2;
            break;

        case 2:
        case 3:
        {
            VECTOR2I v = m_start - m_end;
            double chordBefore = v.SquaredEuclideanNorm();

            if( m_editState == 2 )
                m_start = aPosition;
            else
                m_end = aPosition;

            v = m_start - m_end;

            double chordAfter = v.SquaredEuclideanNorm();
            double ratio = 0.0;

            if( chordBefore > 0 )
                ratio = chordAfter / chordBefore;

            if( ratio != 0 )
                radius = std::max( sqrt( sq( radius ) * ratio ), sqrt( chordAfter ) / 2 );
        }
            break;

        case 4:
        {
            double radialA = m_start.Distance( aPosition );
            double radialB = m_end.Distance( aPosition );
            radius = ( radialA + radialB ) / 2.0;
        }
            break;

        case 5:
            SetArcGeometry( GetStart(), aPosition, GetEnd() );
            return;
        }

        // Calculate center based on start, end, and radius
        //
        // Let 'l' be the length of the chord and 'm' the middle point of the chord
        double   l = m_start.Distance( m_end );
        VECTOR2D m = ( m_start + m_end ) / 2;
        double   sqRadDiff = ( radius * radius ) - ( l * l ) / 4.0;

        // Calculate 'd', the vector from the chord midpoint to the center
        VECTOR2D d;

        if( l > 0 && sqRadDiff >= 0 )
        {
            d.x = sqrt( sqRadDiff ) * ( m_start.y - m_end.y ) / l;
            d.y = sqrt( sqRadDiff ) * ( m_end.x - m_start.x ) / l;
        }

        VECTOR2I c1 = KiROUND( m + d );
        VECTOR2I c2 = KiROUND( m - d );

        // Solution gives us 2 centers; we need to pick one:
        switch( m_editState )
        {
        case 1:
            // Keep arc clockwise while drawing i.e. arc angle = 90 deg.
            // it can be 90 or 270 deg depending on the arc center choice (c1 or c2)
            m_arcCenter = c1; // first trial

            if( GetArcAngle() > ANGLE_180 )
                m_arcCenter = c2;

            break;

        case 2:
        case 3:
            // Pick the one of c1, c2 to keep arc on the same side
            m_arcCenter = c1; // first trial

            if( ( lastAngle < ANGLE_180 ) != ( GetArcAngle() < ANGLE_180 ) )
                m_arcCenter = c2;

            break;

        case 4:
            // Pick the one closer to the mouse position
            m_arcCenter = c1.Distance( aPosition ) < c2.Distance( aPosition ) ? c1 : c2;
            break;
        }
    }
        break;

    case SHAPE_T::POLY:
        m_poly.Outline( 0 ).SetPoint( m_poly.Outline( 0 ).GetPointCount() - 1, aPosition );
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


void EDA_SHAPE::endEdit( bool aClosed )
{
    switch( GetShape() )
    {
    case SHAPE_T::ARC:
    case SHAPE_T::SEGMENT:
    case SHAPE_T::CIRCLE:
    case SHAPE_T::RECTANGLE:
    case SHAPE_T::BEZIER:
        break;

    case SHAPE_T::POLY:
    {
        SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        // do not include last point twice
        if( poly.GetPointCount() > 2 )
        {
            if( poly.CPoint( poly.GetPointCount() - 2 ) == poly.CLastPoint() )
            {
                poly.SetClosed( aClosed );
            }
            else
            {
                poly.SetClosed( false );
                poly.Remove( poly.GetPointCount() - 1 );
            }
        }
    }
        break;

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }
}


void EDA_SHAPE::SwapShape( EDA_SHAPE* aImage )
{
    EDA_SHAPE* image = dynamic_cast<EDA_SHAPE*>( aImage );
    assert( image );

    #define SWAPITEM( x ) std::swap( x, image->x )
    SWAPITEM( m_stroke );
    SWAPITEM( m_start );
    SWAPITEM( m_end );
    SWAPITEM( m_arcCenter );
    SWAPITEM( m_shape );
    SWAPITEM( m_bezierC1 );
    SWAPITEM( m_bezierC2 );
    SWAPITEM( m_bezierPoints );
    SWAPITEM( m_poly );
    SWAPITEM( m_fill );
    SWAPITEM( m_fillColor );
    SWAPITEM( m_editState );
    SWAPITEM( m_endsSwapped );
    #undef SWAPITEM
}


int EDA_SHAPE::Compare( const EDA_SHAPE* aOther ) const
{
#define EPSILON 2       // Should be enough for rounding errors on calculated items

#define TEST( a, b ) { if( a != b ) return a - b; }
#define TEST_E( a, b ) { if( abs( a - b ) > EPSILON ) return a - b; }
#define TEST_PT( a, b ) { TEST_E( a.x, b.x ); TEST_E( a.y, b.y ); }

    TEST_PT( m_start, aOther->m_start );
    TEST_PT( m_end, aOther->m_end );

    TEST( (int) m_shape, (int) aOther->m_shape );

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

    TEST_E( m_stroke.GetWidth(), aOther->m_stroke.GetWidth() );
    TEST( (int) m_stroke.GetLineStyle(), (int) aOther->m_stroke.GetLineStyle() );
    TEST( (int) m_fill, (int) aOther->m_fill );

    return 0;
}


void EDA_SHAPE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, int aClearance, int aError,
                                         ERROR_LOC aErrorLoc, bool ignoreLineWidth ) const
{
    int width = ignoreLineWidth ? 0 : GetWidth();

    width += 2 * aClearance;

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
    {
        int r = GetRadius();

        if( IsFilled() )
            TransformCircleToPolygon( aBuffer, getCenter(), r + width / 2, aError, aErrorLoc );
        else
            TransformRingToPolygon( aBuffer, getCenter(), r, width, aError, aErrorLoc );

        break;
    }

    case SHAPE_T::RECTANGLE:
    {
        std::vector<VECTOR2I> pts = GetRectCorners();

        if( IsFilled() || IsProxyItem() )
        {
            aBuffer.NewOutline();

            for( const VECTOR2I& pt : pts )
                aBuffer.Append( pt );
        }

        if( width > 0 || !IsFilled() )
        {
            // Add in segments
            TransformOvalToPolygon( aBuffer, pts[0], pts[1], width, aError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[1], pts[2], width, aError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[2], pts[3], width, aError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[3], pts[0], width, aError, aErrorLoc );
        }

        break;
    }

    case SHAPE_T::ARC:
        TransformArcToPolygon( aBuffer, GetStart(), GetArcMid(), GetEnd(), width, aError, aErrorLoc );
        break;

    case SHAPE_T::SEGMENT:
        TransformOvalToPolygon( aBuffer, GetStart(), GetEnd(), width, aError, aErrorLoc );
        break;

    case SHAPE_T::POLY:
    {
        if( !IsPolyShapeValid() )
            break;

        if( IsFilled() )
        {
            for( int ii = 0; ii < m_poly.OutlineCount(); ++ii )
            {
                const SHAPE_LINE_CHAIN& poly = m_poly.Outline( ii );
                SHAPE_POLY_SET tmp;
                tmp.NewOutline();

                for( int jj = 0; jj < (int) poly.GetPointCount(); ++jj )
                    tmp.Append( poly.GetPoint( jj ) );

                if( width > 0 )
                {
                    int inflate = width / 2;

                    if( aErrorLoc == ERROR_OUTSIDE )
                        inflate += aError;

                    tmp.Inflate( inflate, CORNER_STRATEGY::ROUND_ALL_CORNERS, aError );
                }

                aBuffer.Append( tmp );
            }
        }
        else
        {
            for( int ii = 0; ii < m_poly.OutlineCount(); ++ii )
            {
                const SHAPE_LINE_CHAIN& poly = m_poly.Outline( ii );

                for( int jj = 0; jj < (int) poly.SegmentCount(); ++jj )
                {
                    const SEG& seg = poly.GetSegment( jj );
                    TransformOvalToPolygon( aBuffer, seg.A, seg.B, width, aError, aErrorLoc );
                }
            }
        }

        break;
    }

    case SHAPE_T::BEZIER:
    {
        std::vector<VECTOR2I> ctrlPts = { GetStart(), GetBezierC1(), GetBezierC2(), GetEnd() };
        BEZIER_POLY converter( ctrlPts );
        std::vector<VECTOR2I> poly;
        converter.GetPoly( poly, aError );

        for( unsigned ii = 1; ii < poly.size(); ii++ )
            TransformOvalToPolygon( aBuffer, poly[ii - 1], poly[ii], width, aError, aErrorLoc );

        break;
    }

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


void EDA_SHAPE::SetLineStyle( const LINE_STYLE aStyle )
{
    m_stroke.SetLineStyle( aStyle );
}


LINE_STYLE EDA_SHAPE::GetLineStyle() const
{
    if( m_stroke.GetLineStyle() != LINE_STYLE::DEFAULT )
        return m_stroke.GetLineStyle();

    return LINE_STYLE::SOLID;
}


bool EDA_SHAPE::operator==( const EDA_SHAPE& aOther ) const
{
    if( GetShape() != aOther.GetShape() )
        return false;

    if( m_fill != aOther.m_fill )
        return false;

    if( m_stroke.GetWidth() != aOther.m_stroke.GetWidth() )
        return false;

    if( m_stroke.GetLineStyle() != aOther.m_stroke.GetLineStyle() )
        return false;

    if( m_fillColor != aOther.m_fillColor )
        return false;

    if( m_start != aOther.m_start )
        return false;

    if( m_end != aOther.m_end )
        return false;

    if( m_arcCenter != aOther.m_arcCenter )
        return false;

    if( m_bezierC1 != aOther.m_bezierC1 )
        return false;

    if( m_bezierC2 != aOther.m_bezierC2 )
        return false;

    if( m_bezierPoints != aOther.m_bezierPoints )
        return false;

    for( int ii = 0; ii < m_poly.TotalVertices(); ++ii )
    {
        if( m_poly.CVertex( ii ) != aOther.m_poly.CVertex( ii ) )
            return false;
    }

    return true;
}


double EDA_SHAPE::Similarity( const EDA_SHAPE& aOther ) const
{
    if( GetShape() != aOther.GetShape() )
        return 0.0;

    double similarity = 1.0;

    if( m_fill != aOther.m_fill )
        similarity *= 0.9;

    if( m_stroke.GetWidth() != aOther.m_stroke.GetWidth() )
        similarity *= 0.9;

    if( m_stroke.GetLineStyle() != aOther.m_stroke.GetLineStyle() )
        similarity *= 0.9;

    if( m_fillColor != aOther.m_fillColor )
        similarity *= 0.9;

    if( m_start != aOther.m_start )
        similarity *= 0.9;

    if( m_end != aOther.m_end )
        similarity *= 0.9;

    if( m_arcCenter != aOther.m_arcCenter )
        similarity *= 0.9;

    if( m_bezierC1 != aOther.m_bezierC1 )
        similarity *= 0.9;

    if( m_bezierC2 != aOther.m_bezierC2 )
        similarity *= 0.9;

    {
        int m = m_bezierPoints.size();
        int n = aOther.m_bezierPoints.size();

        size_t longest = alg::longest_common_subset( m_bezierPoints, aOther.m_bezierPoints );

        similarity *= std::pow( 0.9, m + n - 2 * longest );
    }

    {
        int m = m_poly.TotalVertices();
        int n = aOther.m_poly.TotalVertices();
        std::vector<VECTOR2I> poly;
        std::vector<VECTOR2I> otherPoly;
        VECTOR2I              lastPt( 0, 0 );

        // We look for the longest common subset of the two polygons, but we need to
        // offset each point because we're actually looking for overall similarity, not just
        // exact matches.  So if the zone is moved by 1IU, we only want one point to be
        // considered "moved" rather than the entire polygon.  In this case, the first point
        // will not be a match but the rest of the sequence will.
        for( int ii = 0; ii < m; ++ii )
        {
            poly.emplace_back( lastPt - m_poly.CVertex( ii ) );
            lastPt = m_poly.CVertex( ii );
        }

        lastPt = VECTOR2I( 0, 0 );

        for( int ii = 0; ii < n; ++ii )
        {
            otherPoly.emplace_back( lastPt - aOther.m_poly.CVertex( ii ) );
            lastPt = aOther.m_poly.CVertex( ii );
        }

        size_t longest = alg::longest_common_subset( poly, otherPoly );

        similarity *= std::pow( 0.9, m + n - 2 * longest );
    }

    return similarity;
}


IMPLEMENT_ENUM_TO_WXANY( SHAPE_T )
IMPLEMENT_ENUM_TO_WXANY( LINE_STYLE )


static struct EDA_SHAPE_DESC
{
    EDA_SHAPE_DESC()
    {
        ENUM_MAP<SHAPE_T>::Instance()
                    .Map( SHAPE_T::SEGMENT,   _HKI( "Segment" ) )
                    .Map( SHAPE_T::RECTANGLE, _HKI( "Rectangle" ) )
                    .Map( SHAPE_T::ARC,       _HKI( "Arc" ) )
                    .Map( SHAPE_T::CIRCLE,    _HKI( "Circle" ) )
                    .Map( SHAPE_T::POLY,      _HKI( "Polygon" ) )
                    .Map( SHAPE_T::BEZIER,    _HKI( "Bezier" ) );

        auto& plotDashTypeEnum = ENUM_MAP<LINE_STYLE>::Instance();

        if( plotDashTypeEnum.Choices().GetCount() == 0 )
        {
            plotDashTypeEnum.Map( LINE_STYLE::DEFAULT, _HKI( "Default" ) )
                            .Map( LINE_STYLE::SOLID, _HKI( "Solid" ) )
                            .Map( LINE_STYLE::DASH, _HKI( "Dashed" ) )
                            .Map( LINE_STYLE::DOT, _HKI( "Dotted" ) )
                            .Map( LINE_STYLE::DASHDOT, _HKI( "Dash-Dot" ) )
                            .Map( LINE_STYLE::DASHDOTDOT, _HKI( "Dash-Dot-Dot" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( EDA_SHAPE );

        auto isNotPolygonOrCircle = []( INSPECTABLE* aItem ) -> bool
        {
            // Polygons, unlike other shapes, have no meaningful start or end coordinates
            if( EDA_SHAPE* shape = dynamic_cast<EDA_SHAPE*>( aItem ) )
                return shape->GetShape() != SHAPE_T::POLY && shape->GetShape() != SHAPE_T::CIRCLE;

            return false;
        };

        auto isCircle = []( INSPECTABLE* aItem ) -> bool
        {
            // Polygons, unlike other shapes, have no meaningful start or end coordinates
            if( EDA_SHAPE* shape = dynamic_cast<EDA_SHAPE*>( aItem ) )
                return shape->GetShape() == SHAPE_T::CIRCLE;

            return false;
        };

        auto isRectangle = []( INSPECTABLE* aItem ) -> bool
        {
            // Polygons, unlike other shapes, have no meaningful start or end coordinates
            if( EDA_SHAPE* shape = dynamic_cast<EDA_SHAPE*>( aItem ) )
                return shape->GetShape() == SHAPE_T::RECTANGLE;

            return false;
        };

        const wxString shapeProps = _HKI( "Shape Properties" );

        auto shape = new PROPERTY_ENUM<EDA_SHAPE, SHAPE_T>( _HKI( "Shape" ),
                     NO_SETTER( EDA_SHAPE, SHAPE_T ), &EDA_SHAPE::GetShape );
        propMgr.AddProperty( shape, shapeProps );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Start X" ),
                    &EDA_SHAPE::SetStartX, &EDA_SHAPE::GetStartX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ),
                    shapeProps )
                .SetAvailableFunc( isNotPolygonOrCircle );
        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Start Y" ),
                    &EDA_SHAPE::SetStartY, &EDA_SHAPE::GetStartY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                    shapeProps )
                .SetAvailableFunc( isNotPolygonOrCircle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Center X" ),
                    &EDA_SHAPE::SetCenterX, &EDA_SHAPE::GetStartX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ),
                    shapeProps )
                .SetAvailableFunc( isCircle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Center Y" ),
                    &EDA_SHAPE::SetCenterY, &EDA_SHAPE::GetStartY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                    shapeProps )
                .SetAvailableFunc( isCircle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Radius" ),
                    &EDA_SHAPE::SetRadius, &EDA_SHAPE::GetRadius, PROPERTY_DISPLAY::PT_SIZE,
                    ORIGIN_TRANSFORMS::NOT_A_COORD ),
                    shapeProps )
                .SetAvailableFunc( isCircle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "End X" ),
                    &EDA_SHAPE::SetEndX, &EDA_SHAPE::GetEndX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ),
                    shapeProps )
                .SetAvailableFunc( isNotPolygonOrCircle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "End Y" ),
                    &EDA_SHAPE::SetEndY, &EDA_SHAPE::GetEndY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                    shapeProps )
                .SetAvailableFunc( isNotPolygonOrCircle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Width" ),
                    &EDA_SHAPE::SetRectangleWidth, &EDA_SHAPE::GetRectangleWidth,
                    PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                    shapeProps )
                .SetAvailableFunc( isRectangle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Height" ),
                    &EDA_SHAPE::SetRectangleHeight, &EDA_SHAPE::GetRectangleHeight,
                    PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                    shapeProps )
                .SetAvailableFunc( isRectangle );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Line Width" ),
                    &EDA_SHAPE::SetWidth, &EDA_SHAPE::GetWidth, PROPERTY_DISPLAY::PT_SIZE ),
                    shapeProps );

        propMgr.AddProperty( new PROPERTY_ENUM<EDA_SHAPE, LINE_STYLE>( _HKI( "Line Style" ),
                    &EDA_SHAPE::SetLineStyle, &EDA_SHAPE::GetLineStyle ),
                    shapeProps );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, COLOR4D>( _HKI( "Line Color" ),
                    &EDA_SHAPE::SetLineColor, &EDA_SHAPE::GetLineColor ),
                    shapeProps )
                .SetIsHiddenFromRulesEditor();

        auto angle = new PROPERTY<EDA_SHAPE, EDA_ANGLE>( _HKI( "Angle" ),
                    NO_SETTER( EDA_SHAPE, EDA_ANGLE ), &EDA_SHAPE::GetArcAngle,
                    PROPERTY_DISPLAY::PT_DECIDEGREE );
        angle->SetAvailableFunc(
                [=]( INSPECTABLE* aItem ) -> bool
                {
                    if( EDA_SHAPE* curr_shape = dynamic_cast<EDA_SHAPE*>( aItem ) )
                        return curr_shape->GetShape() == SHAPE_T::ARC;

                    return false;
                } );
        propMgr.AddProperty( angle, shapeProps );

        auto fillAvailable =
                [=]( INSPECTABLE* aItem ) -> bool
                {
                    if( EDA_ITEM* edaItem = dynamic_cast<EDA_ITEM*>( aItem ) )
                    {
                        // For some reason masking "Filled" and "Fill Color" at the
                        // PCB_TABLECELL level doesn't work.
                        if( edaItem->Type() == PCB_TABLECELL_T )
                            return false;
                    }

                    if( EDA_SHAPE* edaShape = dynamic_cast<EDA_SHAPE*>( aItem ) )
                    {
                        switch( edaShape->GetShape() )
                        {
                        case SHAPE_T::POLY:
                        case SHAPE_T::RECTANGLE:
                        case SHAPE_T::CIRCLE:
                        case SHAPE_T::BEZIER:
                            return true;

                        default:
                            return false;
                        }
                    }

                    return false;
                };

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, bool>( _HKI( "Filled" ),
                    &EDA_SHAPE::SetFilled, &EDA_SHAPE::IsFilled ),
                    shapeProps )
                .SetAvailableFunc( fillAvailable );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, COLOR4D>( _HKI( "Fill Color" ),
                    &EDA_SHAPE::SetFillColor, &EDA_SHAPE::GetFillColor ),
                    shapeProps )
                .SetAvailableFunc( fillAvailable )
                .SetIsHiddenFromRulesEditor();
    }
} _EDA_SHAPE_DESC;
