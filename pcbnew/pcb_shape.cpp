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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
#include <geometry/shape_circle.h>
#include <geometry/shape_compound.h>
#include <geometry/point_types.h>
#include <geometry/shape_utils.h>
#include <pcb_painter.h>
#include <api/board/board_types.pb.h>
#include <api/api_enums.h>
#include <api/api_utils.h>


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, KICAD_T aItemType, SHAPE_T aShapeType ) :
    BOARD_CONNECTED_ITEM( aParent, aItemType ),
    EDA_SHAPE( aShapeType, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL )
{
    m_hasSolderMask = false;
}


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, SHAPE_T shapetype ) :
    BOARD_CONNECTED_ITEM( aParent, PCB_SHAPE_T ),
    EDA_SHAPE( shapetype, pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL )
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

    const_cast<KIID&>( m_Uuid ) = KIID( msg.id().value() );
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

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
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

    if( !m_hatching.IsEmpty() )
    {
        PCB_LAYER_ID   layer = GetLayer();
        BOX2I          bbox = GetBoundingBox();
        SHAPE_POLY_SET holes;
        int            maxError = ARC_LOW_DEF;

        auto knockoutItem =
                [&]( BOARD_ITEM* item )
                {
                    int margin = GetHatchLineSpacing() / 2;

                    if( item->Type() == PCB_TEXTBOX_T )
                        margin = 0;

                    item->TransformShapeToPolygon( holes, layer, margin, maxError, ERROR_OUTSIDE );
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

            // Knockout footprint courtyard
            holes.Append( footprint->GetCourtyard( layer ) );

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

        if( !holes.IsEmpty() )
        {
            m_hatching.BooleanSubtract( holes );
            m_hatching.Fracture();
        }
    }
}


int PCB_SHAPE::GetWidth() const
{
    // A stroke width of 0 in PCBNew means no-border, but negative stroke-widths are only used
    // in EEschema (see SCH_SHAPE::GetPenWidth()).
    // Since negative stroke widths can trip up down-stream code (such as the Gerber plotter), we
    // weed them out here.
    return std::max( EDA_SHAPE::GetWidth(), 0 );
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
        if( m_poly.OutlineCount() == 1 && m_poly.Outline( 0 ).SegmentCount() == 4 )
        {
            SHAPE_LINE_CHAIN& outline = m_poly.Outline( 0 );

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
        // we want start point the top left point and end point the bottom right
        // (more easy to compare 2 segments: we are seeing them as equivalent if
        // they have the same end points, not necessary the same order)
        VECTOR2I start = GetStart();
        VECTOR2I end = GetEnd();

        if( ( start.x > end.x )
            || ( start.x == end.x && start.y < end.y ) )
        {
            SetStart( end );
            SetEnd( start );
        }
    }
    else
        Normalize();
}


void PCB_SHAPE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    rotate( aRotCentre, aAngle );
}


void PCB_SHAPE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    flip( aCentre, aFlipDirection );

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
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
        if( !aView->IsLayerVisible( m_layer ) )
            return LOD_HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings.GetHighContrast() )
        {
            if( m_layer != renderSettings.GetPrimaryHighContrastLayer() )
                return LOD_HIDE;
        }
    }

    if( FOOTPRINT* parent = GetParentFootprint() )
    {
        if( parent->GetLayer() == F_Cu && !aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
            return LOD_HIDE;

        if( parent->GetLayer() == B_Cu && !aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
            return LOD_HIDE;
    }

    return LOD_SHOW;
}


std::vector<int> PCB_SHAPE::ViewGetLayers() const
{
    std::vector<int> layers;
    layers.reserve( 4 );

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

        // Only polygons have meaningful Position properties.
        // On other shapes, these are duplicates of the Start properties.
        auto isPolygon =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return shape->GetShape() == SHAPE_T::POLY;

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Position X" ), isPolygon );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Position Y" ), isPolygon );

        propMgr.Mask( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Color" ) );
        propMgr.Mask( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ), _HKI( "Fill Color" ) );

        // BEZIER curves are not closed shapes, and fill is not supported in board editor,
        // only in schematic editor.
        // So disable Fill option for Bezier curves
        auto isNotBezier =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return shape->GetShape() != SHAPE_T::BEZIER;

                    return true;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Fill" ), isNotBezier );

        auto isCircle =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return shape->GetShape() == SHAPE_T::CIRCLE;

                    return false;
                };

        auto isNotCircle =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aItem ) )
                        return shape->GetShape() != SHAPE_T::CIRCLE;

                    return true;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Start X" ), isNotCircle );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "Start Y" ), isNotCircle );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "End X" ), isNotCircle );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ),
                                      _HKI( "End Y" ), isNotCircle );
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
