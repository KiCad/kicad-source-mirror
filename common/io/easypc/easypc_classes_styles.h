/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

/**
 * @file easypc_classes_styles.h
 * @brief Design items, attributes, styles, layers, net classes and spacings.
 *
 * Loaders read each class's fields in stream order and keep only the fields the
 * importers use.  Lengths are design units of 100 nm, angles millidegrees.
 */

#ifndef EASYPC_CLASSES_STYLES_H
#define EASYPC_CLASSES_STYLES_H

#include <cstdint>
#include <map>
#include <memory>

#include <wx/string.h>

#include <io/easypc/easypc_classes_base.h>


namespace EASYPC
{

/// Pad style shape, also the drill and exception shape
enum class PAD_SHAPE : int32_t
{
    ROUND = 0,
    SQUARE = 1,
    RECTANGLE = 2,
    OVAL = 3,
    BULLET = 4,
    CROSS = 5,
    CHAMFERED_RECTANGLE = 6,
    ROUNDED_RECTANGLE = 7,
    DIAMOND = 8,
    OCTAGON = 9,
    TRIANGLE = 10,
    ANNULUS = 11,
    TARGET = 12,
    OFFSET_OVAL = 13,
    OFFSET_BULLET = 14,
    OFFSET_RECTANGLE = 15,
    C_CUT = 18,
    C_OFFSET = 19,
    T_SHAPE = 20,
    H_SHAPE = 21,
    WEDGE = 22,
    EQUILATERAL_TRIANGLE = 23,
    HEXAGON = 24
};


enum class LAYER_SIDE : int32_t
{
    TOP = 0,
    BOTTOM = 1,
    INNER = 2,
    ALL = 3
};


/// Layer usage bits
enum LAYER_USAGE : int32_t
{
    LAYER_USAGE_SET = 1, ///< pseudo layer [All], [Top] or [Bottom]
    LAYER_USAGE_ELECTRICAL = 2,
    LAYER_USAGE_NON_ELECTRICAL = 4,
    LAYER_USAGE_WIRE = 8,
    LAYER_USAGE_DRAWING = 16,
    LAYER_USAGE_SPAN = 32
};


/// How a layer type grows pads
enum class LAYER_OVERSIZE_TYPE : int32_t
{
    NONE = 0,
    FIXED = 1, ///< not ABSOLUTE, which wingdi.h defines
    PERCENT = 2
};


/// Item kind bits; a spacing key is (a << 16) | b
enum SPACING_ITEM : int32_t
{
    SPACING_BOARD = 0x01,
    SPACING_TRACK = 0x02,
    SPACING_PAD = 0x04,
    SPACING_VIA = 0x08,
    SPACING_SHAPE = 0x10,
    SPACING_TEXT = 0x20,
    SPACING_DRILL = 0x40,
    SPACING_AREA = 0x80,
    SPACING_PANEL = 0x100
};


/**
 * The base of nearly every class.  Its format is the nearest
 * self-versioned ancestor's format, the document version or the archive's current read format.
 */
struct SOURCE_DESIGN_ITEM : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override;

    /// An item format, stored only from object schema 2, which governs what follows
    static void LoadFormatted( ARCHIVE& aAr, int& aFormat );

    /// An older item format, stored only while the format is still 0
    void LoadOldFormat( ARCHIVE& aAr, int& aFormat ) const;

    OBJECT* Parent = nullptr;     ///< null is the document in a design, nothing in a library item
    OBJECT* Attributes = nullptr; ///< attribute array
};


struct SOURCE_ATTRIBUTE_NAME : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString Name;
};


struct SOURCE_ATTRIBUTE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    SOURCE_ATTRIBUTE_NAME* AttributeName = nullptr;
    wxString               Value;
    bool                   Displayed = false;
};


struct SOURCE_LINE_STYLE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString Name;
    int32_t  Width = 0;
};


/// A TrueType font, which the importers cannot measure
struct SOURCE_TYPE_FONT : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
};


struct SOURCE_TEXT_STYLE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString Name;
    int32_t  Height = 0; ///< the accessor is GetWidth but it is the text height
    int32_t  LineWidth = 0;
    OBJECT*  Font = nullptr; ///< type or barcode font, or null for the stroke font
    int32_t  InterlineHeightPercent = 120;
    int32_t  CharWidthPercent = 100;
    bool     Proportional = false;
};


struct SOURCE_TRACK_STYLE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString Name;
    int32_t  Width = 0;
};


struct SOURCE_PAD_STYLE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString  Name;
    int32_t   Size = 0; ///< X extent
    PAD_SHAPE Shape = PAD_SHAPE::ROUND;
    int32_t   Drill = 0;
    int32_t   Length = 0;           ///< Y extent for shapes that have one
    OBJECT*   Exceptions = nullptr; ///< pad style exception list
    bool      Plated = true;
    int32_t   Dimension3 = 0;
    PAD_SHAPE DrillShape = PAD_SHAPE::ROUND;
    int32_t   DrillLength = 0;
    int32_t   DrillCornerRadius = 0;
};


struct SOURCE_PAD_STYLE_EXCEPTION : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT*   Layer = nullptr;
    PAD_SHAPE Shape = PAD_SHAPE::ROUND;
    int32_t   Size = 0;
    int32_t   Length = 0;
    int32_t   Dimension3 = 0;
};


/// Layer type names determine silk, mask and paste roles
struct SOURCE_LAYER_TYPE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString            Name;
    int32_t             Usage = LAYER_USAGE_NON_ELECTRICAL;
    bool                AllLayerPadsEnabled = true;
    bool                ViasEnabled = true;
    bool                SurfacePadInstancesEnabled = true;
    bool                AllLayerPadInstancesEnabled = true;
    int32_t             Oversize = 0;
    LAYER_OVERSIZE_TYPE OversizeType = LAYER_OVERSIZE_TYPE::NONE;
    bool                SurfacePadsEnabled = true;
};


struct SOURCE_LAYER : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    /// The usage of the layer's type, or its own for a layer set
    int32_t EffectiveUsage() const { return Type ? Type->Usage : Usage; }

    wxString           Name;
    LAYER_SIDE         Side = LAYER_SIDE::TOP;
    wxString           TypeName;       ///< below v3000 the type is stored by name
    SOURCE_LAYER_TYPE* Type = nullptr; ///< null for the layer sets
    int32_t            Usage = 0;
    bool               Displayed = true;
    OBJECT*            Net = nullptr;       ///< net of a power plane
    OBJECT*            Terminals = nullptr; ///< copper terminal array

    /// The type made from type_name
    std::unique_ptr<SOURCE_LAYER_TYPE> ConvertedType;
};


/// A via span from top_layer to bottom_layer inclusive, in stack order
struct SOURCE_LAYER_SPAN : SOURCE_LAYER
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    SOURCE_LAYER* TopLayer = nullptr;
    SOURCE_LAYER* BottomLayer = nullptr;
};


/// Item order is the physical stack order
struct SOURCE_LAYER_ARRAY : SOURCE_DESIGN_LIST
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
};


/**
 * A thermal rule set as SkipFields ops: owner, four thermal rules (owner and an int array), two
 * flags and a hatch style.  Embedded in net classes, areas and the design parameters.
 */
constexpr char THERMAL_RULE_SET[] = "o o a o a o a o a 12 o";


struct SOURCE_NET_CLASS : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString            Name;
    int32_t             Type = 0;              ///< 0 signal, 1 power
    SOURCE_TRACK_STYLE* TrackStyle1 = nullptr; ///< the narrower of the two is the minimum width
    SOURCE_TRACK_STYLE* TrackStyle2 = nullptr;
    SOURCE_PAD_STYLE*   ViaStyle = nullptr;
    int32_t             TentedVias = 0; ///< 0 none, 1 all, 2 per via
    int32_t             DiffPairGap = 10000;
};


/// values maps (a << 16) | b of SPACING_ITEM bits to a clearance
struct SOURCE_SPACINGS : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    bool                       Bidirectional = true;
    std::map<int32_t, int32_t> Values;
};


/// The map is merged into rather than replaced on load, so the constructor defaults persist
struct SOURCE_DESIGN_SPACINGS : SOURCE_SPACINGS
{
    SOURCE_DESIGN_SPACINGS();

    LOAD_TASK Load( ARCHIVE& aAr ) override;
};

} // namespace EASYPC

#endif // EASYPC_CLASSES_STYLES_H
