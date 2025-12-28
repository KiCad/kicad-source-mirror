/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <convert_basic_shapes_to_polygon.h> // RECT_CHAMFER_POSITIONS
#include "padstack.h"
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/api_pcb_utils.h>
#include <api/board/board_types.pb.h>
#include <layer_range.h>
#include <macros.h>
#include <magic_enum.hpp>
#include <pad.h>
#include <board.h>
#include <pcb_shape.h>


IMPLEMENT_ENUM_TO_WXANY( PAD_DRILL_POST_MACHINING_MODE );
IMPLEMENT_ENUM_TO_WXANY( BACKDRILL_MODE );
IMPLEMENT_ENUM_TO_WXANY( UNCONNECTED_LAYER_MODE );


PADSTACK::PADSTACK( BOARD_ITEM* aParent ) :
        m_parent( aParent ),
        m_mode( MODE::NORMAL ),
        m_orientation( ANGLE_0 ),
        m_unconnectedLayerMode( UNCONNECTED_LAYER_MODE::KEEP_ALL ),
        m_customShapeInZoneMode( CUSTOM_SHAPE_ZONE_MODE::OUTLINE )
{
    m_copperProps[PADSTACK::ALL_LAYERS].shape = SHAPE_PROPS();
    m_copperProps[PADSTACK::ALL_LAYERS].zone_connection = ZONE_CONNECTION::INHERITED;
    m_copperProps[PADSTACK::ALL_LAYERS].thermal_spoke_width = std::nullopt;
    m_copperProps[PADSTACK::ALL_LAYERS].thermal_spoke_angle = ANGLE_45;
    m_copperProps[PADSTACK::ALL_LAYERS].thermal_gap = std::nullopt;

    m_drill.shape = PAD_DRILL_SHAPE::CIRCLE;
    m_drill.start = F_Cu;
    m_drill.end   = B_Cu;

    m_secondaryDrill.shape = PAD_DRILL_SHAPE::UNDEFINED;
    m_secondaryDrill.start = UNDEFINED_LAYER;
    m_secondaryDrill.end   = UNDEFINED_LAYER;

    m_tertiaryDrill.shape = PAD_DRILL_SHAPE::UNDEFINED;
    m_tertiaryDrill.start = UNDEFINED_LAYER;
    m_tertiaryDrill.end   = UNDEFINED_LAYER;
}


PADSTACK::PADSTACK( const PADSTACK& aOther )
{
    m_parent = aOther.m_parent;
    *this = aOther;

    ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            for( std::shared_ptr<PCB_SHAPE>& shape : CopperLayer( aLayer ).custom_shapes )
                shape->SetParent( m_parent );
        } );
}


PADSTACK& PADSTACK::operator=( const PADSTACK &aOther )
{
    // NOTE: m_parent is not copied from operator=, because this operator is commonly used to
    // update the padstack properties, and such an update must not change the parent PAD to point to
    // the parent of some different padstack.

    m_mode                  = aOther.m_mode;
    m_layerSet              = aOther.m_layerSet;
    m_customName            = aOther.m_customName;
    m_orientation           = aOther.m_orientation;
    m_copperProps           = aOther.m_copperProps;
    m_frontMaskProps        = aOther.m_frontMaskProps;
    m_backMaskProps         = aOther.m_backMaskProps;
    m_unconnectedLayerMode  = aOther.m_unconnectedLayerMode;
    m_customShapeInZoneMode = aOther.m_customShapeInZoneMode;
    m_drill                 = aOther.m_drill;
    m_secondaryDrill        = aOther.m_secondaryDrill;
    m_tertiaryDrill         = aOther.m_tertiaryDrill;
    m_frontPostMachining    = aOther.m_frontPostMachining;
    m_backPostMachining     = aOther.m_backPostMachining;

    // Data consistency enforcement logic that used to live in the pad properties dialog
    // TODO(JE) Should these move to individual property setters, so that they are always
    // enforced even through the properties panel and API?

    ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            PAD_SHAPE shape = Shape( aLayer );

            // Make sure leftover primitives don't stick around
            ClearPrimitives( aLayer );

            // For custom pad shape, duplicate primitives of the pad to copy
            if( shape == PAD_SHAPE::CUSTOM )
                ReplacePrimitives( aOther.Primitives( aLayer ), aLayer );

            // rounded rect pads with radius ratio = 0 are in fact rect pads.
            // So set the right shape (and perhaps issues with a radius = 0)
            if( shape == PAD_SHAPE::ROUNDRECT && RoundRectRadiusRatio( aLayer ) == 0.0 )
                SetShape( PAD_SHAPE::RECTANGLE, aLayer );
        } );

    return *this;
}


bool PADSTACK::operator==( const PADSTACK& aOther ) const
{
    if( m_mode != aOther.m_mode )
        return false;

    if( m_layerSet != aOther.m_layerSet )
        return false;

    if(  m_customName != aOther.m_customName )
        return false;

    if(  m_orientation != aOther.m_orientation )
        return false;

    if(  m_frontMaskProps != aOther.m_frontMaskProps )
        return false;

    if(  m_backMaskProps != aOther.m_backMaskProps )
        return false;

    if(  m_unconnectedLayerMode != aOther.m_unconnectedLayerMode )
        return false;

    if(  m_customShapeInZoneMode != aOther.m_customShapeInZoneMode )
        return false;

    if(  m_drill != aOther.m_drill )
        return false;

    if(  m_secondaryDrill != aOther.m_secondaryDrill )
        return false;

    if(  m_tertiaryDrill != aOther.m_tertiaryDrill )
        return false;

    if( m_frontPostMachining != aOther.m_frontPostMachining )
        return false;

    if( m_backPostMachining != aOther.m_backPostMachining )
        return false;

    bool copperMatches = true;

    ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                if( CopperLayer( aLayer ) != aOther.CopperLayer( aLayer ) )
                    copperMatches = false;
            } );

    return copperMatches;
}


bool PADSTACK::unpackCopperLayer( const kiapi::board::types::PadStackLayer& aProto )
{
    using namespace kiapi::board::types;
    PCB_LAYER_ID layer = FromProtoEnum<PCB_LAYER_ID, BoardLayer>( aProto.layer() );

    if( m_mode == MODE::NORMAL && layer != ALL_LAYERS )
        return false;

    if( m_mode == MODE::FRONT_INNER_BACK && layer != F_Cu && layer != INNER_LAYERS && layer != B_Cu )
        return false;

    SetSize( kiapi::common::UnpackVector2( aProto.size() ), layer );
    SetShape( FromProtoEnum<PAD_SHAPE>( aProto.shape() ), layer );
    Offset( layer ) = kiapi::common::UnpackVector2( aProto.offset() );
    SetAnchorShape( FromProtoEnum<PAD_SHAPE>( aProto.custom_anchor_shape() ), layer );

    SHAPE_PROPS& props = CopperLayer( layer ).shape;
    props.chamfered_rect_ratio = aProto.chamfer_ratio();
    props.round_rect_radius_ratio = aProto.corner_rounding_ratio();

    if( Shape( layer ) == PAD_SHAPE::TRAPEZOID && aProto.has_trapezoid_delta() )
        TrapezoidDeltaSize( layer ) = kiapi::common::UnpackVector2( aProto.trapezoid_delta() );

    if( aProto.chamfered_corners().top_left() )
        props.chamfered_rect_positions |= RECT_CHAMFER_TOP_LEFT;

    if( aProto.chamfered_corners().top_right() )
        props.chamfered_rect_positions |= RECT_CHAMFER_TOP_RIGHT;

    if( aProto.chamfered_corners().bottom_left() )
        props.chamfered_rect_positions |= RECT_CHAMFER_BOTTOM_LEFT;

    if( aProto.chamfered_corners().bottom_right() )
        props.chamfered_rect_positions |= RECT_CHAMFER_BOTTOM_RIGHT;

    ClearPrimitives( layer );
    google::protobuf::Any a;

    for( const BoardGraphicShape& shapeProto : aProto.custom_shapes() )
    {
        a.PackFrom( shapeProto );
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_parent );

        if( shape->Deserialize( a ) )
            AddPrimitive( shape.release(), layer );
    }

    return true;
}


bool PADSTACK::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board::types;
    PadStack padstack;

    auto unpackOptional = []<typename ProtoEnum>( const ProtoEnum&     aProto,
                                                  std::optional<bool>& aDest, ProtoEnum aTrueValue,
                                                  ProtoEnum aFalseValue )
    {
        if( aProto == aTrueValue )
            aDest = true;
        else if( aProto == aFalseValue )
            aDest = false;
        else
            aDest = std::nullopt;
    };

    auto unpackPostMachining = []( const PostMachiningProperties& aProto,
                                   PADSTACK::POST_MACHINING_PROPS& aDest )
    {
        switch( aProto.mode() )
        {
        case VDPM_NOT_POST_MACHINED: aDest.mode = PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED; break;
        case VDPM_COUNTERBORE: aDest.mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE; break;
        case VDPM_COUNTERSINK: aDest.mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK; break;
        default: aDest.mode = std::nullopt; break;
        }

        aDest.size = aProto.size();
        aDest.depth = aProto.depth();
        aDest.angle = aProto.angle();
    };

    if( !aContainer.UnpackTo( &padstack ) )
        return false;

    m_mode = FromProtoEnum<MODE>( padstack.type() );
    SetLayerSet( kiapi::board::UnpackLayerSet( padstack.layers() ) );
    m_orientation = EDA_ANGLE( padstack.angle().value_degrees(), DEGREES_T );

    Drill().size = kiapi::common::UnpackVector2( padstack.drill().diameter() );
    Drill().start = FromProtoEnum<PCB_LAYER_ID>( padstack.drill().start_layer() );
    Drill().end = FromProtoEnum<PCB_LAYER_ID>( padstack.drill().end_layer() );
    unpackOptional( padstack.drill().capped(), Drill().is_capped, VDCM_CAPPED, VDCM_UNCAPPED );
    unpackOptional( padstack.drill().filled(), Drill().is_filled, VDFM_FILLED, VDFM_UNFILLED );

    if( padstack.has_front_post_machining() )
        unpackPostMachining( padstack.front_post_machining(), FrontPostMachining() );

    if( padstack.has_back_post_machining() )
        unpackPostMachining( padstack.back_post_machining(), BackPostMachining() );

    Drill().shape = FromProtoEnum<PAD_DRILL_SHAPE>( padstack.drill().shape() );

    if( padstack.has_secondary_drill() )
    {
        const DrillProperties& secondary = padstack.secondary_drill();

        SecondaryDrill().size = kiapi::common::UnpackVector2( secondary.diameter() );
        SecondaryDrill().start = FromProtoEnum<PCB_LAYER_ID>( secondary.start_layer() );
        SecondaryDrill().end = FromProtoEnum<PCB_LAYER_ID>( secondary.end_layer() );
        SecondaryDrill().shape = FromProtoEnum<PAD_DRILL_SHAPE>( secondary.shape() );

        unpackOptional( secondary.capped(), SecondaryDrill().is_capped, VDCM_CAPPED, VDCM_UNCAPPED );
        unpackOptional( secondary.filled(), SecondaryDrill().is_filled, VDFM_FILLED, VDFM_UNFILLED );
    }
    else
    {
        SecondaryDrill().size = { 0, 0 };
        SecondaryDrill().shape = PAD_DRILL_SHAPE::UNDEFINED;
        SecondaryDrill().start = UNDEFINED_LAYER;
        SecondaryDrill().end = UNDEFINED_LAYER;
        SecondaryDrill().is_capped = std::nullopt;
        SecondaryDrill().is_filled = std::nullopt;
    }

    if( padstack.has_tertiary_drill() )
    {
        const DrillProperties& tertiary = padstack.tertiary_drill();

        TertiaryDrill().size = kiapi::common::UnpackVector2( tertiary.diameter() );
        TertiaryDrill().start = FromProtoEnum<PCB_LAYER_ID>( tertiary.start_layer() );
        TertiaryDrill().end = FromProtoEnum<PCB_LAYER_ID>( tertiary.end_layer() );
        TertiaryDrill().shape = FromProtoEnum<PAD_DRILL_SHAPE>( tertiary.shape() );

        unpackOptional( tertiary.capped(), TertiaryDrill().is_capped, VDCM_CAPPED, VDCM_UNCAPPED );
        unpackOptional( tertiary.filled(), TertiaryDrill().is_filled, VDFM_FILLED, VDFM_UNFILLED );
    }
    else
    {
        TertiaryDrill().size = { 0, 0 };
        TertiaryDrill().shape = PAD_DRILL_SHAPE::UNDEFINED;
        TertiaryDrill().start = UNDEFINED_LAYER;
        TertiaryDrill().end = UNDEFINED_LAYER;
        TertiaryDrill().is_capped = std::nullopt;
        TertiaryDrill().is_filled = std::nullopt;
    }

    for( const PadStackLayer& layer : padstack.copper_layers() )
    {
        if( !unpackCopperLayer( layer ) )
            return false;
    }

    CopperLayer( ALL_LAYERS ).thermal_gap = std::nullopt;
    CopperLayer( ALL_LAYERS ).thermal_spoke_width = std::nullopt;

    if( padstack.has_zone_settings() )
    {
        CopperLayer( ALL_LAYERS ).zone_connection =
                FromProtoEnum<ZONE_CONNECTION>( padstack.zone_settings().zone_connection() );

        if( padstack.zone_settings().has_thermal_spokes() )
        {
            const ThermalSpokeSettings& thermals = padstack.zone_settings().thermal_spokes();

            if( thermals.has_gap() )
                CopperLayer( ALL_LAYERS ).thermal_gap = thermals.gap().value_nm();

            if( thermals.has_width() )
                CopperLayer( ALL_LAYERS ).thermal_spoke_width = thermals.width().value_nm();

            SetThermalSpokeAngle( EDA_ANGLE( thermals.angle().value_degrees(), DEGREES_T ), F_Cu );
        }
    }
    else
    {
        CopperLayer( ALL_LAYERS ).zone_connection = ZONE_CONNECTION::INHERITED;
        CopperLayer( ALL_LAYERS ).thermal_spoke_angle = DefaultThermalSpokeAngleForShape( F_Cu );
    }

    SetUnconnectedLayerMode(
        FromProtoEnum<UNCONNECTED_LAYER_MODE>( padstack.unconnected_layer_removal() ) );

    unpackOptional( padstack.front_outer_layers().solder_mask_mode(),
                    FrontOuterLayers().has_solder_mask, SMM_MASKED, SMM_UNMASKED );

    unpackOptional( padstack.back_outer_layers().solder_mask_mode(),
                    BackOuterLayers().has_solder_mask, SMM_MASKED, SMM_UNMASKED );

    unpackOptional( padstack.front_outer_layers().covering_mode(), FrontOuterLayers().has_covering,
                    VCM_COVERED, VCM_UNCOVERED );

    unpackOptional( padstack.back_outer_layers().covering_mode(), BackOuterLayers().has_covering,
                    VCM_COVERED, VCM_UNCOVERED );

    unpackOptional( padstack.front_outer_layers().plugging_mode(), FrontOuterLayers().has_plugging,
                    VPM_PLUGGED, VPM_UNPLUGGED );

    unpackOptional( padstack.back_outer_layers().plugging_mode(), BackOuterLayers().has_plugging,
                    VPM_PLUGGED, VPM_UNPLUGGED );

    unpackOptional( padstack.front_outer_layers().solder_paste_mode(),
                    FrontOuterLayers().has_solder_paste, SPM_PASTE, SPM_NO_PASTE );

    unpackOptional( padstack.back_outer_layers().solder_paste_mode(),
                    BackOuterLayers().has_solder_paste, SPM_PASTE, SPM_NO_PASTE );

    if( padstack.front_outer_layers().has_solder_mask_settings()
        && padstack.front_outer_layers().solder_mask_settings().has_solder_mask_margin() )
    {
        FrontOuterLayers().solder_mask_margin =
            padstack.front_outer_layers().solder_mask_settings().solder_mask_margin().value_nm();
    }
    else
    {
        FrontOuterLayers().solder_mask_margin = std::nullopt;
    }

    if( padstack.back_outer_layers().has_solder_mask_settings()
        && padstack.back_outer_layers().solder_mask_settings().has_solder_mask_margin() )
    {
        BackOuterLayers().solder_mask_margin =
                padstack.back_outer_layers().solder_mask_settings().solder_mask_margin().value_nm();
    }
    else
    {
        BackOuterLayers().solder_mask_margin = std::nullopt;
    }

    if( padstack.front_outer_layers().has_solder_paste_settings()
        && padstack.front_outer_layers().solder_paste_settings().has_solder_paste_margin() )
    {
        FrontOuterLayers().solder_paste_margin =
            padstack.front_outer_layers().solder_paste_settings().solder_paste_margin().value_nm();
    }
    else
    {
        FrontOuterLayers().solder_paste_margin = std::nullopt;
    }

    if( padstack.back_outer_layers().has_solder_paste_settings()
        && padstack.back_outer_layers().solder_paste_settings().has_solder_paste_margin() )
    {
        BackOuterLayers().solder_paste_margin =
            padstack.back_outer_layers().solder_paste_settings().solder_paste_margin().value_nm();
    }
    else
    {
        BackOuterLayers().solder_paste_margin = std::nullopt;
    }

    if( padstack.front_outer_layers().has_solder_paste_settings()
        && padstack.front_outer_layers().solder_paste_settings().has_solder_paste_margin_ratio() )
    {
        FrontOuterLayers().solder_paste_margin_ratio =
            padstack.front_outer_layers().solder_paste_settings().solder_paste_margin_ratio().value();
    }
    else
    {
        FrontOuterLayers().solder_paste_margin_ratio = std::nullopt;
    }

    if( padstack.back_outer_layers().has_solder_paste_settings()
        && padstack.back_outer_layers().solder_paste_settings().has_solder_paste_margin_ratio() )
    {
        BackOuterLayers().solder_paste_margin_ratio =
            padstack.back_outer_layers().solder_paste_settings().solder_paste_margin_ratio().value();
    }
    else
    {
        BackOuterLayers().solder_paste_margin_ratio = std::nullopt;
    }


    return true;
}


BACKDRILL_MODE PADSTACK::GetBackdrillMode() const
{
    bool hasSecondary = m_secondaryDrill.size.x > 0;
    bool hasTertiary = m_tertiaryDrill.size.x > 0;

    if( !hasSecondary && !hasTertiary )
        return BACKDRILL_MODE::NO_BACKDRILL;

    if( hasSecondary && hasTertiary )
        return BACKDRILL_MODE::BACKDRILL_BOTH;

    if( hasSecondary )
    {
        if( m_secondaryDrill.start == F_Cu )
            return BACKDRILL_MODE::BACKDRILL_TOP;

        return BACKDRILL_MODE::BACKDRILL_BOTTOM;
    }

    if( hasTertiary )
    {
        if( m_tertiaryDrill.start == B_Cu )
            return BACKDRILL_MODE::BACKDRILL_BOTTOM;

        return BACKDRILL_MODE::BACKDRILL_TOP;
    }

    return BACKDRILL_MODE::NO_BACKDRILL;
}


void PADSTACK::SetBackdrillMode( BACKDRILL_MODE aMode )
{
    auto initDrill = [this]( DRILL_PROPS& aDrill, PCB_LAYER_ID aStart )
    {
        if( aDrill.size.x <= 0 )
        {
            aDrill.size = m_drill.size * 1.1; // Backdrill slightly larger than main drill
            aDrill.shape = PAD_DRILL_SHAPE::CIRCLE;
            aDrill.start = aStart;
            aDrill.end = aStart;
        }
        else
        {
            aDrill.start = aStart;
        }
    };
    if( aMode == BACKDRILL_MODE::NO_BACKDRILL || aMode == BACKDRILL_MODE::BACKDRILL_BOTTOM )
    {
        m_tertiaryDrill.size = { 0, 0 };
    }

    if( aMode == BACKDRILL_MODE::NO_BACKDRILL || aMode == BACKDRILL_MODE::BACKDRILL_TOP )
    {
        m_secondaryDrill.size = { 0, 0 };
    }

    if( aMode == BACKDRILL_MODE::BACKDRILL_BOTTOM || aMode == BACKDRILL_MODE::BACKDRILL_BOTH )
    {
        m_secondaryDrill.start = B_Cu;

        if( m_secondaryDrill.size.x > 0 ) { /* ok */ }
        else initDrill( m_secondaryDrill, B_Cu );
    }

    if( aMode == BACKDRILL_MODE::BACKDRILL_TOP || aMode == BACKDRILL_MODE::BACKDRILL_BOTH )
    {
        m_tertiaryDrill.start = F_Cu;

        if( m_tertiaryDrill.size.x > 0 ) { /* ok */ }
        else initDrill( m_tertiaryDrill, F_Cu );
    }
}


std::optional<int> PADSTACK::GetBackdrillSize( bool aTop ) const
{
    if( m_secondaryDrill.size.x > 0 )
    {
        if( aTop && m_secondaryDrill.start == F_Cu ) return m_secondaryDrill.size.x;
        if( !aTop && m_secondaryDrill.start == B_Cu ) return m_secondaryDrill.size.x;
    }
    if( m_tertiaryDrill.size.x > 0 )
    {
        if( aTop && m_tertiaryDrill.start == F_Cu ) return m_tertiaryDrill.size.x;
        if( !aTop && m_tertiaryDrill.start == B_Cu ) return m_tertiaryDrill.size.x;
    }
    return std::nullopt;
}


void PADSTACK::SetBackdrillSize( bool aTop, std::optional<int> aSize )
{
    DRILL_PROPS* target = nullptr;

    if( aTop )
    {
        if( m_secondaryDrill.start == F_Cu ) target = &m_secondaryDrill;
        else if( m_tertiaryDrill.start == F_Cu ) target = &m_tertiaryDrill;
    }
    else
    {
        if( m_secondaryDrill.start == B_Cu ) target = &m_secondaryDrill;
        else if( m_tertiaryDrill.start == B_Cu ) target = &m_tertiaryDrill;
    }

    if( !target )
    {
        if( m_secondaryDrill.size.x <= 0 ) target = &m_secondaryDrill;
        else if( m_tertiaryDrill.size.x <= 0 ) target = &m_tertiaryDrill;
    }

    if( !target )
    {
        if( aTop ) target = &m_tertiaryDrill;
        else target = &m_secondaryDrill;
    }

    if( aSize.has_value() )
    {
        target->size = { *aSize, *aSize };
        target->shape = PAD_DRILL_SHAPE::CIRCLE;
        target->start = aTop ? F_Cu : B_Cu;
        if( target->end == UNDEFINED_LAYER ) target->end = UNDEFINED_LAYER;
    }
    else
    {
        target->size = { 0, 0 };
    }
}


PCB_LAYER_ID PADSTACK::GetBackdrillEndLayer( bool aTop ) const
{
    if( m_secondaryDrill.size.x > 0 )
    {
        if( aTop && m_secondaryDrill.start == F_Cu ) return m_secondaryDrill.end;
        if( !aTop && m_secondaryDrill.start == B_Cu ) return m_secondaryDrill.end;
    }
    if( m_tertiaryDrill.size.x > 0 )
    {
        if( aTop && m_tertiaryDrill.start == F_Cu ) return m_tertiaryDrill.end;
        if( !aTop && m_tertiaryDrill.start == B_Cu ) return m_tertiaryDrill.end;
    }
    return UNDEFINED_LAYER;
}


void PADSTACK::SetBackdrillEndLayer( bool aTop, PCB_LAYER_ID aLayer )
{
    DRILL_PROPS* target = nullptr;

    if( aTop )
    {
        if( m_secondaryDrill.start == F_Cu ) target = &m_secondaryDrill;
        else if( m_tertiaryDrill.start == F_Cu ) target = &m_tertiaryDrill;
    }
    else
    {
        if( m_secondaryDrill.start == B_Cu ) target = &m_secondaryDrill;
        else if( m_tertiaryDrill.start == B_Cu ) target = &m_tertiaryDrill;
    }

    if( target )
    {
        target->end = aLayer;
    }
}

void PADSTACK::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board::types;
    PadStack padstack;

    padstack.set_type( ToProtoEnum<MODE, PadStackType>( m_mode ) );
    kiapi::board::PackLayerSet( *padstack.mutable_layers(), m_layerSet );
    padstack.mutable_angle()->set_value_degrees( m_orientation.AsDegrees() );

    kiapi::common::PackVector2( *padstack.mutable_drill()->mutable_diameter(), m_drill.size );
    padstack.mutable_drill()->set_start_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( m_drill.start ) );
    padstack.mutable_drill()->set_end_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( m_drill.end ) );
    padstack.mutable_drill()->set_shape( ToProtoEnum<PAD_DRILL_SHAPE, kiapi::board::types::DrillShape>( m_drill.shape ) );

    if( m_drill.is_capped.has_value() )
        padstack.mutable_drill()->set_capped( m_drill.is_capped.value() ? VDCM_CAPPED : VDCM_UNCAPPED );

    if( m_drill.is_filled.has_value() )
        padstack.mutable_drill()->set_filled( m_drill.is_filled.value() ? VDFM_FILLED : VDFM_UNFILLED );

    if( m_secondaryDrill.size.x > 0 )
    {
        DrillProperties* secondary = padstack.mutable_secondary_drill();
        kiapi::common::PackVector2( *secondary->mutable_diameter(), m_secondaryDrill.size );
        secondary->set_start_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( m_secondaryDrill.start ) );
        secondary->set_end_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( m_secondaryDrill.end ) );
        secondary->set_shape( ToProtoEnum<PAD_DRILL_SHAPE, kiapi::board::types::DrillShape>( m_secondaryDrill.shape ) );

        if( m_secondaryDrill.is_capped.has_value() )
            secondary->set_capped( m_secondaryDrill.is_capped.value() ? VDCM_CAPPED : VDCM_UNCAPPED );

        if( m_secondaryDrill.is_filled.has_value() )
            secondary->set_filled( m_secondaryDrill.is_filled.value() ? VDFM_FILLED : VDFM_UNFILLED );
    }

    if( m_tertiaryDrill.size.x > 0 )
    {
        DrillProperties* tertiary = padstack.mutable_tertiary_drill();
        kiapi::common::PackVector2( *tertiary->mutable_diameter(), m_tertiaryDrill.size );
        tertiary->set_start_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( m_tertiaryDrill.start ) );
        tertiary->set_end_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( m_tertiaryDrill.end ) );
        tertiary->set_shape( ToProtoEnum<PAD_DRILL_SHAPE, kiapi::board::types::DrillShape>( m_tertiaryDrill.shape ) );

        if( m_tertiaryDrill.is_capped.has_value() )
            tertiary->set_capped( m_tertiaryDrill.is_capped.value() ? VDCM_CAPPED : VDCM_UNCAPPED );

        if( m_tertiaryDrill.is_filled.has_value() )
            tertiary->set_filled( m_tertiaryDrill.is_filled.value() ? VDFM_FILLED : VDFM_UNFILLED );
    }

    auto packPostMachining = []( const PADSTACK::POST_MACHINING_PROPS& aProps,
                                 PostMachiningProperties* aProto )
    {
        if( aProps.mode.has_value() )
        {
            switch( aProps.mode.value() )
            {
            case PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED: aProto->set_mode( VDPM_NOT_POST_MACHINED ); break;
            case PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE:       aProto->set_mode( VDPM_COUNTERBORE );       break;
            case PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK:       aProto->set_mode( VDPM_COUNTERSINK );       break;
            default:                                                                                           break;
            }
        }

        aProto->set_size( aProps.size );
        aProto->set_depth( aProps.depth );
        aProto->set_angle( aProps.angle );
    };

    if( m_frontPostMachining.mode.has_value() || m_frontPostMachining.size > 0 )
        packPostMachining( m_frontPostMachining, padstack.mutable_front_post_machining() );

    if( m_backPostMachining.mode.has_value() || m_backPostMachining.size > 0 )
        packPostMachining( m_backPostMachining, padstack.mutable_back_post_machining() );

    ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                PadStackLayer* layer = padstack.add_copper_layers();
                const COPPER_LAYER_PROPS& props = CopperLayer( aLayer );

                layer->set_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( aLayer ) );
                kiapi::common::PackVector2( *layer->mutable_size(), props.shape.size );
                layer->set_shape( ToProtoEnum<PAD_SHAPE, kiapi::board::types::PadStackShape>( props.shape.shape ) );
                kiapi::common::PackVector2( *layer->mutable_offset(), props.shape.offset );
                layer->set_custom_anchor_shape( ToProtoEnum<PAD_SHAPE, kiapi::board::types::PadStackShape>( props.shape.anchor_shape ) );
                layer->set_chamfer_ratio( props.shape.chamfered_rect_ratio );
                layer->set_corner_rounding_ratio( props.shape.round_rect_radius_ratio );

                if( props.shape.shape == PAD_SHAPE::TRAPEZOID )
                    kiapi::common::PackVector2( *layer->mutable_trapezoid_delta(), props.shape.trapezoid_delta_size );

                if( props.shape.chamfered_rect_positions & RECT_CHAMFER_TOP_LEFT )
                    layer->mutable_chamfered_corners()->set_top_left( true );

                if( props.shape.chamfered_rect_positions & RECT_CHAMFER_TOP_RIGHT )
                    layer->mutable_chamfered_corners()->set_top_right( true );

                if( props.shape.chamfered_rect_positions & RECT_CHAMFER_BOTTOM_LEFT )
                    layer->mutable_chamfered_corners()->set_bottom_left( true );

                if( props.shape.chamfered_rect_positions & RECT_CHAMFER_BOTTOM_RIGHT )
                    layer->mutable_chamfered_corners()->set_bottom_right( true );

                for( const std::shared_ptr<PCB_SHAPE>& shape : props.custom_shapes )
                {
                    google::protobuf::Any a;
                    shape->Serialize( a );
                    a.UnpackTo( layer->add_custom_shapes() );
                }
            } );

    if( CopperLayer( ALL_LAYERS ).zone_connection.has_value() )
    {
        padstack.mutable_zone_settings()->set_zone_connection(
                ToProtoEnum<ZONE_CONNECTION, kiapi::board::types::ZoneConnectionStyle>( CopperLayer( ALL_LAYERS ).zone_connection.value() ) );
    }

    if( CopperLayer( ALL_LAYERS ).thermal_gap.has_value() )
    {
        padstack.mutable_zone_settings()->mutable_thermal_spokes()->mutable_gap()->set_value_nm(
                CopperLayer( ALL_LAYERS ).thermal_gap.value() );
    }

    if( CopperLayer( ALL_LAYERS ).thermal_spoke_width.has_value() )
    {
        padstack.mutable_zone_settings()->mutable_thermal_spokes()->mutable_width()->set_value_nm(
                CopperLayer( ALL_LAYERS ).thermal_spoke_width.value() );
    }

    if( CopperLayer( ALL_LAYERS ).thermal_spoke_angle.has_value() )
    {
        padstack.mutable_zone_settings()->mutable_thermal_spokes()->mutable_angle()->set_value_degrees(
                CopperLayer( ALL_LAYERS ).thermal_spoke_angle.value().AsDegrees() );
    }

    padstack.set_unconnected_layer_removal( ToProtoEnum<UNCONNECTED_LAYER_MODE,
                                            kiapi::board::types::UnconnectedLayerRemoval>( m_unconnectedLayerMode ) );

    if( FrontOuterLayers().has_solder_mask.has_value() )
    {
        padstack.mutable_front_outer_layers()->set_solder_mask_mode(
                FrontOuterLayers().has_solder_mask.value() ? SMM_MASKED : SMM_UNMASKED );
    }

    if( BackOuterLayers().has_solder_mask.has_value() )
    {
        padstack.mutable_back_outer_layers()->set_solder_mask_mode(
                BackOuterLayers().has_solder_mask.value() ? SMM_MASKED : SMM_UNMASKED );
    }

    if( FrontOuterLayers().has_covering.has_value() )
    {
        padstack.mutable_front_outer_layers()->set_covering_mode(
                FrontOuterLayers().has_covering.value() ? VCM_COVERED : VCM_UNCOVERED );
    }

    if( BackOuterLayers().has_covering.has_value() )
    {
        padstack.mutable_back_outer_layers()->set_covering_mode(
                BackOuterLayers().has_covering.value() ? VCM_COVERED : VCM_UNCOVERED );
    }

    if( FrontOuterLayers().has_plugging.has_value() )
    {
        padstack.mutable_front_outer_layers()->set_plugging_mode(
                FrontOuterLayers().has_plugging.value() ? VPM_PLUGGED : VPM_UNPLUGGED );
    }

    if( BackOuterLayers().has_plugging.has_value() )
    {
        padstack.mutable_back_outer_layers()->set_plugging_mode(
                BackOuterLayers().has_plugging.value() ? VPM_PLUGGED : VPM_UNPLUGGED );
    }

    if( FrontOuterLayers().has_solder_paste.has_value() )
    {
        padstack.mutable_front_outer_layers()->set_solder_paste_mode(
                FrontOuterLayers().has_solder_paste.value() ? SPM_PASTE : SPM_NO_PASTE );
    }

    if( BackOuterLayers().has_solder_paste.has_value() )
    {
        padstack.mutable_back_outer_layers()->set_solder_paste_mode(
                BackOuterLayers().has_solder_paste.value() ? SPM_PASTE : SPM_NO_PASTE );
    }

    if( FrontOuterLayers().solder_mask_margin.has_value() )
    {
        padstack.mutable_front_outer_layers()->mutable_solder_mask_settings()->mutable_solder_mask_margin()->set_value_nm(
                FrontOuterLayers().solder_mask_margin.value() );
    }

    if( BackOuterLayers().solder_mask_margin.has_value() )
    {
        padstack.mutable_back_outer_layers()->mutable_solder_mask_settings()->mutable_solder_mask_margin()->set_value_nm(
                BackOuterLayers().solder_mask_margin.value() );
    }

    if( FrontOuterLayers().solder_paste_margin.has_value() )
    {
        padstack.mutable_front_outer_layers()->mutable_solder_paste_settings()->mutable_solder_paste_margin()->set_value_nm(
                FrontOuterLayers().solder_paste_margin.value() );
    }

    if( BackOuterLayers().solder_paste_margin.has_value() )
    {
        padstack.mutable_back_outer_layers()->mutable_solder_paste_settings()->mutable_solder_paste_margin()->set_value_nm(
                BackOuterLayers().solder_paste_margin.value() );
    }

    if( FrontOuterLayers().solder_paste_margin_ratio.has_value() )
    {
        padstack.mutable_front_outer_layers()->mutable_solder_paste_settings()->mutable_solder_paste_margin_ratio()->set_value(
                FrontOuterLayers().solder_paste_margin_ratio.value() );
    }

    if( BackOuterLayers().solder_paste_margin_ratio.has_value() )
    {
        padstack.mutable_back_outer_layers()->mutable_solder_paste_settings()->mutable_solder_paste_margin_ratio()->set_value(
                BackOuterLayers().solder_paste_margin_ratio.value() );
    }

    aContainer.PackFrom( padstack );
}


void PADSTACK::SetSize( const VECTOR2I& aSize, PCB_LAYER_ID aLayer )
{
    VECTOR2I size = aSize;

    if( size.x < 0 )
        size.x = 0;

    if( size.y < 0 )
        size.y = 0;

    CopperLayer( aLayer ).shape.size = size;
}


const VECTOR2I& PADSTACK::Size( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.size;
}


PAD_SHAPE PADSTACK::Shape( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.shape;
}


void PADSTACK::SetShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).shape.shape = aShape;
}


PAD_DRILL_SHAPE PADSTACK::DrillShape() const
{
    return m_drill.shape;
}


void PADSTACK::SetDrillShape( PAD_DRILL_SHAPE aShape )
{
    m_drill.shape = aShape;
}


VECTOR2I& PADSTACK::Offset( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).shape.offset;
}


const VECTOR2I& PADSTACK::Offset( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.offset;
}


PAD_SHAPE PADSTACK::AnchorShape( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.anchor_shape;
}


void PADSTACK::SetAnchorShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).shape.anchor_shape = aShape;
}


VECTOR2I& PADSTACK::TrapezoidDeltaSize( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).shape.trapezoid_delta_size;
}


const VECTOR2I& PADSTACK::TrapezoidDeltaSize( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.trapezoid_delta_size;
}


double PADSTACK::RoundRectRadiusRatio( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.round_rect_radius_ratio;
}


void PADSTACK::SetRoundRectRadiusRatio( double aRatio, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).shape.round_rect_radius_ratio = aRatio;
}


int PADSTACK::RoundRectRadius( PCB_LAYER_ID aLayer ) const
{
    const VECTOR2I& size = Size( aLayer );
    return KiROUND( std::min( size.x, size.y ) * RoundRectRadiusRatio( aLayer ) );
}


void PADSTACK::SetRoundRectRadius( double aRadius, PCB_LAYER_ID aLayer )
{
    const VECTOR2I& size = Size( aLayer );
    int min_r = std::min( size.x, size.y );

    if( min_r > 0 )
        SetRoundRectRadiusRatio( aRadius / min_r, aLayer );
}


double PADSTACK::ChamferRatio( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.chamfered_rect_ratio;
}


void PADSTACK::SetChamferRatio( double aRatio, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).shape.chamfered_rect_ratio = aRatio;
}


int& PADSTACK::ChamferPositions( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).shape.chamfered_rect_positions;
}


const int& PADSTACK::ChamferPositions( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.chamfered_rect_positions;
}


void PADSTACK::SetChamferPositions( int aPositions, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).shape.chamfered_rect_positions = aPositions;
}


std::optional<int>& PADSTACK::Clearance( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).clearance;
}


const std::optional<int>& PADSTACK::Clearance( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).clearance;
}


std::optional<int>& PADSTACK::SolderMaskMargin( PCB_LAYER_ID aLayer )
{
    if( IsFrontLayer( aLayer ) )
        return FrontOuterLayers().solder_mask_margin;
    else if( IsBackLayer( aLayer ) )
        return BackOuterLayers().solder_mask_margin;
    else
        return FrontOuterLayers().solder_mask_margin; // Should not happen
}


const std::optional<int>& PADSTACK::SolderMaskMargin( PCB_LAYER_ID aLayer ) const
{
    if( IsFrontLayer( aLayer ) )
        return FrontOuterLayers().solder_mask_margin;
    else if( IsBackLayer( aLayer ) )
        return BackOuterLayers().solder_mask_margin;
    else
        return FrontOuterLayers().solder_mask_margin; // Should not happen
}


std::optional<int>& PADSTACK::SolderPasteMargin( PCB_LAYER_ID aLayer )
{
    if( IsFrontLayer( aLayer ) )
        return FrontOuterLayers().solder_paste_margin;
    else if( IsBackLayer( aLayer ) )
        return BackOuterLayers().solder_paste_margin;
    else
        return FrontOuterLayers().solder_paste_margin; // Should not happen
}


const std::optional<int>& PADSTACK::SolderPasteMargin( PCB_LAYER_ID aLayer ) const
{
    if( IsFrontLayer( aLayer ) )
        return FrontOuterLayers().solder_paste_margin;
    else if( IsBackLayer( aLayer ) )
        return BackOuterLayers().solder_paste_margin;
    else
        return FrontOuterLayers().solder_paste_margin; // Should not happen
}


std::optional<double>& PADSTACK::SolderPasteMarginRatio( PCB_LAYER_ID aLayer )
{
    if( IsFrontLayer( aLayer ) )
        return FrontOuterLayers().solder_paste_margin_ratio;
    else if( IsBackLayer( aLayer ) )
        return BackOuterLayers().solder_paste_margin_ratio;
    else
        return FrontOuterLayers().solder_paste_margin_ratio; // Should not happen
}


const std::optional<double>& PADSTACK::SolderPasteMarginRatio( PCB_LAYER_ID aLayer ) const
{
    if( IsFrontLayer( aLayer ) )
        return FrontOuterLayers().solder_paste_margin_ratio;
    else if( IsBackLayer( aLayer ) )
        return BackOuterLayers().solder_paste_margin_ratio;
    else
        return FrontOuterLayers().solder_paste_margin_ratio; // Should not happen
}


std::optional<ZONE_CONNECTION>& PADSTACK::ZoneConnection( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).zone_connection;
}


const std::optional<ZONE_CONNECTION>& PADSTACK::ZoneConnection( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).zone_connection;
}


std::optional<int>& PADSTACK::ThermalSpokeWidth( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).thermal_spoke_width;
}


const std::optional<int>& PADSTACK::ThermalSpokeWidth( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).thermal_spoke_width;
}


std::optional<int>& PADSTACK::ThermalGap( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).thermal_gap;
}


const std::optional<int>& PADSTACK::ThermalGap( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).thermal_gap;
}


EDA_ANGLE PADSTACK::DefaultThermalSpokeAngleForShape( PCB_LAYER_ID aLayer ) const
{
    if( Shape( aLayer ) == PAD_SHAPE::OVAL || Shape( aLayer ) == PAD_SHAPE::RECTANGLE
        || Shape( aLayer ) == PAD_SHAPE::ROUNDRECT || Shape( aLayer ) == PAD_SHAPE::CHAMFERED_RECT )
    {
        return ANGLE_90;
    }

    return ANGLE_45;
}


EDA_ANGLE PADSTACK::ThermalSpokeAngle( PCB_LAYER_ID aLayer ) const
{
    if( CopperLayer( aLayer ).thermal_spoke_angle.has_value() )
        return CopperLayer( aLayer ).thermal_spoke_angle.value();

    return DefaultThermalSpokeAngleForShape( aLayer );
}


void PADSTACK::SetThermalSpokeAngle( EDA_ANGLE aAngle, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).thermal_spoke_angle = aAngle;
}


std::vector<std::shared_ptr<PCB_SHAPE>>& PADSTACK::Primitives( PCB_LAYER_ID aLayer )
{
    return CopperLayer( aLayer ).custom_shapes;
}


const std::vector<std::shared_ptr<PCB_SHAPE>>& PADSTACK::Primitives( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).custom_shapes;
}


void PADSTACK::AddPrimitive( PCB_SHAPE* aShape, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).custom_shapes.emplace_back( aShape );
}


void PADSTACK::AppendPrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aList,
                                 PCB_LAYER_ID aLayer )
{
    std::vector<std::shared_ptr<PCB_SHAPE>>& list = CopperLayer( aLayer ).custom_shapes;

    for( const std::shared_ptr<PCB_SHAPE>& item : aList )
    {
        PCB_SHAPE* new_shape = static_cast<PCB_SHAPE*>( item->Clone() );
        new_shape->SetParent( m_parent );
        list.emplace_back( new_shape );
    }
}


void PADSTACK::ReplacePrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aList,
                                  PCB_LAYER_ID aLayer )
{
    ClearPrimitives( aLayer );
    AppendPrimitives( aList, aLayer );
}


void PADSTACK::ClearPrimitives( PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).custom_shapes.clear();
}


PADSTACK::COPPER_LAYER_PROPS& PADSTACK::CopperLayer( PCB_LAYER_ID aLayer )
{
    if( m_mode == MODE::NORMAL )
        return m_copperProps[ALL_LAYERS];

    if( m_mode == MODE::FRONT_INNER_BACK )
    {
        if( IsFrontLayer( aLayer ) )
            return m_copperProps[F_Cu];
        else if( IsBackLayer( aLayer ) )
            return m_copperProps[B_Cu];
        else
            return m_copperProps[INNER_LAYERS];
    }

    return m_copperProps[aLayer];
}


const PADSTACK::COPPER_LAYER_PROPS& PADSTACK::CopperLayer( PCB_LAYER_ID aLayer ) const
{
    if( m_mode == MODE::NORMAL )
        return m_copperProps.at( ALL_LAYERS );

    if( m_mode == MODE::FRONT_INNER_BACK )
    {
        if( IsFrontLayer( aLayer ) )
            return m_copperProps.at( F_Cu );
        else if( IsBackLayer( aLayer ) )
            return m_copperProps.at( B_Cu );
        else
            return m_copperProps.at( INNER_LAYERS );
    }

    if( m_copperProps.count( aLayer ) )
        return m_copperProps.at( aLayer );

    return m_copperProps.at( ALL_LAYERS );
}


void PADSTACK::ForEachUniqueLayer( const std::function<void( PCB_LAYER_ID )>& aMethod ) const
{
    if( m_mode == MODE::NORMAL )
    {
        aMethod( ALL_LAYERS );
    }
    else if( m_mode == MODE::FRONT_INNER_BACK )
    {
        aMethod( F_Cu );
        aMethod( INNER_LAYERS );
        aMethod( B_Cu );
    }
    else
    {
        for( const auto& [layer, props] : m_copperProps )
            aMethod( layer );
    }
}


std::vector<PCB_LAYER_ID> PADSTACK::UniqueLayers() const
{
    std::vector<PCB_LAYER_ID> layers;

    ForEachUniqueLayer( [&]( PCB_LAYER_ID layer ) { layers.push_back( layer ); } );

    return layers;
}


PCB_LAYER_ID PADSTACK::EffectiveLayerFor( PCB_LAYER_ID aLayer ) const
{
    if( m_mode == MODE::NORMAL )
        return ALL_LAYERS;

    if( m_mode == MODE::FRONT_INNER_BACK )
    {
        if( IsFrontLayer( aLayer ) )
            return F_Cu;
        else if( IsBackLayer( aLayer ) )
            return B_Cu;
        else
            return INNER_LAYERS;
    }

    if( m_copperProps.count( aLayer ) )
        return aLayer;

    return ALL_LAYERS;
}


LSET PADSTACK::RelevantShapeLayers( const PADSTACK& aOther ) const
{
    LSET layers;

    if( m_mode == MODE::NORMAL && aOther.m_mode == MODE::NORMAL )
    {
        layers.set( ALL_LAYERS );
    }
    else
    {
        ForEachUniqueLayer( [&]( PCB_LAYER_ID layer ) { layers.set( layer ); } );
        aOther.ForEachUniqueLayer( [&]( PCB_LAYER_ID layer ) { layers.set( layer ); } );
    }

    return layers;
}


std::optional<bool> PADSTACK::IsTented( PCB_LAYER_ID aSide ) const
{
    if( IsFrontLayer( aSide ) )
        return FrontOuterLayers().has_solder_mask.has_value() ?
               std::optional<bool>( !FrontOuterLayers().has_solder_mask.value() ) : std::nullopt;
    else if( IsBackLayer( aSide ) )
        return BackOuterLayers().has_solder_mask.has_value() ?
               std::optional<bool>( !BackOuterLayers().has_solder_mask.value() ) : std::nullopt;
    else
        return std::nullopt;
}


std::optional<bool> PADSTACK::IsCovered( PCB_LAYER_ID aSide ) const
{
    if( IsFrontLayer( aSide ) )
        return FrontOuterLayers().has_covering;
    else if( IsBackLayer( aSide ) )
        return BackOuterLayers().has_covering;
    else
        return std::nullopt;
}


std::optional<bool> PADSTACK::IsPlugged( PCB_LAYER_ID aSide ) const
{
    if( IsFrontLayer( aSide ) )
        return FrontOuterLayers().has_plugging;
    else if( IsBackLayer( aSide ) )
        return BackOuterLayers().has_plugging;
    else
        return std::nullopt;
}


std::optional<bool> PADSTACK::IsCapped() const
{
    return m_drill.is_capped;
}


std::optional<bool> PADSTACK::IsFilled() const
{
    return m_drill.is_filled;
}


int PADSTACK::Compare( const PADSTACK* aLeft, const PADSTACK* aRight )
{
    // TODO: Implement full comparison
    if( aLeft == aRight )
        return 0;

    return 1;
}


bool PADSTACK::HasExplicitDefinitionForLayer( PCB_LAYER_ID aLayer ) const
{
    return m_copperProps.count( aLayer ) > 0;
}


double PADSTACK::Similarity( const PADSTACK& aOther ) const
{
    // TODO: Implement similarity
    return 0.0;
}


void PADSTACK::FlipLayers( int aLayerCount )
{
    // TODO: Implement FlipLayers
}


PCB_LAYER_ID PADSTACK::StartLayer() const
{
    return m_drill.start;
}


PCB_LAYER_ID PADSTACK::EndLayer() const
{
    return m_drill.end;
}


wxString PADSTACK::Name() const
{
    return m_customName;
}


PADSTACK::SHAPE_PROPS::SHAPE_PROPS() :
    shape( PAD_SHAPE::CIRCLE ),
    anchor_shape( PAD_SHAPE::RECTANGLE ),
    size( 0, 0 ),
    offset( 0, 0 ),
    round_rect_radius_ratio( 0.0 ),
    chamfered_rect_ratio( 0.0 ),
    chamfered_rect_positions( 0 ),
    trapezoid_delta_size( 0, 0 )
{
}


bool PADSTACK::SHAPE_PROPS::operator==( const SHAPE_PROPS& aOther ) const
{
    return shape == aOther.shape &&
           anchor_shape == aOther.anchor_shape &&
           size == aOther.size &&
           offset == aOther.offset &&
           round_rect_radius_ratio == aOther.round_rect_radius_ratio &&
           chamfered_rect_ratio == aOther.chamfered_rect_ratio &&
           chamfered_rect_positions == aOther.chamfered_rect_positions &&
           trapezoid_delta_size == aOther.trapezoid_delta_size;
}


bool PADSTACK::COPPER_LAYER_PROPS::operator==( const COPPER_LAYER_PROPS& aOther ) const
{
    if( !( shape == aOther.shape ) ) return false;
    if( zone_connection != aOther.zone_connection ) return false;
    if( thermal_spoke_width != aOther.thermal_spoke_width ) return false;
    if( thermal_spoke_angle != aOther.thermal_spoke_angle ) return false;
    if( thermal_gap != aOther.thermal_gap ) return false;
    if( clearance != aOther.clearance ) return false;

    if( custom_shapes.size() != aOther.custom_shapes.size() ) return false;

    // Deep compare of shapes?
    // For now, just check pointers or size
    return true;
}


bool PADSTACK::MASK_LAYER_PROPS::operator==( const MASK_LAYER_PROPS& aOther ) const
{
    return solder_mask_margin == aOther.solder_mask_margin &&
           solder_paste_margin == aOther.solder_paste_margin &&
           solder_paste_margin_ratio == aOther.solder_paste_margin_ratio &&
           has_solder_mask == aOther.has_solder_mask &&
           has_solder_paste == aOther.has_solder_paste &&
           has_covering == aOther.has_covering &&
           has_plugging == aOther.has_plugging;
}


bool PADSTACK::DRILL_PROPS::operator==( const DRILL_PROPS& aOther ) const
{
    return size == aOther.size &&
           shape == aOther.shape &&
           start == aOther.start &&
           end == aOther.end &&
           is_filled == aOther.is_filled &&
           is_capped == aOther.is_capped;
}


bool PADSTACK::POST_MACHINING_PROPS::operator==( const POST_MACHINING_PROPS& aOther ) const
{
    return mode == aOther.mode &&
           size == aOther.size &&
           depth == aOther.depth &&
           angle == aOther.angle;
}


void PADSTACK::SetMode( MODE aMode )
{
    m_mode = aMode;
}