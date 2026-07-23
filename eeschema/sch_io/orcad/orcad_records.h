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

/**
 * @file orcad_records.h
 *
 * Plain data records produced by the OrCAD Capture DSN stream parsers and consumed
 * by ORCAD_CONVERTER.  Header-only; no behavior beyond trivial defaults.
 *
 * Coordinates throughout are in OrCAD schematic database units (DBU): 1 DBU = 10 mil
 * = 0.254 mm on modern designs.  Y grows downward, the same as the KiCad schematic
 * canvas AND KiCad's in-memory LIB_SYMBOL space — no axis flip is required anywhere
 * (the .kicad_sch writer itself flips symbol bodies into the file's Y-up space).
 *
 * Strings hold the raw Windows-1252 stream bytes (std::string); convert with
 * FromOrcadString() (orcad_stream.h) only where the text reaches the UI or a
 * KiCad object.
 */

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

/// Warning sink shared by all parser entry points (recoverable-issue channel).
using ORCAD_WARN_FN = std::function<void( const wxString& aMsg )>;


/**
 * Structure type ids as they appear in the u8 type byte of the long/short prefixes.
 */
enum ORCAD_ST : int
{
    ORCAD_ST_STH_IN_PAGES0            = 2,   ///< nested symbol body inside Graphic*Inst
    ORCAD_ST_DSN_STREAM               = 4,
    ORCAD_ST_PART_CELL                = 6,
    ORCAD_ST_SCH_LIB                  = 9,
    ORCAD_ST_PAGE                     = 10,
    ORCAD_ST_PART_INSTANCE            = 11,
    ORCAD_ST_DRAWN_INSTANCE           = 12,  ///< hierarchical block instance
    ORCAD_ST_PLACED_INSTANCE          = 13,
    ORCAD_ST_T0X10                    = 16,  ///< scalar pin instance (absolute pos)
    ORCAD_ST_T0X11                    = 17,  ///< bus pin instance (absolute pos)
    ORCAD_ST_WIRE_SCALAR              = 20,
    ORCAD_ST_WIRE_BUS                 = 21,
    ORCAD_ST_PORT                     = 23,
    ORCAD_ST_LIBRARY_PART             = 24,
    ORCAD_ST_SYMBOL_PIN_SCALAR        = 26,
    ORCAD_ST_SYMBOL_PIN_BUS           = 27,
    ORCAD_ST_BUS_ENTRY                = 29,
    ORCAD_ST_PACKAGE                  = 31,
    ORCAD_ST_DEVICE                   = 32,
    ORCAD_ST_GLOBAL_SYMBOL            = 33,  ///< power symbol definition
    ORCAD_ST_PORT_SYMBOL              = 34,
    ORCAD_ST_OFFPAGE_SYMBOL           = 35,
    ORCAD_ST_GLOBAL                   = 37,  ///< placed power symbol
    ORCAD_ST_OFFPAGE_CONNECTOR        = 38,
    ORCAD_ST_SYMBOL_DISPLAY_PROP      = 39,
    ORCAD_ST_SYMBOL_VECTOR            = 48,
    ORCAD_ST_ALIAS                    = 49,  ///< net alias attached to a wire
    ORCAD_ST_T0X34                    = 52,  ///< raw record, not prefix-framed
    ORCAD_ST_T0X35                    = 53,  ///< raw record, not prefix-framed
    ORCAD_ST_GRAPHIC_BOX_INST         = 55,
    ORCAD_ST_GRAPHIC_LINE_INST        = 56,
    ORCAD_ST_GRAPHIC_ARC_INST         = 57,
    ORCAD_ST_GRAPHIC_ELLIPSE_INST     = 58,
    ORCAD_ST_GRAPHIC_POLYGON_INST     = 59,
    ORCAD_ST_GRAPHIC_POLYLINE_INST    = 60,
    ORCAD_ST_GRAPHIC_COMMENT_TEXT_INST = 61,
    ORCAD_ST_GRAPHIC_BITMAP_INST      = 62,
    ORCAD_ST_TITLEBLOCK_SYMBOL        = 64,
    ORCAD_ST_TITLEBLOCK               = 65,
    ORCAD_ST_HIERARCHY_LINK           = 66,  ///< block dbId -> child folder link
    ORCAD_ST_ERC_SYMBOL               = 75,
    ORCAD_ST_BOOKMARK_SYMBOL          = 76,
    ORCAD_ST_ERC_OBJECT               = 77,  ///< placed ERC marker (no-connect)
    ORCAD_ST_BOOKMARK_INST            = 78,
    ORCAD_ST_GRAPHIC_BEZIER_INST      = 88,
    ORCAD_ST_GRAPHIC_OLE_INST         = 89,
    ORCAD_ST_PIN_SHAPE_SYMBOL         = 98,
    ORCAD_ST_NET_GROUP                = 103
};


/**
 * Graphic primitive type ids, stored as a doubled u8 pair (t, t) before each
 * primitive body inside symbol definitions.
 */
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


/// Decoded primitive kind after parsing.
enum class ORCAD_PRIM_KIND
{
    GROUP,
    RECT,
    LINE,
    ARC,
    ELLIPSE,
    POLYGON,
    POLYLINE,
    BEZIER,
    TEXT,
    IMAGE
};


/**
 * Pin electrical type codes (u32 portType field of a symbol pin).  Values outside
 * 0..7 must be mapped to PASSIVE by the parser.
 */
enum class ORCAD_PORT_TYPE : int
{
    INPUT          = 0,
    BIDIRECTIONAL  = 1,
    OUTPUT         = 2,
    OPEN_COLLECTOR = 3,
    PASSIVE        = 4,
    TRI_STATE      = 5,
    OPEN_EMITTER   = 6,
    POWER_IN       = 7
};


/// Integer point in OrCAD DBU.
struct ORCAD_POINT
{
    int x = 0;
    int y = 0;

    bool operator==( const ORCAD_POINT& aOther ) const { return x == aOther.x && y == aOther.y; }
    bool operator!=( const ORCAD_POINT& aOther ) const { return !( *this == aOther ); }
};


/// Axis-aligned box in OrCAD DBU; corner order as stored (not normalized).
struct ORCAD_BBOX
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
};


/**
 * One displayed property of an instance (Part Reference, Value, user fields).
 *
 * Body layout: u32 nameIdx (string-table index), i16 x, i16 y (symbol-space
 * position), u16 rotFont (bits 0..13 = 1-based font index, bits 14..15 = quarter
 * turns), u8 color, u16 dispMode, u8 0x00 terminator.
 */
struct ORCAD_DISPLAY_PROP
{
    uint32_t    nameIdx = 0;
    std::string name;          ///< resolved property name (empty when index invalid)
    int         x = 0;
    int         y = 0;
    int         rotation = 0;  ///< 0..3 quarter turns
    int         fontIdx = 0;   ///< 1-based into ORCAD_LIBRARY_INFO::fonts; 0 = default
    int         color = 0;
    int         dispMode = 0;
};


/**
 * Net alias attached to a wire (becomes a local label).
 *
 * Body layout: i32 x, i32 y, u32 color, u32 rotation, u32 fontIdx, lzt name.
 * (x, y) is the free display position; the logical attachment is the owning wire.
 */
struct ORCAD_ALIAS
{
    std::string name;
    int         x = 0;
    int         y = 0;
    int         rotation = 0;  ///< 0..3 quarter turns
    int         color = 0;
    int         fontIdx = 0;
};


/**
 * One wire or bus segment.
 *
 * Body layout: 4 unknown bytes, u32 id, u32 color, i32 x1, y1, x2, y2, 1 unknown
 * byte, u16 alias count (framed Alias structures), u16 property count (framed,
 * skipped), u32 lineWidth, u32 lineStyle.  The id keys the page net table.
 */
struct ORCAD_WIRE
{
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


/**
 * T0x10: one pin of a placed instance, carrying the pin's absolute page position
 * (the connection point wires attach to).
 *
 * Body layout: u16 unknown, i16 x, i16 y, u32, u32, u16 display-prop count
 * (framed), then padding to the structure's outer stop offset.
 */
struct ORCAD_PIN_INST
{
    int                             x = 0;
    int                             y = 0;
    uint32_t                        wordA = 0;   ///< two flag/id words after (x, y)
    uint32_t                        wordB = 0;
    std::vector<ORCAD_DISPLAY_PROP> displayProps;
};


/**
 * A placed part instance on a page (structure type 13).
 *
 * Body layout: 8 unknown bytes, lzt pkgName (cache symbol name, e.g. "C.Normal"),
 * u32 dbId, placed bounding box as i16 y1, x1, y2, x2 (NOTE the y-first order),
 * i16 x, i16 y (anchor), u8 color, u8 orientation (bits 0..1 = quarter turns,
 * bit 2 = mirror), 2 unknown bytes, u16 display-prop count (framed structures),
 * 1 unknown byte, lzt reference, u32 value string-table index (the Part Value
 * directly follows the reference; the next 10 bytes are unknown), u16 pin count
 * (framed T0x10 structures), lzt sourcePackage, 2 unknown bytes.
 *
 * The placed bbox INCLUDES the displayed text; the symbol body box must be derived
 * from the cache definition bbox plus the orientation transform.
 */
struct ORCAD_PLACED_INSTANCE
{
    std::string                        pkgName;
    uint32_t                           dbId = 0;
    int                                x = 0;
    int                                y = 0;
    std::string                        reference;
    std::string                        value;          ///< resolved Part Value
    std::map<std::string, std::string> props;          ///< short-prefix property pairs
    std::vector<ORCAD_DISPLAY_PROP>    displayProps;
    int                                color = 0;
    int                                rotation = 0;   ///< 0..3 quarter turns
    bool                               mirror = false; ///< orientation bit 2
    ORCAD_BBOX                         bbox;           ///< placed box, page DBU
    std::string                        sourcePackage;  ///< package base name
    std::vector<ORCAD_PIN_INST>        pins;           ///< successfully parsed T0x10 records
};


struct ORCAD_SYMBOL_DEF;   // defined below (ORCAD_GRAPHIC_INST holds one by pointer)


/**
 * Shared body of Port / Global / OffPageConnector / TitleBlock / ERCObject and the
 * Graphic*Inst shapes (structure types 23, 37, 38, 55..62, 65, 77, 88, 89).
 *
 * Body layout: u32 nameIdx (ports: logical net/port name string index), u32 source
 * library string index, lzt name, u32 dbId, i16 y, x, y2, x2, x1, y1 (NOTE the
 * interleaved order; (x, y) anchor, (x1, y1, x2, y2) bounding box), u8 color,
 * u8 orientation (bits 0..1 = rotation, bit 2 = mirror), 2 unknown bytes, u16
 * display-prop count (framed), u8 flag; when flag == 0x02 one nested structure
 * follows (normally an SthInPages0 symbol body carrying the drawable primitives).
 *
 * Trailers by type: Port +9 bytes; TitleBlock +12 bytes; ERCObject +3 lzt strings;
 * on pages each Global and each OffPageConnector list entry is followed by 5 bytes.
 *
 * The outer bbox of free Graphic*Inst records is anchor-relative junk; only the
 * nested primitives carry real page coordinates.  For Global/OffPage/Port symbols
 * the orientation transform base is the placed bbox min corner, NOT the anchor.
 */
struct ORCAD_GRAPHIC_INST
{
    int                                typeId = 0;    ///< ORCAD_ST value
    std::string                        name;          ///< cache symbol name
    uint32_t                           dbId = 0;
    int                                x = 0;
    int                                y = 0;
    ORCAD_BBOX                         bbox;
    int                                color = 0;
    std::map<std::string, std::string> props;
    std::vector<ORCAD_DISPLAY_PROP>    displayProps;
    std::unique_ptr<ORCAD_SYMBOL_DEF>  nested;        ///< SthInPages0 body, else nullptr
    int                                rotation = 0;  ///< 0..3 quarter turns
    bool                               mirror = false;
    std::string                        logicalName;   ///< ports: resolved net/port name
};


/**
 * One graphic primitive of a symbol body or nested page graphic.
 *
 * Envelope: doubled u8 type pair, u32 byteLength, u32 zero pad, body.  Two
 * byteLength conventions exist in the wild (sometimes mixed within one file):
 * modern records count the u32 size field + 4-byte pad and are followed by a
 * preamble block; legacy records exclude those 8 bytes and have no trailing
 * preamble.  Detection is per record: a modern record has the preamble magic
 * right at its claimed end.  CommentText uses the exclusive convention in ALL
 * eras (size += 8 unconditionally).
 *
 * Bodies (after the pad):
 *  - rect/ellipse: i32 x1, y1, x2, y2 [, u32 lineStyle, u32 lineWidth [, u32 fill]]
 *  - line:         i32 x1, y1, x2, y2
 *  - arc:          i32 x1, y1, x2, y2 (bbox), i32 sx, sy, ex, ey (start/end points);
 *                  drawn counter-clockwise in screen (Y-down) coordinates
 *  - polygon/polyline/bezier: u16 point count then count x (i16 y, i16 x) — the
 *    point-count offset (16, 8 or 0 bytes into the body for polygons; 8 or 0
 *    otherwise) is found deterministically by reconciling with byteLength under
 *    one of the two length conventions
 *  - comment text: i32 x, y, i32 x2, y2, i32 x1, y1, u16 fontIdx, 2 bytes, lzt text
 *  - bitmap (47):  i32 x, y, i32 x2, y2, 8 bytes duplicate corner, 8 bytes pixel
 *                  width/height, u32 dataSize, dataSize bytes of raw DIB
 *  - OLE image (90): i32 x1, y1, x2, y2, 16 bytes crop/extent, payload to record end
 */
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
    int                          lineStyle = 5; ///< 0 solid, 1 dash, 2 dot, 3 dash-dot, 4 dash-dot-dot, 5 default
    int                          lineWidth = 3; ///< Capture width enum: 0 thin, 1 medium, 2 wide, 3 default
    int                          fillStyle = 1; ///< 0 solid, 1 none, 2 hatch pattern
    int                          hatchStyle = 0;
    std::vector<uint8_t>         data;     ///< kind == IMAGE: raw embedded payload
    std::vector<ORCAD_PRIMITIVE> children; ///< kind == GROUP, translated by (x1, y1)
};


/**
 * One pin of a symbol definition (structure types 26/27).
 *
 * Body layout: lzt name, i32 startX, startY (free end), i32 hotptX, hotptY
 * (connection point), u16 shapeBits (bit 1 = clock, bit 2 = inverted dot),
 * 2 uninitialized bytes, u32 portType (0..7); the remaining body bytes are junk
 * and the pin ends at its outer prefix stop.  A 0x00 byte instead of a prefix
 * chain marks a skipped pin slot.  Coordinates are symbol-space, Y-down.
 */
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
};


/**
 * A symbol definition from the design Cache (LibraryPart / GlobalSymbol /
 * PortSymbol / OffPageSymbol / TitleBlockSymbol / ERCSymbol / ...), or the nested
 * SthInPages0 body of a page graphic.
 *
 * Body layout: lzt name, lzt sourceLib, u32 color, u16 primitive count (with an
 * occasional 8 zero bytes between primitives, detected by the next 2 bytes not
 * forming a valid doubled primitive type pair), then the bounding box stored as
 * the LAST 8 bytes before the next prefix stop offset (the gap is 8, or 16 with
 * 8 unknown/legacy-trailer bytes first) as 4 x i16 (x1, y1, x2, y2), accepted
 * only when x1 <= x2, y1 <= y2 and each span <= 4000 DBU.  Library symbols then
 * carry u16 pin count (pin structures) and u16 property count (framed, skipped);
 * SthInPages0 bodies have no pin/property lists.
 */
struct ORCAD_SYMBOL_DEF
{
    int                                typeId = 0;    ///< ORCAD_ST value
    std::string                        name;          ///< cache name, e.g. "C.Normal"
    std::string                        sourceLib;
    int                                color = 0;
    std::vector<ORCAD_PRIMITIVE>       primitives;
    std::vector<ORCAD_SYMBOL_PIN>      pins;
    std::optional<ORCAD_BBOX>          bbox;          ///< symbol-space body box
    std::map<std::string, std::string> props;

    /**
     * Stale same-name cache entries (older library versions), in cache order,
     * EXCLUDING this entry itself.  Variant index 0 designates this (the first
     * seen, default) definition; index i >= 1 designates variants[i - 1].
     * Per-instance selection matches transformed pin hot points against the
     * instance's T0x10 absolute positions; first entry wins by default.
     */
    std::vector<ORCAD_SYMBOL_DEF>      variants;

    /// LibraryPart GeneralProperties flags (-1 = absent); bit0 = pin numbers visible,
    /// bit2 = pin names hidden.
    int                                generalFlags = -1;

    bool                               synthesized = false;  ///< placeholder built from T0x10 data
};


/**
 * One device of a package: the pin-number map of a unit (structure type 32).
 *
 * Body layout: lzt unitRef (bare unit letter, no ':View' suffix), lzt refDes,
 * u16 pin count; per pin either the 2-byte marker FF FF (empty number, ignored
 * pin) or lzt number + u8 config (bit 7 = pin ignored).
 */
struct ORCAD_DEVICE
{
    std::string              unitRef;
    std::string              refDes;
    std::vector<std::string> pinNumbers;
    std::vector<bool>        pinIgnore;
};


/**
 * A package: refdes prefix, footprint and per-unit devices (structure type 31).
 *
 * Body layout: lzt name, lzt sourceLib, lzt refDes, lzt unknown, lzt pcbFootprint,
 * u16 device count (Device structures).
 */
struct ORCAD_PACKAGE
{
    std::string               name;
    std::string               sourceLib;
    std::string               refDes;
    std::string               pcbFootprint;
    std::vector<ORCAD_DEVICE> devices;
};


/// One interface pin of a hierarchical block, at its absolute page position.
struct ORCAD_BLOCK_PIN
{
    std::string     name;
    ORCAD_PORT_TYPE portType = ORCAD_PORT_TYPE::PASSIVE;
    int             x = 0;
    int             y = 0;
};


/**
 * Structure type 12: a hierarchical block instance placed on a page.  Body =
 * GraphicInst common part with an inline LibraryPart (the block's pin interface;
 * nested flag byte equals 24), then — starting at the second prefix stop offset —
 * lzt reference, 14 bytes, u16 pin count (framed T0x10 structures with the
 * absolute pin positions).  Presence of any such record marks the design as
 * hierarchical (out of scope for the flat importer: converted with warnings).
 */
struct ORCAD_DRAWN_INSTANCE
{
    uint32_t                        dbId = 0;
    std::string                     reference;
    int                             x1 = 0;  ///< block rectangle top-left, page DBU
    int                             y1 = 0;
    int                             w = 0;
    int                             h = 0;
    std::vector<ORCAD_BLOCK_PIN>    pins;
    std::vector<ORCAD_DISPLAY_PROP> displayProps;
    std::string                     childName;  ///< child folder, when embedded
};


/**
 * Bus entry (structure type 29).
 * Body layout: u32 color, i32 x1, y1, x2, y2, 8 unknown bytes
 */
struct ORCAD_BUS_ENTRY
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int color = 0;
};


/**
 * One text font from the Library stream (a Win32 LOGFONTA record, 60 bytes:
 * lfHeight i32 at +0, lfWeight i32 at +16, lfItalic u8 at +20, lfFaceName
 * char[32] at +28).  Height is in negative device units; the point size is
 * approximately -lfHeight * 72 / 96.  Font indexes elsewhere are 1-based with
 * 0 meaning default.
 */
struct ORCAD_FONT
{
    int         height = 0;   ///< raw lfHeight (typically negative)
    std::string face;
    bool        italic = false;
    bool        bold = false;  ///< lfWeight >= 600
};


/**
 * The 156-byte PageSettings block embedded in the Library stream and in every
 * Page stream: 8 bytes create/modify dates, 16 unknown, u32 width, u32 height,
 * u32 pinToPin, 2 bytes, 4 bytes horizontal/vertical count, 2 bytes, 8 bytes
 * horizontal/vertical width, 48 unknown, 24 bytes grid-reference settings,
 * 8 x u32 flags of which flags[0] is isMetric.
 *
 * width/height are in mils when isMetric == 0 and in micrometres when
 * isMetric == 1.
 */
struct ORCAD_PAGE_SETTINGS
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t pinToPin = 100;
    bool     isMetric = false;
};


struct ORCAD_NET_GROUP
{
    uint32_t              id = 0;
    std::string           name;
    std::vector<uint32_t> members;
};


/**
 * Decoded Library stream: format version, fonts, and the string table that all
 * property name/value indices reference throughout the file.
 */
struct ORCAD_LIBRARY_INFO
{
    std::string                                      introduction;   ///< fixed 32-byte NUL-terminated buffer
    int                                              versionMajor = 0;
    int                                              versionMinor = 0;
    std::vector<ORCAD_FONT>                          fonts;
    std::vector<std::string>                         partFields;     ///< the 8 named part fields
    std::vector<std::string>                         strings;        ///< global string table
    std::vector<std::pair<std::string, std::string>> partAliases;
    std::string                                      schematicName;  ///< root schematic folder name
    uint32_t                                         pinToPin = 100;
};


/**
 * One parsed 'Views/<folder>/Pages/<page>' stream, raw structure lists in stream
 * order.  The net table (netmap) is authoritative for net names: it maps net
 * database ids to names, and wires carry the same ids.
 */
struct ORCAD_RAW_PAGE
{
    // Move-only: the graphic-instance lists hold unique_ptrs, and the netmap's
    // non-noexcept move would otherwise make vector growth fall back to the
    // (deleted) copy constructor via move_if_noexcept.
    ORCAD_RAW_PAGE() = default;
    ORCAD_RAW_PAGE( const ORCAD_RAW_PAGE& ) = delete;
    ORCAD_RAW_PAGE& operator=( const ORCAD_RAW_PAGE& ) = delete;
    ORCAD_RAW_PAGE( ORCAD_RAW_PAGE&& ) = default;
    ORCAD_RAW_PAGE& operator=( ORCAD_RAW_PAGE&& ) = default;

    std::string                        name;
    std::string                        pageSize;    ///< page-size name string, e.g. "B"
    uint32_t                           width = 0;   ///< mils, or um when isMetric
    uint32_t                           height = 0;
    bool                               isMetric = false;
    std::vector<ORCAD_GRAPHIC_INST>    titleBlocks;
    std::vector<ORCAD_WIRE>            wires;
    std::vector<ORCAD_PLACED_INSTANCE> instances;
    std::vector<ORCAD_GRAPHIC_INST>    ports;
    std::vector<ORCAD_GRAPHIC_INST>    globals;     ///< placed power symbols
    std::vector<ORCAD_GRAPHIC_INST>    offpage;     ///< off-page connectors
    std::vector<ORCAD_GRAPHIC_INST>    ercObjects;  ///< no-connect markers
    std::vector<ORCAD_BUS_ENTRY>       busEntries;
    std::vector<ORCAD_GRAPHIC_INST>    graphics;  ///< free comment text/shapes/images
    std::map<uint32_t, std::string>    netmap;    ///< net db id -> net name
    std::vector<ORCAD_NET_GROUP>       netGroups; ///< bus net id -> member net ids
    std::vector<ORCAD_DRAWN_INSTANCE>  blocks;    ///< hierarchical blocks (detection only)
};


struct ORCAD_OCC_BLOCK;   // defined below (a scope owns child block occurrences)


/**
 * One node of the design occurrence tree parsed from the root Hierarchy stream.
 * A scope is the set of occurrences visible at one point in the instantiation
 * path: the reference designator of every placed part (keyed by its type-13 db
 * id) and the hierarchical block occurrences that descend into child schematics.
 */
struct ORCAD_OCC_SCOPE
{
    std::map<uint32_t, std::string>   partRefs;   ///< type-13 dbId -> occurrence refdes
    std::vector<ORCAD_OCC_BLOCK>      blocks;     ///< hierarchical block occurrences
};


/**
 * A hierarchical block occurrence: one placement of a child schematic folder on a
 * parent page.  The same child folder appears once per instantiation path, each
 * with its own nested scope (so a schematic reused N times yields N sets of
 * per-occurrence reference designators).
 */
struct ORCAD_OCC_BLOCK
{
    uint32_t        targetDbId = 0;   ///< type-12 drawn-instance dbId on the parent page
    std::string     childFolder;      ///< child schematic folder name
    ORCAD_OCC_SCOPE scope;            ///< the child's occurrences under this path
};


/**
 * Everything parsed from one .DSN, as handed to ORCAD_CONVERTER.  design.pages are
 * the root schematic folder's pages in schematic order; child folder pages are held
 * in childFolderPages and instantiated once per block occurrence via the occurrence
 * tree, so a schematic reused N times imports N times with per-occurrence refs.
 */
struct ORCAD_DESIGN
{
    std::string                             name;      ///< design (root schematic) name
    ORCAD_LIBRARY_INFO                      library;
    std::map<std::string, ORCAD_SYMBOL_DEF> symbols;   ///< cache, keyed by cache name
    std::map<std::string, ORCAD_PACKAGE>    packages;  ///< keyed by package name
    std::vector<ORCAD_RAW_PAGE>             pages;      ///< root schematic folder pages

    /// Child schematic folder pages, keyed by lower-cased folder name; instantiated
    /// once per hierarchical block occurrence during conversion.
    std::map<std::string, std::vector<ORCAD_RAW_PAGE>> childFolderPages;

    /// Occurrence tree from the root folder Hierarchy stream.  occurrenceRoot.partRefs
    /// carries the root folder's occurrence designators (occurrence-annotated designs
    /// leave the placed instance's own field as the "C?" template; this fills it in);
    /// occurrenceRoot.blocks descend into the child schematics with per-path refs.
    ORCAD_OCC_SCOPE                         occurrenceRoot;

    bool                                    hasHierarchyBlocks = false;
};

#endif // ORCAD_RECORDS_H_
