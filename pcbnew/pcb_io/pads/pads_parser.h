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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <wx/string.h>
#include <wx/gdicmn.h>
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
 * Arc as center, radius and angles. Angles are degrees, 0 = +X, CCW positive.
 */
struct ARC
{
    double cx;
    double cy;
    double radius;
    double start_angle;
    double delta_angle;
};

/**
 * A polyline point that may instead be an arc segment.
 *
 * When is_arc is true, arc defines the arc from the previous point to this point's x,y.
 */
struct ARC_POINT
{
    double x;
    double y;
    bool   is_arc;
    ARC    arc;          ///< Only valid when is_arc is true

    ARC_POINT() :
            x( 0 ),
            y( 0 ),
            is_arc( false ),
            arc{ 0, 0, 0, 0, 0 }
    {
    }
    ARC_POINT( double aX, double aY ) :
            x( aX ),
            y( aY ),
            is_arc( false ),
            arc{ 0, 0, 0, 0, 0 }
    {
    }
    ARC_POINT( double aX, double aY, const ARC& aArc ) :
            x( aX ),
            y( aY ),
            is_arc( true ),
            arc( aArc )
    {
    }
};

enum class PADS_FILE_TYPE
{
    PCB,
    LIB_LINE,       ///< Library line items (drafting)
    LIB_SCH_DECAL,
    LIB_PCB_DECAL,  ///< Library footprints
    LIB_PART_TYPE
};

struct FILE_HEADER
{
    std::string product;                ///< POWERPCB, PADS, LAYOUT, PADS-LIBRARY-*
    std::string version;
    std::string units;                  ///< MILS, MM, INCH, BASIC
    std::string mode;
    std::string encoding;
    PADS_FILE_TYPE file_type = PADS_FILE_TYPE::PCB;
};

struct PARAMETERS
{
    UNIT_TYPE units = UNIT_TYPE::MILS;
    int layer_count = 2;
    POINT origin = {0, 0};
    double user_grid = 0.0;                ///< USERGRID

    double thermal_line_width = 30.0;      ///< THERLINEWID, THT
    double thermal_smd_width = 20.0;       ///< THERSMDWID, SMD
    int thermal_flags = 0;                 ///< THERFLAGS
    double thermal_min_clearance = 5.0;    ///< STMINCLEAR
    int thermal_min_spokes = 4;            ///< STMINSPOKES

    double drill_oversize = 0.0;           ///< DRLOVERSIZE, plated holes

    std::string default_signal_via;        ///< VIAPSHVIA
};

struct DESIGN_RULES
{
    double min_clearance = 8.0;           ///< MINCLEAR
    double default_clearance = 10.0;      ///< DEFAULTCLEAR
    double min_track_width = 6.0;         ///< MINTRACKWID
    double default_track_width = 10.0;    ///< DEFAULTTRACKWID
    double min_via_size = 20.0;           ///< MINVIASIZE
    double default_via_size = 40.0;       ///< DEFAULTVIASIZE
    double min_via_drill = 10.0;          ///< MINVIADRILL
    double default_via_drill = 20.0;      ///< DEFAULTVIADRILL
    double hole_to_hole = 10.0;           ///< HOLEHOLE
    double silk_clearance = 5.0;          ///< SILKCLEAR
    double mask_clearance = 3.0;          ///< MASKCLEAR
    double copper_edge_clearance = 10.0;  ///< OUTLINE_TO_*
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
    std::string part_type;                     ///< Used with PARTTYPE@DECAL syntax
    std::vector<std::string> alternate_decals; ///< Remaining decals after ':' splits
    int alt_decal_index = -1;                  ///< ALT placement field, -1 = use primary decal
    std::string value;
    std::string units;
    POINT location;
    double rotation = 0.0;
    bool bottom_layer = false;
    bool glued = false;
    bool explicit_decal = false;               ///< Decal given explicitly with @ syntax
    std::vector<ATTRIBUTE> attributes;
    std::string reuse_instance;
    std::string reuse_part;                    ///< Original part ref des inside the reuse block
};

struct NET_PIN
{
    std::string ref_des;
    std::string pin_name;
    std::string reuse_instance;
    std::string reuse_signal;
};

struct NET
{
    std::string name;
    std::vector<NET_PIN> pins;
    std::vector<std::string> component_refs; ///< Section-24 endpoints whose terminal is unresolved
};

struct NET_CLASS_DEF
{
    std::string name;
    double clearance = 0.0;                 ///< CLEARANCE
    double track_width = 0.0;               ///< TRACKWIDTH
    double via_size = 0.0;                  ///< VIASIZE
    double via_drill = 0.0;                 ///< VIADRILL
    double diff_pair_gap = 0.0;             ///< DIFFPAIRGAP
    double diff_pair_width = 0.0;           ///< DIFFPAIRWIDTH
    std::vector<std::string> net_names;
};

struct DIFF_PAIR_DEF
{
    std::string name;
    std::string positive_net;
    std::string negative_net;
    double gap = 0.0;
    double width = 0.0;
};

struct TEARDROP
{
    double pad_width = 0.0;
    double pad_length = 0.0;
    int pad_flags = 0;
    double net_width = 0.0;
    double net_length = 0.0;
    int net_flags = 0;
    bool has_pad_teardrop = false;
    bool has_net_teardrop = false;
};

struct JUMPER_MARKER
{
    std::string name;
    bool is_start = false;        ///< S = start, E = end
    double x = 0.0;
    double y = 0.0;
};

struct JUMPER_DEF
{
    std::string name;
    bool via_enabled = false;       ///< V flag
    bool wirebond = false;          ///< W flag
    bool display_silk = false;      ///< D flag
    bool glued = false;             ///< G flag
    double min_length = 0.0;
    double max_length = 0.0;
    double length_increment = 0.0;
    std::string padstack;           ///< Start pin, or both if end_padstack empty
    std::string end_padstack;       ///< End pin, optional
    std::vector<ATTRIBUTE> labels;
};

struct TRACK
{
    int layer = 0;
    double width = 0.0;
    std::vector<ARC_POINT> points;
};

struct PAD_STACK_LAYER
{
    int layer = 0;
    std::string shape;       ///< R, S, A, O, OF, RF, RT, ST, RA, SA, RC, OC
    double sizeA = 0.0;      ///< Diameter or width
    double sizeB = 0.0;      ///< Height for rectangles/ovals
    double offsetX = 0.0;
    double offsetY = 0.0;
    double rotation = 0.0;
    double drill = 0.0;      ///< 0 for SMD
    bool plated = true;      ///< PTH vs NPTH
    double inner_diameter = 0.0;   ///< Annular ring, 0 = solid
    double corner_radius = 0.0;    ///< Always positive
    bool chamfered = false;        ///< Negative corner value in PADS
    double finger_offset = 0.0;    ///< Along orientation axis

    double slot_orientation = 0.0; ///< 0-179.999 degrees
    double slot_length = 0.0;
    double slot_offset = 0.0;      ///< From electrical center

    // RT/ST shapes
    double thermal_spoke_orientation = 0.0;  ///< First spoke
    double thermal_outer_diameter = 0.0;     ///< Thermal or void in plane
    double thermal_spoke_width = 0.0;
    int thermal_spoke_count = 0;
};

/**
 * RT and ST rows carry a plane's thermal-relief spoke pattern rather than the pad's own copper.
 */
bool IsThermalReliefPadRow( const PAD_STACK_LAYER& aLayer );

/**
 * RA and SA rows carry a plane's anti-pad clearance rather than the pad's own copper.
 */
bool IsAntiPadRow( const PAD_STACK_LAYER& aLayer );

/**
 * True when the row describes pad or via copper. Relief and anti-pad rows share the layer
 * ordinals of the copper rows they qualify, so every consumer of a padstack has to skip them.
 */
bool IsCopperPadRow( const PAD_STACK_LAYER& aLayer );

enum class VIA_TYPE
{
    THROUGH,    ///< Spans all copper layers
    BLIND,      ///< Surface to inner layer
    BURIED,     ///< Inner layers only
    MICROVIA    ///< Single-layer blind, typically HDI
};

struct VIA_DEF
{
    std::string name;
    double drill = 0.0;
    double size = 0.0;
    std::vector<PAD_STACK_LAYER> stack;
    int start_layer = 0;     ///< First PADS layer in span
    int end_layer = 0;       ///< Last PADS layer in span
    VIA_TYPE via_type = VIA_TYPE::THROUGH;
    int drill_start = 0;     ///< Drill span from file, for blind/buried
    int drill_end = 0;
    bool has_mask_front = false;  ///< Stack includes top soldermask, layer 25
    bool has_mask_back = false;   ///< Stack includes bottom soldermask, layer 28
};

struct VIA
{
    std::string name;
    POINT location;
    std::vector<PAD_STACK_LAYER> stack;
    int                          start_layer = 0;
    int                          end_layer = 0;
};

enum class POUR_STYLE
{
    SOLID,      ///< POUROUT, POLY
    HATCHED,    ///< HATOUT
    VOIDOUT
};

enum class THERMAL_TYPE
{
    NONE,
    PAD,        ///< PADTHERM
    VIA         ///< VIATHERM
};

struct POUR
{
    std::string name;
    std::string net_name;
    int layer = 0;
    int priority = 0;
    double width = 0.0;
    std::vector<ARC_POINT> points;
    bool is_cutout = false;             ///< POCUT piece
    std::string owner_pour;             ///< Parent pour, 7th header field
    POUR_STYLE style = POUR_STYLE::SOLID;
    double hatch_grid = 0.0;
    double hatch_width = 0.0;

    THERMAL_TYPE thermal_type = THERMAL_TYPE::NONE;
    double thermal_spoke_width = 0.0;
    int thermal_spoke_count = 4;
    double thermal_gap = 0.0;
};

struct DECAL_ITEM
{
    std::string type;               ///< CLOSED, OPEN, CIRCLE, COPCLS, TAG, etc.
    int layer = 0;
    double width = 0.0;
    std::vector<ARC_POINT> points;
    int pinnum = -1;                ///< Copper-piece pin association, -1 = none
    std::string restrictions;       ///< KPTCLS/KPTCIR keepout codes R,C,V,T,A
    bool is_tag_open = false;       ///< Opening TAG, level 1
    bool is_tag_close = false;      ///< Closing TAG, level 0
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
    std::map<int, std::pair<int, int>> drill_spans;
};

struct SIGPIN
{
    std::string pin_number;
    double width = 0.0;         ///< Track width for connections
    std::string signal_name;    ///< e.g. VCC, GND
};

enum class PIN_ELEC_TYPE
{
    UNDEFINED,      ///< U
    SOURCE,         ///< S
    BIDIRECTIONAL,  ///< B
    OPEN_COLLECTOR, ///< C, open collector or or-tieable source
    TRISTATE,       ///< T
    LOAD,           ///< L
    TERMINATOR,     ///< Z
    POWER,          ///< P
    GROUND          ///< G
};

struct GATE_PIN
{
    std::string pin_number;
    int swap_type = 0;                  ///< 0 = not swappable
    PIN_ELEC_TYPE elec_type = PIN_ELEC_TYPE::UNDEFINED;
    std::string func_name;
};

struct GATE_DEF
{
    int gate_swap_type = 0;             ///< 0 = not swappable
    std::vector<GATE_PIN> pins;
};

struct PART_TYPE
{
    std::string name;
    std::string decal_name;
    std::map<std::string, int> pin_pad_map;       ///< Pin name to pad stack index
    std::map<std::string, std::string> attributes; ///< From {...} block
    std::vector<SIGPIN> signal_pins;
    std::vector<GATE_DEF> gates;
};

struct ROUTE
{
    std::string net_name;
    std::vector<TRACK> tracks;
    std::vector<VIA> vias;
    std::vector<NET_PIN> pins;             ///< From pin pair lines
    std::vector<TEARDROP> teardrops;
    std::vector<JUMPER_MARKER> jumpers;
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
    std::string hjust;            ///< LEFT, CENTER, RIGHT
    std::string vjust;            ///< UP, CENTER, DOWN
    int ndim = 0;                 ///< Auto-dimensioning text number, 0 if unused
    std::string reuse_instance;
    std::string font_style;       ///< Regular, Bold, Italic, Underline, or combinations
    double font_height = 0.0;
    double font_descent = 0.0;
    std::string font_face;
};

struct LINE
{
    int layer;
    double width;
    POINT start;
    POINT end;
};

enum class LINE_STYLE
{
    SOLID = 0,
    DASHED = 1,
    DOTTED = 2,
    DASH_DOTTED = 3,
    DASH_DOUBLE_DOTTED = 4
};

/**
 * Non-electrical drawing item from the LINES section (type=LINES).
 */
struct GRAPHIC_LINE
{
    std::string name;
    int layer = 0;
    double width = 0.0;
    LINE_STYLE style = LINE_STYLE::SOLID;
    bool closed = false;
    bool filled = false;
    std::vector<ARC_POINT> points;
    std::string reuse_instance;
};

/**
 * A polyline that may contain arc segments, used for board outlines and graphics.
 */
struct POLYLINE
{
    int layer;
    double width;
    bool closed;
    std::vector<ARC_POINT> points;
};

struct BOARD_ITEM
{
    // Base for board outline, etc.
};

/**
 * Standalone copper area from the LINES section (type=COPPER), not part of a pour.
 */
struct COPPER_SHAPE
{
    std::string name;
    std::string net_name;               ///< Empty if unconnected
    int layer = 0;
    double width = 0.0;                 ///< For open polylines
    bool filled = false;                ///< COPCLS, COPCIR
    bool is_cutout = false;             ///< COPCUT, COPCCO
    std::vector<ARC_POINT> outline;
};

enum class PADS_LAYER_FUNCTION
{
    UNKNOWN,
    ROUTING,
    PLANE,
    MIXED,
    UNASSIGNED,
    SOLDER_MASK,
    PASTE_MASK,
    SILK_SCREEN,
    ASSEMBLY,
    DOCUMENTATION,
    DRILL
};

struct LAYER_INFO
{
    int                  number;      ///< PADS layer number
    std::string          name;
    PADS_LAYER_FUNCTION  layer_type;
    bool                 is_copper;
    bool                 required;    ///< Layer must be mapped
    double               layer_thickness = 0.0;     ///< Dielectric, BASIC units
    double               copper_thickness = 0.0;    ///< Copper foil, BASIC units
    double               dielectric_constant = 0.0; ///< Er
    int                  routing_direction = -1;    ///< 0=horizontal, 1=vertical, 2=no preference
};

struct REUSE_NET
{
    bool merge = false;        ///< Merge nets vs rename
    std::string name;          ///< Original net name from reuse definition
};

struct REUSE_INSTANCE
{
    std::string instance_name;
    std::string part_naming;       ///< May be multi-word like "PREFIX pref"
    std::string net_naming;        ///< May be multi-word like "SUFFIX suf"
    POINT location = {};
    double rotation = 0.0;
    bool glued = false;
};

/**
 * A reuse block definition containing parts and routes that can be instantiated.
 */
struct REUSE_BLOCK
{
    std::string name;
    long timestamp = 0;
    std::string part_naming;                    ///< Default scheme
    std::string net_naming;                     ///< Default scheme
    std::vector<std::string> part_names;
    std::vector<REUSE_NET> nets;
    std::vector<REUSE_INSTANCE> instances;       ///< Placements of this block
};

struct CLUSTER
{
    std::string name;
    int id = 0;
    std::vector<std::string> net_names;
    std::vector<std::string> segment_refs;
};

struct TEST_POINT
{
    std::string type;           ///< VIA or PIN
    double x = 0.0;
    double y = 0.0;
    int side = 0;               ///< 0=through, 1=top, 2=bottom
    std::string net_name;
    std::string symbol_name;
};

struct DIMENSION
{
    std::string name;
    double x = 0.0;                     ///< Origin
    double y = 0.0;
    double crossbar_pos = 0.0;          ///< Y for horizontal, X for vertical
    bool is_horizontal = true;
    int layer = 0;
    std::vector<POINT> points;          ///< Measurement endpoints
    std::string text;
    double text_height = 0.0;
    double text_width = 0.0;
    double rotation = 0.0;
};

enum class KEEPOUT_TYPE
{
    ALL,
    ROUTE,
    VIA,
    COPPER,
    PLACEMENT
};

struct KEEPOUT
{
    KEEPOUT_TYPE type = KEEPOUT_TYPE::ALL;
    std::vector<ARC_POINT> outline;
    std::vector<int> layers;                ///< Empty = all
    bool no_traces = true;                  ///< R restriction
    bool no_vias = true;                    ///< V restriction
    bool no_copper = true;                  ///< C restriction
    bool no_components = false;             ///< P restriction
    bool height_restriction = false;        ///< H restriction
    double max_height = 0.0;
    bool no_test_points = false;            ///< T restriction
    bool no_accordion = false;              ///< A restriction
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
     * Get layer information for the layer mapping dialog. Call after Parse().
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
    void clampDesignRuleSentinels();

    // Read next line, skipping comments and empty lines
    bool readLine( std::ifstream& aStream, std::string& aLine );
    void pushBackLine( const std::string& aLine );

    /**
     * Parse the major version number from the file header version string.
     *
     * PADS version strings take the form "Vn.n" where n is a major version.
     * The sequence is V3..V5, then V2003..V2007, V9, V9.4, V9.5, V10, etc.
     * Returns 0 if parsing fails.
     */
    int parseMajorVersion() const;

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

    ///< Per-instance attribute overrides from PART <name> {...} blocks
    std::map<std::string, std::map<std::string, std::string>> m_part_instance_attrs;
    std::map<std::string, REUSE_BLOCK> m_reuse_blocks;
    std::vector<CLUSTER> m_clusters;
    std::vector<TEST_POINT> m_test_points;
    std::vector<DIMENSION> m_dimensions;
    DESIGN_RULES m_design_rules;
    std::vector<NET_CLASS_DEF> m_net_classes;
    std::vector<DIFF_PAIR_DEF> m_diff_pairs;
    std::vector<KEEPOUT> m_keepouts;
    std::vector<JUMPER_DEF> m_jumper_defs;
    std::vector<COPPER_SHAPE> m_copper_shapes;
    std::vector<GRAPHIC_LINE> m_graphic_lines;
    std::map<int, LAYER_INFO> m_layer_defs;      ///< Keyed by layer number
    FILE_HEADER m_file_header;
    bool m_is_basic_units = false;
    bool m_has_font_lines = true;            ///< True if text/label entries include a font line

    std::string m_current_section;
    std::optional<std::string> m_pushed_line;
};

} // namespace PADS_IO
