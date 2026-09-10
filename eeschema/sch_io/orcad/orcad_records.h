/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/** Coordinates use DBU with Y down. Convert raw Windows-1252 strings with FromOrcadString. */

#ifndef ORCAD_RECORDS_H_
#define ORCAD_RECORDS_H_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <wx/string.h>

/** Warning sink shared by all parser entry points (recoverable-issue channel). */
using ORCAD_WARN_FN = std::function<void( const wxString& aMsg )>;


enum ORCAD_ST : int
{
    ORCAD_ST_STH_IN_PAGES0 = 2, ///< nested symbol body inside Graphic*Inst
    ORCAD_ST_DSN_STREAM = 4,
    ORCAD_ST_PART_CELL = 6,
    ORCAD_ST_SCH_LIB = 9,
    ORCAD_ST_PAGE = 10,
    ORCAD_ST_PART_INSTANCE = 11,
    ORCAD_ST_DRAWN_INSTANCE = 12, ///< hierarchical block instance
    ORCAD_ST_PLACED_INSTANCE = 13,
    ORCAD_ST_T0X10 = 16, ///< scalar pin instance (absolute pos)
    ORCAD_ST_T0X11 = 17, ///< bus pin instance (absolute pos)
    ORCAD_ST_WIRE_SCALAR = 20,
    ORCAD_ST_WIRE_BUS = 21,
    ORCAD_ST_PORT = 23,
    ORCAD_ST_LIBRARY_PART = 24,
    ORCAD_ST_SYMBOL_PIN_SCALAR = 26,
    ORCAD_ST_SYMBOL_PIN_BUS = 27,
    ORCAD_ST_BUS_ENTRY = 29,
    ORCAD_ST_PACKAGE = 31,
    ORCAD_ST_DEVICE = 32,
    ORCAD_ST_GLOBAL_SYMBOL = 33, ///< power symbol definition
    ORCAD_ST_PORT_SYMBOL = 34,
    ORCAD_ST_OFFPAGE_SYMBOL = 35,
    ORCAD_ST_GLOBAL = 37, ///< placed power symbol
    ORCAD_ST_OFFPAGE_CONNECTOR = 38,
    ORCAD_ST_SYMBOL_DISPLAY_PROP = 39,
    ORCAD_ST_SYMBOL_VECTOR = 48,
    ORCAD_ST_ALIAS = 49, ///< net alias attached to a wire
    ORCAD_ST_T0X34 = 52, ///< raw record, not prefix-framed
    ORCAD_ST_T0X35 = 53, ///< raw record, not prefix-framed
    ORCAD_ST_GRAPHIC_BOX_INST = 55,
    ORCAD_ST_GRAPHIC_LINE_INST = 56,
    ORCAD_ST_GRAPHIC_ARC_INST = 57,
    ORCAD_ST_GRAPHIC_ELLIPSE_INST = 58,
    ORCAD_ST_GRAPHIC_POLYGON_INST = 59,
    ORCAD_ST_GRAPHIC_POLYLINE_INST = 60,
    ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST = 61,
    ORCAD_ST_GRAPHIC_BITMAP_INST = 62,
    ORCAD_ST_TITLEBLOCK_SYMBOL = 64,
    ORCAD_ST_TITLEBLOCK = 65,
    ORCAD_ST_HIERARCHY_LINK = 66, ///< block dbId -> child folder link
    ORCAD_ST_ERC_SYMBOL = 75,
    ORCAD_ST_BOOKMARK_SYMBOL = 76,
    ORCAD_ST_ERC_OBJECT = 77, ///< saved design-rule-check marker
    ORCAD_ST_BOOKMARK_INST = 78,
    ORCAD_ST_GRAPHIC_BEZIER_INST = 88,
    ORCAD_ST_GRAPHIC_OLE_INST = 89,
    ORCAD_ST_PIN_SHAPE_SYMBOL = 98,
    ORCAD_ST_NET_GROUP = 103
};


/** Primitive types appear as a doubled byte pair before each body. */
enum ORCAD_PRIM : int
{
    ORCAD_PRIM_RECT = 40,
    ORCAD_PRIM_LINE = 41,
    ORCAD_PRIM_ARC = 42,
    ORCAD_PRIM_ELLIPSE = 43,
    ORCAD_PRIM_POLYGON = 44,
    ORCAD_PRIM_POLYLINE = 45,
    ORCAD_PRIM_COMMENT_TEXT = 46,
    ORCAD_PRIM_BITMAP = 47,        ///< plain DIB (BITMAPINFOHEADER + pixels)
    ORCAD_PRIM_SYMBOL_VECTOR = 48, ///< nested prefix-framed vector graphic
    ORCAD_PRIM_BEZIER = 87,
    ORCAD_PRIM_OLE_IMAGE = 90 ///< OLE compound document embed
};


/** Decoded primitive kind after parsing. */
enum class ORCAD_PRIM_KIND
{
    GROUP_PRIM,
    RECTANGLE,
    LINE,
    ARC,
    ELLIPSE,
    POLYGON,
    POLYLINE,
    BEZIER,
    TEXT,
    IMAGE
};


/** Map unknown electrical type codes to PASSIVE. */
enum class ORCAD_PORT_TYPE : int
{
    INPUT_TYPE = 0,
    BIDIRECTIONAL = 1,
    OUTPUT = 2,
    OPEN_COLLECTOR = 3,
    PASSIVE = 4,
    TRI_STATE = 5,
    OPEN_EMITTER = 6,
    POWER_IN = 7
};


/** Integer point in OrCAD DBU. */
struct ORCAD_POINT
{
    int x = 0;
    int y = 0;

    bool operator==( const ORCAD_POINT& aOther ) const { return x == aOther.x && y == aOther.y; }
    bool operator!=( const ORCAD_POINT& aOther ) const { return !( *this == aOther ); }
};


/** Axis-aligned box in OrCAD DBU; corner order as stored (not normalized). */
struct ORCAD_BBOX
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
};


/** Display positions use symbol coordinates; rotFont combines the font index and quarter turns. */
struct ORCAD_DISPLAY_PROP
{
    uint32_t    nameIdx = 0;
    std::string name; ///< resolved property name (empty when index invalid)
    int         x = 0;
    int         y = 0;
    int         rotation = 0; ///< 0..3 quarter turns
    int         fontIdx = 0;  ///< 1-based into ORCAD_LIBRARY_INFO::fonts; 0 = default
    int         color = 0;
    int         dispMode = 0;
};


/** The owning wire defines the connection. The stored position controls only the display. */
struct ORCAD_ALIAS
{
    std::string name;
    int         x = 0;
    int         y = 0;
    int         rotation = 0; ///< 0..3 quarter turns
    int         color = 0;
    int         fontIdx = 0;
};


/** The wire ID refers to the page net table. */
struct ORCAD_WIRE
{
    uint32_t                 dbId = 0;
    uint32_t                 id = 0;
    int                      x1 = 0;
    int                      y1 = 0;
    int                      x2 = 0;
    int                      y2 = 0;
    bool                     isBus = false; ///< true for structure type 21
    int                      color = 0;
    int                      lineWidth = 3;
    int                      lineStyle = 5;
    std::vector<ORCAD_ALIAS> aliases;
};


/** Pin positions are absolute page connection points. Negative indices mark no-connects. */
struct ORCAD_PIN_INST
{
    int16_t                         pinIndex = 0;
    int                             x = 0;
    int                             y = 0;
    uint32_t                        wordA = 0; ///< two flag/id words after (x, y)
    uint32_t                        wordB = 0;
    std::vector<ORCAD_DISPLAY_PROP> displayProps;

    bool IsNoConnect() const { return pinIndex < 0; }
};


/** The placed box includes displayed text. Use the cache definition for body bounds. */
struct ORCAD_PLACED_INSTANCE
{
    std::string                        pkgName;
    uint32_t                           dbId = 0;
    int                                x = 0;
    int                                y = 0;
    std::string                        reference;
    std::string                        value; ///< resolved Part Value
    std::map<std::string, std::string> props; ///< short-prefix property pairs
    std::vector<ORCAD_DISPLAY_PROP>    displayProps;
    int                                color = 0;
    int                                rotation = 0;   ///< 0..3 quarter turns
    bool                               mirror = false; ///< orientation bit 2
    uint8_t                            partIndex = 0;
    uint8_t                            partByte = 0;
    ORCAD_BBOX                         bbox;          ///< placed box, page DBU
    std::string                        sourcePackage; ///< package base name
    std::string                        sourceLibrary; ///< source library path from the placed-instance header
    uint16_t                           unitIndex = 0; ///< zero-based package device index
    std::vector<ORCAD_PIN_INST>        pins;          ///< successfully parsed T0x10 records
};


struct ORCAD_SYMBOL_DEF; // defined below (ORCAD_GRAPHIC_INST holds one by pointer)


/** Free graphics use nested primitive coordinates. Ports and power symbols use the box minimum as the base. */
struct ORCAD_GRAPHIC_INST
{
    int                                typeId = 0; ///< ORCAD_ST value
    std::string                        name;       ///< cache symbol name
    uint32_t                           dbId = 0;
    int                                x = 0;
    int                                y = 0;
    ORCAD_BBOX                         bbox;
    int                                color = 0;
    std::map<std::string, std::string> props;
    std::vector<ORCAD_DISPLAY_PROP>    displayProps;
    std::unique_ptr<ORCAD_SYMBOL_DEF>  nested;       ///< SthInPages0 body, else nullptr
    int                                rotation = 0; ///< 0..3 quarter turns
    bool                               mirror = false;
    std::string                        logicalName; ///< ports: resolved net/port name
    double                             textScaleX = 1.0;
    double                             textScaleY = 1.0;
    bool                               useGenericTextBaseline = false;
    bool                               useSymbolLineWidths = false;
    std::string                        textFaceOverride;
};


/** Primitive byte lengths can include or exclude the eight-byte size envelope.
 * See orcad_dsn.ksy for each body layout. */
struct ORCAD_PRIMITIVE
{
    ORCAD_PRIM_KIND              kind = ORCAD_PRIM_KIND::LINE;
    int                          x1 = 0;
    int                          y1 = 0;
    int                          x2 = 0;
    int                          y2 = 0;
    std::optional<ORCAD_POINT>   start;  ///< arc start point
    std::optional<ORCAD_POINT>   end;    ///< arc end point
    std::vector<ORCAD_POINT>     points; ///< polygon/polyline/bezier vertices
    std::string                  text;   ///< kind == TEXT
    int                          fontIdx = 0;
    std::optional<ORCAD_POINT>   textBoundsStart;
    int                          lineStyle = 5; ///< 0 solid, 1 dash, 2 dot, 3 dash-dot, 4 dash-dot-dot, 5 default
    int                          lineWidth = 3; ///< Capture width enum: 0 thin, 1 medium, 2 wide, 3 default
    int                          fillStyle = 1; ///< 0 solid, 1 none, 2 hatch pattern
    int                          hatchStyle = 0;
    std::vector<uint8_t>         data;     ///< kind == IMAGE: raw embedded payload
    std::vector<ORCAD_PRIMITIVE> children; ///< kind == GROUP, translated by (x1, y1)
};


/** Pin coordinates use symbol space with Y down. A zero prefix byte marks an empty slot. */
struct ORCAD_SYMBOL_PIN
{
    std::string     name;
    int             position = -1; ///< slot in the parent symbol pin vector
    int             startX = 0;
    int             startY = 0;
    int             hotptX = 0;
    int             hotptY = 0;
    ORCAD_PORT_TYPE portType = ORCAD_PORT_TYPE::PASSIVE;
    int             shapeBits = 0;
    std::vector<ORCAD_DISPLAY_PROP> displayProps;
};


/** The bounding box occupies the final eight bytes before the next prefix stop. */
struct ORCAD_SYMBOL_DEF
{
    int                                typeId = 0; ///< ORCAD_ST value
    std::string                        name;       ///< cache name, e.g. "C.Normal"
    std::string                        sourceLib;
    int                                color = 0;
    std::vector<ORCAD_PRIMITIVE>       primitives;
    std::vector<ORCAD_SYMBOL_PIN>      pins;
    std::optional<ORCAD_BBOX>          bbox; ///< symbol-space body box
    std::map<std::string, std::string> props;

    /** Variant zero is this entry. Later variants retain cache order and are selected by placed pin positions. */
    std::vector<ORCAD_SYMBOL_DEF> variants;

    /** LibraryPart GeneralProperties flags (-1 = absent); bit0 = pin names visible,
     * bit1 = pin text rotates with vertical pins, bit2 = pin numbers hidden. */
    int generalFlags = -1;

    bool synthesized = false; ///< placeholder built from T0x10 data
};


/** Device unit names omit the view suffix. FF FF marks an empty, ignored pin. */
struct ORCAD_DEVICE
{
    std::string              unitRef;
    std::string              refDes;
    std::vector<std::string> pinNumbers;
    std::vector<bool>        pinIgnore;
};


struct ORCAD_PACKAGE
{
    std::string                name;
    std::string                sourceLib;
    std::string                refDes;
    std::string                pcbFootprint;
    std::vector<ORCAD_DEVICE>  devices;
    std::vector<ORCAD_PACKAGE> variants; ///< later same-name cache entries in stream order

    /** Part-level properties shared by every placement (Description, Tolerance, ...).
     * Placements carry only their own overrides, so these must be merged in. */
    std::map<std::string, std::string> props;
};


/** One interface pin of a hierarchical block, at its absolute page position. */
struct ORCAD_BLOCK_PIN
{
    std::string     name;
    ORCAD_PORT_TYPE portType = ORCAD_PORT_TYPE::PASSIVE;
    int             x = 0;
    int             y = 0;
    bool            noConnect = false;
};


/** The inline LibraryPart defines the block interface; placed pin records supply absolute positions. */
struct ORCAD_DRAWN_INSTANCE
{
    uint32_t                        dbId = 0;
    std::string                     name;      ///< intrinsic Name property used for flat-net scoping
    std::string                     reference;
    std::map<std::string, std::string> props;
    int                             x1 = 0; ///< block rectangle top-left, page DBU
    int                             y1 = 0;
    int                             w = 0;
    int                             h = 0;
    std::vector<ORCAD_BLOCK_PIN>    pins;
    std::vector<ORCAD_DISPLAY_PROP> displayProps;
    std::string                     childName; ///< child folder, when embedded
};


struct ORCAD_BUS_ENTRY
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int color = 0;
};


/** Font indices are one-based; zero selects the default. Heights use negative device units. */
struct ORCAD_FONT
{
    int         height = 0; ///< raw lfHeight (typically negative)
    int         width = 0;  ///< raw lfWidth; zero lets the font choose its natural aspect ratio
    int         escapement = 0;  ///< baseline direction in tenths of a degree
    int         orientation = 0; ///< glyph direction in tenths of a degree
    uint8_t     pitchAndFamily = 0;
    std::string face;
    bool        italic = false;
    bool        bold = false; ///< lfWeight >= 600
};


/** Dimensions use mils when isMetric is zero, otherwise micrometres. */
struct ORCAD_PAGE_SETTINGS
{
    uint32_t createTimestamp = 0;
    uint32_t modifyTimestamp = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t pinToPin = 100;
    uint16_t horizontalCount = 0;
    uint16_t verticalCount = 0;
    uint32_t horizontalWidth = 0;
    uint32_t verticalWidth = 0;
    bool     horizontalChar = false;
    bool     horizontalAscending = false;
    bool     verticalChar = false;
    bool     verticalAscending = false;
    bool     isMetric = false;
    bool     borderDisplayed = false;
    bool     borderPrinted = false;
    bool     gridRefDisplayed = false;
    bool     gridRefPrinted = false;
    bool     titleblockDisplayed = false;
    bool     titleblockPrinted = false;
    bool     ansiGridRefs = false;
};


struct ORCAD_NET_GROUP
{
    uint32_t              id = 0;
    std::string           name;
    std::vector<uint32_t> members;
};


struct ORCAD_LIBRARY_INFO
{
    std::string                                      introduction; ///< fixed 32-byte NUL-terminated buffer
    int                                              versionMajor = 0;
    int                                              versionMinor = 0;
    uint32_t                                         createTimestamp = 0;
    uint32_t                                         modifyTimestamp = 0;
    std::vector<ORCAD_FONT>                          fonts;
    std::vector<int>                                 templateFonts; ///< template font ID -> 1-based LOGFONT index
    int                                              pinNameFont = 0; ///< Design Template font ID (slot 10)
    int                                              pinNumberFont = 0; ///< Design Template font ID (slot 11)
    std::vector<std::string>                         partFields; ///< the 8 named part fields
    std::vector<std::string>                         strings;    ///< global string table
    std::vector<std::pair<std::string, std::string>> partAliases;
    std::string                                      schematicName; ///< root schematic folder name
    uint32_t                                         pinToPin = 100;
};


/** Wire IDs refer to netmap, which supplies the source net names. */
struct ORCAD_RAW_PAGE
{
    /** Explicit moves prevent vector growth from selecting a copy of the unique_ptr members. */
    ORCAD_RAW_PAGE() = default;
    ORCAD_RAW_PAGE( const ORCAD_RAW_PAGE& ) = delete;
    ORCAD_RAW_PAGE& operator=( const ORCAD_RAW_PAGE& ) = delete;
    ORCAD_RAW_PAGE( ORCAD_RAW_PAGE&& ) = default;
    ORCAD_RAW_PAGE& operator=( ORCAD_RAW_PAGE&& ) = default;

    std::string                                  name;
    std::string                                  pageSize;  ///< page-size name string, e.g. "B"
    size_t                                       sourcePageNumber = 0; ///< 1-based within the OrCAD folder
    size_t                                       sourcePageCount = 0;  ///< pages in the OrCAD folder
    std::map<std::string, std::string>            props;
    uint32_t                                     createTimestamp = 0;
    uint32_t                                     modifyTimestamp = 0;
    uint32_t                                     width = 0; ///< mils, or um when isMetric
    uint32_t                                     height = 0;
    bool                                         isMetric = false;
    uint16_t                                     horizontalCount = 0;
    uint16_t                                     verticalCount = 0;
    uint32_t                                     horizontalWidth = 0;
    uint32_t                                     verticalWidth = 0;
    bool                                         horizontalChar = false;
    bool                                         horizontalAscending = false;
    bool                                         verticalChar = false;
    bool                                         verticalAscending = false;
    bool                                         borderPrinted = false;
    bool                                         gridRefPrinted = false;
    std::vector<ORCAD_GRAPHIC_INST>              titleBlocks;
    std::vector<ORCAD_WIRE>                      wires;
    std::vector<ORCAD_PLACED_INSTANCE>           instances;
    std::vector<ORCAD_GRAPHIC_INST>              ports;
    std::vector<ORCAD_GRAPHIC_INST>              globals;    ///< placed power symbols
    std::vector<ORCAD_GRAPHIC_INST>              offpage;    ///< off-page connectors
    std::vector<ORCAD_GRAPHIC_INST>              ercObjects; ///< saved design-rule-check markers
    std::vector<ORCAD_BUS_ENTRY>                 busEntries;
    std::vector<ORCAD_GRAPHIC_INST>              graphics;   ///< free comment text/shapes/images
    std::map<uint32_t, std::string>              netmap;     ///< net db id -> net name
    std::map<uint32_t, std::vector<std::string>> netAliases; ///< every name recorded for a net db id
    std::vector<ORCAD_NET_GROUP>                 netGroups;  ///< bus net id -> member net ids
    std::vector<ORCAD_DRAWN_INSTANCE>            blocks;     ///< hierarchical blocks (detection only)
};


struct ORCAD_OCC_BLOCK; // defined below (a scope owns child block occurrences)


/** Each scope holds the references and child blocks for one instantiation path. */
struct ORCAD_OCC_SCOPE
{
    std::map<uint32_t, std::string>                        partRefs; ///< type-13 dbId -> occurrence refdes
    std::map<uint32_t, std::string>                        partUnitRefs; ///< dbId -> package unit reference
    std::map<uint32_t, std::map<std::string, std::string>> partProps; ///< dbId -> occurrence properties
    std::map<uint32_t, std::string>                        netNames; ///< occurrence net id -> effective net name
    std::vector<ORCAD_OCC_BLOCK>                           blocks; ///< hierarchical block occurrences
};


/** Repeated child folders have separate scopes and reference designators. */
struct ORCAD_OCC_BLOCK
{
    uint32_t        targetDbId = 0; ///< type-12 drawn-instance dbId on the parent page
    std::string     childFolder;    ///< child schematic folder name
    ORCAD_OCC_SCOPE scope;          ///< the child's occurrences under this path
};


/** Child-folder pages are instantiated for each occurrence, with that occurrence's references. */
struct ORCAD_DESIGN
{
    std::string                             sourceId; ///< stable checksum of the input file
    std::string                             name;     ///< design (root schematic) name
    ORCAD_LIBRARY_INFO                      library;
    std::map<std::string, ORCAD_SYMBOL_DEF> symbols;  ///< cache, keyed by cache name
    std::map<std::string, ORCAD_PACKAGE>    packages; ///< keyed by package name
    std::vector<ORCAD_RAW_PAGE>             pages;    ///< root schematic folder pages

    /** Child schematic folder pages, keyed by lower-cased folder name; instantiated
     * once per hierarchical block occurrence during conversion. */
    std::map<std::string, std::vector<ORCAD_RAW_PAGE>> childFolderPages;

    /** Schematic folder pages not instantiated by the active hierarchy.  They remain
     * visible after import but are excluded from the board netlist. */
    std::map<std::string, std::vector<ORCAD_RAW_PAGE>> unreferencedFolderPages;

    /** Occurrence references distinguish repeated placements of a child schematic. */
    ORCAD_OCC_SCOPE occurrenceRoot;

    bool hasHierarchyBlocks = false;
};

#endif // ORCAD_RECORDS_H_
