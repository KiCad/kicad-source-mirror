/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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

#include "pcb_track.h"

#include <pcb_base_frame.h>
#include <core/mirror.h>
#include <connectivity/connectivity_data.h>
#include <board.h>
#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <base_units.h>
#include <layer_range.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <lset.h>
#include <string_utils.h>
#include <view/view.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <geometry/geometry_utils.h>
#include <geometry/seg.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_arc.h>
#include <drc/drc_engine.h>
#include <pcb_painter.h>
#include <trigo.h>

#include <google/protobuf/any.pb.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/api_pcb_utils.h>
#include <api/board/board_types.pb.h>

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;

PCB_TRACK::PCB_TRACK( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_CONNECTED_ITEM( aParent, idtype )
{
    m_width = pcbIUScale.mmToIU( 0.2 );     // Gives a reasonable default width
    m_hasSolderMask = false;
}


EDA_ITEM* PCB_TRACK::Clone() const
{
    return new PCB_TRACK( *this );
}


void PCB_TRACK::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_TRACE_T, /* void */ );
    *this = *static_cast<const PCB_TRACK*>( aOther );
}


PCB_ARC::PCB_ARC( BOARD_ITEM* aParent, const SHAPE_ARC* aArc ) :
    PCB_TRACK( aParent, PCB_ARC_T )
{
    m_Start = aArc->GetP0();
    m_End = aArc->GetP1();
    m_Mid = aArc->GetArcMid();
}


EDA_ITEM* PCB_ARC::Clone() const
{
    return new PCB_ARC( *this );
}


void PCB_ARC::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_ARC_T, /* void */ );
    *this = *static_cast<const PCB_ARC*>( aOther );
}


PCB_VIA::PCB_VIA( BOARD_ITEM* aParent ) :
        PCB_TRACK( aParent, PCB_VIA_T ),
        m_padStack( this )
{
    SetViaType( VIATYPE::THROUGH );
    Padstack().Drill().start = F_Cu;
    Padstack().Drill().end = B_Cu;
    SetDrillDefault();

    m_padStack.SetUnconnectedLayerMode( PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL );

    // Padstack layerset is not used for vias right now
    m_padStack.LayerSet().reset();

    // For now, vias are always circles
    m_padStack.SetShape( PAD_SHAPE::CIRCLE, PADSTACK::ALL_LAYERS );

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, BoardCopperLayerCount() ) )
        m_zoneLayerOverrides[layer] = ZLO_NONE;

    m_isFree = false;
}


PCB_VIA::PCB_VIA( const PCB_VIA& aOther ) :
        PCB_TRACK( aOther.GetParent(), PCB_VIA_T ),
        m_padStack( this )
{
    PCB_VIA::operator=( aOther );

    const_cast<KIID&>( m_Uuid ) = aOther.m_Uuid;
    m_zoneLayerOverrides = aOther.m_zoneLayerOverrides;
}


PCB_VIA& PCB_VIA::operator=( const PCB_VIA &aOther )
{
    BOARD_CONNECTED_ITEM::operator=( aOther );

    m_Start = aOther.m_Start;
    m_End = aOther.m_End;

    m_viaType = aOther.m_viaType;
    m_padStack = aOther.m_padStack;
    m_isFree = aOther.m_isFree;

    return *this;
}


void PCB_VIA::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_VIA_T, /* void */ );
    *this = *static_cast<const PCB_VIA*>( aOther );
}


EDA_ITEM* PCB_VIA::Clone() const
{
    return new PCB_VIA( *this );
}


wxString PCB_VIA::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    wxString formatStr;

    switch( GetViaType() )
    {
    case VIATYPE::BLIND_BURIED: formatStr = _( "Blind/Buried Via %s on %s" ); break;
    case VIATYPE::MICROVIA:     formatStr = _( "Micro Via %s on %s" );        break;
    default:                    formatStr = _( "Via %s on %s" );              break;
    }

    return wxString::Format( formatStr, GetNetnameMsg(), layerMaskDescribe() );
}


BITMAPS PCB_VIA::GetMenuImage() const
{
    return BITMAPS::via;
}


bool PCB_TRACK::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_TRACK& other = static_cast<const PCB_TRACK&>( aBoardItem );

    return *this == other;
}


bool PCB_TRACK::operator==( const PCB_TRACK& aOther ) const
{
    return m_Start == aOther.m_Start
            && m_End == aOther.m_End
            && m_layer == aOther.m_layer
            && m_width == aOther.m_width
            && m_hasSolderMask == aOther.m_hasSolderMask
            && m_solderMaskMargin == aOther.m_solderMaskMargin;
}


double PCB_TRACK::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_TRACK& other = static_cast<const PCB_TRACK&>( aOther );

    double similarity = 1.0;

    if( m_layer != other.m_layer )
        similarity *= 0.9;

    if( m_width != other.m_width )
        similarity *= 0.9;

    if( m_Start != other.m_Start )
        similarity *= 0.9;

    if( m_End != other.m_End )
        similarity *= 0.9;

    if( m_hasSolderMask != other.m_hasSolderMask )
        similarity *= 0.9;

    if( m_solderMaskMargin != other.m_solderMaskMargin )
        similarity *= 0.9;

    return similarity;
}


bool PCB_ARC::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_ARC& other = static_cast<const PCB_ARC&>( aBoardItem );

    return *this == other;
}


bool PCB_ARC::operator==( const PCB_TRACK& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const PCB_ARC& other = static_cast<const PCB_ARC&>( aOther );

    return *this == other;
}


bool PCB_ARC::operator==( const PCB_ARC& aOther ) const
{
    return m_Start == aOther.m_Start
            && m_End == aOther.m_End
            && m_Mid == aOther.m_Mid
            && m_layer == aOther.m_layer
            && GetWidth() == aOther.GetWidth()
            && m_hasSolderMask == aOther.m_hasSolderMask
            && m_solderMaskMargin == aOther.m_solderMaskMargin;
}


double PCB_ARC::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_ARC& other = static_cast<const PCB_ARC&>( aOther );

    double similarity = 1.0;

    if( m_layer != other.m_layer )
        similarity *= 0.9;

    if( GetWidth() != other.GetWidth() )
        similarity *= 0.9;

    if( m_Start != other.m_Start )
        similarity *= 0.9;

    if( m_End != other.m_End )
        similarity *= 0.9;

    if( m_Mid != other.m_Mid )
        similarity *= 0.9;

    if( m_hasSolderMask != other.m_hasSolderMask )
        similarity *= 0.9;

    if( m_solderMaskMargin != other.m_solderMaskMargin )
        similarity *= 0.9;

    return similarity;
}


bool PCB_VIA::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_VIA& other = static_cast<const PCB_VIA&>( aBoardItem );

    return *this == other;
}


bool PCB_VIA::operator==( const PCB_TRACK& aOther ) const
{
    if( aOther.Type() != Type() )
        return false;

    const PCB_VIA& other = static_cast<const PCB_VIA&>( aOther );

    return *this == other;
}


bool PCB_VIA::operator==( const PCB_VIA& aOther ) const
{
    return m_Start == aOther.m_Start
            && m_End == aOther.m_End
            && m_layer == aOther.m_layer
            && m_padStack == aOther.m_padStack
            && m_viaType == aOther.m_viaType
            && m_zoneLayerOverrides == aOther.m_zoneLayerOverrides;
}


double PCB_VIA::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_VIA& other = static_cast<const PCB_VIA&>( aOther );

    double similarity = 1.0;

    if( m_layer != other.m_layer )
        similarity *= 0.9;

    if( m_Start != other.m_Start )
        similarity *= 0.9;

    if( m_End != other.m_End )
        similarity *= 0.9;

    if( m_padStack != other.m_padStack )
        similarity *= 0.9;

    if( m_viaType != other.m_viaType )
        similarity *= 0.9;

    if( m_zoneLayerOverrides != other.m_zoneLayerOverrides )
        similarity *= 0.9;

    return similarity;
}


void PCB_VIA::SetWidth( int aWidth )
{
    m_padStack.SetSize( { aWidth, aWidth }, PADSTACK::ALL_LAYERS );
}


int PCB_VIA::GetWidth() const
{
    return m_padStack.Size( PADSTACK::ALL_LAYERS ).x;
}


void PCB_VIA::SetWidth( PCB_LAYER_ID aLayer, int aWidth )
{
    m_padStack.SetSize( { aWidth, aWidth }, aLayer );
}


int PCB_VIA::GetWidth( PCB_LAYER_ID aLayer ) const
{
    return m_padStack.Size( aLayer ).x;
}


void PCB_TRACK::Serialize( google::protobuf::Any &aContainer ) const
{
    kiapi::board::types::Track track;

    track.mutable_id()->set_value( m_Uuid.AsStdString() );
    track.mutable_start()->set_x_nm( GetStart().x );
    track.mutable_start()->set_y_nm( GetStart().y );
    track.mutable_end()->set_x_nm( GetEnd().x );
    track.mutable_end()->set_y_nm( GetEnd().y );
    track.mutable_width()->set_value_nm( GetWidth() );
    track.set_layer( ToProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( GetLayer() ) );
    track.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                                 : kiapi::common::types::LockedState::LS_UNLOCKED );
    PackNet( track.mutable_net() );
    // TODO m_hasSolderMask and m_solderMaskMargin

    aContainer.PackFrom( track );
}


bool PCB_TRACK::Deserialize( const google::protobuf::Any &aContainer )
{
    kiapi::board::types::Track track;

    if( !aContainer.UnpackTo( &track ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( track.id().value() );
    SetStart( VECTOR2I( track.start().x_nm(), track.start().y_nm() ) );
    SetEnd( VECTOR2I( track.end().x_nm(), track.end().y_nm() ) );
    SetWidth( track.width().value_nm() );
    SetLayer( FromProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( track.layer() ) );
    UnpackNet( track.net() );
    SetLocked( track.locked() == kiapi::common::types::LockedState::LS_LOCKED );
    // TODO m_hasSolderMask and m_solderMaskMargin

    return true;
}


void PCB_ARC::Serialize( google::protobuf::Any &aContainer ) const
{
    kiapi::board::types::Arc arc;

    arc.mutable_id()->set_value( m_Uuid.AsStdString() );
    arc.mutable_start()->set_x_nm( GetStart().x );
    arc.mutable_start()->set_y_nm( GetStart().y );
    arc.mutable_mid()->set_x_nm( GetMid().x );
    arc.mutable_mid()->set_y_nm( GetMid().y );
    arc.mutable_end()->set_x_nm( GetEnd().x );
    arc.mutable_end()->set_y_nm( GetEnd().y );
    arc.mutable_width()->set_value_nm( GetWidth() );
    arc.set_layer( ToProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( GetLayer() ) );
    arc.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                               : kiapi::common::types::LockedState::LS_UNLOCKED );
    PackNet( arc.mutable_net() );
    // TODO m_hasSolderMask and m_solderMaskMargin

    aContainer.PackFrom( arc );
}


bool PCB_ARC::Deserialize( const google::protobuf::Any &aContainer )
{
    kiapi::board::types::Arc arc;

    if( !aContainer.UnpackTo( &arc ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( arc.id().value() );
    SetStart( VECTOR2I( arc.start().x_nm(), arc.start().y_nm() ) );
    SetMid( VECTOR2I( arc.mid().x_nm(), arc.mid().y_nm() ) );
    SetEnd( VECTOR2I( arc.end().x_nm(), arc.end().y_nm() ) );
    SetWidth( arc.width().value_nm() );
    SetLayer( FromProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( arc.layer() ) );
    UnpackNet( arc.net() );
    SetLocked( arc.locked() == kiapi::common::types::LockedState::LS_LOCKED );
    // TODO m_hasSolderMask and m_solderMaskMargin

    return true;
}


void PCB_VIA::Serialize( google::protobuf::Any &aContainer ) const
{
    kiapi::board::types::Via via;

    via.mutable_id()->set_value( m_Uuid.AsStdString() );
    via.mutable_position()->set_x_nm( GetPosition().x );
    via.mutable_position()->set_y_nm( GetPosition().y );

    PADSTACK padstack = Padstack();

    google::protobuf::Any padStackWrapper;
    padstack.Serialize( padStackWrapper );
    padStackWrapper.UnpackTo( via.mutable_pad_stack() );

    // PADSTACK::m_layerSet is not used by vias
    via.mutable_pad_stack()->clear_layers();
    kiapi::board::PackLayerSet( *via.mutable_pad_stack()->mutable_layers(), GetLayerSet() );

    via.set_type( ToProtoEnum<VIATYPE, kiapi::board::types::ViaType>( GetViaType() ) );
    via.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                               : kiapi::common::types::LockedState::LS_UNLOCKED );
    PackNet( via.mutable_net() );

    aContainer.PackFrom( via );
}


bool PCB_VIA::Deserialize( const google::protobuf::Any &aContainer )
{
    kiapi::board::types::Via via;

    if( !aContainer.UnpackTo( &via ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( via.id().value() );
    SetStart( VECTOR2I( via.position().x_nm(), via.position().y_nm() ) );
    SetEnd( GetStart() );

    google::protobuf::Any padStackWrapper;
    padStackWrapper.PackFrom( via.pad_stack() );

    if( !m_padStack.Deserialize( padStackWrapper ) )
        return false;

    // PADSTACK::m_layerSet is not used by vias
    m_padStack.LayerSet().reset();

    SetViaType( FromProtoEnum<VIATYPE>( via.type() ) );
    UnpackNet( via.net() );
    SetLocked( via.locked() == kiapi::common::types::LockedState::LS_LOCKED );

    return true;
}


bool PCB_TRACK::ApproxCollinear( const PCB_TRACK& aTrack )
{
    SEG a( m_Start, m_End );
    SEG b( aTrack.GetStart(), aTrack.GetEnd() );
    return a.ApproxCollinear( b );
}


MINOPTMAX<int> PCB_TRACK::GetWidthConstraint( wxString* aSource ) const
{
    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRules( TRACK_WIDTH_CONSTRAINT, this, nullptr, m_layer );
    }

    if( aSource )
        *aSource = constraint.GetName();

    return constraint.Value();
}


MINOPTMAX<int> PCB_VIA::GetWidthConstraint( wxString* aSource ) const
{
    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRules( VIA_DIAMETER_CONSTRAINT, this, nullptr, m_layer );
    }

    if( aSource )
        *aSource = constraint.GetName();

    return constraint.Value();
}


MINOPTMAX<int> PCB_VIA::GetDrillConstraint( wxString* aSource ) const
{
    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRules( HOLE_SIZE_CONSTRAINT, this, nullptr, m_layer );
    }

    if( aSource )
        *aSource = constraint.GetName();

    return constraint.Value();
}


int PCB_VIA::GetMinAnnulus( PCB_LAYER_ID aLayer, wxString* aSource ) const
{
    if( !FlashLayer( aLayer ) )
    {
        if( aSource )
            *aSource = _( "removed annular ring" );

        return 0;
    }

    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRules( ANNULAR_WIDTH_CONSTRAINT, this, nullptr, aLayer );
    }

    if( constraint.Value().HasMin() )
    {
        if( aSource )
            *aSource = constraint.GetName();

        return constraint.Value().Min();
    }

    return 0;
}


int PCB_VIA::GetDrillValue() const
{
    if( m_padStack.Drill().size.x > 0 ) // Use the specific value.
        return m_padStack.Drill().size.x;

    // Use the default value from the Netclass
    NETCLASS* netclass = GetEffectiveNetClass();

    if( GetViaType() == VIATYPE::MICROVIA )
        return netclass->GetuViaDrill();

    return netclass->GetViaDrill();
}


EDA_ITEM_FLAGS PCB_TRACK::IsPointOnEnds( const VECTOR2I& point, int min_dist ) const
{
    EDA_ITEM_FLAGS result = 0;

    if( min_dist < 0 )
        min_dist = m_width / 2;

    if( min_dist == 0 )
    {
        if( m_Start == point  )
            result |= STARTPOINT;

        if( m_End == point )
            result |= ENDPOINT;
    }
    else
    {
        double dist = m_Start.Distance( point );

        if( min_dist >= dist )
            result |= STARTPOINT;

        dist = m_End.Distance( point );

        if( min_dist >= dist )
            result |= ENDPOINT;
    }

    return result;
}


const BOX2I PCB_TRACK::GetBoundingBox() const
{
    // end of track is round, this is its radius, rounded up
    int radius = ( m_width + 1 ) / 2;
    int ymax, xmax, ymin, xmin;

    if( Type() == PCB_VIA_T )
    {
        ymax = m_Start.y;
        xmax = m_Start.x;

        ymin = m_Start.y;
        xmin = m_Start.x;
    }
    else if( Type() == PCB_ARC_T )
    {
        std::shared_ptr<SHAPE> arc = GetEffectiveShape();
        BOX2I bbox = arc->BBox();

        xmin = bbox.GetLeft();
        xmax = bbox.GetRight();
        ymin = bbox.GetTop();
        ymax = bbox.GetBottom();
    }
    else
    {
        ymax = std::max( m_Start.y, m_End.y );
        xmax = std::max( m_Start.x, m_End.x );

        ymin = std::min( m_Start.y, m_End.y );
        xmin = std::min( m_Start.x, m_End.x );
    }

    ymax += radius;
    xmax += radius;

    ymin -= radius;
    xmin -= radius;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    return BOX2ISafe( VECTOR2I( xmin, ymin ),
                      VECTOR2L( (int64_t) xmax - xmin + 1, (int64_t) ymax - ymin + 1 ) );
}


const BOX2I PCB_VIA::GetBoundingBox() const
{
    int radius = 0;

    Padstack().ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            radius = std::max( radius, GetWidth( aLayer ) );
        } );

    // via is round, this is its radius, rounded up
    radius = ( radius + 1 ) / 2;

    int ymax = m_Start.y + radius;
    int xmax = m_Start.x + radius;

    int ymin = m_Start.y - radius;
    int xmin = m_Start.x - radius;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    return BOX2ISafe( VECTOR2I( xmin, ymin ),
                      VECTOR2L( (int64_t) xmax - xmin + 1, (int64_t) ymax - ymin + 1 ) );
}


const BOX2I PCB_VIA::GetBoundingBox( PCB_LAYER_ID aLayer ) const
{
    int radius = GetWidth( aLayer );

    // via is round, this is its radius, rounded up
    radius = ( radius + 1 ) / 2;

    int ymax = m_Start.y + radius;
    int xmax = m_Start.x + radius;

    int ymin = m_Start.y - radius;
    int xmin = m_Start.x - radius;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    return BOX2ISafe( VECTOR2I( xmin, ymin ),
                      VECTOR2L( (int64_t) xmax - xmin + 1, (int64_t) ymax - ymin + 1 ) );
}


double PCB_TRACK::GetLength() const
{
    return m_Start.Distance( m_End );
}


double PCB_TRACK::GetDelay() const
{
    const BOARD* board = GetBoard();

    if( !board )
        return 0.0;

    const LENGTH_DELAY_CALCULATION*            calc = board->GetLengthCalculation();
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> items{ calc->GetLengthCalculationItem( this ) };
    constexpr PATH_OPTIMISATIONS               opts = { .OptimiseViaLayers = false,
                                                        .MergeTracks = false,
                                                        .OptimiseTracesInPads = false,
                                                        .InferViaInPad = false
                                                      };

    return (double) calc->CalculateDelay( items, opts );
}


void PCB_TRACK::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_Start, aRotCentre, aAngle );
    RotatePoint( m_End, aRotCentre, aAngle );
}


void PCB_ARC::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_Start, aRotCentre, aAngle );
    RotatePoint( m_End, aRotCentre, aAngle );
    RotatePoint( m_Mid, aRotCentre, aAngle );
}


void PCB_TRACK::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    MIRROR( m_Start, aCentre, aFlipDirection );
    MIRROR( m_End, aCentre, aFlipDirection );
}


void PCB_ARC::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    MIRROR( m_Start, aCentre, aFlipDirection );
    MIRROR( m_End, aCentre, aFlipDirection );
    MIRROR( m_Mid, aCentre, aFlipDirection );
}


void PCB_TRACK::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
    }

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
}


void PCB_ARC::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
        m_Mid.x = aCentre.x - ( m_Mid.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
        m_Mid.y = aCentre.y - ( m_Mid.y - aCentre.y );
    }

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
}


bool PCB_ARC::IsCCW() const
{
    VECTOR2L start = m_Start;
    VECTOR2L start_end = m_End - start;
    VECTOR2L start_mid = m_Mid - start;

    return start_end.Cross( start_mid ) < 0;
}


void PCB_VIA::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
    }

    if( GetViaType() != VIATYPE::THROUGH )
    {
        PCB_LAYER_ID top_layer;
        PCB_LAYER_ID bottom_layer;
        LayerPair( &top_layer, &bottom_layer );
        top_layer    = GetBoard()->FlipLayer( top_layer );
        bottom_layer = GetBoard()->FlipLayer( bottom_layer );
        SetLayerPair( top_layer, bottom_layer );
    }
}


INSPECT_RESULT PCB_TRACK::Visit( INSPECTOR inspector, void* testData,
                                 const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == Type() )
        {
            if( INSPECT_RESULT::QUIT == inspector( this, testData ) )
                return INSPECT_RESULT::QUIT;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


std::shared_ptr<SHAPE_SEGMENT> PCB_VIA::GetEffectiveHoleShape() const
{
    return std::make_shared<SHAPE_SEGMENT>( SEG( m_Start, m_Start ), Padstack().Drill().size.x );
}

// clang-format off: the suggestion is slightly less readable
void PCB_VIA::SetFrontTentingMode( TENTING_MODE aMode )
{
    switch( aMode )
    {
    case TENTING_MODE::FROM_RULES: m_padStack.FrontOuterLayers().has_solder_mask.reset();  break;
    case TENTING_MODE::TENTED:     m_padStack.FrontOuterLayers().has_solder_mask = true;   break;
    case TENTING_MODE::NOT_TENTED: m_padStack.FrontOuterLayers().has_solder_mask = false;  break;
    }
}


TENTING_MODE PCB_VIA::GetFrontTentingMode() const
{
    if( m_padStack.FrontOuterLayers().has_solder_mask.has_value() )
    {
        return *m_padStack.FrontOuterLayers().has_solder_mask ?
                TENTING_MODE::TENTED : TENTING_MODE::NOT_TENTED;
    }

    return TENTING_MODE::FROM_RULES;
}


void PCB_VIA::SetBackTentingMode( TENTING_MODE aMode )
{
    switch( aMode )
    {
    case TENTING_MODE::FROM_RULES: m_padStack.BackOuterLayers().has_solder_mask.reset();  break;
    case TENTING_MODE::TENTED:     m_padStack.BackOuterLayers().has_solder_mask = true;   break;
    case TENTING_MODE::NOT_TENTED: m_padStack.BackOuterLayers().has_solder_mask = false;  break;
    }
}


TENTING_MODE PCB_VIA::GetBackTentingMode() const
{
    if( m_padStack.BackOuterLayers().has_solder_mask.has_value() )
    {
        return *m_padStack.BackOuterLayers().has_solder_mask ?
                TENTING_MODE::TENTED : TENTING_MODE::NOT_TENTED;
    }

    return TENTING_MODE::FROM_RULES;
}


void PCB_VIA::SetFrontCoveringMode( COVERING_MODE aMode )
{
    switch( aMode )
    {
    case COVERING_MODE::FROM_RULES: m_padStack.FrontOuterLayers().has_covering.reset();  break;
    case COVERING_MODE::COVERED:     m_padStack.FrontOuterLayers().has_covering = true;   break;
    case COVERING_MODE::NOT_COVERED: m_padStack.FrontOuterLayers().has_covering = false;  break;
    }
}


COVERING_MODE PCB_VIA::GetFrontCoveringMode() const
{
    if( m_padStack.FrontOuterLayers().has_covering.has_value() )
    {
        return *m_padStack.FrontOuterLayers().has_covering ?
                COVERING_MODE::COVERED : COVERING_MODE::NOT_COVERED;
    }

    return COVERING_MODE::FROM_RULES;
}


void PCB_VIA::SetBackCoveringMode( COVERING_MODE aMode )
{
    switch( aMode )
    {
    case COVERING_MODE::FROM_RULES: m_padStack.BackOuterLayers().has_covering.reset();  break;
    case COVERING_MODE::COVERED:     m_padStack.BackOuterLayers().has_covering = true;   break;
    case COVERING_MODE::NOT_COVERED: m_padStack.BackOuterLayers().has_covering = false;  break;
    }
}


COVERING_MODE PCB_VIA::GetBackCoveringMode() const
{
    if( m_padStack.BackOuterLayers().has_covering.has_value() )
    {
        return *m_padStack.BackOuterLayers().has_covering ?
                COVERING_MODE::COVERED : COVERING_MODE::NOT_COVERED;
    }

    return COVERING_MODE::FROM_RULES;
}


void PCB_VIA::SetFrontPluggingMode( PLUGGING_MODE aMode )
{
    switch( aMode )
    {
    case PLUGGING_MODE::FROM_RULES: m_padStack.FrontOuterLayers().has_plugging.reset();  break;
    case PLUGGING_MODE::PLUGGED:     m_padStack.FrontOuterLayers().has_plugging = true;   break;
    case PLUGGING_MODE::NOT_PLUGGED: m_padStack.FrontOuterLayers().has_plugging = false;  break;
    }
}


PLUGGING_MODE PCB_VIA::GetFrontPluggingMode() const
{
    if( m_padStack.FrontOuterLayers().has_plugging.has_value() )
    {
        return *m_padStack.FrontOuterLayers().has_plugging ?
                PLUGGING_MODE::PLUGGED : PLUGGING_MODE::NOT_PLUGGED;
    }

    return PLUGGING_MODE::FROM_RULES;
}


void PCB_VIA::SetBackPluggingMode( PLUGGING_MODE aMode )
{
    switch( aMode )
    {
    case PLUGGING_MODE::FROM_RULES: m_padStack.BackOuterLayers().has_plugging.reset();  break;
    case PLUGGING_MODE::PLUGGED:     m_padStack.BackOuterLayers().has_plugging = true;   break;
    case PLUGGING_MODE::NOT_PLUGGED: m_padStack.BackOuterLayers().has_plugging = false;  break;
    }
}


PLUGGING_MODE PCB_VIA::GetBackPluggingMode() const
{
    if( m_padStack.BackOuterLayers().has_plugging.has_value() )
    {
        return *m_padStack.BackOuterLayers().has_plugging ?
                PLUGGING_MODE::PLUGGED : PLUGGING_MODE::NOT_PLUGGED;
    }

    return PLUGGING_MODE::FROM_RULES;
}


void PCB_VIA::SetCappingMode( CAPPING_MODE aMode )
{
    switch( aMode )
    {
    case CAPPING_MODE::FROM_RULES: m_padStack.Drill().is_capped.reset();  break;
    case CAPPING_MODE::CAPPED:     m_padStack.Drill().is_capped = true;   break;
    case CAPPING_MODE::NOT_CAPPED: m_padStack.Drill().is_capped = false;  break;
    }
}


CAPPING_MODE PCB_VIA::GetCappingMode() const
{
    if( m_padStack.Drill().is_capped.has_value() )
    {
        return *m_padStack.Drill().is_capped ?
                CAPPING_MODE::CAPPED : CAPPING_MODE::NOT_CAPPED;
    }

    return CAPPING_MODE::FROM_RULES;
}


void PCB_VIA::SetFillingMode( FILLING_MODE aMode )
{
    switch( aMode )
    {
    case FILLING_MODE::FROM_RULES: m_padStack.Drill().is_filled.reset();  break;
    case FILLING_MODE::FILLED:     m_padStack.Drill().is_filled = true;   break;
    case FILLING_MODE::NOT_FILLED: m_padStack.Drill().is_filled = false;  break;
    }
}


FILLING_MODE PCB_VIA::GetFillingMode() const
{
    if( m_padStack.Drill().is_filled.has_value() )
    {
        return *m_padStack.Drill().is_filled ?
                FILLING_MODE::FILLED : FILLING_MODE::NOT_FILLED;
    }

    return FILLING_MODE::FROM_RULES;
}
// clang-format on: the suggestion is slightly less readable


bool PCB_VIA::IsTented( PCB_LAYER_ID aLayer ) const
{
    wxCHECK_MSG( IsFrontLayer( aLayer ) || IsBackLayer( aLayer ), true,
                 "Invalid layer passed to IsTented" );

    bool front = IsFrontLayer( aLayer );

    if( front && m_padStack.FrontOuterLayers().has_solder_mask.has_value() )
        return *m_padStack.FrontOuterLayers().has_solder_mask;

    if( !front && m_padStack.BackOuterLayers().has_solder_mask.has_value() )
        return *m_padStack.BackOuterLayers().has_solder_mask;

    if( const BOARD* board = GetBoard() )
    {
        return front ? board->GetDesignSettings().m_TentViasFront
                     : board->GetDesignSettings().m_TentViasBack;
    }

    return true;
}


int PCB_VIA::GetSolderMaskExpansion() const
{
    if( const BOARD* board = GetBoard() )
        return board->GetDesignSettings().m_SolderMaskExpansion;
    else
        return 0;
}


int PCB_TRACK::GetSolderMaskExpansion() const
{
    int margin = 0;

    if( const BOARD* board = GetBoard() )
    {
        DRC_CONSTRAINT              constraint;
        std::shared_ptr<DRC_ENGINE> drcEngine = board->GetDesignSettings().m_DRCEngine;

        constraint = drcEngine->EvalRules( SOLDER_MASK_EXPANSION_CONSTRAINT, this, nullptr, m_layer );

        if( constraint.m_Value.HasOpt() )
            margin = constraint.m_Value.Opt();
    }
    else if( m_solderMaskMargin.has_value() )
    {
        margin = m_solderMaskMargin.value();
    }

    // Ensure the resulting mask opening has a non-negative size
    if( margin < 0 )
        margin = std::max( margin, -m_width / 2 );

    return margin;
}


bool PCB_TRACK::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    if( aLayer == m_layer )
    {
        return true;
    }

    if( m_hasSolderMask && (   ( aLayer == F_Mask && m_layer == F_Cu )
                            || ( aLayer == B_Mask && m_layer == B_Cu ) ) )
    {
        return true;
    }

    return false;
}


bool PCB_VIA::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
#if 0
    // Nice and simple, but raises its ugly head in performance profiles....
    return GetLayerSet().test( aLayer );
#endif
    if( IsCopperLayer( aLayer ) &&
        LAYER_RANGE::Contains( Padstack().Drill().start, Padstack().Drill().end, aLayer ) )
    {
        return true;
    }

    // Test for via on mask layers: a via on on a mask layer if not tented and if
    // it is on the corresponding external copper layer
    if( aLayer == F_Mask )
        return Padstack().Drill().start == F_Cu && !IsTented( F_Mask );
    else if( aLayer == B_Mask )
        return Padstack().Drill().end == B_Cu && !IsTented( B_Mask );

    return false;
}


bool PCB_VIA::HasValidLayerPair( int aCopperLayerCount )
{
    // return true if top and bottom layers are valid, depending on the copper layer count
    // aCopperLayerCount is expected >= 2

    int layer_id = aCopperLayerCount*2;

    if( Padstack().Drill().start > B_Cu )
    {
        if( Padstack().Drill().start > layer_id )
            return false;
    }
    if( Padstack().Drill().end > B_Cu )
    {
        if( Padstack().Drill().end > layer_id )
            return false;
    }

    return true;
}


PCB_LAYER_ID PCB_VIA::GetLayer() const
{
    return Padstack().Drill().start;
}


void PCB_VIA::SetLayer( PCB_LAYER_ID aLayer )
{
     Padstack().Drill().start = aLayer;
}


void PCB_TRACK::SetLayerSet( const LSET& aLayerSet )
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


LSET PCB_TRACK::GetLayerSet() const
{
    LSET layermask( { m_layer } );

    if( m_hasSolderMask )
    {
        if( layermask.test( F_Cu ) )
            layermask.set( F_Mask );
        else if( layermask.test( B_Cu ) )
            layermask.set( B_Mask );
    }

    return layermask;
}


LSET PCB_VIA::GetLayerSet() const
{
    LSET layermask;

    if( Padstack().Drill().start < PCBNEW_LAYER_ID_START )
        return layermask;

    if( GetViaType() == VIATYPE::THROUGH )
    {
        layermask = LSET::AllCuMask( BoardCopperLayerCount() );
    }
    else
    {
        LAYER_RANGE range( Padstack().Drill().start, Padstack().Drill().end, BoardCopperLayerCount() );

        int cnt = BoardCopperLayerCount();
        // PCB_LAYER_IDs are numbered from front to back, this is top to bottom.
        for( PCB_LAYER_ID id : range )
        {
            layermask.set( id );

            if( --cnt <= 0 )
                break;
        }
    }

    if( !IsTented( F_Mask ) && layermask.test( F_Cu ) )
        layermask.set( F_Mask );

    if( !IsTented( B_Mask ) && layermask.test( B_Cu ) )
        layermask.set( B_Mask );

    return layermask;
}


void PCB_VIA::SetLayerSet( const LSET& aLayerSet )
{
    // Vias do not use a LSET, just a top and bottom layer pair
    // So we need to set these 2 layers according to the allowed layers in aLayerSet

    // For via through, only F_Cu and B_Cu are allowed. aLayerSet is ignored
    if( GetViaType() == VIATYPE::THROUGH )
    {
        Padstack().Drill().start = F_Cu;
        Padstack().Drill().end = B_Cu;
        return;
    }

    // For blind buried vias, find the top and bottom layers
    bool top_found = false;
    bool bottom_found = false;

    aLayerSet.RunOnLayers(
            [&]( PCB_LAYER_ID layer )
            {
                // tpo layer and bottom Layer are copper layers, so consider only copper layers
                if( IsCopperLayer( layer ) )
                {
                    // The top layer is the first layer found in list and
                    // cannot the B_Cu
                    if( !top_found && layer != B_Cu )
                    {
                        Padstack().Drill().start = layer;
                        top_found = true;
                    }

                    // The bottom layer is the last layer found in list or B_Cu
                    if( !bottom_found )
                        Padstack().Drill().end = layer;

                    if( layer == B_Cu )
                        bottom_found = true;
                }
            } );
}


void PCB_VIA::SetLayerPair( PCB_LAYER_ID aTopLayer, PCB_LAYER_ID aBottomLayer )
{
    Padstack().Drill().start = aTopLayer;
    Padstack().Drill().end = aBottomLayer;
    SanitizeLayers();
}


void PCB_VIA::SetTopLayer( PCB_LAYER_ID aLayer )
{
    // refuse invalid via
    if( aLayer == Padstack().Drill().end )
        return;

    Padstack().Drill().start = aLayer;
    SanitizeLayers();
}


void PCB_VIA::SetBottomLayer( PCB_LAYER_ID aLayer )
{
    // refuse invalid via
    if( aLayer == Padstack().Drill().start )
        return;

    Padstack().Drill().end = aLayer;
    SanitizeLayers();
}


void PCB_VIA::LayerPair( PCB_LAYER_ID* top_layer, PCB_LAYER_ID* bottom_layer ) const
{
    PCB_LAYER_ID t_layer = F_Cu;
    PCB_LAYER_ID b_layer = B_Cu;

    if( GetViaType() != VIATYPE::THROUGH )
    {
        b_layer = Padstack().Drill().end;
        t_layer = Padstack().Drill().start;

        if( !IsCopperLayerLowerThan( b_layer, t_layer ) )
            std::swap( b_layer, t_layer );
    }

    if( top_layer )
        *top_layer = t_layer;

    if( bottom_layer )
        *bottom_layer = b_layer;
}


PCB_LAYER_ID PCB_VIA::TopLayer() const
{
    return Padstack().Drill().start;
}


PCB_LAYER_ID PCB_VIA::BottomLayer() const
{
    return Padstack().Drill().end;
}


void PCB_VIA::SanitizeLayers()
{
    if( GetViaType() == VIATYPE::THROUGH )
    {
        Padstack().Drill().start = F_Cu;
        Padstack().Drill().end = B_Cu;
    }

    if( !IsCopperLayerLowerThan( Padstack().Drill().end, Padstack().Drill().start) )
        std::swap( Padstack().Drill().end, Padstack().Drill().start );
}


bool PCB_VIA::FlashLayer( const LSET& aLayers ) const
{
    for( PCB_LAYER_ID layer : aLayers )
    {
        if( FlashLayer( layer ) )
            return true;
    }

    return false;
}


bool PCB_VIA::FlashLayer( int aLayer ) const
{
    // Return the "normal" shape if the caller doesn't specify a particular layer
    if( aLayer == UNDEFINED_LAYER )
        return true;

    const BOARD* board = GetBoard();
    PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( aLayer );

    if( !board )
        return true;

    if( !IsOnLayer( layer ) )
        return false;

    if( !IsCopperLayer( layer ) )
        return true;

    switch( Padstack().UnconnectedLayerMode() )
    {
    case PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL:
        return true;

    case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END:
    {
        if( layer == Padstack().Drill().start || layer == Padstack().Drill().end )
            return true;

        // Check for removal below
        break;
    }

    case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL:
        // Check for removal below
        break;

    case PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY:
        return layer == Padstack().Drill().start || layer == Padstack().Drill().end;
    }

    if( GetZoneLayerOverride( layer ) == ZLO_FORCE_FLASHED )
    {
        return true;
    }
    else
    {
        // Must be static to keep from raising its ugly head in performance profiles
        static std::initializer_list<KICAD_T> nonZoneTypes = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                               PCB_PAD_T };

        return board->GetConnectivity()->IsConnectedOnLayer( this, layer, nonZoneTypes );
    }
}


void PCB_VIA::ClearZoneLayerOverrides()
{
    std::unique_lock<std::mutex> cacheLock( m_zoneLayerOverridesMutex );

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, BoardCopperLayerCount() ) )
        m_zoneLayerOverrides[layer] = ZLO_NONE;
}


const ZONE_LAYER_OVERRIDE& PCB_VIA::GetZoneLayerOverride( PCB_LAYER_ID aLayer ) const
{
    static const ZONE_LAYER_OVERRIDE defaultOverride = ZLO_NONE;
    auto it = m_zoneLayerOverrides.find( aLayer );
    return it != m_zoneLayerOverrides.end() ? it->second : defaultOverride;
}


void PCB_VIA::SetZoneLayerOverride( PCB_LAYER_ID aLayer, ZONE_LAYER_OVERRIDE aOverride )
{
    std::unique_lock<std::mutex> cacheLock( m_zoneLayerOverridesMutex );
    m_zoneLayerOverrides[aLayer] = aOverride;
}


void PCB_VIA::GetOutermostConnectedLayers( PCB_LAYER_ID* aTopmost,
                                           PCB_LAYER_ID* aBottommost ) const
{
    *aTopmost = UNDEFINED_LAYER;
    *aBottommost = UNDEFINED_LAYER;

    static std::initializer_list<KICAD_T> nonZoneTypes = { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                           PCB_PAD_T };

    for( int layer = TopLayer(); layer <= BottomLayer(); ++layer )
    {
        bool connected = false;

        if( GetZoneLayerOverride( static_cast<PCB_LAYER_ID>( layer ) ) == ZLO_FORCE_FLASHED )
        {
            connected = true;
        }
        else if( GetBoard()->GetConnectivity()->IsConnectedOnLayer( this, layer, nonZoneTypes ) )
        {
            connected = true;
        }

        if( connected )
        {
            if( *aTopmost == UNDEFINED_LAYER )
                *aTopmost = ToLAYER_ID( layer );

            *aBottommost = ToLAYER_ID( layer );
        }
    }

}


std::vector<int> PCB_TRACK::ViewGetLayers() const
{
    // Show the track and its netname on different layers
    const PCB_LAYER_ID layer = GetLayer();
    std::vector<int>   layers{
        layer,
        GetNetnameLayer( layer ),
        LAYER_CLEARANCE_START + layer,
    };

    layers.reserve( 6 );

    if( m_hasSolderMask )
    {
        if( m_layer == F_Cu )
            layers.push_back( F_Mask );
        else if( m_layer == B_Cu )
            layers.push_back( B_Mask );
    }

    if( IsLocked() )
        layers.push_back( LAYER_LOCKED_ITEM_SHADOW );

    return layers;
}


double PCB_TRACK::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    if( !aView->IsLayerVisible( LAYER_TRACKS ) )
        return LOD_HIDE;

    if( IsNetnameLayer( aLayer ) )
    {
        if( GetNetCode() <= NETINFO_LIST::UNCONNECTED )
            return LOD_HIDE;

        // Hide netnames on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return LOD_HIDE;
        }

        VECTOR2I start( GetStart() );
        VECTOR2I end( GetEnd() );

        // Calc the approximate size of the netname (assume square chars)
        SEG::ecoord nameSize = GetDisplayNetname().size() * GetWidth();

        if( VECTOR2I( end - start ).SquaredEuclideanNorm() < nameSize * nameSize )
            return LOD_HIDE;

        BOX2I clipBox = BOX2ISafe( aView->GetViewport() );

        ClipLine( &clipBox, start.x, start.y, end.x, end.y );

        if( VECTOR2I( end - start ).SquaredEuclideanNorm() == 0 )
            return LOD_HIDE;

        // Netnames will be shown only if zoom is appropriate
        return lodScaleForThreshold( aView, m_width, pcbIUScale.mmToIU( 4.0 ) );
    }

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow if the main layer is not shown
        if( !aView->IsLayerVisible( m_layer ) )
            return LOD_HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return LOD_HIDE;
        }
    }

    // Other layers are shown without any conditions
    return LOD_SHOW;
}


const BOX2I PCB_TRACK::ViewBBox() const
{
    BOX2I bbox = GetBoundingBox();

    if( const BOARD* board = GetBoard() )
        bbox.Inflate( 2 * board->GetDesignSettings().GetBiggestClearanceValue() );
    else
        bbox.Inflate( GetWidth() );     // Add a bit extra for safety

    return bbox;
}


std::vector<int> PCB_VIA::ViewGetLayers() const
{
    LAYER_RANGE layers( Padstack().Drill().start, Padstack().Drill().end, MAX_CU_LAYERS );
    std::vector<int> ret_layers{ LAYER_VIA_HOLES, LAYER_VIA_HOLEWALLS, LAYER_VIA_NETNAMES };
    ret_layers.reserve( MAX_CU_LAYERS + 6 );

    // TODO(JE) Rendering order issue
#if 0
    // Blind/buried vias (and microvias) use a different net name layer
    PCB_LAYER_ID layerTop, layerBottom;
    LayerPair( &layerTop, &layerBottom );

    bool isBlindBuried =
            m_viaType == VIATYPE::BLIND_BURIED
            || ( m_viaType == VIATYPE::MICROVIA && ( layerTop != F_Cu || layerBottom != B_Cu ) );
#endif
    LSET cuMask = LSET::AllCuMask();

    if( const BOARD* board = GetBoard() )
        cuMask &= board->GetEnabledLayers();

    for( PCB_LAYER_ID layer : layers )
    {
        if( !cuMask.Contains( layer ) )
            continue;

        ret_layers.push_back( LAYER_VIA_COPPER_START + layer );
        ret_layers.push_back( LAYER_CLEARANCE_START + layer );
    }

    if( IsLocked() )
        ret_layers.push_back( LAYER_LOCKED_ITEM_SHADOW );

    // Vias can also be on a solder mask layer. They are on these layers or not,
    // depending on the plot and solder mask options
    if( IsOnLayer( F_Mask ) )
        ret_layers.push_back( F_Mask );

    if( IsOnLayer( B_Mask ) )
        ret_layers.push_back( B_Mask );

    return ret_layers;
}


double PCB_VIA::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();
    const BOARD*         board = GetBoard();

    // Meta control for hiding all vias
    if( !aView->IsLayerVisible( LAYER_VIAS ) )
        return LOD_HIDE;

    // In high contrast mode don't show vias that don't cross the high-contrast layer
    if( renderSettings->GetHighContrast() )
    {
        PCB_LAYER_ID highContrastLayer = renderSettings->GetPrimaryHighContrastLayer();

        if( LSET::FrontTechMask().Contains( highContrastLayer ) )
            highContrastLayer = F_Cu;
        else if( LSET::BackTechMask().Contains( highContrastLayer ) )
            highContrastLayer = B_Cu;

        if( IsCopperLayer( highContrastLayer ) && GetViaType() != VIATYPE::THROUGH )
        {
            if( IsCopperLayerLowerThan( Padstack().Drill().start, highContrastLayer  )
                || IsCopperLayerLowerThan( highContrastLayer, Padstack().Drill().end ) )
            {
                return LOD_HIDE;
            }
        }
    }

    if( IsHoleLayer( aLayer ) )
    {
        LSET visible;

        if( board )
        {
            visible = board->GetVisibleLayers();
            visible &= board->GetEnabledLayers();
        }
        else
        {
            visible = LSET::AllLayersMask();
        }

        if( m_viaType == VIATYPE::THROUGH )
        {
            // Show a through via's hole if any physical layer is shown
            visible &= LSET::PhysicalLayersMask();

            if( !visible.any() )
                return LOD_HIDE;
        }
        else
        {
            // Show a blind or micro via's hole if it crosses a visible layer
            visible &= GetLayerSet();

            if( !visible.any() )
                return LOD_HIDE;
        }

        // The hole won't be visible anyway at this scale
        return lodScaleForThreshold( aView, GetDrillValue(), pcbIUScale.mmToIU( 0.25 ) );
    }
    else if( IsNetnameLayer( aLayer ) )
    {
        if( renderSettings->GetHighContrast() )
        {
            // Hide netnames unless via is flashed to a high-contrast layer
            if( !FlashLayer( renderSettings->GetPrimaryHighContrastLayer() ) )
                return LOD_HIDE;
        }
        else
        {
            LSET visible;

            if( board )
            {
                visible = board->GetVisibleLayers();
                visible &= board->GetEnabledLayers();
            }
            else
            {
                visible = LSET::AllLayersMask();
            }

            // Hide netnames unless pad is flashed to a visible layer
            if( !FlashLayer( visible ) )
                return LOD_HIDE;
        }

        int width = GetWidth( ToLAYER_ID( aLayer ) );

        // Netnames will be shown only if zoom is appropriate
        return lodScaleForThreshold( aView, width, pcbIUScale.mmToIU( 10 ) );
    }

    if( !IsCopperLayer( aLayer ) )
    {
        int width = GetWidth( ToLAYER_ID( aLayer ) );

        return lodScaleForThreshold( aView, width, pcbIUScale.mmToIU( 0.6 ) );
    }

    return LOD_SHOW;
}


wxString PCB_TRACK::GetFriendlyName() const
{
    switch( Type() )
    {
    case PCB_ARC_T:     return _( "Track (arc)" );
    case PCB_VIA_T:     return _( "Via" );
    case PCB_TRACE_T:
    default:            return _( "Track" );
    }
}


void PCB_TRACK::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString  msg;
    BOARD*    board = GetBoard();

    aList.emplace_back( _( "Type" ), GetFriendlyName() );

    GetMsgPanelInfoBase_Common( aFrame, aList );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( m_width ) );

    if( Type() == PCB_ARC_T )
    {
        double radius = static_cast<PCB_ARC*>( this )->GetRadius();
        aList.emplace_back( _( "Radius" ), aFrame->MessageTextFromValue( radius ) );
    }

    double segmentLength = GetLength();
    double segmentDelay = GetDelay();

    if( segmentDelay == 0.0 )
    {
        aList.emplace_back( _( "Segment Length" ), aFrame->MessageTextFromValue( segmentLength ) );
    }
    else
    {
        aList.emplace_back( _( "Segment Delay" ),
                            aFrame->MessageTextFromValue( segmentDelay, true, EDA_DATA_TYPE::TIME ) );
    }

    // Display full track length (in Pcbnew)
    if( board && GetNetCode() > 0 )
    {
        int    count = 0;
        double trackLen = 0.0;
        double lenPadToDie = 0.0;
        double trackDelay = 0.0;
        double delayPadToDie = 0.0;

        std::tie( count, trackLen, lenPadToDie, trackDelay, delayPadToDie ) = board->GetTrackLength( *this );

        if( trackDelay == 0.0 )
        {
            aList.emplace_back( _( "Routed Length" ), aFrame->MessageTextFromValue( trackLen ) );

            if( lenPadToDie != 0 )
            {
                msg = aFrame->MessageTextFromValue( lenPadToDie );
                aList.emplace_back( _( "Pad To Die Length" ), msg );

                msg = aFrame->MessageTextFromValue( trackLen + lenPadToDie );
                aList.emplace_back( _( "Full Length" ), msg );
            }
        }
        else
        {
            aList.emplace_back( _( "Routed Delay" ),
                                aFrame->MessageTextFromValue( trackDelay, true, EDA_DATA_TYPE::TIME ) );

            if( delayPadToDie != 0.0 )
            {
                msg = aFrame->MessageTextFromValue( delayPadToDie, true, EDA_DATA_TYPE::TIME );
                aList.emplace_back( _( "Pad To Die Delay" ), msg );

                msg = aFrame->MessageTextFromValue( trackDelay + delayPadToDie, true, EDA_DATA_TYPE::TIME );
                aList.emplace_back( _( "Full Delay" ), msg );
            }
        }
    }

    SHAPE_POLY_SET copper;
    TransformShapeToPolySet( copper, GetLayer(), 0, ARC_LOW_DEF, ERROR_INSIDE );
    aList.emplace_back( _( "Copper Area" ),
                        aFrame->MessageTextFromValue( copper.Area(), true, EDA_DATA_TYPE::AREA ) );

    wxString source;
    int clearance = GetOwnClearance( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                          aFrame->MessageTextFromValue( clearance ) ),
                        wxString::Format( _( "(from %s)" ), source ) );

    MINOPTMAX<int> constraintValue = GetWidthConstraint( &source );
    msg = aFrame->MessageTextFromMinOptMax( constraintValue );

    if( !msg.IsEmpty() )
    {
        aList.emplace_back( wxString::Format( _( "Width Constraints: %s" ), msg ),
                            wxString::Format( _( "(from %s)" ), source ) );
    }
}


void PCB_VIA::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString  msg;

    switch( GetViaType() )
    {
    case VIATYPE::MICROVIA:     msg = _( "Micro Via" );        break;
    case VIATYPE::BLIND_BURIED: msg = _( "Blind/Buried Via" ); break;
    case VIATYPE::THROUGH:      msg = _( "Through Via" );      break;
    default:                    msg = _( "Via" );              break;
    }

    aList.emplace_back( _( "Type" ), msg );

    GetMsgPanelInfoBase_Common( aFrame, aList );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );
    // TODO(JE) padstacks
    aList.emplace_back( _( "Diameter" ),
                        aFrame->MessageTextFromValue( GetWidth( PADSTACK::ALL_LAYERS ) ) );
    aList.emplace_back( _( "Hole" ), aFrame->MessageTextFromValue( GetDrillValue() ) );

    wxString  source;
    int clearance = GetOwnClearance( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                          aFrame->MessageTextFromValue( clearance ) ),
                        wxString::Format( _( "(from %s)" ), source ) );

    int minAnnulus = GetMinAnnulus( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Annular Width: %s" ),
                                          aFrame->MessageTextFromValue( minAnnulus ) ),
                        wxString::Format( _( "(from %s)" ), source ) );
}


void PCB_TRACK::GetMsgPanelInfoBase_Common( EDA_DRAW_FRAME* aFrame,
                                            std::vector<MSG_PANEL_ITEM>& aList ) const
{
    aList.emplace_back( _( "Net" ), UnescapeString( GetNetname() ) );

    aList.emplace_back( _( "Resolved Netclass" ),
                        UnescapeString( GetEffectiveNetClass()->GetHumanReadableName() ) );

#if 0   // Enable for debugging
    if( GetBoard() )
        aList.emplace_back( _( "NetCode" ), fmt::format( "{}", GetNetCode() ) );

    aList.emplace_back( wxT( "Flags" ), fmt::format( "#08X", m_flags ) );

    aList.emplace_back( wxT( "Start pos" ), fmt::format( "{} {}", m_Start.x, m_Start.y ) );
    aList.emplace_back( wxT( "End pos" ), fmt::format( "{} {}", m_End.x, m_End.y ) );
#endif

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );
}


wxString PCB_VIA::layerMaskDescribe() const
{
    const BOARD* board = GetBoard();
    PCB_LAYER_ID top_layer;
    PCB_LAYER_ID bottom_layer;

    LayerPair( &top_layer, &bottom_layer );

    return board->GetLayerName( top_layer ) + wxT( " - " ) + board->GetLayerName( bottom_layer );
}


bool PCB_TRACK::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    return TestSegmentHit( aPosition, m_Start, m_End, aAccuracy + ( m_width / 2 ) );
}


bool PCB_ARC::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    double max_dist = aAccuracy + ( GetWidth() / 2.0 );

    // Short-circuit common cases where the arc is connected to a track or via at an endpoint
    if( GetStart().Distance( aPosition ) <= max_dist || GetEnd().Distance( aPosition ) <= max_dist )
    {
        return true;
    }

    VECTOR2L center = GetPosition();
    VECTOR2L relpos = aPosition - center;
    int64_t dist = relpos.EuclideanNorm();
    double radius = GetRadius();

    if( std::abs( dist - radius ) > max_dist )
        return false;

    EDA_ANGLE arc_angle = GetAngle();
    EDA_ANGLE arc_angle_start = GetArcAngleStart();    // Always 0.0 ... 360 deg
    EDA_ANGLE arc_hittest( relpos );

    // Calculate relative angle between the starting point of the arc, and the test point
    arc_hittest -= arc_angle_start;

    // Normalise arc_hittest between 0 ... 360 deg
    arc_hittest.Normalize();

    if( arc_angle < ANGLE_0 )
        return arc_hittest >= ANGLE_360 + arc_angle;

    return arc_hittest <= arc_angle;
}


bool PCB_VIA::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    bool hit = false;

    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                if( hit )
                    return;

                int max_dist = aAccuracy + ( GetWidth( aLayer ) / 2 );

                // rel_pos is aPosition relative to m_Start (or the center of the via)
                VECTOR2D rel_pos = aPosition - m_Start;
                double dist = rel_pos.x * rel_pos.x + rel_pos.y * rel_pos.y;

                if( dist <= static_cast<double>( max_dist ) * max_dist )
                    hit = true;
            } );

    return hit;
}


bool PCB_TRACK::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( GetStart() ) && arect.Contains( GetEnd() );
    else
        return arect.Intersects( GetStart(), GetEnd() );
}


bool PCB_ARC::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    BOX2I box( GetStart() );
    box.Merge( GetMid() );
    box.Merge( GetEnd() );

    box.Inflate( GetWidth() / 2 );

    if( aContained )
        return arect.Contains( box );
    else
        return arect.Intersects( box );
}


bool PCB_VIA::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    bool hit = false;

    Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                if( hit )
                    return;

                BOX2I box( GetStart() );
                box.Inflate( GetWidth( aLayer ) / 2 );

                if( aContained )
                    hit = arect.Contains( box );
                else
                    hit = arect.IntersectsCircle( GetStart(), GetWidth( aLayer ) / 2 );
            } );

    return hit;
}


bool PCB_TRACK::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    return KIGEOM::ShapeHitTest( aPoly, *GetEffectiveShape(), aContained );
}


wxString PCB_TRACK::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( Type() == PCB_ARC_T ? _("Track (arc) %s on %s, length %s" )
                                                 : _("Track %s on %s, length %s" ),
                             GetNetnameMsg(),
                             GetLayerName(),
                             aUnitsProvider->MessageTextFromValue( GetLength() ) );
}


BITMAPS PCB_TRACK::GetMenuImage() const
{
    return BITMAPS::add_tracks;
}

void PCB_TRACK::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TRACE_T );

    std::swap( *((PCB_TRACK*) this), *((PCB_TRACK*) aImage) );
}

void PCB_ARC::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_ARC_T );

    std::swap( *this, *static_cast<PCB_ARC*>( aImage ) );
}

void PCB_VIA::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_VIA_T );

    std::swap( *((PCB_VIA*) this), *((PCB_VIA*) aImage) );
}


VECTOR2I PCB_ARC::GetPosition() const
{
    VECTOR2I center = CalcArcCenter( m_Start, m_Mid, m_End );
    return center;
}


double PCB_ARC::GetRadius() const
{
    auto center = CalcArcCenter( m_Start, m_Mid , m_End );
    return std::min( center.Distance( m_Start ), (double) INT_MAX / 2.0 );
}


EDA_ANGLE PCB_ARC::GetAngle() const
{
    VECTOR2D  center = GetPosition();
    EDA_ANGLE angle1 = EDA_ANGLE( m_Mid - center ) - EDA_ANGLE( m_Start - center );
    EDA_ANGLE angle2 = EDA_ANGLE( m_End - center ) - EDA_ANGLE( m_Mid - center );

    return angle1.Normalize180() + angle2.Normalize180();
}


EDA_ANGLE PCB_ARC::GetArcAngleStart() const
{
    VECTOR2D  pos( GetPosition() );
    EDA_ANGLE angleStart( m_Start - pos );

    return angleStart.Normalize();
}


// Note: used in python tests.  Ignore CLion's claim that it's unused....
EDA_ANGLE PCB_ARC::GetArcAngleEnd() const
{
    VECTOR2D  pos( GetPosition() );
    EDA_ANGLE angleEnd( m_End - pos );

    return angleEnd.Normalize();
}

bool PCB_ARC::IsDegenerated( int aThreshold ) const
{
    // We have lots of code that will blow up if the radius overflows an int.
    if( GetRadius() >= (double)INT_MAX/2.0 )
        return true;

    // Too small arcs cannot be really handled: arc center (and arc radius)
    // cannot be safely computed if the distance between mid and end points
    // is too small (a few internal units)

    // len of both segments must be < aThreshold to be a very small degenerated arc
    return ( GetMid() - GetStart() ).EuclideanNorm() < aThreshold
            && ( GetMid() - GetEnd() ).EuclideanNorm() < aThreshold;
}


bool PCB_TRACK::cmp_tracks::operator() ( const PCB_TRACK* a, const PCB_TRACK* b ) const
{
    if( a->GetNetCode() != b->GetNetCode() )
        return a->GetNetCode() < b->GetNetCode();

    if( a->GetLayer() != b->GetLayer() )
        return a->GetLayer() < b->GetLayer();

    if( a->Type() != b->Type() )
        return a->Type() < b->Type();

    if( a->m_Uuid != b->m_Uuid )
        return a->m_Uuid < b->m_Uuid;

    return a < b;
}


std::shared_ptr<SHAPE> PCB_TRACK::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    int width = m_width;

    if( IsSolderMaskLayer( aLayer ) )
        width += 2 * GetSolderMaskExpansion();

    return std::make_shared<SHAPE_SEGMENT>( m_Start, m_End, width );
}


std::shared_ptr<SHAPE> PCB_VIA::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( aFlash == FLASHING::ALWAYS_FLASHED
            || ( aFlash == FLASHING::DEFAULT && FlashLayer( aLayer ) ) )
    {
        int width = 0;

        if( aLayer == UNDEFINED_LAYER )
        {
            Padstack().ForEachUniqueLayer(
                [&]( PCB_LAYER_ID layer )
                {
                    width = std::max( width, GetWidth( layer ) );
                } );

            width /= 2;
        }
        else
        {
            PCB_LAYER_ID cuLayer = m_padStack.EffectiveLayerFor( aLayer );
            width = GetWidth( cuLayer ) / 2;
        }

        return std::make_shared<SHAPE_CIRCLE>( m_Start, width );
    }
    else
    {
        return std::make_shared<SHAPE_CIRCLE>( m_Start, GetDrillValue() / 2 );
    }
}


std::shared_ptr<SHAPE> PCB_ARC::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    int width = GetWidth();

    if( IsSolderMaskLayer( aLayer ) )
        width += 2 * GetSolderMaskExpansion();

    SHAPE_ARC arc( GetStart(), GetMid(), GetEnd(), width );

    if( arc.IsEffectiveLine() )
        return std::make_shared<SHAPE_SEGMENT>( GetStart(), GetEnd(), width );

    return std::make_shared<SHAPE_ARC>( arc );
}


void PCB_TRACK::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                         int aClearance, int aError, ERROR_LOC aErrorLoc,
                                         bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, wxT( "IgnoreLineWidth has no meaning for tracks." ) );


    switch( Type() )
    {
    case PCB_VIA_T:
    {
        int radius = ( static_cast<const PCB_VIA*>( this )->GetWidth( aLayer ) / 2 ) + aClearance;
        TransformCircleToPolygon( aBuffer, m_Start, radius, aError, aErrorLoc );
        break;
    }

    case PCB_ARC_T:
    {
        const PCB_ARC* arc = static_cast<const PCB_ARC*>( this );
        int            width = m_width + ( 2 * aClearance );

        if( IsSolderMaskLayer( aLayer ) )
            width += 2 * GetSolderMaskExpansion();

        TransformArcToPolygon( aBuffer, arc->GetStart(), arc->GetMid(), arc->GetEnd(), width,
                               aError, aErrorLoc );
        break;
    }

    default:
    {
        int width = m_width + ( 2 * aClearance );

        if( IsSolderMaskLayer( aLayer ) )
            width += 2 * GetSolderMaskExpansion();

        TransformOvalToPolygon( aBuffer, m_Start, m_End, width, aError, aErrorLoc );

        break;
    }
    }
}


static struct TRACK_VIA_DESC
{
    TRACK_VIA_DESC()
    {
        // clang-format off: the suggestion is less readable
        ENUM_MAP<VIATYPE>::Instance()
                .Undefined( VIATYPE::NOT_DEFINED )
                .Map( VIATYPE::THROUGH,      _HKI( "Through" ) )
                .Map( VIATYPE::BLIND_BURIED, _HKI( "Blind/buried" ) )
                .Map( VIATYPE::MICROVIA,     _HKI( "Micro" ) );

        ENUM_MAP<TENTING_MODE>::Instance()
                .Undefined( TENTING_MODE::FROM_RULES )
                .Map( TENTING_MODE::FROM_RULES, _HKI( "From design rules" ) )
                .Map( TENTING_MODE::TENTED,     _HKI( "Tented" ) )
                .Map( TENTING_MODE::NOT_TENTED, _HKI( "Not tented" ) );

        ENUM_MAP<COVERING_MODE>::Instance()
                .Undefined( COVERING_MODE::FROM_RULES )
                .Map( COVERING_MODE::FROM_RULES, _HKI( "From design rules" ) )
                .Map( COVERING_MODE::COVERED,     _HKI( "Covered" ) )
                .Map( COVERING_MODE::NOT_COVERED, _HKI( "Not covered" ) );

        ENUM_MAP<PLUGGING_MODE>::Instance()
                .Undefined( PLUGGING_MODE::FROM_RULES )
                .Map( PLUGGING_MODE::FROM_RULES, _HKI( "From design rules" ) )
                .Map( PLUGGING_MODE::PLUGGED,     _HKI( "Plugged" ) )
                .Map( PLUGGING_MODE::NOT_PLUGGED, _HKI( "Not plugged" ) );

        ENUM_MAP<CAPPING_MODE>::Instance()
                .Undefined( CAPPING_MODE::FROM_RULES )
                .Map( CAPPING_MODE::FROM_RULES, _HKI( "From design rules" ) )
                .Map( CAPPING_MODE::CAPPED,     _HKI( "Capped" ) )
                .Map( CAPPING_MODE::NOT_CAPPED, _HKI( "Not capped" ) );

        ENUM_MAP<FILLING_MODE>::Instance()
                .Undefined( FILLING_MODE::FROM_RULES )
                .Map( FILLING_MODE::FROM_RULES, _HKI( "From design rules" ) )
                .Map( FILLING_MODE::FILLED,     _HKI( "Filled" ) )
                .Map( FILLING_MODE::NOT_FILLED, _HKI( "Not filled" ) );
        // clang-format on: the suggestion is less readable

        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

        // Track
        REGISTER_TYPE( PCB_TRACK );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TRACK ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        propMgr.AddProperty( new PROPERTY<PCB_TRACK, int>( _HKI( "Width" ),
            &PCB_TRACK::SetWidth, &PCB_TRACK::GetWidth, PROPERTY_DISPLAY::PT_SIZE ) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ),
            new PROPERTY<PCB_TRACK, int>( _HKI( "Start X" ),
            &PCB_TRACK::SetStartX, &PCB_TRACK::GetStartX, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_X_COORD) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ),
            new PROPERTY<PCB_TRACK, int>( _HKI( "Start Y" ),
            &PCB_TRACK::SetStartY, &PCB_TRACK::GetStartY, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_Y_COORD ) );
        propMgr.AddProperty( new PROPERTY<PCB_TRACK, int>( _HKI( "End X" ),
            &PCB_TRACK::SetEndX, &PCB_TRACK::GetEndX, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_X_COORD) );
        propMgr.AddProperty( new PROPERTY<PCB_TRACK, int>( _HKI( "End Y" ),
            &PCB_TRACK::SetEndY, &PCB_TRACK::GetEndY, PROPERTY_DISPLAY::PT_COORD,
            ORIGIN_TRANSFORMS::ABS_Y_COORD) );

        const wxString groupTechLayers = _HKI( "Technical Layers" );

        auto isExternalLayerTrack =
            []( INSPECTABLE* aItem )
            {
                if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( aItem ) )
                    return IsExternalCopperLayer( track->GetLayer() );

                return false;
            };

        propMgr.AddProperty( new PROPERTY<PCB_TRACK, bool>( _HKI( "Soldermask" ),
            &PCB_TRACK::SetHasSolderMask, &PCB_TRACK::HasSolderMask ), groupTechLayers )
            .SetAvailableFunc( isExternalLayerTrack );
        propMgr.AddProperty( new PROPERTY<PCB_TRACK, std::optional<int>>( _HKI( "Soldermask Margin Override" ),
            &PCB_TRACK::SetLocalSolderMaskMargin, &PCB_TRACK::GetLocalSolderMaskMargin,
            PROPERTY_DISPLAY::PT_SIZE ), groupTechLayers )
            .SetAvailableFunc( isExternalLayerTrack );

        // Arc
        REGISTER_TYPE( PCB_ARC );
        propMgr.InheritsAfter( TYPE_HASH( PCB_ARC ), TYPE_HASH( PCB_TRACK ) );

        // Via
        REGISTER_TYPE( PCB_VIA );
        propMgr.InheritsAfter( TYPE_HASH( PCB_VIA ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        // TODO test drill, use getdrillvalue?
        const wxString groupVia = _HKI( "Via Properties" );

        propMgr.Mask( TYPE_HASH( PCB_VIA ), TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ) );

        // clang-format off: the suggestion is less readable
        propMgr.AddProperty( new PROPERTY<PCB_VIA, int>( _HKI( "Diameter" ),
            &PCB_VIA::SetFrontWidth, &PCB_VIA::GetFrontWidth, PROPERTY_DISPLAY::PT_SIZE ), groupVia );
        propMgr.AddProperty( new PROPERTY<PCB_VIA, int>( _HKI( "Hole" ),
            &PCB_VIA::SetDrill, &PCB_VIA::GetDrillValue, PROPERTY_DISPLAY::PT_SIZE ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, PCB_LAYER_ID>( _HKI( "Layer Top" ),
            &PCB_VIA::SetTopLayer, &PCB_VIA::GetLayer ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, PCB_LAYER_ID>( _HKI( "Layer Bottom" ),
            &PCB_VIA::SetBottomLayer, &PCB_VIA::BottomLayer ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, VIATYPE>( _HKI( "Via Type" ),
            &PCB_VIA::SetViaType, &PCB_VIA::GetViaType ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, TENTING_MODE>( _HKI( "Front tenting" ),
            &PCB_VIA::SetFrontTentingMode, &PCB_VIA::GetFrontTentingMode ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, TENTING_MODE>( _HKI( "Back tenting" ),
            &PCB_VIA::SetBackTentingMode, &PCB_VIA::GetBackTentingMode ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, COVERING_MODE>( _HKI( "Front covering" ),
            &PCB_VIA::SetFrontCoveringMode, &PCB_VIA::GetFrontCoveringMode ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, COVERING_MODE>( _HKI( "Back covering" ),
            &PCB_VIA::SetBackCoveringMode, &PCB_VIA::GetBackCoveringMode ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, PLUGGING_MODE>( _HKI( "Front plugging" ),
            &PCB_VIA::SetFrontPluggingMode, &PCB_VIA::GetFrontPluggingMode ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, PLUGGING_MODE>( _HKI( "Back plugging" ),
            &PCB_VIA::SetBackPluggingMode, &PCB_VIA::GetBackPluggingMode ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, CAPPING_MODE>( _HKI( "Capping" ),
            &PCB_VIA::SetCappingMode, &PCB_VIA::GetCappingMode ), groupVia );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_VIA, FILLING_MODE>( _HKI( "Filling" ),
            &PCB_VIA::SetFillingMode, &PCB_VIA::GetFillingMode ), groupVia );
        // clang-format on: the suggestion is less readable
    }
} _TRACK_VIA_DESC;

ENUM_TO_WXANY( VIATYPE );
ENUM_TO_WXANY( TENTING_MODE );
ENUM_TO_WXANY( COVERING_MODE );
ENUM_TO_WXANY( PLUGGING_MODE );
ENUM_TO_WXANY( CAPPING_MODE );
ENUM_TO_WXANY( FILLING_MODE );
