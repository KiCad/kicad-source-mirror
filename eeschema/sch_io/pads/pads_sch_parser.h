/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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
#include <set>
#include <optional>

#include <math/vector2d.h>

class REPORTER;


namespace PADS_SCH
{

enum class UNIT_TYPE
{
    MILS,
    METRIC,
    INCHES
};


struct FILE_HEADER
{
    std::string product;
    std::string version;
    std::string codepage;
    std::string description;
    bool        valid = false;
};


struct SHEET_SIZE
{
    double      width = 11000.0;
    double      height = 8500.0;
    std::string name;
};


struct PARAMETERS
{
    UNIT_TYPE   units = UNIT_TYPE::MILS;
    double      grid_x = 100.0;
    double      grid_y = 100.0;
    std::string border_template;
    std::string job_name;
    SHEET_SIZE  sheet_size;
    double      text_size = 60.0;
    double      line_width = 1.0;

    int         cur_sheet = 0;
    int         conn_width = 0;
    int         bus_width = 0;
    int         bus_angle = 0;
    int         pin_name_h = 0;
    int         pin_name_w = 0;
    int         ref_name_h = 0;
    int         ref_name_w = 0;
    int         part_name_h = 0;
    int         part_name_w = 0;
    int         pin_no_h = 0;
    int         pin_no_w = 0;
    int         net_name_h = 0;
    int         net_name_w = 0;
    int         text_h = 0;
    int         text_w = 0;
    int         dot_grid = 0;
    int         tied_dot_size = 0;
    int         real_width = 0;
    std::string font_mode;
    std::string default_font;

    std::map<std::string, std::string> fields;
};


/// Midpoint of the arc through @p aStart and @p aEnd about @p aCenter, on the minor-arc side
/// (the perpendicular bisector intersection); callers flip it to the major-arc side per their
/// own sweep convention.
VECTOR2I padsSchArcMidpoint( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter );


struct POINT
{
    double x = 0.0;
    double y = 0.0;
};


struct ARC_DATA
{
    double bulge = 0.0;
    double angle = 0.0;
    double bbox_x1 = 0.0;
    double bbox_y1 = 0.0;
    double bbox_x2 = 0.0;
    double bbox_y2 = 0.0;
};


struct GRAPHIC_POINT
{
    POINT                     coord;
    std::optional<ARC_DATA>   arc;
};


enum class PIN_TYPE
{
    PASSIVE,
    INPUT,
    OUTPUT,
    BIDIRECTIONAL,
    TRISTATE,
    OPEN_COLLECTOR,
    OPEN_EMITTER,
    POWER,
    UNSPECIFIED
};


struct SYMBOL_PIN
{
    std::string name;
    std::string number;
    POINT       position;
    PIN_TYPE    type = PIN_TYPE::UNSPECIFIED;
    double      length = 200.0;
    double      rotation = 0.0;
    bool        inverted = false;
    bool        clock = false;

    int         side = 0;
    int         pn_h = 0;
    int         pn_w = 0;
    int         pn_angle = 0;
    int         pn_just = 0;
    int         pl_h = 0;
    int         pl_w = 0;
    int         pl_angle = 0;
    int         pl_just = 0;
    std::string pin_decal_name;

    POINT       pn_offset;
    int         pn_off_angle = 0;
    int         pn_off_just = 0;
    POINT       pl_offset;
    int         pl_off_angle = 0;
    int         pl_off_just = 0;
    int         p_flags = 0;
};


enum class GRAPHIC_TYPE
{
    LINE,
    RECTANGLE,
    CIRCLE,
    ARC,
    POLYLINE
};


struct SYMBOL_GRAPHIC
{
    GRAPHIC_TYPE        type = GRAPHIC_TYPE::LINE;
    double              line_width = 0.0;
    bool                filled = false;
    int                 line_style = 255;
    std::vector<GRAPHIC_POINT> points;
    POINT               center;
    double              radius = 0.0;
    double              start_angle = 0.0;
    double              end_angle = 0.0;
};


struct SYMBOL_TEXT
{
    std::string content;
    POINT       position;
    double      size = 60.0;
    double      rotation = 0.0;
    bool        visible = true;
    int         justification = 0;
    int         width_factor = 0;
    int         attr_flag = 0;
    std::string font_name;
};


struct CAEDECAL_ATTR
{
    POINT       position;
    int         angle = 0;
    int         justification = 0;
    int         height = 0;
    int         width = 0;
    int         visibility = 0;
    std::string font_name;
    std::string attr_name;
};


struct SYMBOL_DEF
{
    std::string                 name;
    std::string                 timestamp;
    int                         gate_count = 1;
    int                         current_gate = 1;
    std::vector<SYMBOL_PIN>     pins;
    std::vector<SYMBOL_GRAPHIC> graphics;
    std::vector<SYMBOL_TEXT>    texts;

    int         f1 = 0;
    int         f2 = 0;
    int         height = 0;
    int         width = 0;
    int         h2 = 0;
    int         w2 = 0;
    int         num_attrs = 0;
    int         num_pieces = 0;
    int         has_polarity = 0;
    int         num_pins = 0;
    int         pin_origin_code = 0;
    int         is_pin_decal = 0;

    std::string font1;
    std::string font2;
    std::vector<CAEDECAL_ATTR> attrs;
};


struct PART_ATTRIBUTE
{
    std::string name;
    std::string value;
    POINT       position;
    double      rotation = 0.0;
    double      size = 60.0;
    bool        visible = true;

    int         justification = 0;
    int         height = 0;
    int         width = 0;
    int         visibility = 0;
    std::string font_name;
};


struct PART_PLACEMENT
{
    std::string reference;
    std::string symbol_name;
    std::string part_type;
    POINT       position;
    double      rotation = 0.0;
    int         mirror_flags = 0;
    std::string power_net_name;
    int         sheet_number = 1;
    int         gate_number = 1;
    std::vector<PART_ATTRIBUTE> attributes;

    int         h1 = 0;
    int         w1 = 0;
    int         h2 = 0;
    int         w2 = 0;
    int         num_attrs = 0;
    int         num_displayed_values = 0;
    int         num_pins = 0;
    int         gate_index = 0;
    int         pin_origin_code = 0;
    std::string font1;
    std::string font2;

    std::map<std::string, std::string> attr_overrides;

    struct PIN_OVERRIDE
    {
        int height = 0;
        int width = 0;
        int angle = 0;
        int justification = 0;
    };

    std::vector<PIN_OVERRIDE> pin_overrides;
};


/**
 * Wire segment connecting two endpoints through coordinate vertices.
 */
struct WIRE_SEGMENT
{
    POINT start;
    POINT end;
    int   sheet_number = 1;

    std::string             endpoint_a;
    std::string             endpoint_b;
    int                     vertex_count = 0;
    int                     flags = 0;
    std::vector<POINT>      vertices;
};


struct PIN_CONNECTION
{
    std::string reference;
    std::string pin_number;
    int         sheet_number = 1;
};


struct SCH_SIGNAL
{
    std::string                 name;
    std::vector<WIRE_SEGMENT>   wires;
    std::vector<PIN_CONNECTION> connections;

    int         flags1 = 0;
    int         flags2 = 0;
    std::string function;
};


struct OFF_PAGE_CONNECTOR
{
    std::string signal_name;
    int         source_sheet = 1;
    int         target_sheet = 1;
    POINT       position;

    int         id = 0;
    std::string symbol_lib;
    int         rotation = 0;
    int         flags1 = 0;
    int         flags2 = 0;
};


struct BUS_ENTRY
{
    std::string member_net;
    POINT       position;
    int         rotation = 0;
};


struct BUS_DEF
{
    std::string              handle;
    std::string              name;
    int                      sheet_number = 1;
    std::vector<POINT>       path;
    std::vector<BUS_ENTRY>   entries;
    std::vector<std::string> aliases;
    std::vector<std::string> member_nets;
};


struct SHEET_DEF
{
    int         sheet_number = 1;
    std::string name;
    SHEET_SIZE  size;
};


struct SHEET_HEADER
{
    int         sheet_num = 0;
    std::string sheet_name;
    int         parent_num = -1;
    std::string parent_name;
};


struct TIED_DOT
{
    int   id = 0;
    POINT position;
    int   sheet_number = 1;
};


struct TEXT_ITEM
{
    POINT       position;
    int         rotation = 0;
    int         justification = 0;
    int         height = 0;
    int         width_factor = 0;
    int         attr_flag = 0;
    int         sheet_number = 0;
    std::string font_name;
    std::string content;
};


struct LINES_ITEM
{
    std::string                 name;
    POINT                       origin;
    int                         param1 = 0;
    int                         param2 = 0;
    int                         sheet_number = 0;
    std::vector<SYMBOL_GRAPHIC> primitives;
    std::vector<TEXT_ITEM>      texts;
};


struct NETNAME_LABEL
{
    std::string net_name;
    std::string anchor_ref;
    int         x_offset = 0;
    int         y_offset = 0;
    int         rotation = 0;
    int         justification = 0;
    int         f3 = 0;
    int         f4 = 0;
    int         f5 = 0;
    int         f6 = 0;
    int         f7 = 0;
    int         height = 0;
    int         width_pct = 0;
    std::string font_name;
};


struct PARTTYPE_PIN
{
    std::string pin_id;
    int         swap_group = 0;
    char        pin_type = 'U';
    std::string pin_name;
};


struct GATE_DEF
{
    int                         num_decal_variants = 0;
    int                         num_pins = 0;
    int                         swap_flag = 0;
    std::vector<std::string>    decal_names;
    std::vector<PARTTYPE_PIN>   pins;
};


struct PARTTYPE_DEF
{
    std::string                 name;
    std::string                 category;
    int                         num_physical = 0;
    int                         num_sigpins = 0;
    int                         unused = 0;
    int                         num_swap_groups = 0;
    std::string                 timestamp;

    std::vector<GATE_DEF>       gates;

    std::string                 special_keyword;
    struct SPECIAL_VARIANT
    {
        std::string decal_name;
        std::string pin_type;
        std::string net_suffix;
    };

    std::vector<SPECIAL_VARIANT> special_variants;

    bool                        is_connector = false;

    struct SIGPIN
    {
        std::string pin_number;
        std::string net_name;
    };
    std::vector<SIGPIN>         sigpins;

    std::vector<std::string>    swap_lines;
};


/**
 * Parser for PADS Logic ASCII schematic export files.
 */
class PADS_SCH_PARSER
{
public:
    PADS_SCH_PARSER();
    ~PADS_SCH_PARSER();

    void SetReporter( REPORTER* aReporter ) { m_reporter = aReporter; }

    bool Parse( const std::string& aFileName );

    static bool CheckFileHeader( const std::string& aFileName );

    static PIN_TYPE ParsePinTypeChar( char aTypeChar );

    const FILE_HEADER& GetHeader() const { return m_header; }
    const PARAMETERS& GetParameters() const { return m_parameters; }

    const std::vector<SYMBOL_DEF>& GetSymbolDefs() const { return m_symbolDefs; }
    const SYMBOL_DEF* GetSymbolDef( const std::string& aName ) const;

    const std::vector<PART_PLACEMENT>& GetPartPlacements() const { return m_partPlacements; }
    const PART_PLACEMENT* GetPartPlacement( const std::string& aReference ) const;

    const std::vector<SCH_SIGNAL>& GetSignals() const { return m_signals; }
    const SCH_SIGNAL* GetSignal( const std::string& aName ) const;

    std::string GetVersion() const { return m_header.version; }
    bool IsValid() const { return m_header.valid; }

    int GetSheetCount() const;
    std::set<int> GetSheetNumbers() const;

    const std::vector<OFF_PAGE_CONNECTOR>& GetOffPageConnectors() const { return m_offPageConnectors; }
    const std::vector<BUS_DEF>&            GetBuses() const { return m_buses; }

    std::vector<SCH_SIGNAL> GetSignalsOnSheet( int aSheetNumber ) const;
    std::vector<PART_PLACEMENT> GetPartsOnSheet( int aSheetNumber ) const;

    const std::map<std::string, PARTTYPE_DEF>& GetPartTypes() const { return m_partTypes; }
    const std::vector<TIED_DOT>& GetTiedDots() const { return m_tiedDots; }
    const std::vector<SHEET_HEADER>& GetSheetHeaders() const { return m_sheetHeaders; }
    const std::vector<TEXT_ITEM>& GetTextItems() const { return m_textItems; }
    const std::vector<LINES_ITEM>& GetLinesItems() const { return m_linesItems; }
    const std::vector<NETNAME_LABEL>& GetNetNameLabels() const { return m_netNameLabels; }

private:
    bool parseHeader( const std::string& aLine );

    size_t parseSectionSCH( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionFIELDS( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionSHT( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionCAE( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionTEXT( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionLINES( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionCAEDECAL( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionPARTTYPE( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionPART( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionBUSSES( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionOFFPAGEREFS( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionTIEDOTS( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionCONNECTION( const std::vector<std::string>& aLines, size_t aStartLine );
    size_t parseSectionNETNAMES( const std::vector<std::string>& aLines, size_t aStartLine );

    size_t skipBraceDelimitedSection( const std::vector<std::string>& aLines, size_t aStartLine );

    size_t parseSymbolDef( const std::vector<std::string>& aLines, size_t aStartLine, SYMBOL_DEF& aSymbol );
    size_t parsePartPlacement( const std::vector<std::string>& aLines, size_t aStartLine, PART_PLACEMENT& aPart );
    size_t parseSignalDef( const std::vector<std::string>& aLines, size_t aStartLine, SCH_SIGNAL& aSignal );
    size_t parseGraphicPrimitive( const std::vector<std::string>& aLines, size_t aStartLine, SYMBOL_GRAPHIC& aGraphic );

    void mergePartTypeData();

    PIN_TYPE parsePinType( const std::string& aTypeStr );

    bool isSectionMarker( const std::string& aLine ) const;
    std::string extractSectionName( const std::string& aLine ) const;

    REPORTER*                               m_reporter;
    FILE_HEADER                             m_header;
    PARAMETERS                              m_parameters;
    std::vector<SYMBOL_DEF>                 m_symbolDefs;
    std::vector<PART_PLACEMENT>             m_partPlacements;
    std::vector<SCH_SIGNAL>                 m_signals;
    std::vector<OFF_PAGE_CONNECTOR>         m_offPageConnectors;
    std::vector<BUS_DEF>                m_buses;
    int                                     m_lineNumber;
    int                                     m_currentSheet;
    std::map<std::string, PARTTYPE_DEF>     m_partTypes;
    std::vector<TIED_DOT>                   m_tiedDots;
    std::vector<SHEET_HEADER>               m_sheetHeaders;
    std::vector<TEXT_ITEM>                   m_textItems;
    std::vector<LINES_ITEM>                 m_linesItems;
    std::vector<NETNAME_LABEL>              m_netNameLabels;
};

} // namespace PADS_SCH
