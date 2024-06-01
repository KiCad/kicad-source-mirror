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

#ifndef KICAD_PADSTACK_H
#define KICAD_PADSTACK_H

#include <memory>
#include <optional>
#include <wx/string.h>

#include <api/serializable.h>
#include <geometry/eda_angle.h>
#include <layer_ids.h>
#include <math/vector2d.h>
#include <properties/property.h>
#include <zones.h>

class PCB_SHAPE;


/**
 * The set of pad shapes, used with PAD::{Set,Get}Shape()
 *
 * --> DO NOT REORDER, PCB_IO_KICAD_LEGACY is dependent on the integer values <--
 */
enum class PAD_SHAPE : int
{
    CIRCLE,
    RECTANGLE,      // do not use just RECT: it collides in a header on MSYS2
    OVAL,
    TRAPEZOID,
    ROUNDRECT,

    // Rectangle with a chamfered corner ( and with rounded other corners).
    CHAMFERED_RECT,
    CUSTOM            // A shape defined by user, using a set of basic shapes
                      // (thick segments, circles, arcs, polygons).
};

/**
 * The set of pad drill shapes, used with PAD::{Set,Get}DrillShape()
 */
enum class PAD_DRILL_SHAPE
{
    CIRCLE,
    OBLONG,
};

/**
 * The set of pad shapes, used with PAD::{Set,Get}Attribute().
 *
 * The double name is for convenience of Python devs
 */
enum class PAD_ATTRIB
{
    PTH,        ///< Plated through hole pad
    SMD,        ///< Smd pad, appears on the solder paste layer (default)
    CONN,       ///< Like smd, does not appear on the solder paste layer (default)
                ///<   Note: also has a special attribute in Gerber X files
                ///<   Used for edgecard connectors for instance
    NPTH,       ///< like PAD_PTH, but not plated
                ///<   mechanical use only, no connection allowed
};


/**
 * The set of pad properties used in Gerber files (Draw files, and P&P files)
 * to define some properties in fabrication or test files.  Also used by
 * DRC to check some properties.
 */
enum class PAD_PROP
{
    NONE,                  ///< no special fabrication property
    BGA,                   ///< Smd pad, used in BGA footprints
    FIDUCIAL_GLBL,         ///< a fiducial (usually a smd) for the full board
    FIDUCIAL_LOCAL,        ///< a fiducial (usually a smd) local to the parent footprint
    TESTPOINT,             ///< a test point pad
    HEATSINK,              ///< a pad used as heat sink, usually in SMD footprints
    CASTELLATED,           ///< a pad with a castellated through hole
    MECHANICAL,            ///< a pad used for mechanical support
};


/**
 * A PADSTACK defines the characteristics of a single or multi-layer pad, in the IPC sense of
 * the word.  This means that a PCB_PAD has a padstack, but also a PCB_VIA.  The padstack for
 * a pad defines its geometry on copper, soldermask, and paste layers, as well as any drilling
 * or milling associated with the pad (round or slot hole, back-drilling, etc).  Padstacks also
 * define thermal relief settings for all copper layers, clearance overrides for all copper
 * layers, and potentially other properties in the future.  In other words, the padstack defines
 * most of the geometric features of a pad on all layers.  It does not define electrical properties
 * or other metadata.
 *
 * For padstacks that do not vary between layers, F_Cu is used as the copper layer to store all
 * padstack properties.
 */
class PADSTACK : public SERIALIZABLE
{
public:
    ///! Padstack type, mostly for IPC-7351 naming and attributes
    ///! Note that TYPE::MOUNTING is probably not currently supported by KiCad
    enum class TYPE
    {
        NORMAL,     ///< Padstack for a footprint pad
        VIA,        ///< Padstack for a via
        MOUNTING    ///< A mounting hole (plated or unplated, not associated with a footprint)
    };

    enum class MODE
    {
        NORMAL,           ///< Shape is the same on all layers
        TOP_INNER_BOTTOM, ///< Up to three shapes can be defined (top, inner, bottom)
        CUSTOM            ///< Shapes can be defined on arbitrary layers
    };

    ///! Whether or not to remove the copper shape for unconnected layers
    enum class UNCONNECTED_LAYER_MODE
    {
        KEEP_ALL,
        REMOVE_ALL,
        REMOVE_EXCEPT_START_AND_END
    };

    enum class CUSTOM_SHAPE_ZONE_MODE
    {
        OUTLINE,
        CONVEXHULL
    };

    ///! The set of properties that define a pad's shape on a given layer
    struct SHAPE_PROPS
    {
        PAD_SHAPE shape;    ///< Shape of the pad
        PAD_SHAPE anchor_shape; ///< Shape of the anchor when shape == PAD_SHAPE::CUSTOM
        VECTOR2I size;  ///< Size of the shape, or of the anchor pad for custom shape pads

        /*
         * Most of the time the hole is the center of the shape (m_Offset = 0). But some designers
         * use oblong/rect pads with a hole moved to one of the oblong/rect pad shape ends.
         * In all cases the hole is at the pad position.  This offset is from the hole to the center
         * of the pad shape (ie: the copper area around the hole).
         * ShapePos() returns the board shape position according to the offset and the pad rotation.
         */
        VECTOR2I offset; ///< Offset of the shape center from the pad center

        double round_rect_corner_radius;
        double round_rect_radius_ratio;
        double chamfered_rect_ratio;    ///< Size of chamfer: ratio of smallest of X,Y size
        int chamfered_rect_positions;   ///< @see RECT_CHAMFER_POSITIONS

        /**
         *  Delta for PAD_SHAPE::TRAPEZOID; half the delta squeezes one end and half expands the
         *  other.  It is only valid to have a single axis be non-zero.
         */
        VECTOR2I trapezoid_delta_size;

        SHAPE_PROPS();
        bool operator==( const SHAPE_PROPS& aOther ) const;
    };

    /**
     * The features of a padstack that can vary between copper layers
     * All parameters are optional; leaving them un-set means "use parent/rule defaults"
     * Pad clearances, margins, etc. exist in a hierarchy.  If a given level is specified then
     * the remaining levels are NOT consulted.
     *
     * LEVEL 1: (highest priority) local overrides (pad, footprint, etc.)
     * LEVEL 2: Rules
     * LEVEL 3: Accumulated local settings, netclass settings, & board design settings
     *
     * These are the LEVEL 1 settings (overrides) for a pad.
     */
    struct COPPER_LAYER_PROPS
    {
        SHAPE_PROPS shape;
        std::optional<ZONE_CONNECTION> zone_connection;
        std::optional<int> thermal_spoke_width;
        std::optional<EDA_ANGLE> thermal_spoke_angle;
        std::optional<int> thermal_gap;
        std::optional<int> clearance;

        std::vector<std::shared_ptr<PCB_SHAPE>> custom_shapes;

        bool operator==( const COPPER_LAYER_PROPS& aOther ) const;
    };

    ///! The features of a padstack that can vary on outer layers.
    ///! All parameters are optional; leaving them un-set means "use parent/rule defaults"
    struct OUTER_LAYER_PROPS
    {
        std::optional<int> solder_mask_margin;
        std::optional<int> solder_paste_margin;
        std::optional<double> solder_paste_margin_ratio;
        std::optional<bool> has_solder_mask;   ///< True if this outer layer has mask (is not tented)
        std::optional<bool> has_solder_paste;  ///< True if this outer layer has solder paste

        bool operator==( const OUTER_LAYER_PROPS& aOther ) const;
    };

    ///! The properties of a padstack drill.  Drill position is always the pad position (origin).
    struct DRILL_PROPS
    {
        VECTOR2I size;  ///< Drill diameter (x == y) or slot dimensions (x != y)
        PAD_DRILL_SHAPE shape;
        PCB_LAYER_ID start;
        PCB_LAYER_ID end;

        bool operator==( const DRILL_PROPS& aOther ) const;
    };

public:
    PADSTACK();
    virtual ~PADSTACK() = default;
    PADSTACK( const PADSTACK& aOther );
    PADSTACK& operator=( const PADSTACK &aOther );

    bool operator==( const PADSTACK& aOther ) const;
    bool operator!=( const PADSTACK& aOther ) const { return !operator==( aOther ); }

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    const LSET& LayerSet() const { return m_layerSet; }
    LSET& LayerSet() { return m_layerSet; }
    void SetLayerSet( const LSET& aSet ) { m_layerSet = aSet; }

    PCB_LAYER_ID StartLayer() const;
    PCB_LAYER_ID EndLayer() const;

    MODE Mode() const { return m_mode; }
    void SetMode( MODE aMode ) { m_mode = aMode; }

    ///! Returns the name of this padstack in IPC-7351 format
    wxString Name() const;

    EDA_ANGLE GetOrientation() const { return m_orientation; }
    void SetOrientation( EDA_ANGLE aAngle )
    {
        m_orientation = aAngle;
        m_orientation.Normalize();
    }

    DRILL_PROPS& Drill() { return m_drill; }
    const DRILL_PROPS& Drill() const { return m_drill; }

    DRILL_PROPS& SecondaryDrill() { return m_secondaryDrill; }
    const DRILL_PROPS& SecondaryDrill() const { return m_secondaryDrill; }

    UNCONNECTED_LAYER_MODE UnconnectedLayerMode() const { return m_unconnectedLayerMode; }
    void SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE aMode ) { m_unconnectedLayerMode = aMode; }

    COPPER_LAYER_PROPS& CopperLayerDefaults() { return m_defaultCopperProps; }
    const COPPER_LAYER_PROPS& CopperLayerDefaults() const { return m_defaultCopperProps; }

    OUTER_LAYER_PROPS& OuterLayerDefaults() { return m_defaultOuterProps; }
    const OUTER_LAYER_PROPS& OuterLayerDefaults() const { return m_defaultOuterProps; }

    CUSTOM_SHAPE_ZONE_MODE CustomShapeInZoneMode() const { return m_customShapeInZoneMode; }
    void SetCustomShapeInZoneMode( CUSTOM_SHAPE_ZONE_MODE aM ) { m_customShapeInZoneMode = aM; }

    // The following section has convenience getters for the padstack properties on a given layer.

    PAD_SHAPE Shape( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer = F_Cu );

    VECTOR2I& Size( PCB_LAYER_ID aLayer = F_Cu );
    const VECTOR2I& Size( PCB_LAYER_ID aLayer = F_Cu ) const;

    PAD_DRILL_SHAPE DrillShape( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetDrillShape( PAD_DRILL_SHAPE aShape, PCB_LAYER_ID aLayer = F_Cu );

    VECTOR2I& Offset( PCB_LAYER_ID aLayer = F_Cu );
    const VECTOR2I& Offset( PCB_LAYER_ID aLayer = F_Cu ) const;

    PAD_SHAPE AnchorShape( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetAnchorShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer = F_Cu );

    VECTOR2I& TrapezoidDeltaSize( PCB_LAYER_ID aLayer = F_Cu );
    const VECTOR2I& TrapezoidDeltaSize( PCB_LAYER_ID aLayer = F_Cu ) const;

    double RoundRectRadiusRatio( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetRoundRectRadiusRatio( double aRatio, PCB_LAYER_ID aLayer = F_Cu );

    int RoundRectRadius( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetRoundRectRadius( double aRadius, PCB_LAYER_ID aLayer = F_Cu );

    double ChamferRatio( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetChamferRatio( double aRatio, PCB_LAYER_ID aLayer = F_Cu );

    int& ChamferPositions( PCB_LAYER_ID aLayer = F_Cu );
    const int& ChamferPositions( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetChamferPositions( int aPositions, PCB_LAYER_ID aLayer = F_Cu );

    std::optional<int>& Clearance( PCB_LAYER_ID aLayer = F_Cu );
    const std::optional<int>& Clearance( PCB_LAYER_ID aLayer = F_Cu ) const;

    std::optional<int>& SolderMaskMargin( PCB_LAYER_ID aLayer = F_Cu );
    const std::optional<int>& SolderMaskMargin( PCB_LAYER_ID aLayer = F_Cu ) const;

    std::optional<int>& SolderPasteMargin( PCB_LAYER_ID aLayer = F_Cu );
    const std::optional<int>& SolderPasteMargin( PCB_LAYER_ID aLayer = F_Cu ) const;

    std::optional<double>& SolderPasteMarginRatio( PCB_LAYER_ID aLayer = F_Cu );
    const std::optional<double>& SolderPasteMarginRatio( PCB_LAYER_ID aLayer = F_Cu ) const;

    std::optional<ZONE_CONNECTION>& ZoneConnection( PCB_LAYER_ID aLayer = F_Cu );
    const std::optional<ZONE_CONNECTION>& ZoneConnection( PCB_LAYER_ID aLayer = F_Cu ) const;

    std::optional<int>& ThermalSpokeWidth( PCB_LAYER_ID aLayer = F_Cu );
    const std::optional<int>& ThermalSpokeWidth( PCB_LAYER_ID aLayer = F_Cu ) const;

    std::optional<int>& ThermalGap( PCB_LAYER_ID aLayer = F_Cu );
    const std::optional<int>& ThermalGap( PCB_LAYER_ID aLayer = F_Cu ) const;

    EDA_ANGLE ThermalSpokeAngle( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetThermalSpokeAngle( EDA_ANGLE aAngle, PCB_LAYER_ID aLayer = F_Cu );

private:

    MODE m_mode;

    ///! The board layers that this padstack is active on
    LSET m_layerSet;

    ///! An override for the IPC-7351 padstack name
    wxString m_customName;

    ///! The rotation of the pad relative to an outer reference frame
    EDA_ANGLE m_orientation;

    ///! The properties applied to copper layers if they aren't overridden
    COPPER_LAYER_PROPS m_defaultCopperProps;

    ///! The properties applied to outer technical layers if they aren't overridden
    OUTER_LAYER_PROPS m_defaultOuterProps;

    UNCONNECTED_LAYER_MODE m_unconnectedLayerMode;

    /**
     * How to build the custom shape in zone, to create the clearance area:
     * CUSTOM_SHAPE_ZONE_MODE::OUTLINE = use pad shape
     * CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL = use the convex hull of the pad shape
     */
    CUSTOM_SHAPE_ZONE_MODE m_customShapeInZoneMode;

    ///! Any entries here override the copper layer settings on the given copper layer.
    ///! If m_mode == MODE::TOP_INNER_BOTTOM, the inner layer setting is always In1_Cu and the only
    ///!        keys in this map that are used are F_Cu, In1_Cu, and B_Cu.
    ///! If m_mode == MODE::NORMAL, this map is ignored.
    std::unordered_map<PCB_LAYER_ID, COPPER_LAYER_PROPS> m_copperOverrides;

    ///! Any non-null optional values here override the mask/paste settings for the top layers
    OUTER_LAYER_PROPS m_topOverrides;

    ///! Any non-null optional values here override the mask/paste settings for bottom layers
    OUTER_LAYER_PROPS m_bottomOverrides;

    ///! The primary drill parameters, which also define the start and end layers for through-hole
    ///! vias and pads (F_Cu to B_Cu for normal holes; a subset of layers for blind/buried vias)
    DRILL_PROPS m_drill;

    ///! Secondary drill, used to define back-drilling
    DRILL_PROPS m_secondaryDrill;
};

#ifndef SWIG
DECLARE_ENUM_TO_WXANY( PADSTACK::UNCONNECTED_LAYER_MODE );
#endif


#endif //KICAD_PADSTACK_H
