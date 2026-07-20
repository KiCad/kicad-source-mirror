/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pcb_shape.h"

#include <google/protobuf/any.pb.h>
#include <magic_enum.hpp>

#include <bitmaps.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <board.h>
#include <footprint.h>
#include <lset.h>
#include <pad.h>
#include <base_units.h>
#include <trigo.h>
#include <drc/drc_engine.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_ellipse.h>
#include <geometry/point_types.h>
#include <geometry/shape_utils.h>
#include <pcb_painter.h>
#include <api/board/board_types.pb.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <properties/property.h>
#include <properties/property_mgr.h>


namespace
{
struct BOARD_ELLIPSE
{
    double    major;
    double    minor;
    EDA_ANGLE rotation;
    EDA_ANGLE startShift;
};


// SVD of a 2x2 matrix into ellipse major / minor / rotation and the arc angle shift.
static BOARD_ELLIPSE decompose2x2( double m00, double m01, double m10, double m11 )
{
    const double A = m00 * m00 + m10 * m10;
    const double B = m00 * m01 + m10 * m11;
    const double C = m01 * m01 + m11 * m11;

    const double diff = A - C;
    const double rad = std::hypot( diff, 2.0 * B );
    const double lambda1 = ( A + C + rad ) * 0.5;
    const double lambda2 = ( A + C - rad ) * 0.5;

    const double sigma1 = std::sqrt( std::max( 0.0, lambda1 ) );
    const double sigma2 = std::sqrt( std::max( 0.0, lambda2 ) );

    double v0x;
    double v0y;

    if( std::abs( B ) > 1e-12 )
    {
        v0x = lambda1 - C;
        v0y = B;
    }
    else if( A >= C )
    {
        v0x = 1.0;
        v0y = 0.0;
    }
    else
    {
        v0x = 0.0;
        v0y = 1.0;
    }

    const double vn = std::hypot( v0x, v0y );
    v0x /= vn;
    v0y /= vn;

    double u0x = 1.0;
    double u0y = 0.0;

    if( sigma1 > 1e-12 )
    {
        u0x = ( m00 * v0x + m01 * v0y ) / sigma1;
        u0y = ( m10 * v0x + m11 * v0y ) / sigma1;
    }

    BOARD_ELLIPSE out;
    out.major = sigma1;
    out.minor = sigma2;
    out.rotation = EDA_ANGLE( std::atan2( u0y, u0x ), RADIANS_T );
    out.startShift = EDA_ANGLE( -std::atan2( v0y, v0x ), RADIANS_T );
    return out;
}


// Board ellipse from a lib ellipse: M = R(theta) * diag(sx, sy) * R(phi) * diag(a, b).
static BOARD_ELLIPSE decomposeBoardEllipse( const TRANSFORM_TRS& aXform, int aLibMajor, int aLibMinor,
                                            const EDA_ANGLE& aLibRotation )
{
    const double sx = aXform.GetScaleX();
    const double sy = aXform.GetScaleY();
    // Lib rotation and xform rotation use opposite signs, negate to match.
    const double theta = -aXform.GetRotate().AsRadians();
    const double phi = aLibRotation.AsRadians();
    const double cTheta = std::cos( theta );
    const double sTheta = std::sin( theta );
    const double cPhi = std::cos( phi );
    const double sPhi = std::sin( phi );
    const double a = aLibMajor;
    const double b = aLibMinor;

    const double m00 = a * ( sx * cTheta * cPhi - sy * sTheta * sPhi );
    const double m01 = -b * ( sx * cTheta * sPhi + sy * sTheta * cPhi );
    const double m10 = a * ( sx * sTheta * cPhi + sy * cTheta * sPhi );
    const double m11 = b * ( -sx * sTheta * sPhi + sy * cTheta * cPhi );

    return decompose2x2( m00, m01, m10, m11 );
}


// Inverse of decomposeBoardEllipse: lib ellipse from a board ellipse.
static BOARD_ELLIPSE composeLibEllipse( const TRANSFORM_TRS& aXform, double aBoardMajor, double aBoardMinor,
                                        const EDA_ANGLE& aBoardRotation )
{
    const double sx = aXform.GetScaleX();
    const double sy = aXform.GetScaleY();
    const double theta = -aXform.GetRotate().AsRadians();
    const double beta = aBoardRotation.AsRadians();

    const double li00 = std::cos( theta ) / sx;
    const double li01 = std::sin( theta ) / sx;
    const double li10 = -std::sin( theta ) / sy;
    const double li11 = std::cos( theta ) / sy;

    const double e00 = aBoardMajor * std::cos( beta );
    const double e01 = -aBoardMinor * std::sin( beta );
    const double e10 = aBoardMajor * std::sin( beta );
    const double e11 = aBoardMinor * std::cos( beta );

    return decompose2x2( li00 * e00 + li01 * e10, li00 * e01 + li01 * e11, li10 * e00 + li11 * e10,
                         li10 * e01 + li11 * e11 );
}
} // namespace


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, KICAD_T aItemType, SHAPE_T aShapeType ) :
        BOARD_CONNECTED_ITEM( aParent, aItemType ),
        EDA_SHAPE( aShapeType, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL ),
        m_libStart( 0, 0 ),
        m_libEnd( 0, 0 ),
        m_libArcMid( 0, 0 ),
        m_libBezierC1( 0, 0 ),
        m_libBezierC2( 0, 0 ),
        m_libEllipseCenter( 0, 0 ),
        m_libEllipseMajorRadius( 0 ),
        m_libEllipseMinorRadius( 0 ),
        m_libEllipseRotation( ANGLE_0 ),
        m_libEllipseStartAngle( ANGLE_0 ),
        m_libEllipseEndAngle( ANGLE_0 ),
        m_libShape( aShapeType )
{
    m_hasSolderMask = false;
}


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, SHAPE_T shapetype ) :
        BOARD_CONNECTED_ITEM( aParent, PCB_SHAPE_T ),
        EDA_SHAPE( shapetype, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL ),
        m_libStart( 0, 0 ),
        m_libEnd( 0, 0 ),
        m_libArcMid( 0, 0 ),
        m_libBezierC1( 0, 0 ),
        m_libBezierC2( 0, 0 ),
        m_libEllipseCenter( 0, 0 ),
        m_libEllipseMajorRadius( 0 ),
        m_libEllipseMinorRadius( 0 ),
        m_libEllipseRotation( ANGLE_0 ),
        m_libEllipseStartAngle( ANGLE_0 ),
        m_libEllipseEndAngle( ANGLE_0 ),
        m_libShape( shapetype )
{
    m_hasSolderMask = false;
}


PCB_SHAPE::~PCB_SHAPE()
{
}


void PCB_SHAPE::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_SHAPE_T, /* void */ );
    *this = *static_cast<const PCB_SHAPE*>( aOther );
}


void PCB_SHAPE::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    using namespace kiapi::board::types;
    BoardGraphicShape msg;

    msg.set_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( GetLayer() ) );
    PackNet( msg.mutable_net() );
    msg.mutable_id()->set_value( m_Uuid.AsStdString() );
    msg.set_locked( IsLocked() ? types::LockedState::LS_LOCKED : types::LockedState::LS_UNLOCKED );

    google::protobuf::Any any;
    EDA_SHAPE::Serialize( any );
    any.UnpackTo( msg.mutable_shape() );

    // TODO m_hasSolderMask and m_solderMaskMargin

    aContainer.PackFrom( msg );
}


bool PCB_SHAPE::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;
    using namespace kiapi::board::types;

    BoardGraphicShape msg;

    if( !aContainer.UnpackTo( &msg ) )
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

    SetUuidDirect( KIID( msg.id().value() ) );
    SetLocked( msg.locked() == types::LS_LOCKED );
    SetLayer( FromProtoEnum<PCB_LAYER_ID, BoardLayer>( msg.layer() ) );
    UnpackNet( msg.net() );

    google::protobuf::Any any;
    any.PackFrom( msg.shape() );
    EDA_SHAPE::Deserialize( any );

    // TODO m_hasSolderMask and m_solderMaskMargin

    return true;
}


bool PCB_SHAPE::IsType( const std::vector<KICAD_T>& aScanTypes ) const
{
    if( BOARD_ITEM::IsType( aScanTypes ) )
        return true;

    bool sametype = false;

    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == PCB_LOCATE_BOARD_EDGE_T )
            sametype = m_layer == Edge_Cuts;
        else if( scanType == PCB_SHAPE_LOCATE_ARC_T )
            sametype = m_shape == SHAPE_T::ARC;
        else if( scanType == PCB_SHAPE_LOCATE_CIRCLE_T )
            sametype = m_shape == SHAPE_T::CIRCLE;
        else if( scanType == PCB_SHAPE_LOCATE_RECT_T )
            sametype = m_shape == SHAPE_T::RECTANGLE;
        else if( scanType == PCB_SHAPE_LOCATE_SEGMENT_T )
            sametype = m_shape == SHAPE_T::SEGMENT;
        else if( scanType == PCB_SHAPE_LOCATE_POLY_T )
            sametype = m_shape == SHAPE_T::POLY;
        else if( scanType == PCB_SHAPE_LOCATE_BEZIER_T )
            sametype = m_shape == SHAPE_T::BEZIER;
        else if( scanType == PCB_SHAPE_LOCATE_ELLIPSE_T )
            sametype = m_shape == SHAPE_T::ELLIPSE;
        else if( scanType == PCB_SHAPE_LOCATE_ELLIPSE_ARC_T )
            sametype = m_shape == SHAPE_T::ELLIPSE_ARC;

        if( sametype )
            return true;
    }

    return false;
}


bool PCB_SHAPE::IsConnected() const
{
    // Only board-level copper shapes are connectable
    return IsOnCopperLayer() && !GetParentFootprint();
}


void PCB_SHAPE::SetLayer( PCB_LAYER_ID aLayer )
{
    BOARD_ITEM::SetLayer( aLayer );

    if( !IsOnCopperLayer() )
        SetNetCode( -1 );
}


int PCB_SHAPE::GetSolderMaskExpansion() const
{
    int margin = 0;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine
        && GetBoard()->GetDesignSettings().m_DRCEngine->HasRulesForConstraintType(
                   SOLDER_MASK_EXPANSION_CONSTRAINT ) )
    {
        DRC_CONSTRAINT              constraint;
        std::shared_ptr<DRC_ENGINE> drcEngine = GetBoard()->GetDesignSettings().m_DRCEngine;

        constraint = drcEngine->EvalRules( SOLDER_MASK_EXPANSION_CONSTRAINT, this, nullptr, m_layer );

        if( constraint.m_Value.HasOpt() )
            margin = constraint.m_Value.Opt();
    }
    else if( m_solderMaskMargin.has_value() )
    {
        margin = m_solderMaskMargin.value();
    }
    else if( const BOARD* board = GetBoard() )
    {
        margin = board->GetDesignSettings().m_SolderMaskExpansion;
    }

    // Ensure the resulting mask opening has a non-negative size
    if( margin < 0 && !IsSolidFill() )
        margin = std::max( margin, -GetWidth() / 2 );

    return margin;
}


bool PCB_SHAPE::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    if( aLayer == m_layer )
    {
        return true;
    }

    if( m_hasSolderMask
        && ( ( aLayer == F_Mask && m_layer == F_Cu )
               || ( aLayer == B_Mask && m_layer == B_Cu ) ) )
    {
        return true;
    }

    return false;
}


LSET PCB_SHAPE::GetLayerSet() const
{
    LSET layermask( { m_layer } );

    if( m_hasSolderMask )
    {
        if( layermask.test( F_Cu ) )
            layermask.set( F_Mask );

        if( layermask.test( B_Cu ) )
            layermask.set( B_Mask );
    }

    return layermask;
}


void PCB_SHAPE::SetLayerSet( const LSET& aLayerSet )
{
    aLayerSet.RunOnLayers(
            [&]( PCB_LAYER_ID layer )
            {
                if( IsCopperLayer( layer ) )
                    SetLayer( layer );
                else if( IsSolderMaskLayer( layer ) )
                    SetHasSolderMask( true );
            } );
}


std::vector<VECTOR2I> PCB_SHAPE::GetConnectionPoints() const
{
    std::vector<VECTOR2I> ret;

    // For filled shapes, we may as well use a centroid
    if( IsSolidFill() )
    {
        ret.emplace_back( GetCenter() );
        return ret;
    }

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
    {
        const CIRCLE circle( GetCenter(), GetRadius() );

        for( const TYPED_POINT2I& pt : KIGEOM::GetCircleKeyPoints( circle, false ) )
            ret.emplace_back( pt.m_point );

        break;
    }

    case SHAPE_T::ARC:
        ret.emplace_back( GetArcMid() );
        KI_FALLTHROUGH;
    case SHAPE_T::SEGMENT:
    case SHAPE_T::BEZIER:
        ret.emplace_back( GetStart() );
        ret.emplace_back( GetEnd() );
        break;

    case SHAPE_T::POLY:
        for( auto iter = GetPolyShape().CIterate(); iter; ++iter )
            ret.emplace_back( *iter );

        break;

    case SHAPE_T::RECTANGLE:
        for( const VECTOR2I& pt : GetRectCorners() )
            ret.emplace_back( pt );

        break;

    case SHAPE_T::ELLIPSE:
    {
        const double   phi = GetEllipseRotation().AsRadians();
        const double   cosPhi = std::cos( phi );
        const double   sinPhi = std::sin( phi );
        const int      a = GetEllipseMajorRadius();
        const int      b = GetEllipseMinorRadius();
        const VECTOR2I c = GetEllipseCenter();

        ret.emplace_back( c + VECTOR2I( KiROUND( a * cosPhi ), KiROUND( a * sinPhi ) ) );
        ret.emplace_back( c + VECTOR2I( KiROUND( -a * cosPhi ), KiROUND( -a * sinPhi ) ) );
        ret.emplace_back( c + VECTOR2I( KiROUND( -b * sinPhi ), KiROUND( b * cosPhi ) ) );
        ret.emplace_back( c + VECTOR2I( KiROUND( b * sinPhi ), KiROUND( -b * cosPhi ) ) );
        break;
    }

    case SHAPE_T::ELLIPSE_ARC:
    {
        const double   a = GetEllipseMajorRadius();
        const double   b = GetEllipseMinorRadius();
        const double   phi = GetEllipseRotation().AsRadians();
        const double   cosPhi = std::cos( phi );
        const double   sinPhi = std::sin( phi );
        const VECTOR2I c = GetEllipseCenter();

        auto eval = [&]( double theta ) -> VECTOR2I
        {
            const double lx = a * std::cos( theta );
            const double ly = b * std::sin( theta );
            return c + VECTOR2I( KiROUND( lx * cosPhi - ly * sinPhi ), KiROUND( lx * sinPhi + ly * cosPhi ) );
        };

        double thetaStart = GetEllipseStartAngle().AsRadians();
        double thetaEnd = GetEllipseEndAngle().AsRadians();

        if( thetaEnd < thetaStart )
            thetaEnd += 2.0 * M_PI;

        ret.emplace_back( eval( thetaStart ) );
        ret.emplace_back( eval( thetaEnd ) );
        ret.emplace_back( eval( 0.5 * ( thetaStart + thetaEnd ) ) );
        break;
    }

    case SHAPE_T::UNDEFINED:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }

    return ret;
}


void PCB_SHAPE::UpdateHatching() const
{
    // Force update; we don't bother to propagate damage from all the things that might
    // knock-out parts of our hatching.
    m_hatchingDirty = true;

    EDA_SHAPE::UpdateHatching();
}


SHAPE_POLY_SET PCB_SHAPE::getHatchingKnockouts() const
{
    SHAPE_POLY_SET knockouts;
    PCB_LAYER_ID   layer = GetLayer();
    BOX2I          bbox = GetBoundingBox();
    int            maxError = ARC_LOW_DEF;

    auto knockoutItem =
            [&]( BOARD_ITEM* item )
            {
                int margin = GetHatchLineSpacing() / 2;

                if( item->Type() == PCB_TEXTBOX_T )
                    margin = 0;

                item->TransformShapeToPolygon( knockouts, layer, margin, maxError, ERROR_OUTSIDE );
            };

    for( BOARD_ITEM* item : GetBoard()->Drawings() )
    {
        if( item == this )
            continue;

        if( item->Type() == PCB_FIELD_T
                || item->Type() == PCB_TEXT_T
                || item->Type() == PCB_TEXTBOX_T
                || item->Type() == PCB_SHAPE_T )
        {
            if( item->GetLayer() == layer && item->GetBoundingBox().Intersects( bbox ) )
                knockoutItem( item );
        }
    }

    for( FOOTPRINT* footprint : GetBoard()->Footprints() )
    {
        if( footprint == GetParentFootprint() )
            continue;

        // GetCourtyard() returns the front courtyard for any non-back layer, so only knock it
        // out when the hatched shape actually lives on a courtyard layer.
        if( layer == F_CrtYd || layer == B_CrtYd )
            knockouts.Append( footprint->GetCourtyard( layer ) );

        // Knockout footprint fields
        footprint->RunOnChildren(
                [&]( BOARD_ITEM* item )
                {
                    if( ( item->Type() == PCB_FIELD_T || item->Type() == PCB_SHAPE_T )
                            && item->GetLayer() == layer
                            && !( item->Type() == PCB_FIELD_T && !static_cast<PCB_FIELD*>(item)->IsVisible() )
                            && item->GetBoundingBox().Intersects( bbox ) )
                    {
                        knockoutItem( item );
                    }
                },
                RECURSE_MODE::RECURSE );
    }

    return knockouts;
}


static double fpScaleLinear( const FOOTPRINT* aFp )
{
    if( !aFp )
        return 1.0;

    const TRANSFORM_TRS& xform = aFp->GetTransform();
    return ( xform.GetScaleX() + xform.GetScaleY() ) * 0.5;
}


const FOOTPRINT* PCB_SHAPE::transformFp() const
{
    if( GetParent() && GetParent()->Type() == PCB_PAD_T )
        return nullptr;

    return GetParentFootprint();
}


int PCB_SHAPE::GetWidth() const
{
    // Clamp negative widths to zero. They mean something in eeschema but break
    // plotters and exporters here.
    const int    lib = std::max( EDA_SHAPE::GetWidth(), 0 );
    const double s = fpScaleLinear( transformFp() );
    return s == 1.0 ? lib : KiROUND( lib * s );
}


void PCB_SHAPE::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings, bool aCheckSide )
{
    m_stroke.SetWidth( settings.GetLineThickness( GetLayer() ) );
}


const VECTOR2I PCB_SHAPE::GetFocusPosition() const
{
    // For some shapes return the visual center, but for not filled polygonal shapes,
    // the center is usually far from the shape: a point on the outline is better

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
        if( !IsAnyFill() )
            return VECTOR2I( GetCenter().x + GetRadius(), GetCenter().y );
        else
            return GetCenter();

    case SHAPE_T::RECTANGLE:
        if( !IsAnyFill() )
            return GetStart();
        else
            return GetCenter();

    case SHAPE_T::POLY:
        if( !IsAnyFill() )
        {
            VECTOR2I pos = GetPolyShape().Outline(0).CPoint(0);
            return VECTOR2I( pos.x, pos.y );
        }
        else
        {
            return GetCenter();
        }

    case SHAPE_T::ARC:
        return GetArcMid();

    case SHAPE_T::BEZIER:
        return GetStart();

    default:
        return GetCenter();
    }
}


std::vector<VECTOR2I> PCB_SHAPE::GetCorners() const
{
    std::vector<VECTOR2I> pts;

    if( GetShape() == SHAPE_T::RECTANGLE )
    {
        pts = GetRectCorners();
    }
    else if( GetShape() == SHAPE_T::POLY )
    {
        for( int ii = 0; ii < GetPolyShape().OutlineCount(); ++ii )
        {
            for( const VECTOR2I& pt : GetPolyShape().Outline( ii ).CPoints() )
                pts.emplace_back( pt );
        }
    }
    else
    {
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
    }

    while( pts.size() < 4 )
        pts.emplace_back( pts.back() + VECTOR2I( 10, 10 ) );

    return pts;
}


void PCB_SHAPE::Move( const VECTOR2I& aMoveVector )
{
    move( aMoveVector );
    syncLibCoords();
}


void PCB_SHAPE::Scale( double aScale )
{
    scale( aScale );
}


void PCB_SHAPE::Normalize()
{
    if( m_shape == SHAPE_T::RECTANGLE )
    {
        VECTOR2I start = GetStart();
        VECTOR2I end = GetEnd();

        BOX2I rect( start, end - start );
        rect.Normalize();

        SetStart( rect.GetPosition() );
        SetEnd( rect.GetEnd() );
    }
    else if( m_shape == SHAPE_T::POLY )
    {
        auto horizontal =
                []( const SEG& seg )
                {
                    return seg.A.y == seg.B.y;
                };

        auto vertical =
                []( const SEG& seg )
                {
                    return seg.A.x == seg.B.x;
                };

        // Convert a poly back to a rectangle if appropriate
        if( GetPolyShape().OutlineCount() == 1 && GetPolyShape().Outline( 0 ).SegmentCount() == 4 )
        {
            SHAPE_LINE_CHAIN& outline = GetPolyShape().Outline( 0 );

            if( horizontal( outline.Segment( 0 ) )
                && vertical( outline.Segment( 1 ) )
                && horizontal( outline.Segment( 2 ) )
                && vertical( outline.Segment( 3 ) ) )
            {
                m_shape = SHAPE_T::RECTANGLE;
                m_start.x = std::min( outline.Segment( 0 ).A.x, outline.Segment( 0 ).B.x );
                m_start.y = std::min( outline.Segment( 1 ).A.y, outline.Segment( 1 ).B.y );
                m_end.x = std::max( outline.Segment( 0 ).A.x, outline.Segment( 0 ).B.x );
                m_end.y = std::max( outline.Segment( 1 ).A.y, outline.Segment( 1 ).B.y );
            }
            else if( vertical( outline.Segment( 0 ) )
                  && horizontal( outline.Segment( 1 ) )
                  && vertical( outline.Segment( 2 ) )
                  && horizontal( outline.Segment( 3 ) ) )
            {
                m_shape = SHAPE_T::RECTANGLE;
                m_start.x = std::min( outline.Segment( 1 ).A.x, outline.Segment( 1 ).B.x );
                m_start.y = std::min( outline.Segment( 0 ).A.y, outline.Segment( 0 ).B.y );
                m_end.x = std::max( outline.Segment( 1 ).A.x, outline.Segment( 1 ).B.x );
                m_end.y = std::max( outline.Segment( 0 ).A.y, outline.Segment( 0 ).B.y );
            }
        }
    }
}


void PCB_SHAPE::NormalizeForCompare()
{
    if( m_shape == SHAPE_T::SEGMENT )
    {
        VECTOR2I libStart = GetLibraryStart();
        VECTOR2I libEnd = GetLibraryEnd();

        if( ( libStart.x > libEnd.x ) || ( libStart.x == libEnd.x && libStart.y < libEnd.y ) )
        {
            VECTOR2I s = GetStart();
            VECTOR2I e = GetEnd();
            SetStart( e );
            SetEnd( s );
        }
    }
    else
        Normalize();
}


void PCB_SHAPE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    rotate( aRotCentre, aAngle );
    syncLibCoords();
}


void PCB_SHAPE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    // Null for pad-local primitives, which mirror directly rather than through the FP transform.
    const FOOTPRINT* fp = transformFp();

    if( fp
        && ( m_libShape == SHAPE_T::SEGMENT || m_libShape == SHAPE_T::CIRCLE || m_libShape == SHAPE_T::ARC
             || m_libShape == SHAPE_T::RECTANGLE || m_libShape == SHAPE_T::BEZIER || m_libShape == SHAPE_T::POLY ) )
    {
        const VECTOR2I libCenter = fp->GetTransform().InverseApply( aCentre );

        auto mirrorPt = [&]( VECTOR2I& p )
        {
            if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
                p.x = 2 * libCenter.x - p.x;
            else
                p.y = 2 * libCenter.y - p.y;
        };

        if( m_libShape == SHAPE_T::ARC )
        {
            mirrorPt( m_libStart );
            mirrorPt( m_libEnd );
            mirrorPt( m_libArcMid );
            std::swap( m_libStart, m_libEnd );
        }
        else
        {
            mirrorPt( m_libStart );
            mirrorPt( m_libEnd );
        }

        if( m_libShape == SHAPE_T::BEZIER )
        {
            mirrorPt( m_libBezierC1 );
            mirrorPt( m_libBezierC2 );
        }

        if( m_libShape == SHAPE_T::POLY )
        {
            for( auto it = m_libPoly.IterateWithHoles(); it; it++ )
            {
                VECTOR2I p = *it;
                mirrorPt( p );
                m_libPoly.SetVertex( it.GetIndex(), p );
            }
        }

        SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
        RebakeFromLib();
        return;
    }

    if( fp && ( m_libShape == SHAPE_T::ELLIPSE || m_libShape == SHAPE_T::ELLIPSE_ARC ) )
    {
        const VECTOR2I libCenter = fp->GetTransform().InverseApply( aCentre );

        if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
            m_libEllipseCenter.x = 2 * libCenter.x - m_libEllipseCenter.x;
        else
            m_libEllipseCenter.y = 2 * libCenter.y - m_libEllipseCenter.y;

        m_libEllipseRotation = -m_libEllipseRotation;

        const EDA_ANGLE oldStart = m_libEllipseStartAngle;
        const EDA_ANGLE oldEnd = m_libEllipseEndAngle;

        if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        {
            m_libEllipseStartAngle = ANGLE_180 - oldEnd;
            m_libEllipseEndAngle = ANGLE_180 - oldStart;
        }
        else
        {
            m_libEllipseStartAngle = -oldEnd;
            m_libEllipseEndAngle = -oldStart;
        }

        SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
        RebakeFromLib();
        return;
    }

    flip( aCentre, aFlipDirection );

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
    syncLibCoords();
}


void PCB_SHAPE::SetStart( const VECTOR2I& aStart )
{
    EDA_SHAPE::SetStart( aStart );
    syncLibCoords();
}


void PCB_SHAPE::SetEnd( const VECTOR2I& aEnd )
{
    EDA_SHAPE::SetEnd( aEnd );
    syncLibCoords();
}


void PCB_SHAPE::OnFootprintRescaled( double aRatioX, double aRatioY, double aLinearFactor, const VECTOR2I& aAnchor,
                                     const EDA_ANGLE& aParentRotate )
{
    RebakeFromLib();
}


void PCB_SHAPE::SetWidth( int aWidth )
{
    const double s = fpScaleLinear( transformFp() );

    if( s == 1.0 )
        m_stroke.SetWidth( aWidth );
    else
        m_stroke.SetWidth( KiROUND( aWidth / s ) );

    m_hatchingDirty = true;
}


void PCB_SHAPE::SetLibStrokeWidth( int aWidth )
{
    m_stroke.SetWidth( aWidth );
    m_hatchingDirty = true;
}


STROKE_PARAMS PCB_SHAPE::GetStroke() const
{
    STROKE_PARAMS s = m_stroke;
    s.SetWidth( GetWidth() );
    return s;
}


void PCB_SHAPE::SetStroke( const STROKE_PARAMS& aStroke )
{
    m_stroke = aStroke;
    SetWidth( aStroke.GetWidth() );
}


void PCB_SHAPE::RebakeWithScale( double aScaleX, double aScaleY )
{
    TRANSFORM_TRS xform;
    xform.SetScale( aScaleX, aScaleY );
    rebakeFromTransform( xform );
}


void PCB_SHAPE::RebakeFromLib()
{
    const FOOTPRINT* fp = transformFp();

    if( !fp )
        return;

    rebakeFromTransform( fp->GetTransform() );
}


void PCB_SHAPE::rebakeFromTransform( const TRANSFORM_TRS& xform )
{
    const bool nonUniform = xform.GetScaleX() != xform.GetScaleY();

    if( m_libShape == SHAPE_T::CIRCLE )
    {
        VECTOR2I libCenter = m_libStart;
        int      libRadius = ( m_libEnd - m_libStart ).EuclideanNorm();
        VECTOR2I newCenter = xform.Apply( libCenter );

        if( nonUniform )
        {
            int       majorRadius = std::abs( KiROUND( libRadius * xform.GetScaleX() ) );
            int       minorRadius = std::abs( KiROUND( libRadius * xform.GetScaleY() ) );

            EDA_ANGLE rotation = -xform.GetRotate();

            if( minorRadius > majorRadius )
            {
                std::swap( majorRadius, minorRadius );
                rotation += EDA_ANGLE( 90.0, DEGREES_T );
            }

            m_shape = SHAPE_T::ELLIPSE;
            SetEllipseCenter( newCenter );
            SetEllipseMajorRadius( majorRadius );
            SetEllipseMinorRadius( minorRadius );
            SetEllipseRotation( rotation );
        }
        else
        {
            m_shape = SHAPE_T::CIRCLE;
            EDA_SHAPE::SetStart( newCenter );
            EDA_SHAPE::SetEnd( xform.Apply( m_libEnd ) );
        }

        return;
    }

    if( m_libShape == SHAPE_T::ARC )
    {
        VECTOR2I libCenter = CalcArcCenter( m_libStart, m_libArcMid, m_libEnd );
        int      libRadius = ( m_libStart - libCenter ).EuclideanNorm();
        VECTOR2I newCenter = xform.Apply( libCenter );

        if( nonUniform )
        {
            int       majorRadius = std::abs( KiROUND( libRadius * xform.GetScaleX() ) );
            int       minorRadius = std::abs( KiROUND( libRadius * xform.GetScaleY() ) );

            EDA_ANGLE rotation = -xform.GetRotate();
            EDA_ANGLE startAngle( VECTOR2D( m_libStart - libCenter ) );
            EDA_ANGLE midAngle( VECTOR2D( m_libArcMid - libCenter ) );
            EDA_ANGLE endAngle( VECTOR2D( m_libEnd - libCenter ) );

            auto wrap = []( EDA_ANGLE a, EDA_ANGLE base )
            {
                while( a < base )
                    a += ANGLE_360;
                return a;
            };

            if( wrap( midAngle, startAngle ) > wrap( endAngle, startAngle ) )
                std::swap( startAngle, endAngle );

            while( endAngle < startAngle )
                endAngle += ANGLE_360;

            if( minorRadius > majorRadius )
            {
                std::swap( majorRadius, minorRadius );
                rotation += EDA_ANGLE( 90.0, DEGREES_T );
                startAngle -= EDA_ANGLE( 90.0, DEGREES_T );
                endAngle -= EDA_ANGLE( 90.0, DEGREES_T );
            }

            m_shape = SHAPE_T::ELLIPSE_ARC;
            SetEllipseCenter( newCenter );
            SetEllipseMajorRadius( majorRadius );
            SetEllipseMinorRadius( minorRadius );
            SetEllipseRotation( rotation );
            SetEllipseStartAngle( startAngle );
            SetEllipseEndAngle( endAngle );
        }
        else
        {
            m_shape = SHAPE_T::ARC;
            EDA_SHAPE::SetArcGeometry( xform.Apply( m_libStart ), xform.Apply( m_libArcMid ), xform.Apply( m_libEnd ) );
        }

        return;
    }

    if( m_libShape == SHAPE_T::ELLIPSE || m_libShape == SHAPE_T::ELLIPSE_ARC )
    {
        // The linear transform preserves ellipse-ness only when scale is uniform
        // or the lib ellipse's axes are aligned with the scale axes (cardinal lib
        // rotation). Otherwise the result is a sheared shape that no standard
        // ellipse can represent; tessellate to POLY in that case.
        const bool keepNative = xform.IsUniformScale() || m_libEllipseRotation.IsCardinal();

        if( keepNative )
        {
            m_shape = m_libShape;
            EDA_SHAPE::SetEllipseCenter( xform.Apply( m_libEllipseCenter ) );

            if( xform.IsUniformScale() )
            {
                double absScale = std::abs( xform.GetScaleX() );
                EDA_SHAPE::SetEllipseMajorRadius( KiROUND( m_libEllipseMajorRadius * absScale ) );
                EDA_SHAPE::SetEllipseMinorRadius( KiROUND( m_libEllipseMinorRadius * absScale ) );
                EDA_SHAPE::SetEllipseRotation( m_libEllipseRotation - xform.GetRotate() );

                if( m_libShape == SHAPE_T::ELLIPSE_ARC )
                {
                    EDA_SHAPE::SetEllipseStartAngle( m_libEllipseStartAngle );
                    EDA_SHAPE::SetEllipseEndAngle( m_libEllipseEndAngle );
                }
            }
            else
            {
                BOARD_ELLIPSE be = decomposeBoardEllipse( xform, m_libEllipseMajorRadius,
                                                          m_libEllipseMinorRadius,
                                                          m_libEllipseRotation );

                EDA_SHAPE::SetEllipseMajorRadius( KiROUND( be.major ) );
                EDA_SHAPE::SetEllipseMinorRadius( KiROUND( be.minor ) );
                EDA_SHAPE::SetEllipseRotation( be.rotation );

                if( m_libShape == SHAPE_T::ELLIPSE_ARC )
                {
                    EDA_SHAPE::SetEllipseStartAngle( m_libEllipseStartAngle + be.startShift );
                    EDA_SHAPE::SetEllipseEndAngle( m_libEllipseEndAngle + be.startShift );
                }
            }
        }
        else
        {
            const bool isArc = ( m_libShape == SHAPE_T::ELLIPSE_ARC );

            std::unique_ptr<SHAPE_ELLIPSE> libEllipse;

            if( isArc )
            {
                libEllipse = std::make_unique<SHAPE_ELLIPSE>(
                        m_libEllipseCenter, m_libEllipseMajorRadius, m_libEllipseMinorRadius,
                        m_libEllipseRotation, m_libEllipseStartAngle, m_libEllipseEndAngle );
            }
            else
            {
                libEllipse = std::make_unique<SHAPE_ELLIPSE>(
                        m_libEllipseCenter, m_libEllipseMajorRadius, m_libEllipseMinorRadius,
                        m_libEllipseRotation );
            }

            SHAPE_LINE_CHAIN chain = libEllipse->ConvertToPolyline( getMaxError() );

            m_shape = SHAPE_T::POLY;
            SHAPE_POLY_SET& poly = GetPolyShape();
            poly.RemoveAllContours();
            poly.NewOutline();

            for( int ii = 0; ii < chain.PointCount(); ++ii )
                poly.Append( xform.Apply( chain.CPoint( ii ) ) );

            poly.Outline( 0 ).SetClosed( !isArc );
        }

        return;
    }

    if( m_libShape == SHAPE_T::RECTANGLE )
    {
        const VECTOR2I c1 = xform.Apply( m_libStart );
        const VECTOR2I c2 = xform.Apply( VECTOR2I( m_libEnd.x, m_libStart.y ) );
        const VECTOR2I c3 = xform.Apply( m_libEnd );
        const VECTOR2I c4 = xform.Apply( VECTOR2I( m_libStart.x, m_libEnd.y ) );

        if( xform.GetRotate().IsCardinal() )
        {
            m_shape = SHAPE_T::RECTANGLE;
            BOX2I bbox( c1, VECTOR2I( 0, 0 ) );
            bbox.Merge( c2 );
            bbox.Merge( c3 );
            bbox.Merge( c4 );
            EDA_SHAPE::SetStart( bbox.GetOrigin() );
            EDA_SHAPE::SetEnd( bbox.GetEnd() );
        }
        else
        {
            m_shape = SHAPE_T::POLY;
            SHAPE_POLY_SET& poly = GetPolyShape();
            poly.RemoveAllContours();
            poly.NewOutline();
            poly.Append( c1 );
            poly.Append( c2 );
            poly.Append( c3 );
            poly.Append( c4 );

            EDA_SHAPE::SetStart( c1 );
            EDA_SHAPE::SetEnd( c3 );
        }

        return;
    }

    if( m_libShape == SHAPE_T::POLY )
    {
        SHAPE_POLY_SET& poly = GetPolyShape();
        poly = m_libPoly;

        for( auto it = poly.IterateWithHoles(); it; it++ )
            poly.SetVertex( it.GetIndex(), xform.Apply( *it ) );

        // m_libStart/m_libEnd are not seeded for POLY, skip the start/end fall-through below.
        return;
    }

    EDA_SHAPE::SetStart( xform.Apply( m_libStart ) );
    EDA_SHAPE::SetEnd( xform.Apply( m_libEnd ) );

    if( m_libShape == SHAPE_T::BEZIER )
    {
        EDA_SHAPE::SetBezierC1( xform.Apply( m_libBezierC1 ) );
        EDA_SHAPE::SetBezierC2( xform.Apply( m_libBezierC2 ) );
        RebuildBezierToSegmentsPointsList( getMaxError() );
    }
}


VECTOR2I PCB_SHAPE::GetLibraryArcMid() const
{
    if( m_libShape == SHAPE_T::ARC )
        return m_libArcMid;

    if( const FOOTPRINT* fp = transformFp() )
        return fp->GetTransform().InverseApply( GetArcMid() );

    return GetArcMid();
}


VECTOR2I PCB_SHAPE::GetLibraryBezierC1() const
{
    if( GetParent() && GetParent()->Type() == PCB_PAD_T )
        return m_libBezierC1;

    if( const FOOTPRINT* fp = transformFp() )
        return fp->GetTransform().InverseApply( GetBezierC1() );

    return GetBezierC1();
}


VECTOR2I PCB_SHAPE::GetLibraryBezierC2() const
{
    if( GetParent() && GetParent()->Type() == PCB_PAD_T )
        return m_libBezierC2;

    if( const FOOTPRINT* fp = transformFp() )
        return fp->GetTransform().InverseApply( GetBezierC2() );

    return GetBezierC2();
}


SHAPE_POLY_SET PCB_SHAPE::GetLibraryPolyShape() const
{
    if( GetParent() && GetParent()->Type() == PCB_PAD_T )
        return m_libPoly;

    SHAPE_POLY_SET poly = GetPolyShape();

    if( const FOOTPRINT* fp = transformFp() )
    {
        const TRANSFORM_TRS& xform = fp->GetTransform();

        for( auto it = poly.IterateWithHoles(); it; it++ )
            poly.SetVertex( it.GetIndex(), xform.InverseApply( *it ) );
    }

    return poly;
}


void PCB_SHAPE::SetArcGeometry( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    EDA_SHAPE::SetArcGeometry( aStart, aMid, aEnd );
    syncLibCoords();
}


void PCB_SHAPE::SetBezierC1( const VECTOR2I& aPt )
{
    EDA_SHAPE::SetBezierC1( aPt );
    syncLibCoords();
}


void PCB_SHAPE::SetBezierC2( const VECTOR2I& aPt )
{
    EDA_SHAPE::SetBezierC2( aPt );
    syncLibCoords();
}


void PCB_SHAPE::SetPolyShape( const SHAPE_POLY_SET& aShape )
{
    EDA_SHAPE::SetPolyShape( aShape );
    syncLibCoords();
}


void PCB_SHAPE::SetEllipseCenter( const VECTOR2I& aPt )
{
    EDA_SHAPE::SetEllipseCenter( aPt );
    syncLibCoords();
}


void PCB_SHAPE::SetEllipseMajorRadius( int aR )
{
    EDA_SHAPE::SetEllipseMajorRadius( aR );
    syncLibCoords();
}


void PCB_SHAPE::SetEllipseMinorRadius( int aR )
{
    EDA_SHAPE::SetEllipseMinorRadius( aR );
    syncLibCoords();
}


void PCB_SHAPE::SetEllipseRotation( const EDA_ANGLE& aA )
{
    EDA_SHAPE::SetEllipseRotation( aA );
    syncLibCoords();
}


void PCB_SHAPE::SetEllipseStartAngle( const EDA_ANGLE& aA )
{
    EDA_SHAPE::SetEllipseStartAngle( aA );
    syncLibCoords();
}


void PCB_SHAPE::SetEllipseEndAngle( const EDA_ANGLE& aA )
{
    EDA_SHAPE::SetEllipseEndAngle( aA );
    syncLibCoords();
}


void PCB_SHAPE::syncLibCoords()
{
    TRANSFORM_TRS xform;
    bool          hasXform = false;

    if( GetParent() && GetParent()->Type() == PCB_PAD_T )
    {
        double sx = 1.0, sy = 1.0;
        static_cast<const PAD*>( static_cast<const BOARD_ITEM*>( GetParent() ) )->GetPrimitiveLibScale( sx, sy );
        xform.SetScale( sx, sy );
        hasXform = ( sx != 1.0 || sy != 1.0 );
    }
    else if( const FOOTPRINT* fp = transformFp() )
    {
        xform = fp->GetTransform();
        hasXform = true;
    }

    if( m_shape == SHAPE_T::ELLIPSE || m_shape == SHAPE_T::ELLIPSE_ARC )
    {
        if( hasXform )
        {
            m_libEllipseCenter = xform.InverseApply( GetEllipseCenter() );

            if( xform.IsUniformScale() )
            {
                double invScale = 1.0 / std::abs( xform.GetScaleX() );
                m_libEllipseMajorRadius = KiROUND( GetEllipseMajorRadius() * invScale );
                m_libEllipseMinorRadius = KiROUND( GetEllipseMinorRadius() * invScale );
                m_libEllipseRotation = GetEllipseRotation() + xform.GetRotate();
                m_libEllipseStartAngle = GetEllipseStartAngle();
                m_libEllipseEndAngle = GetEllipseEndAngle();
            }
            else
            {
                // Non-uniform scale shears the ellipse, so invert the whole board ellipse to lib.
                BOARD_ELLIPSE le = composeLibEllipse( xform, GetEllipseMajorRadius(), GetEllipseMinorRadius(),
                                                      GetEllipseRotation() );
                m_libEllipseMajorRadius = KiROUND( le.major );
                m_libEllipseMinorRadius = KiROUND( le.minor );
                m_libEllipseRotation = le.rotation;
                m_libEllipseStartAngle = GetEllipseStartAngle() + le.startShift;
                m_libEllipseEndAngle = GetEllipseEndAngle() + le.startShift;
            }
        }
        else
        {
            m_libEllipseCenter = GetEllipseCenter();
            m_libEllipseMajorRadius = GetEllipseMajorRadius();
            m_libEllipseMinorRadius = GetEllipseMinorRadius();
            m_libEllipseRotation = GetEllipseRotation();
            m_libEllipseStartAngle = GetEllipseStartAngle();
            m_libEllipseEndAngle = GetEllipseEndAngle();
        }

        return;
    }

    if( m_shape == SHAPE_T::POLY )
    {
        m_libPoly = GetPolyShape();

        if( hasXform )
        {
            for( auto it = m_libPoly.IterateWithHoles(); it; it++ )
                m_libPoly.SetVertex( it.GetIndex(), xform.InverseApply( *it ) );
        }

        return;
    }

    if( hasXform )
    {
        m_libStart = xform.InverseApply( GetStart() );
        m_libEnd = xform.InverseApply( GetEnd() );

        if( m_shape == SHAPE_T::ARC )
            m_libArcMid = xform.InverseApply( GetArcMid() );

        if( m_shape == SHAPE_T::BEZIER )
        {
            m_libBezierC1 = xform.InverseApply( GetBezierC1() );
            m_libBezierC2 = xform.InverseApply( GetBezierC2() );
        }
    }
    else
    {
        m_libStart = GetStart();
        m_libEnd = GetEnd();

        if( m_shape == SHAPE_T::ARC )
            m_libArcMid = GetArcMid();

        if( m_shape == SHAPE_T::BEZIER )
        {
            m_libBezierC1 = GetBezierC1();
            m_libBezierC2 = GetBezierC2();
        }
    }
}


void PCB_SHAPE::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    flip( aCentre, aFlipDirection );
}


void PCB_SHAPE::SetIsProxyItem( bool aIsProxy )
{
    PAD* parentPad = nullptr;

    if( GetBoard() && GetBoard()->IsFootprintHolder() )
    {
        for( FOOTPRINT* fp : GetBoard()->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->IsEntered() )
                {
                    parentPad = pad;
                    break;
                }
            }
        }
    }

    if( aIsProxy && !m_proxyItem )
    {
        if( GetShape() == SHAPE_T::SEGMENT )
        {
            if( parentPad && parentPad->GetLocalThermalSpokeWidthOverride().has_value() )
                SetWidth( parentPad->GetLocalThermalSpokeWidthOverride().value() );
            else
                SetWidth( pcbIUScale.mmToIU( ZONE_THERMAL_RELIEF_COPPER_WIDTH_MM ) );
        }
        else
        {
            SetWidth( 1 );
        }
    }
    else if( m_proxyItem && !aIsProxy )
    {
        SetWidth( pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ) );
    }

    m_proxyItem = aIsProxy;
}


double PCB_SHAPE::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    KIGFX::PCB_PAINTER&         painter = static_cast<KIGFX::PCB_PAINTER&>( *aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS& renderSettings = *painter.GetSettings();

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow if the main layer is not shown
        if( !aView->IsLayerVisibleCached( m_layer ) )
            return LOD_HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings.GetHighContrast() )
        {
            if( m_layer != renderSettings.GetPrimaryHighContrastLayer() )
                return LOD_HIDE;
        }
    }

    if( aLayer == LAYER_CONSTRAINT_SHADOW )
    {
        // Shadow always appended gate draw here on live constrained-item set not in
        // ViewGetLayers which caches
        if( !renderSettings.GetConstrainedItems().count( m_Uuid ) )
            return LOD_HIDE;

        if( !aView->IsLayerVisibleCached( m_layer ) )
            return LOD_HIDE;

        if( renderSettings.GetHighContrast() && m_layer != renderSettings.GetPrimaryHighContrastLayer() )
            return LOD_HIDE;
    }

    if( FOOTPRINT* parent = GetParentFootprint() )
    {
        PCB_LAYER_ID checkLayer = m_layer;

        if( !IsFrontLayer( checkLayer ) && !IsBackLayer( checkLayer ) )
            checkLayer = parent->GetLayer();

        if( IsFrontLayer( checkLayer ) && !aView->IsLayerVisibleCached( LAYER_FOOTPRINTS_FR ) )
            return LOD_HIDE;

        if( IsBackLayer( checkLayer ) && !aView->IsLayerVisibleCached( LAYER_FOOTPRINTS_BK ) )
            return LOD_HIDE;
    }

    return LOD_SHOW;
}


std::vector<int> PCB_SHAPE::ViewGetLayers() const
{
    std::vector<int> layers;
    layers.reserve( 5 );

    layers.push_back( GetLayer() );

    if( IsOnCopperLayer() )
    {
        layers.push_back( GetNetnameLayer( GetLayer() ) );

        if( m_hasSolderMask )
        {
            if( m_layer == F_Cu )
                layers.push_back( F_Mask );
            else if( m_layer == B_Cu )
                layers.push_back( B_Mask );
        }
    }

    if( IsLocked() || ( GetParentFootprint() && GetParentFootprint()->IsLocked() ) )
        layers.push_back( LAYER_LOCKED_ITEM_SHADOW );

    // Always advertise constraint-shadow layer ViewGetLOD gates draw by constrained state
    layers.push_back( LAYER_CONSTRAINT_SHADOW );

    return layers;
}


void PCB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME )
    {
        if( FOOTPRINT* parent = GetParentFootprint() )
            aList.emplace_back( _( "Footprint" ), parent->GetReference() );
    }

    aList.emplace_back( _( "Type" ), _( "Drawing" ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    ShapeGetMsgPanelInfo( aFrame, aList );

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    if( IsOnCopperLayer() )
    {
        if( GetNetCode() > 0 )  // Only graphics connected to a net have a netcode > 0
            aList.emplace_back( _( "Net" ), GetNetname() );
    }
}


wxString PCB_SHAPE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    FOOTPRINT* parentFP = GetParentFootprint();

    // Don't report parent footprint info from footprint editor, viewer, etc.
    if( GetBoard() && GetBoard()->GetBoardUse() == BOARD_USE::FPHOLDER )
        parentFP = nullptr;

    if( IsOnCopperLayer() )
    {
        if( parentFP )
        {
            return wxString::Format( _( "%s %s of %s on %s" ),
                                     GetFriendlyName(),
                                     GetNetnameMsg(),
                                     parentFP->GetReference(),
                                     GetLayerName() );
        }
        else
        {
            return wxString::Format( _( "%s %s on %s" ),
                                     GetFriendlyName(),
                                     GetNetnameMsg(),
                                     GetLayerName() );
        }
    }
    else
    {
        if( parentFP )
        {
            return wxString::Format( _( "%s of %s on %s" ),
                                     GetFriendlyName(),
                                     parentFP->GetReference(),
                                     GetLayerName() );
        }
        else
        {
            return wxString::Format( _( "%s on %s" ),
                                     GetFriendlyName(),
                                     GetLayerName() );
        }
    }
}


BITMAPS PCB_SHAPE::GetMenuImage() const
{
    if( GetParentFootprint() )
        return BITMAPS::show_mod_edge;
    else
        return BITMAPS::add_dashed_line;
}


EDA_ITEM* PCB_SHAPE::Clone() const
{
    return new PCB_SHAPE( *this );
}


const BOX2I PCB_SHAPE::ViewBBox() const
{
    BOX2I return_box = EDA_ITEM::ViewBBox();

    // Inflate the bounding box by just a bit more for safety.
    return_box.Inflate( GetWidth() );

    return return_box;
}


std::shared_ptr<SHAPE> PCB_SHAPE::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    return std::make_shared<SHAPE_COMPOUND>( MakeEffectiveShapes() );
}


int PCB_SHAPE::getMaxError() const
{
    return GetMaxError();
}


void PCB_SHAPE::swapData( BOARD_ITEM* aImage )
{
    PCB_SHAPE* image = dynamic_cast<PCB_SHAPE*>( aImage );
    wxCHECK( image, /* void */ );

    SwapShape( image );

    // Swap params not handled by SwapShape( image )
    std::swap( m_layer, image->m_layer );
    std::swap( m_isKnockout, image->m_isKnockout );
    std::swap( m_isLocked, image->m_isLocked );
    std::swap( m_flags, image->m_flags );
    std::swap( m_parent, image->m_parent );
    std::swap( m_forceVisible, image->m_forceVisible );
    std::swap( m_netinfo, image->m_netinfo );
    std::swap( m_hasSolderMask, image->m_hasSolderMask );
    std::swap( m_solderMaskMargin, image->m_solderMaskMargin );
}


bool PCB_SHAPE::cmp_drawings::operator()( const BOARD_ITEM* aFirst,
                                          const BOARD_ITEM* aSecond ) const
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


void PCB_SHAPE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                         int aClearance, int aError, ERROR_LOC aErrorLoc,
                                         bool ignoreLineWidth ) const
{
    EDA_SHAPE::TransformShapeToPolygon( aBuffer, aClearance, aError, aErrorLoc, ignoreLineWidth,
                                        false );
}


void PCB_SHAPE::TransformShapeToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                         int aClearance, int aError, ERROR_LOC aErrorLoc,
                                         KIGFX::RENDER_SETTINGS* aRenderSettings ) const
{
    EDA_SHAPE::TransformShapeToPolygon( aBuffer, aClearance, aError, aErrorLoc, false, true );
}


bool PCB_SHAPE::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const PCB_SHAPE& other = static_cast<const PCB_SHAPE&>( aOther );

    return *this == other;
}


bool PCB_SHAPE::operator==( const PCB_SHAPE& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const PCB_SHAPE& other = static_cast<const PCB_SHAPE&>( aOther );

    if( m_layer != other.m_layer )
        return false;

    if( m_isKnockout != other.m_isKnockout )
        return false;

    if( m_isLocked != other.m_isLocked )
        return false;

    if( m_flags != other.m_flags )
        return false;

    if( m_forceVisible != other.m_forceVisible )
        return false;

    if( m_netinfo->GetNetCode() != other.m_netinfo->GetNetCode() )
        return false;

    if( m_hasSolderMask != other.m_hasSolderMask )
        return false;

    if( m_solderMaskMargin != other.m_solderMaskMargin )
        return false;

    return EDA_SHAPE::operator==( other );
}


double PCB_SHAPE::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_SHAPE& other = static_cast<const PCB_SHAPE&>( aOther );

    double similarity = 1.0;

    if( GetLayer() != other.GetLayer() )
        similarity *= 0.9;

    if( m_isKnockout != other.m_isKnockout )
        similarity *= 0.9;

    if( m_isLocked != other.m_isLocked )
        similarity *= 0.9;

    if( m_flags != other.m_flags )
        similarity *= 0.9;

    if( m_forceVisible != other.m_forceVisible )
        similarity *= 0.9;

    if( m_netinfo->GetNetCode() != other.m_netinfo->GetNetCode() )
        similarity *= 0.9;

    if( m_hasSolderMask != other.m_hasSolderMask )
        similarity *= 0.9;

    if( m_solderMaskMargin != other.m_solderMaskMargin )
        similarity *= 0.9;

    similarity *= EDA_SHAPE::Similarity( other );

    return similarity;
}


static struct PCB_SHAPE_DESC
{
    PCB_SHAPE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_SHAPE );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_SHAPE, BOARD_CONNECTED_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_SHAPE, EDA_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ) );

        // Need to initialise enum_map before we can use a Property enum for it
        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        void ( PCB_SHAPE::*shapeLayerSetter )( PCB_LAYER_ID ) = &PCB_SHAPE::SetLayer;
        PCB_LAYER_ID ( PCB_SHAPE::*shapeLayerGetter )() const = &PCB_SHAPE::GetLayer;

        auto layerProperty = new PROPERTY_ENUM<PCB_SHAPE, PCB_LAYER_ID>(
                _HKI( "Layer" ), shapeLayerSetter, shapeLayerGetter );

        propMgr.ReplaceProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ), layerProperty );

        auto isPolygonOrEllipse = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
            {
                const SHAPE_T t = shape->GetShape();
                return t == SHAPE_T::POLY || t == SHAPE_T::ELLIPSE || t == SHAPE_T::ELLIPSE_ARC;
            }
            return false;
        };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ),
                                      isPolygonOrEllipse );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ),
                                      isPolygonOrEllipse );

        propMgr.Mask( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Color" ) );
        propMgr.Mask( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Fill Color" ) );

        auto isNotBezierOrEllipseArc = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
            {
                const SHAPE_T t = shape->GetShape();
                return t != SHAPE_T::BEZIER && t != SHAPE_T::ELLIPSE_ARC;
            }
            return true;
        };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Fill" ),
                                      isNotBezierOrEllipseArc );

        auto isCircle =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return shape->GetShape() == SHAPE_T::CIRCLE;

                    return false;
                };

        auto isNotCircleOrEllipse = []( INSPECTABLE* aItem ) -> bool
        {
            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
            {
                const SHAPE_T t = shape->GetShape();
                return t != SHAPE_T::CIRCLE && t != SHAPE_T::ELLIPSE && t != SHAPE_T::ELLIPSE_ARC;
            }
            return true;
        };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start X" ),
                                      isNotCircleOrEllipse );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start Y" ),
                                      isNotCircleOrEllipse );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "End X" ),
                                      isNotCircleOrEllipse );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "End Y" ),
                                      isNotCircleOrEllipse );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Center X" ), isCircle );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Center Y" ), isCircle );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Radius" ), isCircle );

        auto isCopper =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return shape->IsOnCopperLayer();

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_CONNECTED_ITEM ),
                                      _HKI( "Net" ), isCopper );

        auto isPadEditMode =
                []( BOARD* aBoard ) -> bool
                {
                    if( aBoard && aBoard->IsFootprintHolder() )
                    {
                        for( FOOTPRINT* fp : aBoard->Footprints() )
                        {
                            for( PAD* pad : fp->Pads() )
                            {
                                if( pad->IsEntered() )
                                    return true;
                            }
                        }
                    }

                    return false;
                };

        auto showNumberBoxProperty =
                [&]( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                    {
                        if( shape->GetShape() == SHAPE_T::RECTANGLE )
                            return isPadEditMode( shape->GetBoard() );
                    }

                    return false;
                };

        auto showSpokeTemplateProperty =
                [&]( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                    {
                        if( shape->GetShape() == SHAPE_T::SEGMENT )
                            return isPadEditMode( shape->GetBoard() );
                    }

                    return false;
                };

        const wxString groupPadPrimitives = _HKI( "Pad Primitives" );

        propMgr.AddProperty( new PROPERTY<PCB_SHAPE, bool>( _HKI( "Number Box" ),
                                                            &PCB_SHAPE::SetIsProxyItem,
                                                            &PCB_SHAPE::IsProxyItem ),
                             groupPadPrimitives )
                .SetAvailableFunc( showNumberBoxProperty )
                .SetIsHiddenFromRulesEditor();

        propMgr.AddProperty( new PROPERTY<PCB_SHAPE, bool>( _HKI( "Thermal Spoke Template" ),
                     &PCB_SHAPE::SetIsProxyItem, &PCB_SHAPE::IsProxyItem ),
                     groupPadPrimitives )
                .SetAvailableFunc( showSpokeTemplateProperty )
                .SetIsHiddenFromRulesEditor();

        const wxString groupTechLayers = _HKI( "Technical Layers" );

        auto isExternalCuLayer =
                []( INSPECTABLE* aItem )
                {
                    if( auto shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return IsExternalCopperLayer( shape->GetLayer() );

                    return false;
                };

        propMgr.AddProperty( new PROPERTY<PCB_SHAPE, bool>( _HKI( "Soldermask" ),
                    &PCB_SHAPE::SetHasSolderMask, &PCB_SHAPE::HasSolderMask ),
                    groupTechLayers )
                .SetAvailableFunc( isExternalCuLayer );

        propMgr.AddProperty( new PROPERTY<PCB_SHAPE, std::optional<int>>( _HKI( "Soldermask Margin Override" ),
                    &PCB_SHAPE::SetLocalSolderMaskMargin, &PCB_SHAPE::GetLocalSolderMaskMargin,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    groupTechLayers )
                .SetAvailableFunc( isExternalCuLayer );
    }
} _PCB_SHAPE_DESC;
