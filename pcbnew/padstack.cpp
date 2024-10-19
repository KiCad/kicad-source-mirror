/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_shape.h>


PADSTACK::PADSTACK( BOARD_ITEM* aParent ) :
        m_parent( aParent ),
        m_mode( MODE::NORMAL ),
        m_orientation( ANGLE_0 ),
        m_unconnectedLayerMode( UNCONNECTED_LAYER_MODE::KEEP_ALL ),
        m_customShapeInZoneMode( CUSTOM_SHAPE_ZONE_MODE::OUTLINE )
{
    m_copperProps[PADSTACK::ALL_LAYERS].shape = SHAPE_PROPS();
    m_copperProps[PADSTACK::ALL_LAYERS].zone_connection = ZONE_CONNECTION::INHERITED;
    m_copperProps[PADSTACK::ALL_LAYERS].thermal_spoke_width = 0;
    m_copperProps[PADSTACK::ALL_LAYERS].thermal_spoke_angle = ANGLE_45;
    m_copperProps[PADSTACK::ALL_LAYERS].thermal_gap = 0;

    m_drill.shape = PAD_DRILL_SHAPE::CIRCLE;
    m_drill.start = F_Cu;
    m_drill.end   = B_Cu;

    m_secondaryDrill.shape = PAD_DRILL_SHAPE::UNDEFINED;
    m_secondaryDrill.start = UNDEFINED_LAYER;
    m_secondaryDrill.end   = UNDEFINED_LAYER;
}


PADSTACK::PADSTACK( const PADSTACK& aOther )
{
    *this = aOther;
}


PADSTACK& PADSTACK::operator=( const PADSTACK &aOther )
{
    m_parent                = aOther.m_parent;
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

    bool copperMatches = true;

    ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                if( CopperLayer( aLayer ) != aOther.CopperLayer( aLayer ) )
                    copperMatches = false;
            } );

    return copperMatches;
}


bool PADSTACK::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board::types;
    PadStack padstack;

    if( !aContainer.UnpackTo( &padstack ) )
        return false;

    m_mode = FromProtoEnum<MODE>( padstack.type() );

    // TODO
    m_layerSet.reset();

    m_orientation = EDA_ANGLE( padstack.angle().value_degrees(), DEGREES_T );

    Drill().size = kiapi::common::UnpackVector2( padstack.drill_diameter() );
    Drill().start = FromProtoEnum<PCB_LAYER_ID>( padstack.start_layer() );
    Drill().end = FromProtoEnum<PCB_LAYER_ID>( padstack.end_layer() );

    // We don't yet support complex padstacks
    // TODO(JE) Rework for full padstacks
    if( padstack.layers_size() == 1 )
    {
        const PadStackLayer& layer = padstack.layers( 0 );
        SetSize( kiapi::common::UnpackVector2( layer.size() ), ALL_LAYERS );
        SetLayerSet( kiapi::board::UnpackLayerSet( layer.layers() ) );
        SetShape( FromProtoEnum<PAD_SHAPE>( layer.shape() ), F_Cu );
        SetAnchorShape( FromProtoEnum<PAD_SHAPE>( layer.custom_anchor_shape() ), F_Cu );

        SHAPE_PROPS& props = CopperLayer( ALL_LAYERS ).shape;
        props.chamfered_rect_ratio = layer.chamfer_ratio();
        props.round_rect_radius_ratio = layer.corner_rounding_ratio();

        if( layer.chamfered_corners().top_left() )
            props.chamfered_rect_positions |= RECT_CHAMFER_TOP_LEFT;

        if( layer.chamfered_corners().top_right() )
            props.chamfered_rect_positions |= RECT_CHAMFER_TOP_RIGHT;

        if( layer.chamfered_corners().bottom_left() )
            props.chamfered_rect_positions |= RECT_CHAMFER_BOTTOM_LEFT;

        if( layer.chamfered_corners().bottom_right() )
            props.chamfered_rect_positions |= RECT_CHAMFER_BOTTOM_RIGHT;

        ClearPrimitives( F_Cu );
        google::protobuf::Any a;

        for( const GraphicShape& shapeProto : layer.custom_shapes() )
        {
            a.PackFrom( shapeProto );
            std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( m_parent );

            if( shape->Deserialize( a ) )
                AddPrimitive( shape.release(), F_Cu );
        }

        if( layer.has_zone_settings() )
        {
            CopperLayer( ALL_LAYERS ).zone_connection =
                    FromProtoEnum<ZONE_CONNECTION>( layer.zone_settings().zone_connection() );

            if( layer.zone_settings().has_thermal_spokes() )
            {
                const ThermalSpokeSettings& thermals = layer.zone_settings().thermal_spokes();

                CopperLayer( ALL_LAYERS ).thermal_gap = thermals.gap();
                CopperLayer( ALL_LAYERS ).thermal_spoke_width = thermals.width();
                SetThermalSpokeAngle( thermals.angle().value_degrees(), F_Cu );
            }
        }
        else
        {
            CopperLayer( ALL_LAYERS ).zone_connection = ZONE_CONNECTION::INHERITED;
            CopperLayer( ALL_LAYERS ).thermal_gap = 0;
            CopperLayer( ALL_LAYERS ).thermal_spoke_width = 0;
            CopperLayer( ALL_LAYERS ).thermal_spoke_angle = DefaultThermalSpokeAngleForShape( F_Cu );
        }
    }

    SetUnconnectedLayerMode(
        FromProtoEnum<UNCONNECTED_LAYER_MODE>( padstack.unconnected_layer_removal() ) );

    auto unpackMask =
            []( const SolderMaskMode& aProto, std::optional<bool>& aDest )
            {
                switch( aProto )
                {
                case kiapi::board::types::SMM_MASKED:
                    aDest = true;
                    break;

                case kiapi::board::types::SMM_UNMASKED:
                    aDest = false;
                    break;

                default:
                case kiapi::board::types::SMM_FROM_DESIGN_RULES:
                    aDest = std::nullopt;
                    break;
                }
            };

    unpackMask( padstack.front_outer_layers().solder_mask_mode(),
                FrontOuterLayers().has_solder_mask );

    unpackMask( padstack.back_outer_layers().solder_mask_mode(),
                BackOuterLayers().has_solder_mask );

    auto unpackPaste =
            []( const SolderPasteMode& aProto, std::optional<bool>& aDest )
            {
                switch( aProto )
                {
                case kiapi::board::types::SPM_PASTE:
                    aDest = true;
                    break;

                case kiapi::board::types::SPM_NO_PASTE:
                    aDest = false;
                    break;

                default:
                case kiapi::board::types::SPM_FROM_DESIGN_RULES:
                    aDest = std::nullopt;
                    break;
                }
            };

    unpackPaste( padstack.front_outer_layers().solder_paste_mode(),
                 FrontOuterLayers().has_solder_paste );

    unpackPaste( padstack.back_outer_layers().solder_paste_mode(),
                 BackOuterLayers().has_solder_paste );

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


void PADSTACK::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board::types;
    PadStack padstack;

    padstack.set_type( ToProtoEnum<MODE, PadStackType>( m_mode ) );
    padstack.set_start_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( StartLayer() ) );
    padstack.set_end_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( EndLayer() ) );
    kiapi::common::PackVector2( *padstack.mutable_drill_diameter(), Drill().size );
    padstack.mutable_angle()->set_value_degrees( m_orientation.AsDegrees() );

    // TODO(JE) Rework for full padstacks
    PadStackLayer* stackLayer = padstack.add_layers();
    kiapi::board::PackLayerSet( *stackLayer->mutable_layers(), LayerSet() );
    kiapi::common::PackVector2( *stackLayer->mutable_size(), Size( PADSTACK::ALL_LAYERS ) );
    stackLayer->set_shape( ToProtoEnum<PAD_SHAPE, PadStackShape>( Shape( ALL_LAYERS ) ) );
    stackLayer->set_custom_anchor_shape( ToProtoEnum<PAD_SHAPE, PadStackShape>( AnchorShape( ALL_LAYERS ) ) );
    stackLayer->set_chamfer_ratio( CopperLayer( ALL_LAYERS ).shape.chamfered_rect_ratio );
    stackLayer->set_corner_rounding_ratio( CopperLayer( ALL_LAYERS ).shape.round_rect_radius_ratio );

    google::protobuf::Any a;

    // TODO(JE) Rework for full padstacks
    for( const std::shared_ptr<PCB_SHAPE>& shape : Primitives( ALL_LAYERS ) )
    {
        shape->Serialize( a );
        GraphicShape* s = stackLayer->add_custom_shapes();
        a.UnpackTo( s );
    }

    const int& corners = CopperLayer( ALL_LAYERS ).shape.chamfered_rect_positions;
    stackLayer->mutable_chamfered_corners()->set_top_left( corners & RECT_CHAMFER_TOP_LEFT );
    stackLayer->mutable_chamfered_corners()->set_top_right( corners & RECT_CHAMFER_TOP_RIGHT );
    stackLayer->mutable_chamfered_corners()->set_bottom_left( corners & RECT_CHAMFER_BOTTOM_LEFT );
    stackLayer->mutable_chamfered_corners()->set_bottom_right( corners & RECT_CHAMFER_BOTTOM_RIGHT );

    ZoneConnectionSettings* zoneSettings = stackLayer->mutable_zone_settings();
    ThermalSpokeSettings* thermalSettings = zoneSettings->mutable_thermal_spokes();

    if( CopperLayer( ALL_LAYERS ).zone_connection.has_value() )
    {
        zoneSettings->set_zone_connection( ToProtoEnum<ZONE_CONNECTION, ZoneConnectionStyle>(
            *CopperLayer( ALL_LAYERS ).zone_connection ) );
    }

    thermalSettings->set_width( CopperLayer( ALL_LAYERS ).thermal_spoke_width.value_or( 0 ) );
    thermalSettings->set_gap( CopperLayer( ALL_LAYERS ).thermal_gap.value_or( 0 ) );
    thermalSettings->mutable_angle()->set_value_degrees( ThermalSpokeAngle( F_Cu ).AsDegrees() );

    padstack.set_unconnected_layer_removal(
        ToProtoEnum<UNCONNECTED_LAYER_MODE, UnconnectedLayerRemoval>( m_unconnectedLayerMode ) );

    auto packOptional =
        []<typename ProtoEnum>( const std::optional<bool>& aVal, ProtoEnum aTrueVal,
                                ProtoEnum aFalseVal, ProtoEnum aNullVal ) -> ProtoEnum
        {
            if( aVal.has_value() )
                return *aVal ? aTrueVal : aFalseVal;

            return aNullVal;
        };

    PadStackOuterLayer* frontOuter = padstack.mutable_front_outer_layers();
    PadStackOuterLayer* backOuter = padstack.mutable_back_outer_layers();

    frontOuter->set_solder_mask_mode( packOptional( FrontOuterLayers().has_solder_mask,
                                                    SMM_MASKED, SMM_UNMASKED,
                                                    SMM_FROM_DESIGN_RULES ) );

    backOuter->set_solder_mask_mode( packOptional( BackOuterLayers().has_solder_mask,
                                                   SMM_MASKED, SMM_UNMASKED,
                                                   SMM_FROM_DESIGN_RULES ) );

    frontOuter->set_solder_paste_mode( packOptional( FrontOuterLayers().has_solder_paste,
                                                     SPM_PASTE, SPM_NO_PASTE,
                                                     SPM_FROM_DESIGN_RULES ) );

    backOuter->set_solder_paste_mode( packOptional( BackOuterLayers().has_solder_paste,
                                                    SPM_PASTE, SPM_NO_PASTE,
                                                    SPM_FROM_DESIGN_RULES ) );

    if( FrontOuterLayers().solder_mask_margin.has_value() )
    {
        frontOuter->mutable_solder_mask_settings()->mutable_solder_mask_margin()->set_value_nm(
            *FrontOuterLayers().solder_mask_margin );
    }

    if( BackOuterLayers().solder_mask_margin.has_value() )
    {
        backOuter->mutable_solder_mask_settings()->mutable_solder_mask_margin()->set_value_nm(
            *BackOuterLayers().solder_mask_margin );
    }

    if( FrontOuterLayers().solder_paste_margin.has_value() )
    {
        frontOuter->mutable_solder_paste_settings()->mutable_solder_paste_margin()->set_value_nm(
            *FrontOuterLayers().solder_paste_margin );
    }

    if( BackOuterLayers().solder_paste_margin.has_value() )
    {
        backOuter->mutable_solder_paste_settings()->mutable_solder_paste_margin()->set_value_nm(
            *BackOuterLayers().solder_paste_margin );
    }

    if( FrontOuterLayers().solder_paste_margin_ratio.has_value() )
    {
        frontOuter->mutable_solder_paste_settings()->mutable_solder_paste_margin_ratio()->set_value(
            *FrontOuterLayers().solder_paste_margin_ratio );
    }

    if( BackOuterLayers().solder_paste_margin_ratio.has_value() )
    {
        backOuter->mutable_solder_paste_settings()->mutable_solder_paste_margin_ratio()->set_value(
            *BackOuterLayers().solder_paste_margin_ratio );
    }

    aContainer.PackFrom( padstack );
}


int PADSTACK::Compare( const PADSTACK* aPadstackRef, const PADSTACK* aPadstackCmp )
{
    int diff;

    auto compareCopperProps =
        [&]( PCB_LAYER_ID aLayer )
        {
            if( ( diff = static_cast<int>( aPadstackRef->Shape( aLayer ) ) -
                  static_cast<int>( aPadstackCmp->Shape( aLayer ) ) ) != 0 )
                return diff;

            if( ( diff = aPadstackRef->Size( aLayer ).x - aPadstackCmp->Size( aLayer ).x ) != 0 )
                return diff;

            if( ( diff = aPadstackRef->Size( aLayer ).y - aPadstackCmp->Size( aLayer ).y ) != 0 )
                return diff;

            if( ( diff = aPadstackRef->Offset( aLayer ).x
                         - aPadstackCmp->Offset( aLayer ).x ) != 0 )
                return diff;

            if( ( diff = aPadstackRef->Offset( aLayer ).y
                         - aPadstackCmp->Offset( aLayer ).y ) != 0 )
                return diff;

            if( ( diff = aPadstackRef->TrapezoidDeltaSize( aLayer ).x
                         - aPadstackCmp->TrapezoidDeltaSize( aLayer ).x )
                != 0 )
            {
                return diff;
            }

            if( ( diff = aPadstackRef->TrapezoidDeltaSize( aLayer ).y
                         - aPadstackCmp->TrapezoidDeltaSize( aLayer ).y )
                != 0 )
            {
                return diff;
            }

            if( ( diff = aPadstackRef->RoundRectRadiusRatio( aLayer )
                         - aPadstackCmp->RoundRectRadiusRatio( aLayer ) )
                != 0 )
            {
                return diff;
            }

            if( ( diff = aPadstackRef->ChamferPositions( aLayer )
                         - aPadstackCmp->ChamferPositions( aLayer ) ) != 0 )
                return diff;

            if( ( diff = aPadstackRef->ChamferRatio( aLayer )
            - aPadstackCmp->ChamferRatio( aLayer ) ) != 0 )
                return diff;

            if( ( diff = static_cast<int>( aPadstackRef->Primitives( aLayer ).size() ) -
                  static_cast<int>( aPadstackCmp->Primitives( aLayer ).size() ) ) != 0 )
                return diff;

            // @todo: Compare custom pad primitives for pads that have the same number of primitives
            //        here.  Currently there is no compare function for PCB_SHAPE objects.
            return 0;
        };

    aPadstackRef->ForEachUniqueLayer( compareCopperProps );

    if( ( diff = static_cast<int>( aPadstackRef->DrillShape() ) -
          static_cast<int>( aPadstackCmp->DrillShape() ) ) != 0 )
        return diff;

    if( ( diff = aPadstackRef->Drill().size.x - aPadstackCmp->Drill().size.x ) != 0 )
        return diff;

    if( ( diff = aPadstackRef->Drill().size.y - aPadstackCmp->Drill().size.y ) != 0 )
        return diff;

    return aPadstackRef->LayerSet().compare( aPadstackCmp->LayerSet() );
}


double PADSTACK::Similarity( const PADSTACK& aOther ) const
{
    double similarity = 1.0;

    ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            if( Shape( aLayer ) != aOther.Shape( aLayer ) )
                similarity *= 0.9;

            if( Size( aLayer ) != aOther.Size( aLayer ) )
                similarity *= 0.9;

            if( Offset( aLayer ) != aOther.Offset( aLayer ) )
                similarity *= 0.9;

            if( RoundRectRadiusRatio( aLayer ) != aOther.RoundRectRadiusRatio( aLayer ) )
                similarity *= 0.9;

            if( ChamferRatio( aLayer ) != aOther.ChamferRatio( aLayer ) )
                similarity *= 0.9;

            if( ChamferPositions( aLayer ) != aOther.ChamferPositions( aLayer ) )
                similarity *= 0.9;

            if( Primitives( aLayer ).size() != aOther.Primitives( aLayer ).size() )
                similarity *= 0.9;

            if( AnchorShape( aLayer ) != aOther.AnchorShape( aLayer ) )
                similarity *= 0.9;
        } );

    if( Drill() != aOther.Drill() )
        similarity *= 0.9;

    if( DrillShape() != aOther.DrillShape() )
        similarity *= 0.9;

    if( GetOrientation() != aOther.GetOrientation() )
        similarity *= 0.9;

    if( ZoneConnection() != aOther.ZoneConnection() )
        similarity *= 0.9;

    if( ThermalSpokeWidth() != aOther.ThermalSpokeWidth() )
        similarity *= 0.9;

    if( ThermalSpokeAngle() != aOther.ThermalSpokeAngle() )
        similarity *= 0.9;

    if( ThermalGap() != aOther.ThermalGap() )
        similarity *= 0.9;

    if( CustomShapeInZoneMode() != aOther.CustomShapeInZoneMode() )
        similarity *= 0.9;

    if( Clearance() != aOther.Clearance() )
        similarity *= 0.9;

    if( SolderMaskMargin() != aOther.SolderMaskMargin() )
        similarity *= 0.9;

    if( SolderPasteMargin() != aOther.SolderPasteMargin() )
        similarity *= 0.9;

    if( SolderPasteMarginRatio() != aOther.SolderPasteMarginRatio() )
        similarity *= 0.9;

    if( ThermalGap() != aOther.ThermalGap() )
        similarity *= 0.9;

    if( ThermalSpokeWidth() != aOther.ThermalSpokeWidth() )
        similarity *= 0.9;

    if( ThermalSpokeAngle() != aOther.ThermalSpokeAngle() )
        similarity *= 0.9;

    if( LayerSet() != aOther.LayerSet() )
        similarity *= 0.9;

    return similarity;
}


wxString PADSTACK::Name() const
{
    // TODO
    return wxEmptyString;
}


PCB_LAYER_ID PADSTACK::StartLayer() const
{
    return m_drill.start;
}


PCB_LAYER_ID PADSTACK::EndLayer() const
{
    return m_drill.end;
}


void PADSTACK::FlipLayers( int aCopperLayerCount )
{
    switch( m_mode )
    {
    case MODE::NORMAL:
        // Same shape on all layers; nothing to do
        break;

    case MODE::CUSTOM:
    {
        if( aCopperLayerCount > 2 )
        {
            int innerCount = ( aCopperLayerCount - 2 );
            int halfInnerLayerCount = innerCount / 2;
            PCB_LAYER_ID lastInner
                    = static_cast<PCB_LAYER_ID>( In1_Cu + ( innerCount - 1 ) * 2 );
            PCB_LAYER_ID midpointInner
                    = static_cast<PCB_LAYER_ID>( In1_Cu + ( halfInnerLayerCount - 1 ) * 2 );

            for( PCB_LAYER_ID layer : LAYER_RANGE( In1_Cu, midpointInner, MAX_CU_LAYERS ) )
            {
                auto conjugate =
                        magic_enum::enum_cast<PCB_LAYER_ID>( lastInner - ( layer - In1_Cu ) );
                wxCHECK2_MSG( conjugate.has_value() && m_copperProps.contains( conjugate.value() ),
                              continue, "Invalid inner layer conjugate!" );
                std::swap( m_copperProps[layer], m_copperProps[conjugate.value()] );
            }
        }

        KI_FALLTHROUGH;
    }

    case MODE::FRONT_INNER_BACK:
        std::swap( m_copperProps[F_Cu], m_copperProps[B_Cu] );
        std::swap( m_frontMaskProps, m_backMaskProps );
        break;
    }
}


PADSTACK::SHAPE_PROPS::SHAPE_PROPS() :
    shape( PAD_SHAPE::CIRCLE ),
    anchor_shape( PAD_SHAPE::CIRCLE ),
    round_rect_corner_radius( 0 ),
    round_rect_radius_ratio( 0.25 ),
    chamfered_rect_ratio( 0.2 ),
    chamfered_rect_positions( RECT_NO_CHAMFER )
{
}


bool PADSTACK::SHAPE_PROPS::operator==( const SHAPE_PROPS& aOther ) const
{
    return shape == aOther.shape && offset == aOther.offset
            && round_rect_corner_radius == aOther.round_rect_corner_radius
            && round_rect_radius_ratio == aOther.round_rect_radius_ratio
            && chamfered_rect_ratio == aOther.chamfered_rect_ratio
            && chamfered_rect_positions == aOther.chamfered_rect_positions;
}


bool PADSTACK::COPPER_LAYER_PROPS::operator==( const COPPER_LAYER_PROPS& aOther ) const
{
    if( shape != aOther.shape )
        return false;

    if( zone_connection != aOther.zone_connection )
        return false;

    if( thermal_spoke_width != aOther.thermal_spoke_width )
        return false;

    if( thermal_spoke_angle != aOther.thermal_spoke_angle )
        return false;

    if( thermal_gap != aOther.thermal_gap )
        return false;

    if( custom_shapes.size() != aOther.custom_shapes.size() )
        return false;

    if( !std::equal( custom_shapes.begin(), custom_shapes.end(),
                     aOther.custom_shapes.begin(), aOther.custom_shapes.end(),
                     []( const std::shared_ptr<PCB_SHAPE>& aFirst,
                         const std::shared_ptr<PCB_SHAPE>& aSecond )
                     {
                         return *aFirst == *aSecond;
                     } ) )
    {
        return false;
    }

    return true;
}


bool PADSTACK::MASK_LAYER_PROPS::operator==( const MASK_LAYER_PROPS& aOther ) const
{
    return solder_mask_margin == aOther.solder_mask_margin
            && solder_paste_margin == aOther.solder_paste_margin
            && solder_paste_margin_ratio == aOther.solder_paste_margin_ratio
            && has_solder_mask == aOther.has_solder_mask
            && has_solder_paste == aOther.has_solder_paste;
}


bool PADSTACK::DRILL_PROPS::operator==( const DRILL_PROPS& aOther ) const
{
    return size == aOther.size && shape == aOther.shape
            && start == aOther.start && end == aOther.end;
}


void PADSTACK::SetMode( MODE aMode )
{
    if( m_mode == aMode )
        return;

    switch( aMode )
    {
    case MODE::NORMAL:
        std::erase_if( m_copperProps,
            []( const auto& aEntry )
            {
                const auto& [key, value] = aEntry;
                return key != ALL_LAYERS;
            } );
        break;

    case MODE::FRONT_INNER_BACK:
        // When coming from normal, these layers may be missing or have junk values
        if( m_mode == MODE::NORMAL )
        {
            m_copperProps[INNER_LAYERS] = m_copperProps[ALL_LAYERS];
            m_copperProps[B_Cu] = m_copperProps[ALL_LAYERS];
        }

        break;

    case MODE::CUSTOM:
    {
        PCB_LAYER_ID innerLayerTemplate = ( m_mode == MODE::NORMAL ) ? ALL_LAYERS : INNER_LAYERS;

        for( PCB_LAYER_ID layer : LAYER_RANGE( In1_Cu, In30_Cu, MAX_CU_LAYERS ) )
            m_copperProps[layer] = m_copperProps[innerLayerTemplate];

        if( m_mode == MODE::NORMAL )
            m_copperProps[B_Cu] = m_copperProps[ALL_LAYERS];

        break;
    }
    }

    m_mode = aMode;

    // Changing mode invalidates cached shapes
    // TODO(JE) clean this up -- maybe PADSTACK should own shape caches
    if( PAD* parentPad = dynamic_cast<PAD*>( m_parent ) )
        parentPad->SetDirty();
}


void PADSTACK::ForEachUniqueLayer( const std::function<void( PCB_LAYER_ID )>& aMethod ) const
{
    switch( Mode() )
    {
    case MODE::NORMAL:
        aMethod( F_Cu );
        break;

    case MODE::FRONT_INNER_BACK:
        aMethod( F_Cu );
        aMethod( INNER_LAYERS );
        aMethod( B_Cu );
        break;

    case MODE::CUSTOM:
    {
        int layerCount = m_parent ? m_parent->BoardCopperLayerCount() : MAX_CU_LAYERS;

        for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, layerCount ) )
            aMethod( layer );

        break;
    }
    }
}


PCB_LAYER_ID PADSTACK::EffectiveLayerFor( PCB_LAYER_ID aLayer ) const
{
    switch( static_cast<int>( aLayer ) )
    {
    case LAYER_PAD_FR_NETNAMES:
        return F_Cu;

    case LAYER_PAD_BK_NETNAMES:
        return Mode() == MODE::NORMAL ? F_Cu : B_Cu;

        // For these, just give the front copper geometry, it doesn't matter.
    case LAYER_PAD_NETNAMES:
    case LAYER_VIA_NETNAMES:
    case LAYER_PADS_TH:
    case LAYER_PAD_PLATEDHOLES:
    case LAYER_VIA_HOLES:
    case LAYER_PAD_HOLEWALLS:
    case LAYER_VIA_HOLEWALLS:
        return ALL_LAYERS;

    default:
        break;
    }

    switch( Mode() )
    {
    case MODE::CUSTOM:
    case MODE::FRONT_INNER_BACK:
    {
        if( IsFrontLayer( aLayer ) )
            return F_Cu;

        if( IsBackLayer( aLayer ) )
            return B_Cu;

        wxASSERT_MSG( IsCopperLayer( aLayer ),
                      wxString::Format( wxT( "Unhandled layer %d in PADSTACK::EffectiveLayerFor" ),
                                        aLayer ) );

        return Mode() == MODE::CUSTOM ? aLayer : INNER_LAYERS;
    }

    case MODE::NORMAL:
        break;
    }

    return F_Cu;
}


PADSTACK::COPPER_LAYER_PROPS& PADSTACK::CopperLayer( PCB_LAYER_ID aLayer )
{
    PCB_LAYER_ID layer = EffectiveLayerFor( aLayer );
    // Create on-demand
    return m_copperProps[layer];
}


const PADSTACK::COPPER_LAYER_PROPS& PADSTACK::CopperLayer( PCB_LAYER_ID aLayer ) const
{
    PCB_LAYER_ID layer = EffectiveLayerFor( aLayer );

    wxCHECK_MSG( m_copperProps.contains( layer ), m_copperProps.at( ALL_LAYERS ),
        "Attempt to retrieve layer " + std::string( magic_enum::enum_name( layer ) ) + " from a "
        "padstack that does not contain it" );

    return m_copperProps.at( layer );
}


PAD_SHAPE PADSTACK::Shape( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.shape;
}


void PADSTACK::SetShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).shape.shape = aShape;
}


void PADSTACK::SetSize( const VECTOR2I& aSize, PCB_LAYER_ID aLayer )
{
    // File formats do not enforce that sizes are always positive, but KiCad requires it
    VECTOR2I& size = CopperLayer( aLayer ).shape.size;
    size.x = std::abs( aSize.x );
    size.y = std::abs( aSize.y );
}


const VECTOR2I& PADSTACK::Size( PCB_LAYER_ID aLayer ) const
{
    return CopperLayer( aLayer ).shape.size;
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
    return IsFrontLayer( aLayer ) ? m_frontMaskProps.solder_mask_margin
                                  : m_backMaskProps.solder_mask_margin;
}


const std::optional<int>& PADSTACK::SolderMaskMargin( PCB_LAYER_ID aLayer ) const
{
    return IsFrontLayer( aLayer ) ? m_frontMaskProps.solder_mask_margin
                                  : m_backMaskProps.solder_mask_margin;
}


std::optional<int>& PADSTACK::SolderPasteMargin( PCB_LAYER_ID aLayer )
{
    return IsFrontLayer( aLayer ) ? m_frontMaskProps.solder_paste_margin
                                  : m_backMaskProps.solder_paste_margin;
}


const std::optional<int>& PADSTACK::SolderPasteMargin( PCB_LAYER_ID aLayer ) const
{
    return IsFrontLayer( aLayer ) ? m_frontMaskProps.solder_paste_margin
                                  : m_backMaskProps.solder_paste_margin;}


std::optional<double>& PADSTACK::SolderPasteMarginRatio( PCB_LAYER_ID aLayer )
{
    return IsFrontLayer( aLayer ) ? m_frontMaskProps.solder_paste_margin_ratio
                                  : m_backMaskProps.solder_paste_margin_ratio;
}


const std::optional<double>& PADSTACK::SolderPasteMarginRatio( PCB_LAYER_ID aLayer ) const
{
    return IsFrontLayer( aLayer ) ? m_frontMaskProps.solder_paste_margin_ratio
                                  : m_backMaskProps.solder_paste_margin_ratio;
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
    const COPPER_LAYER_PROPS& defaults = CopperLayer( aLayer );

    return ( defaults.shape.shape == PAD_SHAPE::CIRCLE
             || ( defaults.shape.shape == PAD_SHAPE::CUSTOM
                  && defaults.shape.anchor_shape == PAD_SHAPE::CIRCLE ) )
                   ? ANGLE_45 : ANGLE_90;
}


EDA_ANGLE PADSTACK::ThermalSpokeAngle( PCB_LAYER_ID aLayer ) const
{
    const COPPER_LAYER_PROPS& defaults = CopperLayer( aLayer );

    return defaults.thermal_spoke_angle.value_or( DefaultThermalSpokeAngleForShape( aLayer ) );
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


void PADSTACK::AppendPrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList,
                                 PCB_LAYER_ID aLayer )
{
    for( const std::shared_ptr<PCB_SHAPE>& prim : aPrimitivesList )
        AddPrimitive( new PCB_SHAPE( *prim ), aLayer );
}


void PADSTACK::ReplacePrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList,
                                  PCB_LAYER_ID aLayer )
{
    ClearPrimitives( aLayer );

    if( aPrimitivesList.size() )
        AppendPrimitives( aPrimitivesList, aLayer );
}


void PADSTACK::ClearPrimitives( PCB_LAYER_ID aLayer )
{
    CopperLayer( aLayer ).custom_shapes.clear();
}


std::optional<bool> PADSTACK::IsTented( PCB_LAYER_ID aSide ) const
{
    if( IsFrontLayer( aSide ) )
        return m_frontMaskProps.has_solder_mask;

    if( IsBackLayer( aSide ) )
        return m_backMaskProps.has_solder_mask;

    wxCHECK_MSG( false, std::nullopt, "IsTented expects a front or back layer" );
}


IMPLEMENT_ENUM_TO_WXANY( PADSTACK::UNCONNECTED_LAYER_MODE )
