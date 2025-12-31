/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#ifndef ALTIUM_PARSER_PCB_H
#define ALTIUM_PARSER_PCB_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <footprint.h>
#include <wx/gdicmn.h>

// tthis constant specifies an unconnected net
const uint16_t ALTIUM_NET_UNCONNECTED = std::numeric_limits<uint16_t>::max();

// this constant specifies a item which is not inside an component
const uint16_t ALTIUM_COMPONENT_NONE = std::numeric_limits<uint16_t>::max();

// this constant specifies a item which does not define a polygon
const uint16_t ALTIUM_POLYGON_NONE = std::numeric_limits<uint16_t>::max();

// 65534 seems to be belonging to board outline
const uint16_t ALTIUM_POLYGON_BOARD = std::numeric_limits<uint16_t>::max() - 1;


enum class ALTIUM_UNIT
{
    UNKNOWN = 0,

    INCH    = 1,
    MILS    = 2,
    MM      = 3,
    CM      = 4
};

enum class ALTIUM_CLASS_KIND
{
    UNKNOWN = -1,

    NET_CLASS              = 0,
    SOURCE_SCHEMATIC_CLASS = 1,
    FROM_TO                = 2,
    PAD_CLASS              = 3,
    LAYER_CLASS            = 4,
    UNKNOWN_CLASS          = 5,
    DIFF_PAIR_CLASS        = 6,
    POLYGON_CLASS          = 7
};

enum class ALTIUM_DIMENSION_KIND
{
    UNKNOWN = 0,

    LINEAR = 1,
    ANGULAR = 2,
    RADIAL = 3,
    LEADER = 4,
    DATUM = 5,
    BASELINE = 6,
    CENTER = 7,
    UNKNOWN_2 = 8,
    LINEAR_DIAMETER = 9,
    RADIAL_DIAMETER = 10
};

enum class ALTIUM_REGION_KIND
{
    UNKNOWN = -1,

    COPPER            = 0, // KIND=0
    POLYGON_CUTOUT    = 1, // KIND=1
    DASHED_OUTLINE    = 2, // KIND=2
    UNKNOWN_3         = 3, // KIND=3
    CAVITY_DEFINITION = 4, // KIND=4
    BOARD_CUTOUT      = 5, // KIND=0 AND ISBOARDCUTOUT=TRUE
};

enum class ALTIUM_RULE_KIND
{
    UNKNOWN = 0,

    CLEARANCE              = 1,
    DIFF_PAIR_ROUTINGS     = 2,
    HEIGHT                 = 3,
    HOLE_SIZE              = 4,
    HOLE_TO_HOLE_CLEARANCE = 5,
    WIDTH                  = 6,
    PASTE_MASK_EXPANSION   = 7,
    SOLDER_MASK_EXPANSION  = 8,
    PLANE_CLEARANCE        = 9,
    POLYGON_CONNECT        = 10,
    ROUTING_VIAS           = 11
};

enum class ALTIUM_CONNECT_STYLE
{
    UNKNOWN = 0,
    DIRECT  = 1,
    RELIEF  = 2,
    NONE    = 3
};

enum class ALTIUM_RECORD
{
    UNKNOWN = -1,

    ARC = 1,
    PAD = 2,
    VIA = 3,
    TRACK = 4,
    TEXT = 5,
    FILL = 6,
    REGION = 11,
    MODEL = 12
};

enum class ALTIUM_PAD_SHAPE
{
    UNKNOWN   = 0,
    CIRCLE    = 1,
    RECT      = 2,
    OCTAGONAL = 3
};

enum class ALTIUM_PAD_SHAPE_ALT
{
    UNKNOWN   = 0,
    CIRCLE    = 1,
    RECT      = 2, // TODO: valid?
    OCTAGONAL = 3, // TODO: valid?
    ROUNDRECT = 9
};

enum class ALTIUM_PAD_HOLE_SHAPE
{
    UNKNOWN = -1,
    ROUND   = 0,
    SQUARE  = 1,
    SLOT    = 2
};

enum class ALTIUM_PAD_MODE
{
    SIMPLE            = 0,
    TOP_MIDDLE_BOTTOM = 1,
    FULL_STACK        = 2
};

enum class ALTIUM_MODE
{
    UNKNOWN = -1,
    NONE = 0, // TODO: correct ID?
    RULE = 1,
    MANUAL = 2
};

enum class ALTIUM_POLYGON_HATCHSTYLE
{
    UNKNOWN = 0,

    SOLID      = 1,
    DEGREE_45  = 2,
    DEGREE_90  = 3,
    HORIZONTAL = 4,
    VERTICAL   = 5,
    NONE       = 6
};

enum class ALTIUM_TEXT_POSITION
{
    MANUAL        = 0, // only relevant for NAMEAUTOPOSITION and COMMENTAUTOPOSITION
    LEFT_TOP      = 1,
    LEFT_CENTER   = 2,
    LEFT_BOTTOM   = 3,
    CENTER_TOP    = 4,
    CENTER_CENTER = 5,
    CENTER_BOTTOM = 6,
    RIGHT_TOP     = 7,
    RIGHT_CENTER  = 8,
    RIGHT_BOTTOM  = 9
};

enum class ALTIUM_TEXT_TYPE
{
    UNKNOWN = -1,

    STROKE   = 0,
    TRUETYPE = 1,
    BARCODE  = 2
};

enum class ALTIUM_BARCODE_TYPE
{
    CODE39  = 0,
    CODE128 = 1
};

struct ALTIUM_VERTICE
{
    const bool    isRound;
    const int32_t radius;
    const double  startangle;
    const double  endangle;
    const VECTOR2I position;
    const VECTOR2I center;

    explicit ALTIUM_VERTICE( const VECTOR2I& aPosition )
            : isRound( false ),
              radius( 0 ),
              startangle( 0. ),
              endangle( 0. ),
              position( aPosition ),
              center( VECTOR2I( 0, 0 ) )
    {
    }

    explicit ALTIUM_VERTICE( bool aIsRound, int32_t aRadius, double aStartAngle, double aEndAngle,
                             const VECTOR2I aPosition, const VECTOR2I aCenter )
            : isRound( aIsRound ),
              radius( aRadius ),
              startangle( aStartAngle ),
              endangle( aEndAngle ),
              position( aPosition ),
              center( aCenter )
    {
    }
};

enum class ALTIUM_LAYER
{
    UNKNOWN = 0,

    TOP_LAYER    = 1,
    MID_LAYER_1  = 2,
    MID_LAYER_2  = 3,
    MID_LAYER_3  = 4,
    MID_LAYER_4  = 5,
    MID_LAYER_5  = 6,
    MID_LAYER_6  = 7,
    MID_LAYER_7  = 8,
    MID_LAYER_8  = 9,
    MID_LAYER_9  = 10,
    MID_LAYER_10 = 11,
    MID_LAYER_11 = 12,
    MID_LAYER_12 = 13,
    MID_LAYER_13 = 14,
    MID_LAYER_14 = 15,
    MID_LAYER_15 = 16,
    MID_LAYER_16 = 17,
    MID_LAYER_17 = 18,
    MID_LAYER_18 = 19,
    MID_LAYER_19 = 20,
    MID_LAYER_20 = 21,
    MID_LAYER_21 = 22,
    MID_LAYER_22 = 23,
    MID_LAYER_23 = 24,
    MID_LAYER_24 = 25,
    MID_LAYER_25 = 26,
    MID_LAYER_26 = 27,
    MID_LAYER_27 = 28,
    MID_LAYER_28 = 29,
    MID_LAYER_29 = 30,
    MID_LAYER_30 = 31,
    BOTTOM_LAYER = 32,

    TOP_OVERLAY    = 33,
    BOTTOM_OVERLAY = 34,
    TOP_PASTE      = 35,
    BOTTOM_PASTE   = 36,
    TOP_SOLDER     = 37,
    BOTTOM_SOLDER  = 38,

    INTERNAL_PLANE_1  = 39,
    INTERNAL_PLANE_2  = 40,
    INTERNAL_PLANE_3  = 41,
    INTERNAL_PLANE_4  = 42,
    INTERNAL_PLANE_5  = 43,
    INTERNAL_PLANE_6  = 44,
    INTERNAL_PLANE_7  = 45,
    INTERNAL_PLANE_8  = 46,
    INTERNAL_PLANE_9  = 47,
    INTERNAL_PLANE_10 = 48,
    INTERNAL_PLANE_11 = 49,
    INTERNAL_PLANE_12 = 50,
    INTERNAL_PLANE_13 = 51,
    INTERNAL_PLANE_14 = 52,
    INTERNAL_PLANE_15 = 53,
    INTERNAL_PLANE_16 = 54,

    DRILL_GUIDE    = 55,
    KEEP_OUT_LAYER = 56,

    MECHANICAL_1  = 57,
    MECHANICAL_2  = 58,
    MECHANICAL_3  = 59,
    MECHANICAL_4  = 60,
    MECHANICAL_5  = 61,
    MECHANICAL_6  = 62,
    MECHANICAL_7  = 63,
    MECHANICAL_8  = 64,
    MECHANICAL_9  = 65,
    MECHANICAL_10 = 66,
    MECHANICAL_11 = 67,
    MECHANICAL_12 = 68,
    MECHANICAL_13 = 69,
    MECHANICAL_14 = 70,
    MECHANICAL_15 = 71,
    MECHANICAL_16 = 72,

    DRILL_DRAWING     = 73,
    MULTI_LAYER       = 74,
    CONNECTIONS       = 75,
    BACKGROUND        = 76,
    DRC_ERROR_MARKERS = 77,
    SELECTIONS        = 78,
    VISIBLE_GRID_1    = 79,
    VISIBLE_GRID_2    = 80,
    PAD_HOLES         = 81,
    VIA_HOLES         = 82,
};

class ALTIUM_BINARY_PARSER;

enum class AEXTENDED_PRIMITIVE_INFORMATION_TYPE
{
    UNKNOWN = -1,

    MASK
};

struct AEXTENDED_PRIMITIVE_INFORMATION
{
    int           primitiveIndex;
    ALTIUM_RECORD primitiveObjectId;

    AEXTENDED_PRIMITIVE_INFORMATION_TYPE type;

    // Type == Mask
    ALTIUM_MODE pastemaskexpansionmode;
    int32_t     pastemaskexpansionmanual;
    ALTIUM_MODE soldermaskexpansionmode;
    int32_t     soldermaskexpansionmanual;

    explicit AEXTENDED_PRIMITIVE_INFORMATION( ALTIUM_BINARY_PARSER& aReader );
};

struct ABOARD6_LAYER_STACKUP
{
    wxString name;

    size_t nextId;
    size_t prevId;

    int32_t copperthick;

    double   dielectricconst;
    int32_t  dielectricthick;
    wxString dielectricmaterial;
};

struct ABOARD6
{
    VECTOR2I sheetpos;
    wxSize   sheetsize;

    int                                layercount;
    std::vector<ABOARD6_LAYER_STACKUP> stackup;
    std::set<wxString>                 layerNames;

    std::vector<ALTIUM_VERTICE> board_vertices;

    explicit ABOARD6( ALTIUM_BINARY_PARSER& aReader );
};

struct ACLASS6
{
    wxString name;
    wxString uniqueid;

    ALTIUM_CLASS_KIND kind;

    std::vector<wxString> names;

    explicit ACLASS6( ALTIUM_BINARY_PARSER& aReader );
};

struct ACOMPONENT6
{
    ALTIUM_LAYER layer;
    VECTOR2I     position;
    double       rotation;
    bool         locked;
    bool         nameon;
    bool         commenton;
    wxString     sourceUniqueID;
    wxString     sourceHierachicalPath;
    wxString     sourcedesignator;
    wxString     sourcefootprintlibrary;
    wxString     pattern;
    wxString     sourcecomponentlibrary;
    wxString     sourcelibreference;

    ALTIUM_TEXT_POSITION nameautoposition;
    ALTIUM_TEXT_POSITION commentautoposition;

    explicit ACOMPONENT6( ALTIUM_BINARY_PARSER& aReader );
};

struct ADIMENSION6
{
    ALTIUM_LAYER          layer;
    ALTIUM_DIMENSION_KIND kind;

    wxString textformat;
    wxString textprefix;
    wxString textsuffix;

    int32_t height;
    double  angle;

    uint32_t linewidth;
    uint32_t textheight;
    uint32_t textlinewidth;
    int32_t  textprecision;
    uint32_t textgap;
    bool     textbold;
    bool     textitalic;

    int32_t arrowsize;

    ALTIUM_UNIT textunit;

    VECTOR2I xy1;

    std::vector<VECTOR2I> referencePoint;
    std::vector<VECTOR2I> textPoint;

    explicit ADIMENSION6( ALTIUM_BINARY_PARSER& aReader );
};

struct AMODEL
{
    wxString name;
    wxString id;
    bool     isEmbedded;

    VECTOR3D rotation;
    double   z_offset;
    int32_t  checksum;

    explicit AMODEL( ALTIUM_BINARY_PARSER& aReader );
};

struct ANET6
{
    wxString name;

    explicit ANET6( ALTIUM_BINARY_PARSER& aReader );
};

struct APOLYGON6
{
    ALTIUM_LAYER layer;
    uint16_t     net;
    bool         locked;

    ALTIUM_POLYGON_HATCHSTYLE hatchstyle;

    int32_t gridsize;
    int32_t trackwidth;
    int32_t minprimlength;
    bool    useoctagons;

    // Note: Altium pour index is the opposite of KiCad zone priority!
    int32_t pourindex;

    std::vector<ALTIUM_VERTICE> vertices;

    explicit APOLYGON6( ALTIUM_BINARY_PARSER& aReader );
};


struct ARULE6
{
    wxString name;
    int      priority = 0;

    ALTIUM_RULE_KIND kind;

    wxString scope1expr;
    wxString scope2expr;

    // ALTIUM_RULE_KIND::CLEARANCE
    // ALTIUM_RULE_KIND::HOLE_TO_HOLE_CLEARANCE
    int clearanceGap = 0;

    // ALTIUM_RULE_KIND::WIDTH
    // ALTIUM_RULE_KIND::HOLE_SIZE
    int minLimit = 0;
    int maxLimit = 0;

    // ALTIUM_RULE_KIND::WIDTH
    int preferredWidth = 0;

    // ALTIUM_RULE_KIND::ROUTING_VIAS
    int width        = 0;
    int minWidth     = 0;
    int maxWidth     = 0;
    int holeWidth    = 0;
    int minHoleWidth = 0;
    int maxHoleWidth = 0;

    // ALTIUM_RULE_KIND::PLANE_CLEARANCE
    int planeclearanceClearance = 0;

    // ALTIUM_RULE_KIND::SOLDER_MASK_EXPANSION
    int soldermaskExpansion = 0;

    // ALTIUM_RULE_KIND::PASTE_MASK_EXPANSION
    int pastemaskExpansion = 0;

    // ALTIUM_RULE_KIND::POLYGON_CONNECT
    int32_t              polygonconnectAirgapwidth          = 0;
    int32_t              polygonconnectReliefconductorwidth = 0;
    int                  polygonconnectReliefentries        = 0;
    ALTIUM_CONNECT_STYLE polygonconnectStyle                = ALTIUM_CONNECT_STYLE::UNKNOWN;

    // TODO: implement different types of rules we need to parse

    explicit ARULE6( ALTIUM_BINARY_PARSER& aReader );
};

struct AREGION6
{
    bool is_locked;
    bool is_teardrop;
    bool is_keepout;

    bool is_shapebased;

    ALTIUM_LAYER layer;
    uint16_t     net;
    uint16_t     component;
    uint16_t     polygon;
    uint16_t     subpolyindex;
    uint8_t      keepoutrestrictions;
    uint16_t     holecount;

    ALTIUM_REGION_KIND kind; // I assume this means if normal or keepout?

    std::vector<ALTIUM_VERTICE>              outline;
    std::vector<std::vector<ALTIUM_VERTICE>> holes;

    explicit AREGION6( ALTIUM_BINARY_PARSER& aReader, bool aExtendedVertices );
};

struct AARC6
{
    bool is_locked;
    bool is_keepout;
    bool is_polygonoutline;

    ALTIUM_LAYER layer;
    uint16_t     net;
    uint16_t     component;
    uint16_t     polygon;
    uint16_t     subpolyindex;
    uint8_t      keepoutrestrictions;

    VECTOR2I center;
    uint32_t radius;
    double   startangle;
    double   endangle;
    uint32_t width;

    explicit AARC6( ALTIUM_BINARY_PARSER& aReader );
};

struct ACOMPONENTBODY6
{
    uint16_t             component = 0;

    wxString             body_name;
    int                  kind             = 0;
    int                  subpolyindex     = 0;
    int                  unionindex       = 0;
    int                  arc_resolution   = 0;
    bool                 is_shape_based   = false;
    int                  cavity_height    = 0;
    int                  standoff_height  = 0;
    int                  overall_height   = 0;
    int                  body_projection  = 0;
    int                  body_color_3d    = 0;
    double               body_opacity_3d  = 0.0;
    wxString             identifier;
    wxString             texture;
    int                  texture_center_x = 0;
    int                  texture_center_y = 0;
    int                  texture_size_x   = 0;
    int                  texture_size_y   = 0;
    int                  texture_rotation = 0;

    wxString             modelId;
    wxString             modelChecksum;
    bool                 modelIsEmbedded = false;
    wxString             modelName;
    int                  modelType       = 0;
    int                  modelSource     = 0;
    int                  modelSnapCount  = 0;

    VECTOR3D             modelPosition;
    VECTOR3D             modelRotation;
    double               rotation = 0.0;

    explicit ACOMPONENTBODY6( ALTIUM_BINARY_PARSER& aReader );
};

struct APAD6_SIZE_AND_SHAPE
{
    ALTIUM_PAD_HOLE_SHAPE holeshape;
    uint32_t              slotsize;
    double                slotrotation;

    wxSize               inner_size[29];
    ALTIUM_PAD_SHAPE     inner_shape[29];
    VECTOR2I             holeoffset[32];
    ALTIUM_PAD_SHAPE_ALT alt_shape[32];
    uint8_t              cornerradius[32];
};

struct APAD6
{
    bool is_locked;
    bool is_tent_top;
    bool is_tent_bottom;
    bool is_test_fab_top;
    bool is_test_fab_bottom;

    wxString name;

    ALTIUM_LAYER layer;
    uint16_t     net;
    uint16_t     component;

    VECTOR2I position;
    VECTOR2I topsize;
    VECTOR2I midsize;
    VECTOR2I botsize;
    uint32_t holesize;

    ALTIUM_PAD_SHAPE topshape;
    ALTIUM_PAD_SHAPE midshape;
    ALTIUM_PAD_SHAPE botshape;

    ALTIUM_PAD_MODE padmode;

    double          direction;
    bool            plated;
    ALTIUM_MODE     pastemaskexpansionmode;
    int32_t         pastemaskexpansionmanual;
    ALTIUM_MODE     soldermaskexpansionmode;
    int32_t         soldermaskexpansionmanual;
    double          holerotation;

    ALTIUM_LAYER tolayer;
    ALTIUM_LAYER fromlayer;

    int32_t pad_to_die_length;
    int32_t pad_to_die_delay;

    std::unique_ptr<APAD6_SIZE_AND_SHAPE> sizeAndShape;

    explicit APAD6( ALTIUM_BINARY_PARSER& aReader );
};

struct AVIA6
{
    bool is_locked          = false;
    bool is_tent_top        = false;
    bool is_tent_bottom     = false;
    bool is_test_fab_top    = false;
    bool is_test_fab_bottom = false;

    uint16_t net = 0;

    VECTOR2I position;
    uint32_t pos_tolerance = 2147483640; // 2147483640 is N/A
    uint32_t neg_tolerance = 2147483640; // 2147483640 is N/A
    uint32_t diameter      = 0;
    uint32_t holesize      = 0;

    int32_t  thermal_relief_airgap         = 0;
    uint32_t thermal_relief_conductorcount = 0;
    uint32_t thermal_relief_conductorwidth = 0;

    int32_t soldermask_expansion_front  = 0;
    int32_t soldermask_expansion_back   = 0;
    bool    soldermask_expansion_manual = false;
    bool    soldermask_expansion_linked = false;

    ALTIUM_LAYER    layer_start;
    ALTIUM_LAYER    layer_end;
    ALTIUM_PAD_MODE viamode;

    // In PAD_MODE::SIMPLE, this is the same as the diameter
    // In PAD_MODE::TOP_MIDDLE_BOTTOM, layer 0 is top, layer 1 is middle, layer 31 is bottom
    // In PAD_MODE::FULL_STACK, layers correspond to the layer number
    uint32_t diameter_by_layer[32];

    explicit AVIA6( ALTIUM_BINARY_PARSER& aReader );
};

struct ATRACK6
{
    bool is_locked;
    bool is_keepout;
    bool is_polygonoutline;

    ALTIUM_LAYER layer;
    uint16_t     net;
    uint16_t     component;
    uint16_t     polygon;
    uint16_t     subpolyindex;
    uint8_t      keepoutrestrictions;

    VECTOR2I start;
    VECTOR2I end;
    uint32_t width;

    explicit ATRACK6( ALTIUM_BINARY_PARSER& aReader );
};

struct ATEXT6
{
    enum class STROKE_FONT_TYPE
    {
        DEFAULT = 1,
        SANSSERIF = 2,
        SERIF = 3
    };


    ALTIUM_LAYER layer;
    uint16_t     component = 0;

    VECTOR2I             position;
    uint32_t             height       = 0;
    double               rotation     = 0.0;
    uint32_t             strokewidth  = 0;
    STROKE_FONT_TYPE     strokefonttype;

    bool isBold               = false;
    bool isItalic             = false;
    bool isMirrored           = false;
    bool isInverted           = false;
    bool isInvertedRect       = false;
    bool isFrame              = false;
    bool isOffsetBorder       = false;
    bool isJustificationValid = false;

    uint32_t margin_border_width = 0;
    uint32_t textbox_rect_width  = 0;
    uint32_t textbox_rect_height = 0;
    uint32_t text_offset_width   = 0;

    // Justification only applies when there is a text box size specified
    // Then, the text is justified within the box
    ALTIUM_TEXT_POSITION textbox_rect_justification = ALTIUM_TEXT_POSITION::CENTER_CENTER;

    uint32_t widestring_index = 0;

    bool isComment    = false;
    bool isDesignator = false;

    ALTIUM_TEXT_TYPE    fonttype;
    wxString            fontname;
    wxString            barcode_fontname;

    // Barcode specific parameters
    VECTOR2I            barcode_margin     = VECTOR2I( 0, 0 );
    ALTIUM_BARCODE_TYPE barcode_type       = ALTIUM_BARCODE_TYPE::CODE39;
    bool                barcode_inverted   = false;
    wxString            barcode_name;
    bool                barcode_show_text  = false;

    wxString            text;

    explicit ATEXT6( ALTIUM_BINARY_PARSER& aReader, std::map<uint32_t, wxString>& aStringTable );
};

struct AFILL6
{
    bool is_locked;
    bool is_keepout;

    ALTIUM_LAYER layer;
    uint16_t     component;
    uint16_t     net;
    uint8_t      keepoutrestrictions;

    VECTOR2I pos1;
    VECTOR2I pos2;
    double   rotation;

    explicit AFILL6( ALTIUM_BINARY_PARSER& aReader );
};


#endif //ALTIUM_PARSER_PCB_H
