/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <wx/string.h>
#include <wx/gdicmn.h> // For wxPoint if needed, or use VECTOR2I
#include <math/vector2d.h>

namespace PADS_IO
{

enum class UNIT_TYPE
{
    MILS,
    METRIC,
    INCHES
};

struct POINT
{
    double x;
    double y;
};

/**
 * Arc definition using center point, radius, and angles.
 *
 * PADS uses center/radius/angles representation for arcs. Angles are in degrees,
 * with 0 pointing right (+X) and positive angles going counter-clockwise.
 */
struct ARC
{
    double cx;           ///< Center X coordinate
    double cy;           ///< Center Y coordinate
    double radius;       ///< Arc radius
    double start_angle;  ///< Start angle in degrees (0 = +X, CCW positive)
    double delta_angle;  ///< Arc sweep angle in degrees (positive = CCW)
};

/**
 * A point that may be either a line endpoint or an arc segment.
 *
 * In PADS format, polylines can contain arc corners. When is_arc is true,
 * the arc field defines the arc from the previous point to this point's x,y.
 */
struct ARC_POINT
{
    double x;            ///< Endpoint X coordinate
    double y;            ///< Endpoint Y coordinate
    bool   is_arc;       ///< True if this segment is an arc, false for line
    ARC    arc;          ///< Arc parameters (only valid when is_arc is true)

    ARC_POINT() : x( 0 ), y( 0 ), is_arc( false ), arc{ 0, 0, 0, 0, 0 } {}
    ARC_POINT( double aX, double aY ) : x( aX ), y( aY ), is_arc( false ), arc{ 0, 0, 0, 0, 0 } {}
    ARC_POINT( double aX, double aY, const ARC& aArc ) : x( aX ), y( aY ), is_arc( true ), arc( aArc ) {}
};

/**
 * File header information from PADS header line.
 */
/**
 * PADS file types (PCB design or library).
 */
enum class PADS_FILE_TYPE
{
    PCB,            ///< PCB design file (POWERPCB, PADS-LAYOUT, etc.)
    LIB_LINE,       ///< Library line items (drafting)
    LIB_SCH_DECAL,  ///< Library schematic decals
    LIB_PCB_DECAL,  ///< Library PCB decals (footprints)
    LIB_PART_TYPE   ///< Library part types
};

struct FILE_HEADER
{
    std::string product;                ///< Product name: POWERPCB, PADS, LAYOUT, PADS-LIBRARY-*
    std::string version;                ///< Version string like V9.4, V9
    std::string units;                  ///< Units: MILS, MM, INCH, BASIC
    std::string mode;                   ///< Optional mode like 250L
    std::string encoding;               ///< Optional encoding like CP1252
    PADS_FILE_TYPE file_type = PADS_FILE_TYPE::PCB;  ///< Type of PADS file
};

struct PARAMETERS
{
    UNIT_TYPE units = UNIT_TYPE::MILS;
    int layer_count = 2;
    POINT origin = {0, 0};
    double user_grid = 0.0;                ///< User-defined snap grid (USERGRID)

    // Thermal relief settings (global defaults)
    double thermal_line_width = 30.0;      ///< Thermal line width for THT (THERLINEWID)
    double thermal_smd_width = 20.0;       ///< Thermal line width for SMD (THERSMDWID)
    int thermal_flags = 0;                 ///< Thermal relief flags (THERFLAGS)
    double thermal_min_clearance = 5.0;    ///< Starved thermal minimum clearance (STMINCLEAR)
    int thermal_min_spokes = 4;            ///< Starved thermal minimum spokes (STMINSPOKES)

    // Drill parameters
    double drill_oversize = 0.0;           ///< Drill oversize for plated holes (DRLOVERSIZE)

    // Via defaults extracted from file
    std::string default_signal_via;        ///< Default signal routing via (VIAPSHVIA)
};

/**
 * Design rule definitions from *PCB* section.
 */
struct DESIGN_RULES
{
    double min_clearance = 8.0;           ///< Minimum copper clearance (MINCLEAR)
    double default_clearance = 10.0;      ///< Default copper clearance (DEFAULTCLEAR)
    double min_track_width = 6.0;         ///< Minimum track width (MINTRACKWID)
    double default_track_width = 10.0;    ///< Default track width (DEFAULTTRACKWID)
    double min_via_size = 20.0;           ///< Minimum via outer diameter (MINVIASIZE)
    double default_via_size = 40.0;       ///< Default via outer diameter (DEFAULTVIASIZE)
    double min_via_drill = 10.0;          ///< Minimum via drill diameter (MINVIADRILL)
    double default_via_drill = 20.0;      ///< Default via drill diameter (DEFAULTVIADRILL)
    double hole_to_hole = 10.0;           ///< Minimum hole-to-hole spacing (HOLEHOLE)
    double silk_clearance = 5.0;          ///< Minimum silkscreen clearance (SILKCLEAR)
    double mask_clearance = 3.0;          ///< Solder mask clearance (MASKCLEAR)
    double copper_edge_clearance = 10.0;  ///< Board outline clearance (OUTLINE_TO_*)
};

struct ATTRIBUTE
{
    // Line 1
    bool visible = true;
    double x = 0.0;
    double y = 0.0;
    double orientation = 0.0;
    int level = 0;
    double height = 0.0;
    double width = 0.0;
    bool mirrored = false;
    std::string hjust;
    std::string vjust;
    bool right_reading = false;

    // Line 2
    std::string font_info;

    // Line 3
    std::string name; // "Ref.Des.", "Part Type", "VALUE", etc.
};

struct PART
{
    std::string name;
    std::string decal;                         ///< Primary decal (first in colon-separated list)
    std::string part_type;                     ///< Part type name when using PARTTYPE@DECAL syntax
    std::vector<std::string> alternate_decals; ///< Alternate decals (remaining after ':' splits)
    int alt_decal_index = -1;                  ///< ALT field from placement (-1 = use primary decal)
    std::string value;
    std::string units;
    POINT location;
    double rotation = 0.0;
    bool bottom_layer = false;
    bool glued = false;
    bool explicit_decal = false;               ///< True if decal was explicitly specified with @ syntax
    std::vector<ATTRIBUTE> attributes;
    std::string reuse_instance;                ///< Reuse block instance name (if member of reuse)
    std::string reuse_part;                    ///< Original part ref des inside the reuse block
};

struct NET_PIN
{
    std::string ref_des;
    std::string pin_name;
    std::string reuse_instance;   ///< Reuse block instance name (if .REUSE. suffix)
    std::string reuse_signal;     ///< Reuse block signal name (if .REUSE. suffix)
};

struct NET
{
    std::string name;
    std::vector<NET_PIN> pins;
};

/**
 * Net class definition with routing constraints.
 */
struct NET_CLASS_DEF
{
    std::string name;                       ///< Net class name
    double clearance = 0.0;                 ///< Copper clearance (CLEARANCE)
    double track_width = 0.0;               ///< Track width (TRACKWIDTH)
    double via_size = 0.0;                  ///< Via diameter (VIASIZE)
    double via_drill = 0.0;                 ///< Via drill diameter (VIADRILL)
    double diff_pair_gap = 0.0;             ///< Differential pair gap (DIFFPAIRGAP)
    double diff_pair_width = 0.0;           ///< Differential pair width (DIFFPAIRWIDTH)
    std::vector<std::string> net_names;     ///< Nets assigned to this class
};

/**
 * Differential pair definition.
 */
struct DIFF_PAIR_DEF
{
    std::string name;               ///< Pair name
    std::string positive_net;       ///< Positive net name
    std::string negative_net;       ///< Negative net name
    double gap = 0.0;               ///< Spacing between traces
    double width = 0.0;             ///< Trace width
};

/**
 * Teardrop parameters for a route point.
 */
struct TEARDROP
{
    double pad_width = 0.0;       ///< Teardrop width at pad side
    double pad_length = 0.0;      ///< Teardrop length toward pad
    int pad_flags = 0;            ///< Pad-side teardrop flags
    double net_width = 0.0;       ///< Teardrop width at net side
    double net_length = 0.0;      ///< Teardrop length toward net
    int net_flags = 0;            ///< Net-side teardrop flags
    bool has_pad_teardrop = false;
    bool has_net_teardrop = false;
};

/**
 * Jumper endpoint marker in a route.
 */
struct JUMPER_MARKER
{
    std::string name;             ///< Jumper part name
    bool is_start = false;        ///< True if start (S), false if end (E)
    double x = 0.0;
    double y = 0.0;
};

/**
 * Jumper definition from *JUMPER* section.
 *
 * Format: name flags minlen maxlen lenincr lcount padstack [end_padstack]
 * Followed by lcount label entries (2 lines each).
 */
struct JUMPER_DEF
{
    std::string name;               ///< Jumper name/reference designator
    bool via_enabled = false;       ///< V flag: via enabled
    bool wirebond = false;          ///< W flag: wirebond jumper
    bool display_silk = false;      ///< D flag: display special silk
    bool glued = false;             ///< G flag: glued
    double min_length = 0.0;        ///< Minimum possible length
    double max_length = 0.0;        ///< Maximum possible length
    double length_increment = 0.0;  ///< Length increment
    std::string padstack;           ///< Pad stack for start pin (or both if end_padstack empty)
    std::string end_padstack;       ///< Pad stack for end pin (optional)
    std::vector<ATTRIBUTE> labels;  ///< Reference designator labels
};

struct TRACK
{
    int layer = 0;
    double width = 0.0;
    std::vector<ARC_POINT> points;  ///< Track points, may include arc segments
};

struct PAD_STACK_LAYER
{
    int layer = 0;
    std::string shape;       ///< Shape code: R, S, A, O, OF, RF, RT, ST, RA, SA, RC, OC
    double sizeA = 0.0;      ///< Primary size (diameter or width)
    double sizeB = 0.0;      ///< Secondary size (height for rectangles/ovals)
    double offsetX = 0.0;    ///< Pad offset X from terminal position
    double offsetY = 0.0;    ///< Pad offset Y from terminal position
    double rotation = 0.0;   ///< Pad rotation angle in degrees
    double drill = 0.0;      ///< Drill hole diameter (0 for SMD)
    bool plated = true;      ///< True if drill is plated (PTH vs NPTH)
    double inner_diameter = 0.0;   ///< Inner diameter for annular ring (0 = solid)
    double corner_radius = 0.0;    ///< Corner radius magnitude (always positive)
    bool chamfered = false;        ///< True if corners are chamfered (negative corner in PADS)
    double finger_offset = 0.0;    ///< Finger pad offset along orientation axis

    // Slotted drill parameters
    double slot_orientation = 0.0; ///< Slot orientation in degrees (0-179.999)
    double slot_length = 0.0;      ///< Slot length
    double slot_offset = 0.0;      ///< Slot offset from electrical center

    // Thermal pad parameters (for RT/ST shapes)
    double thermal_spoke_orientation = 0.0;  ///< First spoke orientation in degrees
    double thermal_outer_diameter = 0.0;     ///< Outer diameter of thermal or void in plane
    double thermal_spoke_width = 0.0;        ///< Width of thermal spokes
    int thermal_spoke_count = 0;             ///< Number of thermal spokes (typically 4)
};

enum class VIA_TYPE
{
    THROUGH,    ///< Via spans all copper layers
    BLIND,      ///< Via starts at top or bottom and ends at inner layer
    BURIED,     ///< Via spans only inner layers
    MICROVIA    ///< Single-layer blind via (typically HDI)
};

struct VIA_DEF
{
    std::string name;
    double drill = 0.0;
    double size = 0.0;
    std::vector<PAD_STACK_LAYER> stack;
    int start_layer = 0;     ///< First PADS layer number in via span
    int end_layer = 0;       ///< Last PADS layer number in via span
    VIA_TYPE via_type = VIA_TYPE::THROUGH;   ///< Classified via type
    int drill_start = 0;     ///< Drill start layer from file (for blind/buried vias)
    int drill_end = 0;       ///< Drill end layer from file (for blind/buried vias)
    bool has_mask_front = false;  ///< Stack includes top soldermask opening (layer 25)
    bool has_mask_back = false;   ///< Stack includes bottom soldermask opening (layer 28)
};

struct VIA
{
    std::string name; // Via type name
    POINT location;
};

/**
 * Pour style types from PADS piece types.
 */
enum class POUR_STYLE
{
    SOLID,      ///< Solid filled pour (POUROUT, POLY)
    HATCHED,    ///< Hatched pour (HATOUT)
    VOIDOUT     ///< Void/empty region (VOIDOUT)
};

/**
 * Thermal relief type for pad/via connections.
 */
enum class THERMAL_TYPE
{
    NONE,       ///< No thermal relief defined
    PAD,        ///< Pad thermal relief (PADTHERM)
    VIA         ///< Via thermal relief (VIATHERM)
};

struct POUR
{
    std::string name;                   ///< This pour record's name
    std::string net_name;
    int layer = 0;
    int priority = 0;
    double width = 0.0;
    std::vector<ARC_POINT> points;      ///< Pour outline, may include arc segments
    bool is_cutout = false;             ///< True if this is a cutout (POCUT) piece
    std::string owner_pour;             ///< Name of parent pour (7th field in header)
    POUR_STYLE style = POUR_STYLE::SOLID;   ///< Pour fill style
    double hatch_grid = 0.0;            ///< Hatch grid spacing for hatched pours
    double hatch_width = 0.0;           ///< Hatch line width

    // Thermal relief definitions (PADTHERM, VIATHERM)
    THERMAL_TYPE thermal_type = THERMAL_TYPE::NONE;
    double thermal_spoke_width = 0.0;   ///< Spoke width for thermal relief
    int thermal_spoke_count = 4;        ///< Number of spokes (typically 4)
    double thermal_gap = 0.0;           ///< Gap between pad and pour
};

struct DECAL_ITEM
{
    std::string type;               ///< CLOSED, OPEN, CIRCLE, COPCLS, TAG, etc.
    int layer = 0;
    double width = 0.0;
    std::vector<ARC_POINT> points;  ///< Shape points, may include arc segments
    int pinnum = -1;                ///< Pin association for copper pieces (-1 = none, 0+ = pin index)
    std::string restrictions;       ///< Keepout restrictions (R,C,V,T,A) for KPTCLS/KPTCIR
    bool is_tag_open = false;       ///< True if this is an opening TAG (level=1)
    bool is_tag_close = false;      ///< True if this is a closing TAG (level=0)
};

struct DECAL_PAD
{
    int pin_number; // 0 for default
    POINT position;
    std::string name; // e.g. "1", "A1"
    std::vector<PAD_STACK_LAYER> stack;
    bool custom_stack = false;
};


struct TERMINAL
{
    double x;
    double y;
    std::string name; // Pin number
};

struct PART_DECAL
{
    std::string name;
    std::string units; // M, I, U, etc.
    std::vector<DECAL_ITEM> items;
    std::vector<ATTRIBUTE> attributes;
    std::vector<TERMINAL> terminals;
    std::map<int, std::vector<PAD_STACK_LAYER>> pad_stacks;
};

/**
 * Standard signal pin definition (power, ground, etc.)
 */
struct SIGPIN
{
    std::string pin_number;     ///< Pin number
    double width = 0.0;         ///< Track width for connections
    std::string signal_name;    ///< Standard signal name (e.g., VCC, GND)
};

/**
 * Pin type classification for gate definitions.
 */
enum class PIN_ELEC_TYPE
{
    UNDEFINED,      ///< U - Undefined
    SOURCE,         ///< S - Source pin
    BIDIRECTIONAL,  ///< B - Bidirectional pin
    OPEN_COLLECTOR, ///< C - Open collector or or-tieable source
    TRISTATE,       ///< T - Tri-state pin
    LOAD,           ///< L - Load pin
    TERMINATOR,     ///< Z - Terminator pin
    POWER,          ///< P - Power pin
    GROUND          ///< G - Ground pin
};

/**
 * Pin definition within a gate.
 */
struct GATE_PIN
{
    std::string pin_number;             ///< Electrical pin number
    int swap_type = 0;                  ///< Swap type (0 = not swappable)
    PIN_ELEC_TYPE elec_type = PIN_ELEC_TYPE::UNDEFINED;
    std::string func_name;              ///< Optional functional name
};

/**
 * Gate definition for gate-swappable parts.
 */
struct GATE_DEF
{
    int gate_swap_type = 0;             ///< Gate swap type (0 = not swappable)
    std::vector<GATE_PIN> pins;         ///< Pins in this gate
};

struct PART_TYPE
{
    std::string name;
    std::string decal_name;
    std::map<std::string, int> pin_pad_map;       ///< Maps pin name to pad stack index
    std::map<std::string, std::string> attributes; ///< Attribute name-value pairs from {...} block
    std::vector<SIGPIN> signal_pins;               ///< Standard signal pin definitions
    std::vector<GATE_DEF> gates;                   ///< Gate definitions for swap support
};

struct ROUTE
{
    std::string net_name;
    std::vector<TRACK> tracks;
    std::vector<VIA> vias;
    std::vector<NET_PIN> pins;             ///< Pins connected to this net (from pin pair lines)
    std::vector<TEARDROP> teardrops;       ///< Teardrop locations in this route
    std::vector<JUMPER_MARKER> jumpers;    ///< Jumper start/end points in this route
};

struct TEXT
{
    std::string content;
    POINT location;
    double height = 0.0;
    double width = 0.0;
    int layer = 0;
    double rotation = 0.0;
    bool mirrored = false;
    std::string hjust;            ///< Horizontal justification: LEFT, CENTER, RIGHT
    std::string vjust;            ///< Vertical justification: UP, CENTER, DOWN
    int ndim = 0;                 ///< Dimension number for auto-dimensioning text (0 if not used)
    std::string reuse_instance;   ///< Reuse block instance name (if .REUSE. suffix)
    std::string font_style;       ///< Font style (Regular, Bold, Italic, Underline, or combinations)
    double font_height = 0.0;     ///< Font height for text box calculation (optional)
    double font_descent = 0.0;    ///< Font descent for text box calculation (optional)
    std::string font_face;        ///< Font face name
};

struct LINE
{
    int layer;
    double width;
    POINT start;
    POINT end;
};

/**
 * Line style enumeration for 2D graphics.
 */
enum class LINE_STYLE
{
    SOLID = 0,
    DASHED = 1,
    DOTTED = 2,
    DASH_DOTTED = 3,
    DASH_DOUBLE_DOTTED = 4
};

/**
 * A 2D graphic line/shape from the LINES section (type=LINES).
 *
 * These are drawing items like polylines, polygons, and circles used for
 * documentation, assembly drawings, or other non-electrical purposes.
 */
struct GRAPHIC_LINE
{
    std::string name;                   ///< Item name
    int layer = 0;                      ///< Layer number
    double width = 0.0;                 ///< Line width
    LINE_STYLE style = LINE_STYLE::SOLID;  ///< Line style (solid, dashed, etc.)
    bool closed = false;                ///< True if shape is closed (polygon/circle)
    bool filled = false;                ///< True if shape should be filled
    std::vector<ARC_POINT> points;      ///< Shape vertices, may include arcs
    std::string reuse_instance;         ///< Reuse block instance name (if member of reuse)
};

/**
 * A polyline that may contain arc segments.
 *
 * Used for board outlines and complex graphics that can include arc corners.
 */
struct POLYLINE
{
    int layer;
    double width;
    bool closed;                    ///< True if polyline forms a closed shape
    std::vector<ARC_POINT> points;  ///< Polyline vertices, may include arcs
};

struct BOARD_ITEM
{
    // Base for board outline, etc.
};

/**
 * A copper shape from the LINES section (type=COPPER).
 *
 * Copper shapes are standalone copper areas not part of a pour. They can be
 * associated with a net and may include cutouts.
 */
struct COPPER_SHAPE
{
    std::string name;                   ///< Shape name
    std::string net_name;               ///< Associated net (empty if unconnected)
    int layer = 0;                      ///< Layer number
    double width = 0.0;                 ///< Line width (for open polylines)
    bool filled = false;                ///< True for filled shapes (COPCLS, COPCIR)
    bool is_cutout = false;             ///< True for cutouts (COPCUT, COPCCO)
    std::vector<ARC_POINT> outline;     ///< Shape outline vertices
};

/**
 * Layer types from PADS LAYER_TYPE field.
 */
enum class PADS_LAYER_FUNCTION
{
    UNKNOWN,
    ROUTING,        ///< Copper routing layer
    PLANE,          ///< Power/ground plane
    MIXED,          ///< Mixed signal/plane
    UNASSIGNED,     ///< Unassigned layer
    SOLDER_MASK,    ///< Solder mask
    PASTE_MASK,     ///< Solder paste mask
    SILK_SCREEN,    ///< Silkscreen/legend
    ASSEMBLY,       ///< Assembly drawing
    DOCUMENTATION,  ///< Documentation layer
    DRILL           ///< Drill drawing
};

struct LAYER_INFO
{
    int                  number;      ///< PADS layer number
    std::string          name;        ///< Layer name
    PADS_LAYER_FUNCTION  layer_type;  ///< Parsed layer type from file
    bool                 is_copper;   ///< True if copper layer
    bool                 required;    ///< True if layer must be mapped
    double               layer_thickness = 0.0;     ///< Dielectric thickness (BASIC units)
    double               copper_thickness = 0.0;    ///< Copper foil thickness (BASIC units)
    double               dielectric_constant = 0.0; ///< Relative permittivity (Er)
};

/**
 * A reuse block instance placement.
 */
struct REUSE_NET
{
    bool merge = false;        ///< True to merge nets, false to rename
    std::string name;          ///< Original net name from reuse definition
};

struct REUSE_INSTANCE
{
    std::string instance_name;     ///< Instance name
    std::string part_naming;       ///< Part naming scheme (may be multi-word like "PREFIX pref")
    std::string net_naming;        ///< Net naming scheme (may be multi-word like "SUFFIX suf")
    POINT location = {};           ///< Placement location
    double rotation = 0.0;         ///< Rotation angle in degrees
    bool glued = false;            ///< True if glued in place
};

/**
 * A reuse block definition containing parts and routes that can be instantiated.
 */
struct REUSE_BLOCK
{
    std::string name;                           ///< Block type name
    long timestamp = 0;                         ///< Creation/modification timestamp
    std::string part_naming;                    ///< Default part naming scheme
    std::string net_naming;                     ///< Default net naming scheme
    std::vector<std::string> part_names;        ///< Parts contained in this block
    std::vector<REUSE_NET> nets;                ///< Nets contained in this block with merge flags
    std::vector<REUSE_INSTANCE> instances;      ///< Placements of this block
};

/**
 * A cluster of related route segments that should be grouped together.
 */
struct CLUSTER
{
    std::string name;                           ///< Cluster name/identifier
    int id = 0;                                 ///< Cluster ID number
    std::vector<std::string> net_names;         ///< Nets belonging to this cluster
    std::vector<std::string> segment_refs;      ///< References to route segments in cluster
};

/**
 * A test point definition for manufacturing/testing access.
 */
struct TEST_POINT
{
    std::string type;           ///< VIA or PIN
    double x = 0.0;             ///< X coordinate
    double y = 0.0;             ///< Y coordinate
    int side = 0;               ///< Probe side (0=through, 1=top, 2=bottom)
    std::string net_name;       ///< Net this test point connects to
    std::string symbol_name;    ///< Symbol/pad name for the test point
};

/**
 * A dimension annotation for measurement display.
 */
struct DIMENSION
{
    std::string name;                   ///< Dimension identifier
    double x = 0.0;                     ///< Origin X coordinate
    double y = 0.0;                     ///< Origin Y coordinate
    double crossbar_pos = 0.0;          ///< Crossbar position (Y for horizontal, X for vertical)
    bool is_horizontal = true;          ///< True for horizontal dimension
    int layer = 0;                      ///< Layer for dimension graphics
    std::vector<POINT> points;          ///< Dimension geometry points (measurement endpoints)
    std::string text;                   ///< Dimension text/value
    double text_height = 0.0;           ///< Text height
    double text_width = 0.0;            ///< Text width
    double rotation = 0.0;              ///< Text rotation angle
};

/**
 * Keepout types in PADS.
 */
enum class KEEPOUT_TYPE
{
    ALL,        ///< All objects
    ROUTE,      ///< Routing keepout (traces)
    VIA,        ///< Via keepout
    COPPER,     ///< Copper pour keepout
    PLACEMENT   ///< Component placement keepout
};

/**
 * A keepout area definition.
 */
struct KEEPOUT
{
    KEEPOUT_TYPE type = KEEPOUT_TYPE::ALL;  ///< Type of keepout
    std::vector<ARC_POINT> outline;         ///< Keepout boundary
    std::vector<int> layers;                ///< Affected layers (empty = all)
    bool no_traces = true;                  ///< Prohibit traces (R restriction)
    bool no_vias = true;                    ///< Prohibit vias (V restriction)
    bool no_copper = true;                  ///< Prohibit copper pours (C restriction)
    bool no_components = false;             ///< Prohibit component placement (P restriction)
    bool height_restriction = false;        ///< Component height restriction (H restriction)
    double max_height = 0.0;                ///< Maximum component height when height_restriction is true
    bool no_test_points = false;            ///< Prohibit test points (T restriction)
    bool no_accordion = false;              ///< Prohibit accordion flex (A restriction for accordion, not all)
};

class PARSER
{
public:
    PARSER();
    ~PARSER();

    void Parse( const wxString& aFileName );

    const PARAMETERS& GetParameters() const { return m_parameters; }
    const std::vector<PART>& GetParts() const { return m_parts; }
    const std::vector<NET>& GetNets() const { return m_nets; }
    const std::vector<ROUTE>& GetRoutes() const { return m_routes; }
    const std::vector<TEXT>& GetTexts() const { return m_texts; }
    const std::vector<LINE>& GetLines() const { return m_lines; }
    const std::vector<POLYLINE>& GetBoardOutlines() const { return m_board_outlines; }
    const std::vector<POUR>& GetPours() const { return m_pours; }
    const std::map<std::string, VIA_DEF>& GetViaDefs() const { return m_via_defs; }
    const std::map<std::string, PART_DECAL>& GetPartDecals() const { return m_decals; }
    const std::map<std::string, PART_TYPE>& GetPartTypes() const { return m_part_types; }
    const std::map<std::string, std::map<std::string, std::string>>& GetPartInstanceAttrs() const
    {
        return m_part_instance_attrs;
    }
    const std::map<std::string, REUSE_BLOCK>& GetReuseBlocks() const { return m_reuse_blocks; }
    const std::vector<CLUSTER>& GetClusters() const { return m_clusters; }
    const std::vector<TEST_POINT>& GetTestPoints() const { return m_test_points; }
    const std::vector<DIMENSION>& GetDimensions() const { return m_dimensions; }
    const DESIGN_RULES& GetDesignRules() const { return m_design_rules; }
    const std::vector<NET_CLASS_DEF>& GetNetClasses() const { return m_net_classes; }
    const std::vector<DIFF_PAIR_DEF>& GetDiffPairs() const { return m_diff_pairs; }
    const std::vector<KEEPOUT>& GetKeepouts() const { return m_keepouts; }
    const std::vector<JUMPER_DEF>& GetJumperDefs() const { return m_jumper_defs; }
    const std::vector<COPPER_SHAPE>& GetCopperShapes() const { return m_copper_shapes; }
    const std::vector<GRAPHIC_LINE>& GetGraphicLines() const { return m_graphic_lines; }
    const FILE_HEADER& GetFileHeader() const { return m_file_header; }
    bool IsBasicUnits() const { return m_is_basic_units; }

    /**
     * Get layer information for layer mapping dialog.
     *
     * This generates layer info based on the parsed file header information.
     * Call after Parse() to get accurate layer information.
     */
    std::vector<LAYER_INFO> GetLayerInfos() const;

private:
    void parseLine( const std::string& aLine );
    void parseSectionPCB( std::ifstream& aStream );
    void parseSectionPARTS( std::ifstream& aStream );
    void parseSectionNETS( std::ifstream& aStream );
    void parseSectionROUTES( std::ifstream& aStream );
    void parseSectionTEXT( std::ifstream& aStream );
    void parseSectionBOARD( std::ifstream& aStream );
    void parseSectionLINES( std::ifstream& aStream );
    void parseSectionVIA( std::ifstream& aStream );
    void parseSectionPOUR( std::ifstream& aStream );
    void parseSectionPARTDECAL( std::ifstream& aStream );
    void parseSectionPARTTYPE( std::ifstream& aStream );
    void parseSectionREUSE( std::ifstream& aStream );
    void parseSectionCLUSTER( std::ifstream& aStream );
    void parseSectionJUMPER( std::ifstream& aStream );
    void parseSectionTESTPOINT( std::ifstream& aStream );
    void parseSectionNETCLASS( std::ifstream& aStream );
    void parseSectionDIFFPAIR( std::ifstream& aStream );
    void parseSectionLAYERDEFS( std::ifstream& aStream );
    void parseSectionMISC( std::ifstream& aStream );

    // Helper to read next line skipping comments and empty lines
    bool readLine( std::ifstream& aStream, std::string& aLine );
    void pushBackLine( const std::string& aLine );

    PARAMETERS m_parameters;
    std::vector<PART> m_parts;
    std::vector<NET> m_nets;
    std::vector<ROUTE> m_routes;
    std::vector<TEXT> m_texts;
    std::vector<LINE> m_lines;
    std::vector<POLYLINE> m_board_outlines;
    std::vector<POUR> m_pours;
    std::map<std::string, VIA_DEF> m_via_defs;
    std::map<std::string, PART_DECAL> m_decals;
    std::map<std::string, PART_TYPE> m_part_types;

    ///< Per-instance attribute overrides from PART <name> {...} blocks in *PARTTYPE* section
    std::map<std::string, std::map<std::string, std::string>> m_part_instance_attrs;
    std::map<std::string, REUSE_BLOCK> m_reuse_blocks;
    std::vector<CLUSTER> m_clusters;
    std::vector<TEST_POINT> m_test_points;
    std::vector<DIMENSION> m_dimensions;
    DESIGN_RULES m_design_rules;
    std::vector<NET_CLASS_DEF> m_net_classes;
    std::vector<DIFF_PAIR_DEF> m_diff_pairs;
    std::vector<KEEPOUT> m_keepouts;
    std::vector<JUMPER_DEF> m_jumper_defs;       ///< Jumper definitions from *JUMPER* section
    std::vector<COPPER_SHAPE> m_copper_shapes;   ///< Copper shapes from *LINES* section
    std::vector<GRAPHIC_LINE> m_graphic_lines;   ///< 2D graphic lines from *LINES* section
    std::map<int, LAYER_INFO> m_layer_defs;      ///< Parsed layer definitions by layer number
    FILE_HEADER m_file_header;               ///< Parsed file header info
    bool m_is_basic_units = false;

    std::string m_current_section;
    std::optional<std::string> m_pushed_line;
};

} // namespace PADS_IO
