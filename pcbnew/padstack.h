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

#ifndef KICAD_PADSTACK_H
#define KICAD_PADSTACK_H

#include <memory>
#include <optional>
#include <wx/string.h>

#include <api/serializable.h>
#include <geometry/eda_angle.h>
#include <layer_ids.h>
#include <lset.h>
#include <math/vector2d.h>
#include <properties/property.h>
#include <zones.h>

class BOARD_ITEM;
class PCB_SHAPE;

namespace kiapi::board::types
{
    class PadStack;
    class PadStackLayer;
}


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
    UNDEFINED,
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

    ///! Copper geometry mode: controls how many unique copper layer shapes this padstack has
    enum class MODE
    {
        NORMAL,           ///< Shape is the same on all layers
        FRONT_INNER_BACK, ///< Up to three shapes can be defined (F_Cu, inner copper layers, B_Cu)
        CUSTOM            ///< Shapes can be defined on arbitrary layers
    };

    ///! Temporary layer identifier to identify code that is not padstack-aware
    static constexpr PCB_LAYER_ID ALL_LAYERS = F_Cu;

    ///! The layer identifier to use for "inner layers" on top/inner/bottom padstacks
    static constexpr PCB_LAYER_ID INNER_LAYERS = In1_Cu;

    ///! Whether or not to remove the copper shape for unconnected layers
    enum class UNCONNECTED_LAYER_MODE
    {
        KEEP_ALL,
        START_END_ONLY,
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

        /*
         * Editing definitions of primitives for custom pad shapes.  In local coordinates relative
         * to m_Pos (NOT shapePos) at orient 0.
         */
        std::vector<std::shared_ptr<PCB_SHAPE>> custom_shapes;

        bool operator==( const COPPER_LAYER_PROPS& aOther ) const;
    };

    ///! The features of a padstack that can vary on outer layers.
    ///! All parameters are optional; leaving them un-set means "use parent/rule defaults"
    struct MASK_LAYER_PROPS
    {
        std::optional<int> solder_mask_margin;
        std::optional<int> solder_paste_margin;
        std::optional<double> solder_paste_margin_ratio;

        std::optional<bool> has_solder_mask;   ///< True if this outer layer has mask (is not tented)
        std::optional<bool> has_solder_paste;  ///< True if this outer layer has solder paste
        std::optional<bool> has_covering; ///< True if the pad on this side should have covering
        std::optional<bool> has_plugging; ///< True if the drill hole should be plugged on this side

        bool operator==( const MASK_LAYER_PROPS& aOther ) const;
    };

    ///! The properties of a padstack drill.  Drill position is always the pad position (origin).
    struct DRILL_PROPS
    {
        VECTOR2I size;  ///< Drill diameter (x == y) or slot dimensions (x != y)
        PAD_DRILL_SHAPE shape;
        PCB_LAYER_ID start;
        PCB_LAYER_ID end;

        std::optional<bool> is_filled; ///< True if the drill hole should be filled completely
        std::optional<bool> is_capped; ///< True if the drill hole should be capped

        bool operator==( const DRILL_PROPS& aOther ) const;
    };

public:
    PADSTACK( BOARD_ITEM* aParent );
    virtual ~PADSTACK() = default;
    PADSTACK( const PADSTACK& aOther );
    PADSTACK& operator=( const PADSTACK &aOther );

    bool operator==( const PADSTACK& aOther ) const;
    bool operator!=( const PADSTACK& aOther ) const { return !operator==( aOther ); }

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    /**
       * Compare two padstacks and return 0 if they are equal.
       *
       * @return less than 0 if left less than right, 0 if equal, or greater than 0 if left
       *         greater than right.
       */
    static int Compare( const PADSTACK* aPadstackRef, const PADSTACK* aPadstackCmp );

    /**
     * Return a measure of how likely the other object is to represent the same
     * object.  The scale runs from 0.0 (definitely different objects) to 1.0 (same)
     */
    double Similarity( const PADSTACK& aOther ) const;

    const LSET& LayerSet() const { return m_layerSet; }
    LSET& LayerSet() { return m_layerSet; }
    void SetLayerSet( const LSET& aSet ) { m_layerSet = aSet; }

    /**
     * Flips the padstack layers in the case that the pad's parent footprint is flipped to the
     * other side of the board.
     */
    void FlipLayers( int aCopperLayerCount );

    PCB_LAYER_ID StartLayer() const;
    PCB_LAYER_ID EndLayer() const;

    MODE Mode() const { return m_mode; }
    void SetMode( MODE aMode );

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

    COPPER_LAYER_PROPS& CopperLayer( PCB_LAYER_ID aLayer );
    const COPPER_LAYER_PROPS& CopperLayer( PCB_LAYER_ID aLayer ) const;

    MASK_LAYER_PROPS& FrontOuterLayers() { return m_frontMaskProps; }
    const MASK_LAYER_PROPS& FrontOuterLayers() const { return m_frontMaskProps; }

    MASK_LAYER_PROPS& BackOuterLayers() { return m_backMaskProps; }
    const MASK_LAYER_PROPS& BackOuterLayers() const { return m_backMaskProps; }

    /**
     * Checks if this padstack is tented (covered in soldermask) on the given side
     * @param aSide is a front or back layer (any will do)
     * @return true or false if this padstack contains a tenting override on the given layer, or
     *         std::nullopt if there is no override (meaning design rules should be used)
     */
    std::optional<bool> IsTented( PCB_LAYER_ID aSide ) const;

    std::optional<bool> IsCovered( PCB_LAYER_ID aSide ) const;

    std::optional<bool> IsPlugged( PCB_LAYER_ID aSide ) const;

    std::optional<bool> IsCapped() const;

    std::optional<bool> IsFilled() const;

    CUSTOM_SHAPE_ZONE_MODE CustomShapeInZoneMode() const { return m_customShapeInZoneMode; }
    void SetCustomShapeInZoneMode( CUSTOM_SHAPE_ZONE_MODE aM ) { m_customShapeInZoneMode = aM; }

    /**
     * Runs the given callable for each active unique copper layer in this padstack, meaning
     * F_Cu for MODE::NORMAL; F_Cu, PADSTACK::INNER_LAYERS, and B_Cu for top/inner/bottom,
     * and an arbitrary set of layers for full-custom padstacks.
     * @param aMethod will be called once for each independent copper layer in the padstack
     */
    void ForEachUniqueLayer( const std::function<void( PCB_LAYER_ID )>& aMethod ) const;

    std::vector<PCB_LAYER_ID> UniqueLayers() const;

    /**
     * Determines which geometry layer should be used for the given input layer.
     * For example, for MODE::NORMAL, this will always be F_Cu, and for MODE::FRONT_INNER_BACK,
     * this will be one of F_Cu, PADSTACK::INNER_LAYERS, and B_Cu depending on the input
     * layer.
     *
     * @param aLayer is a valid board layer
     * @return the layer that the padstack's geometry is stored on for the given input layer
     */
    PCB_LAYER_ID EffectiveLayerFor( PCB_LAYER_ID aLayer ) const;

    /**
     * Returns the set of layers that must be considered if checking one padstack against another.
     * For example, two normal padstacks will just return a set with ALL_LAYERS, but if one of them
     * is FRONT_INNER_BACK, there are three layers to check.
     */
    LSET RelevantShapeLayers( const PADSTACK& aOther ) const;

    // The following section has convenience getters for the padstack properties on a given layer.

    PAD_SHAPE Shape( PCB_LAYER_ID aLayer ) const;
    void SetShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer );

    // Setter rather than direct access to enforce only positive sizes
    void SetSize( const VECTOR2I& aSize, PCB_LAYER_ID aLayer );
    const VECTOR2I& Size( PCB_LAYER_ID aLayer ) const;

    PAD_DRILL_SHAPE DrillShape() const;
    void SetDrillShape( PAD_DRILL_SHAPE aShape );

    VECTOR2I& Offset( PCB_LAYER_ID aLayer );
    const VECTOR2I& Offset( PCB_LAYER_ID aLayer ) const;

    PAD_SHAPE AnchorShape( PCB_LAYER_ID aLayer ) const;
    void SetAnchorShape( PAD_SHAPE aShape, PCB_LAYER_ID aLayer );

    VECTOR2I& TrapezoidDeltaSize( PCB_LAYER_ID aLayer );
    const VECTOR2I& TrapezoidDeltaSize( PCB_LAYER_ID aLayer ) const;

    double RoundRectRadiusRatio( PCB_LAYER_ID aLayer ) const;
    void SetRoundRectRadiusRatio( double aRatio, PCB_LAYER_ID aLayer );

    int RoundRectRadius( PCB_LAYER_ID aLayer ) const;
    void SetRoundRectRadius( double aRadius, PCB_LAYER_ID aLayer );

    double ChamferRatio( PCB_LAYER_ID aLayer ) const;
    void SetChamferRatio( double aRatio, PCB_LAYER_ID aLayer );

    int& ChamferPositions( PCB_LAYER_ID aLayer );
    const int& ChamferPositions( PCB_LAYER_ID aLayer ) const;
    void SetChamferPositions( int aPositions, PCB_LAYER_ID aLayer );

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

    EDA_ANGLE DefaultThermalSpokeAngleForShape( PCB_LAYER_ID aLayer = F_Cu ) const;
    EDA_ANGLE ThermalSpokeAngle( PCB_LAYER_ID aLayer = F_Cu ) const;
    void SetThermalSpokeAngle( EDA_ANGLE aAngle, PCB_LAYER_ID aLayer = F_Cu );

    std::vector<std::shared_ptr<PCB_SHAPE>>& Primitives( PCB_LAYER_ID aLayer );
    const std::vector<std::shared_ptr<PCB_SHAPE>>& Primitives( PCB_LAYER_ID aLayer ) const;

    /**
     * Adds a custom shape primitive to the padstack.
     * @param aShape is a shape to add as a custom primitive. Ownership is passed to this PADSTACK.
     * @param aLayer is the padstack layer to add to.
     */
    void AddPrimitive( PCB_SHAPE* aShape, PCB_LAYER_ID aLayer );

    /**
     * Appends a copy of each shape in the given list to this padstack's custom shape list
     * @param aPrimitivesList is a list of shapes to add copies of to this PADSTACK
     * @param aLayer is the padstack layer to add to.
     */
    void AppendPrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList,
                           PCB_LAYER_ID aLayer );

    /**
     * Clears the existing primitive list (freeing the owned shapes) and adds copies of the given
     * shapes to the padstack for the given layer.
     * @param aPrimitivesList is a list of shapes to add copies of to this PADSTACK
     * @param aLayer is the padstack layer to add to.
     */
    void ReplacePrimitives( const std::vector<std::shared_ptr<PCB_SHAPE>>& aPrimitivesList,
                            PCB_LAYER_ID aLayer );

    void ClearPrimitives( PCB_LAYER_ID aLayer );

private:
    void packCopperLayer( PCB_LAYER_ID aLayer, kiapi::board::types::PadStack& aProto ) const;

    bool unpackCopperLayer( const kiapi::board::types::PadStackLayer& aProto );

    ///! The BOARD_ITEM this PADSTACK belongs to; will be used as the parent for owned shapes
    BOARD_ITEM* m_parent;

    ///! The copper layer variation mode this padstack is in
    MODE m_mode;

    ///! The board layers that this padstack is active on
    LSET m_layerSet;

    ///! An override for the IPC-7351 padstack name
    wxString m_customName;

    ///! The rotation of the pad relative to an outer reference frame
    EDA_ANGLE m_orientation;

    ///! The properties applied to copper layers if they aren't overridden
    //COPPER_LAYER_PROPS m_defaultCopperProps;
    std::unordered_map<PCB_LAYER_ID, COPPER_LAYER_PROPS> m_copperProps;

    ///! The overrides applied to front outer technical layers
    MASK_LAYER_PROPS m_frontMaskProps;

    ///! The overrides applied to back outer technical layers
    MASK_LAYER_PROPS m_backMaskProps;

    UNCONNECTED_LAYER_MODE m_unconnectedLayerMode;

    /**
     * How to build the custom shape in zone, to create the clearance area:
     * CUSTOM_SHAPE_ZONE_MODE::OUTLINE = use pad shape
     * CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL = use the convex hull of the pad shape
     */
    CUSTOM_SHAPE_ZONE_MODE m_customShapeInZoneMode;

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
