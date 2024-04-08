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


PADSTACK::PADSTACK() :
        m_mode( MODE::NORMAL ),
        m_unconnectedLayerMode( UNCONNECTED_LAYER_MODE::KEEP_ALL ),
        m_customShapeInZoneMode( CUSTOM_SHAPE_ZONE_MODE::OUTLINE )
{
    m_defaultCopperProps.shape = SHAPE_PROPS();
    m_defaultCopperProps.zone_connection = ZONE_CONNECTION::INHERITED;
    m_defaultCopperProps.thermal_spoke_width = std::nullopt;
    m_defaultCopperProps.thermal_spoke_angle = ANGLE_45;
    m_defaultCopperProps.thermal_gap = std::nullopt;

    m_drill.shape = PAD_DRILL_SHAPE::CIRCLE;
    m_drill.start = F_Cu;
    m_drill.end   = B_Cu;

    m_secondaryDrill.start = UNDEFINED_LAYER;
    m_secondaryDrill.end   = UNDEFINED_LAYER;
}


PADSTACK::PADSTACK( const PADSTACK& aOther )
{
    *this = aOther;
}


PADSTACK& PADSTACK::operator=( const PADSTACK &aOther )
{
    m_mode                 = aOther.m_mode;
    m_layerSet             = aOther.m_layerSet;
    m_customName           = aOther.m_customName;
    m_defaultCopperProps   = aOther.m_defaultCopperProps;
    m_defaultOuterProps    = aOther.m_defaultOuterProps;
    m_unconnectedLayerMode = aOther.m_unconnectedLayerMode;
    m_copperOverrides      = aOther.m_copperOverrides;
    m_topOverrides         = aOther.m_topOverrides;
    m_bottomOverrides      = aOther.m_bottomOverrides;
    m_drill                = aOther.m_drill;
    m_secondaryDrill       = aOther.m_secondaryDrill;
    return *this;
}


bool PADSTACK::operator==( const PADSTACK& aOther ) const
{
    return m_mode == aOther.m_mode
            && m_layerSet == aOther.m_layerSet
            && m_customName == aOther.m_customName
            && m_defaultCopperProps == aOther.m_defaultCopperProps
            && m_defaultOuterProps == aOther.m_defaultOuterProps
            && m_unconnectedLayerMode == aOther.m_unconnectedLayerMode
            && m_copperOverrides == aOther.m_copperOverrides
            && m_topOverrides == aOther.m_topOverrides
            && m_bottomOverrides == aOther.m_bottomOverrides
            && m_drill == aOther.m_drill
            && m_secondaryDrill == aOther.m_secondaryDrill;
}


bool PADSTACK::Deserialize( const google::protobuf::Any& aContainer )
{
    return false;
}


void PADSTACK::Serialize( google::protobuf::Any& aContainer ) const
{
}


wxString PADSTACK::Name() const
{
    // TODO
    return wxEmptyString;
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
    return shape == aOther.shape && zone_connection == aOther.zone_connection
            && thermal_spoke_width == aOther.thermal_spoke_width
            && thermal_spoke_angle == aOther.thermal_spoke_angle
            && thermal_gap == aOther.thermal_gap
            && custom_shapes == aOther.custom_shapes;
}


bool PADSTACK::OUTER_LAYER_PROPS::operator==( const OUTER_LAYER_PROPS& aOther ) const
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


PAD_SHAPE PADSTACK::Shape( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().shape.shape;
}


void PADSTACK::SetShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer )
{
    CopperLayerDefaults().shape.shape = aShape;
}


VECTOR2I& PADSTACK::Size( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().shape.size;
}


const VECTOR2I& PADSTACK::Size( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().shape.size;
}


PAD_DRILL_SHAPE PADSTACK::DrillShape( PCB_LAYER_ID aLayer ) const
{
    return m_drill.shape;
}


void PADSTACK::SetDrillShape( PAD_DRILL_SHAPE aShape, PCB_LAYER_ID aLayer )
{
    m_drill.shape = aShape;
}


VECTOR2I& PADSTACK::Offset( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().shape.offset;
}


const VECTOR2I& PADSTACK::Offset( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().shape.offset;
}


PAD_SHAPE PADSTACK::AnchorShape( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().shape.anchor_shape;
}


void PADSTACK::SetAnchorShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer )
{
    CopperLayerDefaults().shape.anchor_shape = aShape;
}


VECTOR2I& PADSTACK::TrapezoidDeltaSize( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().shape.trapezoid_delta_size;
}


const VECTOR2I& PADSTACK::TrapezoidDeltaSize( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().shape.trapezoid_delta_size;
}


double PADSTACK::RoundRectRadiusRatio( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().shape.round_rect_radius_ratio;
}


void PADSTACK::SetRoundRectRadiusRatio( double aRatio, PCB_LAYER_ID aLayer )
{
    CopperLayerDefaults().shape.round_rect_radius_ratio = aRatio;
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
    return CopperLayerDefaults().shape.chamfered_rect_ratio;
}


void PADSTACK::SetChamferRatio( double aRatio, PCB_LAYER_ID aLayer )
{
    CopperLayerDefaults().shape.chamfered_rect_ratio = aRatio;
}


int& PADSTACK::ChamferPositions( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().shape.chamfered_rect_positions;
}


const int& PADSTACK::ChamferPositions( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().shape.chamfered_rect_positions;
}


void PADSTACK::SetChamferPositions( int aPositions, PCB_LAYER_ID aLayer )
{
    CopperLayerDefaults().shape.chamfered_rect_positions = aPositions;
}


std::optional<int>& PADSTACK::Clearance( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().clearance;
}


const std::optional<int>& PADSTACK::Clearance( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().clearance;
}


std::optional<int>& PADSTACK::SolderMaskMargin( PCB_LAYER_ID aLayer )
{
    return OuterLayerDefaults().solder_mask_margin;
}


const std::optional<int>& PADSTACK::SolderMaskMargin( PCB_LAYER_ID aLayer ) const
{
    return OuterLayerDefaults().solder_mask_margin;
}


std::optional<int>& PADSTACK::SolderPasteMargin( PCB_LAYER_ID aLayer )
{
    return OuterLayerDefaults().solder_paste_margin;
}


const std::optional<int>& PADSTACK::SolderPasteMargin( PCB_LAYER_ID aLayer ) const
{
    return OuterLayerDefaults().solder_paste_margin;
}


std::optional<double>& PADSTACK::SolderPasteMarginRatio( PCB_LAYER_ID aLayer )
{
    return OuterLayerDefaults().solder_paste_margin_ratio;
}


const std::optional<double>& PADSTACK::SolderPasteMarginRatio( PCB_LAYER_ID aLayer ) const
{
    return OuterLayerDefaults().solder_paste_margin_ratio;
}


std::optional<ZONE_CONNECTION>& PADSTACK::ZoneConnection( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().zone_connection;
}


const std::optional<ZONE_CONNECTION>& PADSTACK::ZoneConnection( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().zone_connection;
}


std::optional<int>& PADSTACK::ThermalSpokeWidth( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().thermal_spoke_width;
}


const std::optional<int>& PADSTACK::ThermalSpokeWidth( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().thermal_spoke_width;
}


std::optional<int>& PADSTACK::ThermalGap( PCB_LAYER_ID aLayer )
{
    return CopperLayerDefaults().thermal_gap;
}


const std::optional<int>& PADSTACK::ThermalGap( PCB_LAYER_ID aLayer ) const
{
    return CopperLayerDefaults().thermal_gap;
}


EDA_ANGLE PADSTACK::ThermalSpokeAngle( PCB_LAYER_ID aLayer ) const
{
    const COPPER_LAYER_PROPS& defaults = CopperLayerDefaults();

    return defaults.thermal_spoke_angle.value_or(
            ( defaults.shape.shape == PAD_SHAPE::CIRCLE
              || ( defaults.shape.shape == PAD_SHAPE::CUSTOM
                   && defaults.shape.anchor_shape == PAD_SHAPE::CIRCLE ) )
            ? ANGLE_45 : ANGLE_90 );
}


void PADSTACK::SetThermalSpokeAngle( EDA_ANGLE aAngle, PCB_LAYER_ID aLayer )
{
    CopperLayerDefaults().thermal_spoke_angle = aAngle;
}


IMPLEMENT_ENUM_TO_WXANY( PADSTACK::UNCONNECTED_LAYER_MODE )
