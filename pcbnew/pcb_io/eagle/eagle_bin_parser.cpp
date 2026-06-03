/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Binary Eagle parsing logic and the eagle_script[] format table ported from
 * pcb-rnd src_plugins/io_eagle (eagle_bin.c) by Tibor 'Igor2' Palinkas and
 * Erich S. Heinzle.
 *
 *                            COPYRIGHT (pcb-rnd, eagle_bin.c / eagle_bin.h)
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2017 Erich S. Heinzle
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "eagle_bin_parser.h"

#include <cmath>
#include <cstring>

#include <wx/intl.h>
#include <wx/stream.h>
#include <wx/xml/xml.h>

#include <ki_exception.h>
#include <macros.h>
#include <trace_helpers.h>
#include <wx/log.h>

// Section keyword ids; the high byte selects the record kind in the binary stream.
enum EGKW
{
    EGKW_SECT_START = 0x1000,
    EGKW_SECT_UNKNOWN11 = 0x1100,
    EGKW_SECT_GRID = 0x1200,
    EGKW_SECT_LAYER = 0x1300,
    EGKW_SECT_SCHEMA = 0x1400,
    EGKW_SECT_LIBRARY = 0x1500,
    EGKW_SECT_DEVICES = 0x1700,
    EGKW_SECT_SYMBOLS = 0x1800,
    EGKW_SECT_PACKAGES = 0x1900,
    EGKW_SECT_SCHEMASHEET = 0x1a00,
    EGKW_SECT_BOARD = 0x1b00,
    EGKW_SECT_SIGNAL = 0x1c00,
    EGKW_SECT_SYMBOL = 0x1d00,
    EGKW_SECT_PACKAGE = 0x1e00,
    EGKW_SECT_SCHEMANET = 0x1f00,
    EGKW_SECT_PATH = 0x2000,
    EGKW_SECT_POLYGON = 0x2100,
    EGKW_SECT_LINE = 0x2200,
    EGKW_SECT_ARC = 0x2400,
    EGKW_SECT_CIRCLE = 0x2500,
    EGKW_SECT_RECTANGLE = 0x2600,
    EGKW_SECT_JUNCTION = 0x2700,
    EGKW_SECT_HOLE = 0x2800,
    EGKW_SECT_VIA = 0x2900,
    EGKW_SECT_PAD = 0x2a00,
    EGKW_SECT_SMD = 0x2b00,
    EGKW_SECT_PIN = 0x2c00,
    EGKW_SECT_GATE = 0x2d00,
    EGKW_SECT_ELEMENT = 0x2e00,
    EGKW_SECT_ELEMENT2 = 0x2f00,
    EGKW_SECT_INSTANCE = 0x3000,
    EGKW_SECT_TEXT = 0x3100,
    EGKW_SECT_NETBUSLABEL = 0x3300,
    EGKW_SECT_SMASHEDNAME = 0x3400,
    EGKW_SECT_SMASHEDVALUE = 0x3500,
    EGKW_SECT_PACKAGEVARIANT = 0x3600,
    EGKW_SECT_DEVICE = 0x3700,
    EGKW_SECT_PART = 0x3800,
    EGKW_SECT_SCHEMABUS = 0x3a00,
    EGKW_SECT_VARIANTCONNECTIONS = 0x3c00,
    EGKW_SECT_SCHEMACONNECTION = 0x3d00,
    EGKW_SECT_CONTACTREF = 0x3e00,
    EGKW_SECT_SMASHEDPART = 0x3f00,
    EGKW_SECT_SMASHEDGATE = 0x4000,
    EGKW_SECT_ATTRIBUTE = 0x4100,
    EGKW_SECT_ATTRIBUTEVALUE = 0x4200,
    EGKW_SECT_FRAME = 0x4300,
    EGKW_SECT_SMASHEDXREF = 0x4400,
    EGKW_SECT_FREETEXT = 0x1312,

    // Synthetic nodes created during post-processing.
    EGKW_SECT_LAYERS = 0x11300,
    EGKW_SECT_DRC = 0x11100
};

namespace
{
enum ATTR_TYPE
{
    T_BMB, // bit-mask-bool: apply mask in len to byte at offs, result is a boolean
    T_UBF, // unsigned bitfield, len is a BITFIELD() descriptor
    T_INT, // signed little-endian integer
    T_DBL, // 8-byte IEEE double
    T_STR  // fixed-length NUL-padded string
};

enum SS_TYPE
{
    SS_DIRECT,           // number of direct children
    SS_RECURSIVE,        // number of all children, recursively
    SS_RECURSIVE_MINUS_1 // same, but decrement the count first
};

// Describe a bitfield hosted in a field of the given width; first/last are
// inclusive bit offsets counted from the LSB.
constexpr uint32_t BITFIELD( uint32_t aWidth, uint32_t aFirst, uint32_t aLast )
{
    return ( aWidth << 16 ) | ( aFirst << 8 ) | aLast;
}

struct FMATCH
{
    int      offs; // 0 terminates the list
    unsigned len;
    int      val;
};

struct SUBSECT
{
    int         offs; // 0 terminates the list
    int         len;
    SS_TYPE     ssType;
    const char* treeName; // if set, wrap children in a synthetic subtree
};

struct ATTR
{
    const char* name; // nullptr terminates the list
    ATTR_TYPE   type;
    int         offs;
    uint32_t    len;
};

struct SCRIPT_ROW
{
    unsigned    cmd, cmdMask; // matches when (block[0..1] & mask) == cmd
    const char* name;
    FMATCH      fmatch[4];
    SUBSECT     subs[8];
    ATTR        attrs[32];
};

#define TERM_F                                                                                                         \
    {                                                                                                                  \
        0, 0, 0                                                                                                        \
    }
#define TERM_S                                                                                                         \
    {                                                                                                                  \
        0, 0, SS_DIRECT, nullptr                                                                                       \
    }
#define TERM_A                                                                                                         \
    {                                                                                                                  \
        nullptr, T_INT, 0, 0                                                                                           \
    }

// The format spec. Each row decodes one record kind; every offset is exact.
const SCRIPT_ROW g_script[] = {
    { EGKW_SECT_START,
      0xFF7F,
      "drawing",
      { TERM_F },
      { { 4, 4, SS_RECURSIVE_MINUS_1, nullptr }, TERM_S },
      { { "subsecs", T_INT, 2, 2 },
        { "numsecs", T_INT, 4, 4 },
        { "subsecsMSB", T_INT, 3, 1 },
        { "subsecsLSB", T_INT, 2, 1 },
        { "numsecsMSB2", T_INT, 7, 1 },
        { "numsecsMSB1", T_INT, 6, 1 },
        { "numsecsMSB0", T_INT, 5, 1 },
        { "numsecsLSB", T_INT, 4, 1 },
        { "v1", T_INT, 8, 1 },
        { "v2", T_INT, 9, 1 },
        TERM_A } },
    { EGKW_SECT_UNKNOWN11, 0xFFFF, "unknown11", { TERM_F }, { TERM_S }, { TERM_A } },
    { EGKW_SECT_GRID,
      0xFFFF,
      "grid",
      { TERM_F },
      { TERM_S },
      { { "display", T_BMB, 2, 0x01 },
        { "visible", T_BMB, 2, 0x02 },
        { "unit", T_UBF, 3, BITFIELD( 1, 0, 3 ) },
        { "altunit", T_UBF, 3, BITFIELD( 1, 4, 7 ) },
        { "multiple", T_INT, 4, 3 },
        { "size", T_DBL, 8, 8 },
        { "altsize", T_DBL, 16, 8 },
        TERM_A } },
    { EGKW_SECT_LAYER,
      0xFF7F,
      "layer",
      { TERM_F },
      { TERM_S },
      { { "side", T_BMB, 2, 0x10 },
        { "visible", T_UBF, 2, BITFIELD( 1, 2, 3 ) },
        { "active", T_BMB, 2, 0x02 },
        { "number", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "other", T_INT, 4, 1 },
        { "fill", T_UBF, 5, BITFIELD( 1, 0, 3 ) },
        { "color", T_UBF, 6, BITFIELD( 1, 0, 5 ) },
        { "name", T_STR, 15, 9 },
        TERM_A } },
    { EGKW_SECT_SCHEMA,
      0xFFFF,
      "schema",
      { TERM_F },
      { { 4, 4, SS_DIRECT, nullptr }, TERM_S },
      { { "shtsubsecs", T_INT, 8, 4 }, { "atrsubsecs", T_INT, 12, 4 }, { "xref_format", T_STR, 19, 5 }, TERM_A } },
    { EGKW_SECT_LIBRARY,
      0xFF7F,
      "library",
      { TERM_F },
      { { 4, 4, SS_RECURSIVE, nullptr }, { 8, 4, SS_RECURSIVE, nullptr }, { 12, 4, SS_RECURSIVE, nullptr }, TERM_S },
      { { "devsubsecs", T_INT, 4, 4 },
        { "symsubsecs", T_INT, 8, 4 },
        { "pacsubsecs", T_INT, 12, 4 },
        { "children", T_INT, 8, 4 },
        { "name", T_STR, 16, 8 },
        TERM_A } },
    { EGKW_SECT_DEVICES,
      0xFF7F,
      "devices",
      { TERM_F },
      { { 4, 4, SS_DIRECT, nullptr }, TERM_S },
      { { "children", T_INT, 8, 4 }, { "library", T_STR, 16, 8 }, TERM_A } },
    { EGKW_SECT_SYMBOLS,
      0xFF7F,
      "symbols",
      { TERM_F },
      { { 4, 4, SS_RECURSIVE, nullptr }, TERM_S },
      { { "children", T_INT, 8, 4 }, { "library", T_STR, 16, 8 }, TERM_A } },
    { EGKW_SECT_PACKAGES,
      0xFF5F,
      "packages",
      { TERM_F },
      { { 4, 4, SS_RECURSIVE, nullptr }, TERM_S },
      { { "subsects", T_INT, 4, 4 },
        { "children", T_INT, 8, 2 },
        { "desc", T_STR, 10, 6 },
        { "library", T_STR, 16, 8 },
        TERM_A } },
    { EGKW_SECT_SCHEMASHEET,
      0xFFFF,
      "schemasheet",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "minx", T_INT, 4, 2 },
        { "miny", T_INT, 6, 2 },
        { "maxx", T_INT, 8, 2 },
        { "maxy", T_INT, 10, 2 },
        { "partsubsecs", T_INT, 12, 4 },
        { "bussubsecs", T_INT, 16, 4 },
        { "netsubsecs", T_INT, 20, 4 },
        TERM_A } },
    { EGKW_SECT_BOARD,
      0xFF37,
      "board",
      { TERM_F },
      { { 12, 4, SS_RECURSIVE, "libraries" },
        { 2, 2, SS_DIRECT, "plain" },
        { 16, 4, SS_RECURSIVE, "elements" },
        { 20, 4, SS_RECURSIVE, "signals" },
        TERM_S },
      { { "minx", T_INT, 4, 2 },
        { "miny", T_INT, 6, 2 },
        { "maxx", T_INT, 8, 2 },
        { "maxy", T_INT, 10, 2 },
        { "defsubsecs", T_INT, 12, 4 },
        { "pacsubsecs", T_INT, 16, 4 },
        { "netsubsecs", T_INT, 20, 4 },
        TERM_A } },
    { EGKW_SECT_SIGNAL,
      0xFFB3,
      "signal",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "minx", T_INT, 4, 2 },
        { "miny", T_INT, 6, 2 },
        { "maxx", T_INT, 8, 2 },
        { "maxy", T_INT, 10, 2 },
        { "airwires", T_BMB, 12, 0x02 },
        { "netclass", T_UBF, 13, BITFIELD( 1, 0, 3 ) },
        { "name", T_STR, 16, 8 },
        TERM_A } },
    { EGKW_SECT_SYMBOL,
      0xFFFF,
      "symbol",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "minx", T_INT, 4, 2 },
        { "miny", T_INT, 6, 2 },
        { "maxx", T_INT, 8, 2 },
        { "maxy", T_INT, 10, 2 },
        { "name", T_STR, 16, 8 },
        TERM_A } },
    { EGKW_SECT_PACKAGE,
      0xFFDF,
      "package",
      { TERM_F },
      { { 2, 2, SS_RECURSIVE, nullptr }, TERM_S },
      { { "minx", T_INT, 4, 2 },
        { "miny", T_INT, 6, 2 },
        { "maxx", T_INT, 8, 2 },
        { "maxy", T_INT, 10, 2 },
        { "desc", T_STR, 13, 5 },
        { "name", T_STR, 18, 6 },
        TERM_A } },
    { EGKW_SECT_SCHEMANET,
      0xFFFF,
      "schemanet",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "minx", T_INT, 4, 2 },
        { "miny", T_INT, 6, 2 },
        { "maxx", T_INT, 8, 2 },
        { "maxy", T_INT, 10, 2 },
        { "netclass", T_UBF, 13, BITFIELD( 1, 0, 3 ) },
        { "name", T_STR, 18, 6 },
        TERM_A } },
    { EGKW_SECT_PATH,
      0xFFFF,
      "path",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "minx", T_INT, 4, 2 }, { "miny", T_INT, 6, 2 }, { "maxx", T_INT, 8, 2 }, { "maxy", T_INT, 10, 2 }, TERM_A } },
    { EGKW_SECT_POLYGON,
      0xFFF7,
      "polygon",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "minx", T_INT, 4, 2 },
        { "miny", T_INT, 6, 2 },
        { "maxx", T_INT, 8, 2 },
        { "maxy", T_INT, 10, 2 },
        { "width", T_INT, 12, 2 },
        { "spacing", T_INT, 14, 2 },
        { "isolate", T_INT, 16, 2 },
        { "layer", T_UBF, 18, BITFIELD( 1, 0, 7 ) },
        { "pour", T_BMB, 19, 0x01 },
        { "rank", T_BMB, 19, BITFIELD( 1, 1, 3 ) },
        { "thermals", T_BMB, 19, 0x80 },
        { "orphans", T_BMB, 19, 0x40 },
        TERM_A } },
    { EGKW_SECT_LINE,
      0xFF43,
      "wire",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "half_width", T_INT, 20, 2 },
        { "stflags", T_BMB, 22, 0x33 },
        { "clockwise", T_BMB, 22, 0x20 },
        { "linetype", T_UBF, 23, BITFIELD( 1, 0, 7 ) },
        { "linetype_0_x1", T_INT, 4, 4 },
        { "linetype_0_y1", T_INT, 8, 4 },
        { "linetype_0_x2", T_INT, 12, 4 },
        { "linetype_0_y2", T_INT, 16, 4 },
        { "arc_negflags", T_UBF, 19, BITFIELD( 1, 0, 4 ) },
        { "arc_c1", T_INT, 7, 1 },
        { "arc_c2", T_INT, 11, 1 },
        { "arc_c3", T_INT, 15, 1 },
        { "arc_x1", T_INT, 4, 3 },
        { "arc_y1", T_INT, 8, 3 },
        { "arc_x2", T_INT, 12, 3 },
        { "arc_y2", T_INT, 16, 3 },
        TERM_A } },
    { EGKW_SECT_ARC,
      0xFFFF,
      "arc",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "half_width", T_INT, 20, 2 },
        { "clockwise", T_BMB, 22, 0x20 },
        { "arctype", T_UBF, 23, BITFIELD( 1, 0, 7 ) },
        { "arc_negflags", T_UBF, 19, BITFIELD( 1, 0, 7 ) },
        { "arc_c1", T_INT, 7, 1 },
        { "arc_c2", T_INT, 11, 1 },
        { "arc_c3", T_INT, 15, 1 },
        { "arc_x1", T_INT, 4, 3 },
        { "arc_y1", T_INT, 8, 3 },
        { "arc_x2", T_INT, 12, 3 },
        { "arc_y2", T_INT, 16, 3 },
        { "arctype_other_x1", T_INT, 4, 4 },
        { "arctype_other_y1", T_INT, 8, 4 },
        { "arctype_other_x2", T_INT, 12, 4 },
        { "arctype_other_y2", T_INT, 16, 4 },
        TERM_A } },
    { EGKW_SECT_CIRCLE,
      0xFF53,
      "circle",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "radius", T_INT, 12, 4 },
        { "half_width", T_INT, 20, 4 },
        TERM_A } },
    { EGKW_SECT_RECTANGLE,
      0xFF5F,
      "rectangle",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x1", T_INT, 4, 4 },
        { "y1", T_INT, 8, 4 },
        { "x2", T_INT, 12, 4 },
        { "y2", T_INT, 16, 4 },
        { "bin_rot", T_INT, 20, 2 },
        TERM_A } },
    { EGKW_SECT_JUNCTION,
      0xFFFF,
      "junction",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "width_2", T_INT, 12, 2 },
        TERM_A } },
    { EGKW_SECT_HOLE,
      0xFF53,
      "hole",
      { TERM_F },
      { TERM_S },
      { { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "half_diameter", T_UBF, 12, BITFIELD( 2, 0, 15 ) },
        { "half_drill", T_UBF, 12, BITFIELD( 2, 0, 15 ) },
        TERM_A } },
    { EGKW_SECT_VIA,
      0xFF7F,
      "via",
      { TERM_F },
      { TERM_S },
      { { "shape", T_INT, 2, 1 },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "half_drill", T_UBF, 12, BITFIELD( 2, 0, 15 ) },
        { "half_diameter", T_UBF, 14, BITFIELD( 2, 0, 15 ) },
        { "layers", T_UBF, 16, BITFIELD( 1, 0, 7 ) },
        { "stop", T_BMB, 17, 0x01 },
        TERM_A } },
    { EGKW_SECT_PAD,
      0xFF5F,
      "pad",
      { TERM_F },
      { TERM_S },
      { { "shape", T_INT, 2, 1 },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "half_drill", T_UBF, 12, BITFIELD( 2, 0, 15 ) },
        { "half_diameter", T_UBF, 14, BITFIELD( 2, 0, 15 ) },
        { "bin_rot", T_INT, 16, 2 },
        { "stop", T_BMB, 18, 0x01 },
        { "thermals", T_BMB, 18, 0x04 },
        { "first", T_BMB, 18, 0x08 },
        { "name", T_STR, 19, 5 },
        TERM_A } },
    { EGKW_SECT_SMD,
      0xFF7F,
      "smd",
      { TERM_F },
      { TERM_S },
      { { "roundness", T_INT, 2, 1 },
        { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "half_dx", T_UBF, 12, BITFIELD( 2, 0, 15 ) },
        { "half_dy", T_UBF, 14, BITFIELD( 2, 0, 15 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "stop", T_BMB, 18, 0x01 },
        { "cream", T_BMB, 18, 0x02 },
        { "thermals", T_BMB, 18, 0x04 },
        { "first", T_BMB, 18, 0x08 },
        { "name", T_STR, 19, 5 },
        TERM_A } },
    { EGKW_SECT_PIN,
      0xFF7F,
      "pin",
      { TERM_F },
      { TERM_S },
      { { "function", T_UBF, 2, BITFIELD( 1, 0, 1 ) },
        { "visible", T_UBF, 2, BITFIELD( 1, 6, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "direction", T_UBF, 12, BITFIELD( 1, 0, 3 ) },
        { "length", T_UBF, 12, BITFIELD( 1, 4, 5 ) },
        { "bin_rot", T_UBF, 12, BITFIELD( 1, 6, 7 ) },
        { "swaplevel", T_INT, 13, 1 },
        { "name", T_STR, 14, 10 },
        TERM_A } },
    { EGKW_SECT_GATE,
      0xFF7F,
      "gate",
      { TERM_F },
      { TERM_S },
      { { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "addlevel", T_INT, 12, 1 },
        { "swap", T_INT, 13, 1 },
        { "symno", T_INT, 14, 2 },
        { "name", T_STR, 16, 8 },
        TERM_A } },
    // Masking the element low byte with 0x53 leaks bit 6, so a real-world element
    // with low byte 0x60 (spin plus another flag) fails to match. The whole low
    // byte is flags here; rotation/mirror/spin are decoded from bytes 16-17, and
    // 0x2e is a unique high byte, so match on the high byte alone.
    { EGKW_SECT_ELEMENT,
      0xFF00,
      "element",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "library", T_INT, 12, 2 },
        { "package", T_INT, 14, 2 },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_BMB, 17, 0x10 },
        { "spin", T_BMB, 17, 0x40 },
        TERM_A } },
    { EGKW_SECT_ELEMENT2,
      0xFF5F,
      "element2",
      { TERM_F },
      { TERM_S },
      { { "name", T_STR, 2, 8 }, { "value", T_STR, 10, 14 }, TERM_A } },
    { EGKW_SECT_INSTANCE,
      0xFFFF,
      "instance",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "placed", T_INT, 12, 2 },
        { "gateno", T_INT, 14, 2 },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 10, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "smashed", T_BMB, 18, 0x01 },
        TERM_A } },
    { EGKW_SECT_TEXT,
      0xFF53,
      "text",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "half_size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_NETBUSLABEL,
      0xFFFF,
      "netbuslabel",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_SMASHEDNAME,
      0xFF00,
      "name",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_SMASHEDVALUE,
      0xFF73,
      "value",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_PACKAGEVARIANT,
      0xFF7F,
      "packagevariant",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "package", T_INT, 4, 2 }, { "table", T_STR, 6, 13 }, { "name", T_STR, 19, 5 }, TERM_A } },
    { EGKW_SECT_DEVICE,
      0xFF7F,
      "device",
      { TERM_F },
      { { 2, 2, SS_RECURSIVE, "gates" }, { 4, 2, SS_RECURSIVE, "variants" }, TERM_S },
      { { "gates", T_INT, 2, 2 },
        { "variants", T_INT, 4, 2 },
        { "prefix", T_STR, 8, 5 },
        { "desc", T_STR, 13, 5 },
        { "name", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_PART,
      0xFFFF,
      "part",
      { TERM_F },
      { { 2, 2, SS_DIRECT, nullptr }, TERM_S },
      { { "lib", T_INT, 4, 2 },
        { "device", T_INT, 6, 2 },
        { "variant", T_INT, 8, 1 },
        { "technology", T_INT, 9, 2 },
        { "name", T_STR, 11, 5 },
        { "value", T_STR, 16, 8 },
        TERM_A } },
    { EGKW_SECT_SCHEMABUS, 0xFFFF, nullptr, { TERM_F }, { TERM_S }, { TERM_A } },
    { EGKW_SECT_VARIANTCONNECTIONS, 0xFF7F, "variantconnections", { TERM_F }, { TERM_S }, { TERM_A } },
    { EGKW_SECT_SCHEMACONNECTION, 0xFFFF, nullptr, { TERM_F }, { TERM_S }, { TERM_A } },
    { EGKW_SECT_CONTACTREF,
      0xFF57,
      "contactref",
      { TERM_F },
      { TERM_S },
      { { "partnumber", T_INT, 4, 2 }, { "pin", T_INT, 6, 2 }, TERM_A } },
    { EGKW_SECT_SMASHEDPART,
      0xFFFF,
      "smashedpart",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_SMASHEDGATE,
      0xFFFF,
      "smashedgate",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_ATTRIBUTE,
      0xFF7F,
      "attribute",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },
    { EGKW_SECT_ATTRIBUTEVALUE,
      0xFFFF,
      "attribute-value",
      { TERM_F },
      { TERM_S },
      { { "symbol", T_STR, 2, 5 }, { "attribute", T_STR, 7, 17 }, TERM_A } },
    { EGKW_SECT_FRAME,
      0xFFFF,
      "frame",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x1", T_INT, 4, 4 },
        { "y1", T_INT, 8, 4 },
        { "x2", T_INT, 12, 4 },
        { "y2", T_INT, 16, 4 },
        { "cols", T_INT, 20, 1 },
        { "rows", T_INT, 21, 1 },
        { "borders", T_INT, 22, 1 },
        TERM_A } },
    { EGKW_SECT_SMASHEDXREF,
      0xFFFF,
      "smashedxref",
      { TERM_F },
      { TERM_S },
      { { "layer", T_UBF, 3, BITFIELD( 1, 0, 7 ) },
        { "x", T_INT, 4, 4 },
        { "y", T_INT, 8, 4 },
        { "size", T_INT, 12, 2 },
        { "ratio", T_UBF, 14, BITFIELD( 2, 2, 6 ) },
        { "bin_rot", T_UBF, 16, BITFIELD( 2, 0, 11 ) },
        { "mirrored", T_UBF, 16, BITFIELD( 2, 12, 12 ) },
        { "spin", T_UBF, 16, BITFIELD( 2, 14, 14 ) },
        { "textfield", T_STR, 18, 5 },
        TERM_A } },

    // unknown leaves
    { 0x5300, 0xFFFF, nullptr, { TERM_F }, { TERM_S }, { TERM_A } },
    { 0x2d84, 0xFFFF, nullptr, { TERM_F }, { TERM_S }, { TERM_A } },
    { 0, 0, nullptr, { TERM_F }, { TERM_S }, { TERM_A } } // end of table
};
} // namespace


EAGLE_BIN_PARSER::EGB_NODE* EAGLE_BIN_PARSER::EGB_NODE::AddChild( int aId, const wxString& aName )
{
    auto child = std::make_unique<EGB_NODE>();
    child->id = aId;
    child->name = aName;
    child->parent = this;
    children.push_back( std::move( child ) );

    return children.back().get();
}


wxString EAGLE_BIN_PARSER::EGB_NODE::Prop( const wxString& aKey ) const
{
    auto it = props.find( aKey );
    return it == props.end() ? wxString() : it->second;
}


long EAGLE_BIN_PARSER::EGB_NODE::PropLong( const wxString& aKey ) const
{
    long val = 0;
    Prop( aKey ).ToLong( &val );

    return val;
}


wxString EAGLE_BIN_PARSER::EGB_NODE::PropDoubled( const wxString& aKey ) const
{
    wxLongLong_t val = 0;
    Prop( aKey ).ToLongLong( &val );

    return wxString::Format( wxS( "%lld" ), val * 2 );
}


EAGLE_BIN_PARSER::EGB_NODE* EAGLE_BIN_PARSER::EGB_NODE::FindChildById( int aId ) const
{
    for( const auto& child : children )
    {
        if( child->id == aId )
            return child.get();
    }

    return nullptr;
}


EAGLE_BIN_PARSER::EGB_NODE* EAGLE_BIN_PARSER::EGB_NODE::FindChildByName( const wxString& aName ) const
{
    for( const auto& child : children )
    {
        if( child->name == aName )
            return child.get();
    }

    return nullptr;
}


EAGLE_BIN_PARSER::EAGLE_BIN_PARSER() = default;
EAGLE_BIN_PARSER::~EAGLE_BIN_PARSER() = default;


bool EAGLE_BIN_PARSER::IsBinaryEagle( wxInputStream& aStream )
{
    uint8_t buf[2] = { 0, 0 };

    if( !aStream.IsOk() )
        return false;

    aStream.Read( buf, 2 );

    if( aStream.LastRead() != 2 )
        return false;

    if( buf[0] == 0x10 && ( buf[1] == 0x00 || buf[1] == 0x80 ) )
        return true;

    return false;
}


void EAGLE_BIN_PARSER::requireBytes( size_t aOffs, size_t aLen ) const
{
    if( m_buf == nullptr || aOffs > m_buf->size() || aLen > m_buf->size() - aOffs )
        THROW_IO_ERROR( _( "Short read in Eagle binary file (field out of bounds)." ) );
}


uint32_t EAGLE_BIN_PARSER::loadU32( size_t aOffs, unsigned aLen ) const
{
    requireBytes( aOffs, aLen );

    uint32_t l = 0;

    for( unsigned n = 0; n < aLen; n++ )
    {
        l <<= 8;
        l |= ( *m_buf )[aOffs + aLen - n - 1];
    }

    return l;
}


int32_t EAGLE_BIN_PARSER::loadS32( size_t aOffs, unsigned aLen ) const
{
    requireBytes( aOffs, aLen );

    uint32_t l = 0;

    if( ( *m_buf )[aOffs + aLen - 1] & 0x80 )
        l = 0xFFFFFFFF;

    for( unsigned n = 0; n < aLen; n++ )
    {
        l <<= 8;
        l |= ( *m_buf )[aOffs + aLen - n - 1];
    }

    return static_cast<int32_t>( l );
}


bool EAGLE_BIN_PARSER::loadBmb( size_t aOffs, uint32_t aMask ) const
{
    requireBytes( aOffs, 1 );

    return ( ( *m_buf )[aOffs] & aMask ) != 0;
}


uint32_t EAGLE_BIN_PARSER::loadUbf( size_t aOffs, uint32_t aField ) const
{
    unsigned first = ( aField >> 8 ) & 0xff;
    unsigned last = aField & 0xff;
    uint32_t mask = ( 1u << ( last - first + 1 ) ) - 1;

    // The high byte of the descriptor is the read length; keeping it inline ties
    // the offset and field together rather than splitting them into locals.
    uint32_t val = loadU32( aOffs, ( aField >> 16 ) & 0xff ) >> first;

    return val & mask;
}


wxString EAGLE_BIN_PARSER::loadStr( size_t aOffs, unsigned aLen ) const
{
    requireBytes( aOffs, aLen );

    const char* start = reinterpret_cast<const char*>( m_buf->data() + aOffs );

    // The field is fixed length and NUL padded; stop at the first NUL but never
    // run past the field.
    size_t n = 0;

    while( n < aLen && start[n] != '\0' )
        n++;

    return wxString::FromUTF8( start, n );
}


double EAGLE_BIN_PARSER::loadDouble( size_t aOffs ) const
{
    static_assert( sizeof( double ) == 8, "Eagle binary doubles are 8-byte IEEE-754" );

    requireBytes( aOffs, sizeof( double ) );

    // The file stores a little-endian IEEE-754 double. Assemble the bit pattern
    // from individual bytes so decoding does not depend on host byte order, then
    // reinterpret those bits as a double.
    uint64_t bits = 0;

    for( unsigned n = 0; n < sizeof( double ); n++ )
        bits |= static_cast<uint64_t>( ( *m_buf )[aOffs + n] ) << ( 8 * n );

    double d = 0.0;
    memcpy( &d, &bits, sizeof( d ) );

    return d;
}


int EAGLE_BIN_PARSER::readBlock( long& aNumBlocks, EGB_NODE* aParent )
{
    if( m_pos + 24 > m_buf->size() )
        THROW_IO_ERROR( _( "Short read in Eagle binary file (truncated block)." ) );

    size_t blockStart = m_pos;
    m_pos += 24;

    int processed = 1;

    // The top-level drawing record carries the total block count.
    if( aNumBlocks < 0 && ( *m_buf )[blockStart] == 0x10 )
        aNumBlocks = loadS32( blockStart + 4, 4 );

    const SCRIPT_ROW* sc = nullptr;

    for( const SCRIPT_ROW* row = g_script; row->cmd != 0; row++ )
    {
        unsigned cmdh = ( row->cmd >> 8 ) & 0xFF;
        unsigned cmdl = row->cmd & 0xFF;
        unsigned mskh = ( row->cmdMask >> 8 ) & 0xFF;
        unsigned mskl = row->cmdMask & 0xFF;

        if( ( cmdh != ( ( *m_buf )[blockStart] & mskh ) ) || ( cmdl != ( ( *m_buf )[blockStart + 1] & mskl ) ) )
        {
            continue;
        }

        bool match = true;

        for( const FMATCH* fm = row->fmatch; fm->offs != 0; fm++ )
        {
            if( loadS32( blockStart + fm->offs, fm->len ) != fm->val )
            {
                match = false;
                break;
            }
        }

        if( match )
        {
            sc = row;
            break;
        }
    }

    if( sc == nullptr )
    {
        THROW_IO_ERROR( wxString::Format( _( "Unknown Eagle binary block id 0x%02x%02x at offset %zu." ),
                                          (unsigned) ( *m_buf )[blockStart], (unsigned) ( *m_buf )[blockStart + 1],
                                          blockStart ) );
    }

    EGB_NODE* node =
            aParent->AddChild( static_cast<int>( sc->cmd ),
                               sc->name ? wxString::FromUTF8( sc->name ) : wxString( wxS( "UNKNOWN" ) ) );

    for( const ATTR* at = sc->attrs; at->name != nullptr; at++ )
    {
        wxString val;

        switch( at->type )
        {
        // KiCad's Eagle XML reader parses boolean attributes as "yes"/"no", so
        // emit T_BMB fields that way rather than "1"/"0".
        case T_BMB: val = loadBmb( blockStart + at->offs, at->len ) ? wxS( "yes" ) : wxS( "no" ); break;
        case T_UBF: val = wxString::Format( wxS( "%u" ), loadUbf( blockStart + at->offs, at->len ) ); break;
        case T_INT: val = wxString::Format( wxS( "%d" ), loadS32( blockStart + at->offs, at->len ) ); break;
        case T_DBL: val = wxString::FromCDouble( loadDouble( blockStart + at->offs ) ); break;
        case T_STR: val = loadStr( blockStart + at->offs, at->len ); break;
        }

        node->props[wxString::FromUTF8( at->name )] = val;
    }

    aNumBlocks--;

    for( const SUBSECT* ss = sc->subs; ss->offs != 0; ss++ )
    {
        uint32_t  numch = loadU32( blockStart + ss->offs, ss->len );
        EGB_NODE* lpar = node;

        if( ss->treeName != nullptr )
            lpar = node->AddChild( 0, wxString::FromUTF8( ss->treeName ) );

        if( ss->ssType == SS_DIRECT )
        {
            for( uint32_t n = 0; n < numch && aNumBlocks > 0; n++ )
            {
                int res = readBlock( aNumBlocks, lpar );
                processed += res;
            }
        }
        else
        {
            if( ss->ssType == SS_RECURSIVE_MINUS_1 && numch > 0 )
                numch--;

            long rem = numch;

            for( uint32_t n = 0; n < numch && rem > 0; n++ )
            {
                int res = readBlock( rem, lpar );
                aNumBlocks -= res;
                processed += res;
            }
        }
    }

    return processed;
}


bool EAGLE_BIN_PARSER::readNotes()
{
    m_freeText.clear();
    m_freeTextCursor = 0;

    if( m_pos + 8 > m_buf->size() )
        return false;

    // The free-text section starts with the 0x1312 sentinel.
    if( ( *m_buf )[m_pos] != 0x13 || ( *m_buf )[m_pos + 1] != 0x12 )
        return false;

    int textLen = loadS32( m_pos + 4, 2 );
    m_pos += 8;

    if( textLen < 0 )
        return false;

    // A trailing 4-byte checksum follows the text payload.
    size_t total = static_cast<size_t>( textLen ) + 4;

    if( m_pos + total > m_buf->size() )
        return false;

    // Split the blob into NUL-delimited strings; an empty string terminates.
    size_t end = m_pos + total;
    size_t cur = m_pos;

    while( cur < end && ( *m_buf )[cur] != '\0' )
    {
        size_t s = cur;

        while( cur < end && ( *m_buf )[cur] != '\0' )
            cur++;

        m_freeText.push_back( wxString::FromUTF8( reinterpret_cast<const char*>( m_buf->data() + s ), cur - s ) );
        cur++; // skip the NUL
    }

    m_pos = end;
    return true;
}


const wxString& EAGLE_BIN_PARSER::nextLongText()
{
    if( m_freeTextCursor >= m_freeText.size() )
    {
        wxLogTrace( traceEagleIo, wxS( "Eagle bin: free-text reference out of strings" ) );
        m_invalidText = wxS( "<invalid>" );
        return m_invalidText;
    }

    return m_freeText[m_freeTextCursor++];
}


bool EAGLE_BIN_PARSER::readDrc( DRC_CTX& aDrc )
{
    if( m_pos + 4 > m_buf->size() )
        return false;

    // DRC start sentinel 0x10 0x04 0x00 0x20.
    if( !( ( *m_buf )[m_pos] == 0x10 && ( *m_buf )[m_pos + 1] == 0x04 && ( *m_buf )[m_pos + 2] == 0x00
           && ( *m_buf )[m_pos + 3] == 0x20 ) )
    {
        return false;
    }

    m_pos += 4;

    // Walk the variable-length preamble looking for the 0x12345678 end marker
    // that immediately follows a NUL byte.
    bool found = false;

    while( !found )
    {
        if( m_pos + 1 > m_buf->size() )
            return false;

        uint8_t c = ( *m_buf )[m_pos++];

        if( c == '\0' )
        {
            if( m_pos + 4 > m_buf->size() )
                return false;

            if( ( *m_buf )[m_pos] == 0x78 && ( *m_buf )[m_pos + 1] == 0x56 && ( *m_buf )[m_pos + 2] == 0x34
                && ( *m_buf )[m_pos + 3] == 0x12 )
            {
                found = true;
            }

            m_pos += 4;
        }
    }

    const size_t kDrcLen = 244;

    if( m_pos + kDrcLen > m_buf->size() )
        return false;

    size_t b = m_pos;

    auto mil = [&]( size_t aOffs ) -> long
    {
        return static_cast<long>( loadS32( b + aOffs, 4 ) / 2.54 / 100 );
    };

    aDrc.mdWireWire = mil( 0 );
    aDrc.msWidth = mil( 64 );
    aDrc.rvPadTop = loadDouble( b + 84 );
    aDrc.rvPadInner = loadDouble( b + 92 );
    aDrc.rvPadBottom = loadDouble( b + 100 );

    m_pos += kDrcLen;
    return true;
}


void EAGLE_BIN_PARSER::fixLongText( EGB_NODE* aNode, const wxString& aField )
{
    auto it = aNode->props.find( aField );

    if( it == aNode->props.end() || it->second.IsEmpty() )
        return;

    // A leading 0x7F byte marks a deferred long-text reference into the notes.
    if( static_cast<unsigned char>( it->second[0] ) == 0x7F )
        it->second = nextLongText();
}


void EAGLE_BIN_PARSER::arcDecode( EGB_NODE* aElem, int aArcType, int aLineType )
{
    auto fixThreeByte = []( long num, bool neg ) -> long
    {
        if( num < 0 && neg )
            return num;
        else if( num > 0 && neg )
            return num - 0x800000;
        else if( num < 0 && !neg )
            return num + 0x800000;

        return num;
    };

    auto fixOneByte = []( long num ) -> long
    {
        return num < 0 ? num + 0x80 : num;
    };

    auto setLong = [&]( const wxString& aKey, long aVal )
    {
        aElem->props[aKey] = wxString::Format( wxS( "%ld" ), aVal );
    };

    // Linear interpolation of the unconstrained center coordinate. The 64-bit
    // product cannot overflow (Eagle stores 24-bit coordinates) while a plain
    // long would on 32-bit platforms, and integer division truncates toward zero.
    auto interpolate = []( int64_t aNumerator, int64_t aSpan, int64_t aDivisor, int64_t aOffset ) -> long
    {
        return static_cast<long>( aNumerator * aSpan / aDivisor + aOffset );
    };

    if( aLineType == 129 || aArcType == 0 )
    {
        long arcFlags = aElem->PropLong( wxS( "arc_negflags" ) );
        long x1 = fixThreeByte( aElem->PropLong( wxS( "arc_x1" ) ), arcFlags & 0x02 );
        long y1 = fixThreeByte( aElem->PropLong( wxS( "arc_y1" ) ), arcFlags & 0x04 );
        long x2 = fixThreeByte( aElem->PropLong( wxS( "arc_x2" ) ), arcFlags & 0x08 );
        long y2 = fixThreeByte( aElem->PropLong( wxS( "arc_y2" ) ), arcFlags & 0x10 );

        long c = fixOneByte( aElem->PropLong( wxS( "arc_c1" ) ) )
                 + 256 * fixOneByte( aElem->PropLong( wxS( "arc_c2" ) ) )
                 + 256L * 256 * fixOneByte( aElem->PropLong( wxS( "arc_c3" ) ) );
        c = fixThreeByte( c, arcFlags & 0x01 );

        setLong( wxS( "x1" ), x1 );
        setLong( wxS( "y1" ), y1 );
        setLong( wxS( "x2" ), x2 );
        setLong( wxS( "y2" ), y2 );

        long x3 = ( x1 + x2 ) / 2;
        long y3 = ( y1 + y2 ) / 2;
        long cx = 0, cy = 0;

        if( x1 == x2 && y1 == y2 )
        {
            // Degenerate arc with coincident endpoints; both interpolation
            // branches would divide by zero, so collapse it to a point.
            cx = x1;
            cy = y1;
        }
        else if( std::abs( x2 - x1 ) < std::abs( y2 - y1 ) )
        {
            cx = c;
            cy = interpolate( x3 - cx, x2 - x1, y2 - y1, y3 );
        }
        else
        {
            cy = c;
            cx = interpolate( y3 - cy, y2 - y1, x2 - x1, x3 );
        }

        long radius = static_cast<long>( std::hypot( cx - x2, cy - y2 ) );
        setLong( wxS( "radius" ), radius );
        setLong( wxS( "x" ), cx );
        setLong( wxS( "y" ), cy );

        if( cx == x2 && cy == y1 && x2 < x1 && y2 > y1 )
        {
            aElem->props[wxS( "StartAngle" )] = wxS( "90" );
            aElem->props[wxS( "Delta" )] = wxS( "90" );
        }
        else if( cx == x1 && cy == y2 && x2 < x1 && y1 > y2 )
        {
            aElem->props[wxS( "StartAngle" )] = wxS( "0" );
            aElem->props[wxS( "Delta" )] = wxS( "90" );
        }
        else if( cx == x2 && cy == y1 && x2 > x1 && y1 > y2 )
        {
            aElem->props[wxS( "StartAngle" )] = wxS( "270" );
            aElem->props[wxS( "Delta" )] = wxS( "90" );
        }
        else if( cx == x1 && cy == y2 && x2 > x1 && y2 > y1 )
        {
            aElem->props[wxS( "StartAngle" )] = wxS( "180" );
            aElem->props[wxS( "Delta" )] = wxS( "90" );
        }
        else
        {
            double theta1 = 180.0 - 180.0 / M_PI * atan2( cy - y1, x1 - cx );
            double theta2 = 180.0 - 180.0 / M_PI * atan2( cy - y2, x2 - cx );
            double deltaTheta = theta2 - theta1;

            while( theta1 > 360 )
                theta1 -= 360;

            while( deltaTheta < -180 )
                deltaTheta += 360;

            while( deltaTheta > 180 )
                deltaTheta -= 360;

            setLong( wxS( "StartAngle" ), static_cast<long>( theta1 ) );
            setLong( wxS( "Delta" ), static_cast<long>( deltaTheta ) );
        }
    }
    else if( ( aLineType > 0 && aLineType < 129 ) || aArcType > 0 )
    {
        long x1 = 0, y1 = 0, x2 = 0, y2 = 0, cx = 0, cy = 0;

        if( aElem->HasProp( wxS( "arctype_other_x1" ) ) )
        {
            x1 = aElem->PropLong( wxS( "arctype_other_x1" ) );
            y1 = aElem->PropLong( wxS( "arctype_other_y1" ) );
            x2 = aElem->PropLong( wxS( "arctype_other_x2" ) );
            y2 = aElem->PropLong( wxS( "arctype_other_y2" ) );
        }
        else
        {
            x1 = aElem->PropLong( wxS( "linetype_0_x1" ) );
            y1 = aElem->PropLong( wxS( "linetype_0_y1" ) );
            x2 = aElem->PropLong( wxS( "linetype_0_x2" ) );
            y2 = aElem->PropLong( wxS( "linetype_0_y2" ) );
        }

        bool cxyOk = true;
        auto setAngles = [&]( const char* aStart, const char* aDelta )
        {
            aElem->props[wxS( "StartAngle" )] = wxString::FromUTF8( aStart );
            aElem->props[wxS( "Delta" )] = wxString::FromUTF8( aDelta );
        };

        if( aLineType == 0x78 || aArcType == 0x01 )
        {
            cx = std::min( x1, x2 );
            cy = std::min( y1, y2 );
            setAngles( "180", "90" );
        }
        else if( aLineType == 0x79 || aArcType == 0x02 )
        {
            cx = std::max( x1, x2 );
            cy = std::min( y1, y2 );
            setAngles( "270", "90" );
        }
        else if( aLineType == 0x7a || aArcType == 0x03 )
        {
            cx = std::max( x1, x2 );
            cy = std::max( y1, y2 );
            setAngles( "0", "90" );
        }
        else if( aLineType == 0x7b || aArcType == 0x04 )
        {
            cx = std::min( x1, x2 );
            cy = std::max( y1, y2 );
            setAngles( "90", "90" );
        }
        else if( aLineType == 0x7c || aArcType == 0x05 )
        {
            cx = ( x1 + x2 ) / 2;
            cy = ( y1 + y2 ) / 2;
            setAngles( "90", "180" );
        }
        else if( aLineType == 0x7d || aArcType == 0x06 )
        {
            cx = ( x1 + x2 ) / 2;
            cy = ( y1 + y2 ) / 2;
            setAngles( "270", "180" );
        }
        else if( aLineType == 0x7e || aArcType == 0x07 )
        {
            cx = ( x1 + x2 ) / 2;
            cy = ( y1 + y2 ) / 2;
            setAngles( "180", "180" );
        }
        else if( aLineType == 0x7f || aArcType == 0x08 )
        {
            cx = ( x1 + x2 ) / 2;
            cy = ( y1 + y2 ) / 2;
            setAngles( "0", "180" );
        }
        else
        {
            cxyOk = false;
        }

        if( !cxyOk )
            cx = cy = 0;

        long radius = static_cast<long>( std::hypot( cx - x2, cy - y2 ) );
        setLong( wxS( "radius" ), radius );
        setLong( wxS( "x" ), cx );
        setLong( wxS( "y" ), cy );
    }
}


void EAGLE_BIN_PARSER::postprocWires( EGB_NODE* aRoot )
{
    if( aRoot->id == EGKW_SECT_LINE )
    {
        int lineType = aRoot->HasProp( wxS( "linetype" ) ) ? (int) aRoot->PropLong( wxS( "linetype" ) ) : -1;

        if( lineType >= 0 )
        {
            // Straight and arc wires both keep their endpoints in the linetype_0
            // fields.
            aRoot->props[wxS( "x1" )] = aRoot->Prop( wxS( "linetype_0_x1" ) );
            aRoot->props[wxS( "y1" )] = aRoot->Prop( wxS( "linetype_0_y1" ) );
            aRoot->props[wxS( "x2" )] = aRoot->Prop( wxS( "linetype_0_x2" ) );
            aRoot->props[wxS( "y2" )] = aRoot->Prop( wxS( "linetype_0_y2" ) );
            aRoot->props[wxS( "width" )] = aRoot->PropDoubled( wxS( "half_width" ) );
        }

        if( lineType > 0 )
        {
            // arcDecode adds the center, radius and swept angle. KiCad's wire
            // reader needs the endpoints plus a "curve" (the swept angle).
            arcDecode( aRoot, -1, lineType );

            if( aRoot->HasProp( wxS( "Delta" ) ) )
                aRoot->props[wxS( "curve" )] = aRoot->Prop( wxS( "Delta" ) );
        }
    }

    for( const auto& child : aRoot->children )
        postprocWires( child.get() );
}


void EAGLE_BIN_PARSER::postprocArcs( EGB_NODE* aRoot )
{
    if( aRoot->id == EGKW_SECT_ARC )
    {
        int arcType = aRoot->HasProp( wxS( "arctype" ) ) ? (int) aRoot->PropLong( wxS( "arctype" ) ) : -1;

        if( arcType == 0 )
        {
            aRoot->props[wxS( "x1" )] = aRoot->Prop( wxS( "arc_x1" ) );
            aRoot->props[wxS( "y1" )] = aRoot->Prop( wxS( "arc_y1" ) );
            aRoot->props[wxS( "x2" )] = aRoot->Prop( wxS( "arc_x2" ) );
            aRoot->props[wxS( "y2" )] = aRoot->Prop( wxS( "arc_y2" ) );
        }
        else if( arcType > 0 )
        {
            aRoot->props[wxS( "x1" )] = aRoot->Prop( wxS( "arctype_other_x1" ) );
            aRoot->props[wxS( "y1" )] = aRoot->Prop( wxS( "arctype_other_y1" ) );
            aRoot->props[wxS( "x2" )] = aRoot->Prop( wxS( "arctype_other_x2" ) );
            aRoot->props[wxS( "y2" )] = aRoot->Prop( wxS( "arctype_other_y2" ) );
        }

        if( arcType >= 0 )
            aRoot->props[wxS( "width" )] = aRoot->PropDoubled( wxS( "half_width" ) );

        arcDecode( aRoot, arcType, -1 );

        if( aRoot->HasProp( wxS( "Delta" ) ) )
            aRoot->props[wxS( "curve" )] = aRoot->Prop( wxS( "Delta" ) );
    }

    for( const auto& child : aRoot->children )
        postprocArcs( child.get() );
}


void EAGLE_BIN_PARSER::postprocVias( EGB_NODE* aRoot )
{
    if( aRoot->id == EGKW_SECT_VIA )
    {
        // KiCad requires an "extent" layer-range string. The binary layers byte
        // is not a 1:1 map and pre-v6 vias are through-hole (0xF0 sentinel), so
        // span the full copper stack. Blind/buried vias are out of scope.
        aRoot->props[wxS( "extent" )] = wxS( "1-16" );
    }

    for( const auto& child : aRoot->children )
        postprocVias( child.get() );
}


void EAGLE_BIN_PARSER::postprocUnits( EGB_NODE* aRoot )
{
    // Binary coordinates are decimicrons (0.1 um); KiCad's XML reader assumes
    // millimetres for unitless values. Rewrite every dimensional attribute as a
    // millimetre decimal so the reader scales it correctly. Counts, layer
    // numbers, ratios, angles and booleans are left untouched.
    static const wxString dimAttrs[] = { wxS( "x" ),      wxS( "y" ),     wxS( "x1" ),       wxS( "y1" ),
                                         wxS( "x2" ),     wxS( "y2" ),    wxS( "x3" ),       wxS( "y3" ),
                                         wxS( "width" ),  wxS( "drill" ), wxS( "diameter" ), wxS( "radius" ),
                                         wxS( "size" ),   wxS( "dx" ),    wxS( "dy" ),       wxS( "spacing" ),
                                         wxS( "isolate" ) };

    for( const wxString& key : dimAttrs )
    {
        auto it = aRoot->props.find( key );

        if( it == aRoot->props.end() )
            continue;

        double du = 0;

        if( it->second.ToCDouble( &du ) )
            it->second = wxString::FromCDouble( du * 0.0001, 4 );
    }

    for( const auto& child : aRoot->children )
        postprocUnits( child.get() );
}


void EAGLE_BIN_PARSER::postprocCircles( EGB_NODE* aRoot )
{
    if( aRoot->id == EGKW_SECT_CIRCLE && aRoot->HasProp( wxS( "half_width" ) ) )
    {
        aRoot->props[wxS( "width" )] = aRoot->PropDoubled( wxS( "half_width" ) );
    }

    for( const auto& child : aRoot->children )
        postprocCircles( child.get() );
}


void EAGLE_BIN_PARSER::postprocSmd( EGB_NODE* aRoot )
{
    if( aRoot->id == EGKW_SECT_SMD )
    {
        if( aRoot->HasProp( wxS( "half_dx" ) ) )
        {
            aRoot->props[wxS( "dx" )] = aRoot->PropDoubled( wxS( "half_dx" ) );
        }

        if( aRoot->HasProp( wxS( "half_dy" ) ) )
        {
            aRoot->props[wxS( "dy" )] = aRoot->PropDoubled( wxS( "half_dy" ) );
        }
    }

    for( const auto& child : aRoot->children )
        postprocSmd( child.get() );
}


void EAGLE_BIN_PARSER::postprocDimensions( EGB_NODE* aRoot )
{
    if( aRoot->id == EGKW_SECT_PAD || aRoot->id == EGKW_SECT_HOLE || aRoot->id == EGKW_SECT_VIA
        || aRoot->id == EGKW_SECT_TEXT )
    {
        if( aRoot->HasProp( wxS( "half_drill" ) ) )
        {
            aRoot->props[wxS( "drill" )] = aRoot->PropDoubled( wxS( "half_drill" ) );
        }

        if( aRoot->HasProp( wxS( "half_diameter" ) ) )
        {
            aRoot->props[wxS( "diameter" )] = aRoot->PropDoubled( wxS( "half_diameter" ) );
        }

        if( aRoot->HasProp( wxS( "half_size" ) ) )
        {
            aRoot->props[wxS( "size" )] = aRoot->PropDoubled( wxS( "half_size" ) );
        }
    }

    for( const auto& child : aRoot->children )
        postprocDimensions( child.get() );
}


bool EAGLE_BIN_PARSER::isRotatable( int aId ) const
{
    switch( aId )
    {
    case EGKW_SECT_SMD:
    case EGKW_SECT_PIN:
    case EGKW_SECT_RECTANGLE:
    case EGKW_SECT_PAD:
    case EGKW_SECT_TEXT:
    case EGKW_SECT_SMASHEDVALUE:
    case EGKW_SECT_SMASHEDNAME:
    case EGKW_SECT_NETBUSLABEL:
    case EGKW_SECT_SMASHEDXREF:
    case EGKW_SECT_ATTRIBUTE:
    case EGKW_SECT_SMASHEDGATE:
    case EGKW_SECT_SMASHEDPART:
    case EGKW_SECT_INSTANCE:
    case EGKW_SECT_ELEMENT: return true;
    default: return false;
    }
}


void EAGLE_BIN_PARSER::postprocRotation( EGB_NODE* aRoot )
{
    if( isRotatable( aRoot->id ) && aRoot->HasProp( wxS( "bin_rot" ) ) )
    {
        // mirrored/spin are read as T_BMB ("yes"/"no") or as a T_UBF integer
        // depending on the record, so treat anything other than the false tokens
        // as set.
        auto flagSet = [&]( const wxString& aKey )
        {
            if( !aRoot->HasProp( aKey ) )
                return false;

            const wxString v = aRoot->Prop( aKey );
            return v != wxS( "no" ) && v != wxS( "0" );
        };

        bool mirrored = flagSet( wxS( "mirrored" ) );
        bool spin = flagSet( wxS( "spin" ) );

        long     deg = aRoot->PropLong( wxS( "bin_rot" ) );
        wxString rot;

        if( spin )
            rot << wxS( "S" );

        if( mirrored )
            rot << wxS( "M" );

        rot << wxS( "R" );

        if( deg >= 1024 )
            rot << wxString::Format( wxS( "%ld" ), ( 360 * deg ) / 4096 ); // v4/v5
        else if( deg > 0 )
            rot << wxString::Format( wxS( "%ld" ), ( deg & 0x00f0 ) * 90 ); // v3
        else
            rot << wxS( "0" );

        aRoot->props[wxS( "rot" )] = rot;
    }

    for( const auto& child : aRoot->children )
        postprocRotation( child.get() );
}


void EAGLE_BIN_PARSER::postprocFreeText( EGB_NODE* aRoot )
{
    switch( aRoot->id )
    {
    case EGKW_SECT_TEXT:
    case EGKW_SECT_NETBUSLABEL:
    case EGKW_SECT_SMASHEDNAME:
    case EGKW_SECT_SMASHEDVALUE:
    case EGKW_SECT_SMASHEDPART:
    case EGKW_SECT_SMASHEDGATE:
    case EGKW_SECT_ATTRIBUTE:
    case EGKW_SECT_SMASHEDXREF: fixLongText( aRoot, wxS( "textfield" ) ); break;

    case EGKW_SECT_LAYER:
    case EGKW_SECT_LIBRARY:
    case EGKW_SECT_SIGNAL:
    case EGKW_SECT_SYMBOL:
    case EGKW_SECT_SCHEMANET:
    case EGKW_SECT_PAD:
    case EGKW_SECT_SMD:
    case EGKW_SECT_PIN:
    case EGKW_SECT_GATE: fixLongText( aRoot, wxS( "name" ) ); break;

    case EGKW_SECT_ELEMENT2:
    case EGKW_SECT_PART:
        fixLongText( aRoot, wxS( "name" ) );
        fixLongText( aRoot, wxS( "value" ) );
        break;

    case EGKW_SECT_DEVICES:
    case EGKW_SECT_SYMBOLS: fixLongText( aRoot, wxS( "library" ) ); break;

    case EGKW_SECT_PACKAGEVARIANT:
        fixLongText( aRoot, wxS( "table" ) );
        fixLongText( aRoot, wxS( "name" ) );
        break;

    case EGKW_SECT_PACKAGE:
        fixLongText( aRoot, wxS( "desc" ) );
        fixLongText( aRoot, wxS( "name" ) );
        break;

    case EGKW_SECT_PACKAGES:
        fixLongText( aRoot, wxS( "desc" ) );
        fixLongText( aRoot, wxS( "library" ) );
        break;

    case EGKW_SECT_SCHEMA: fixLongText( aRoot, wxS( "xref_format" ) ); break;

    case EGKW_SECT_ATTRIBUTEVALUE:
        fixLongText( aRoot, wxS( "symbol" ) );
        fixLongText( aRoot, wxS( "attribute" ) );
        break;

    case EGKW_SECT_DEVICE:
        fixLongText( aRoot, wxS( "prefix" ) );
        fixLongText( aRoot, wxS( "desc" ) );
        fixLongText( aRoot, wxS( "name" ) );
        break;

    default: break;
    }

    for( const auto& child : aRoot->children )
        postprocFreeText( child.get() );
}


void EAGLE_BIN_PARSER::postprocLayers( EGB_NODE* aDrawing, EGB_NODE* aLayers )
{
    // Move every drawing/layer under the synthetic drawing/layers node, keeping
    // order. Reparenting is by ownership transfer.
    std::vector<std::unique_ptr<EGB_NODE>> kept;

    for( auto& child : aDrawing->children )
    {
        if( child->id == EGKW_SECT_LAYER )
        {
            // The binary stores "visible" as a 2-bit field, but KiCad's reader
            // parses it as a yes/no bool. Normalize to a truthy token.
            if( child->HasProp( wxS( "visible" ) ) )
            {
                child->props[wxS( "visible" )] = child->PropLong( wxS( "visible" ) ) != 0 ? wxS( "yes" ) : wxS( "no" );
            }

            child->parent = aLayers;
            aLayers->children.push_back( std::move( child ) );
        }
        else
        {
            kept.push_back( std::move( child ) );
        }
    }

    aDrawing->children = std::move( kept );
}


void EAGLE_BIN_PARSER::postprocDrc( EGB_NODE* aDrcNode, const DRC_CTX& aDrc )
{
    auto addParam = [&]( const wxString& aName, const wxString& aValue )
    {
        EGB_NODE* p = aDrcNode->AddChild( EGKW_SECT_DRC, wxS( "param" ) );
        p->props[wxS( "name" )] = aName;
        p->props[wxS( "value" )] = aValue;
    };

    addParam( wxS( "mdWireWire" ), wxString::Format( wxS( "%ldmil" ), aDrc.mdWireWire ) );
    addParam( wxS( "msWidth" ), wxString::Format( wxS( "%ldmil" ), aDrc.msWidth ) );
    addParam( wxS( "rvPadTop" ), wxString::FromCDouble( aDrc.rvPadTop ) );
    addParam( wxS( "rvPadInner" ), wxString::FromCDouble( aDrc.rvPadInner ) );
    addParam( wxS( "rvPadBottom" ), wxString::FromCDouble( aDrc.rvPadBottom ) );
}


void EAGLE_BIN_PARSER::postprocLibs( EGB_NODE* aLibraries )
{
    // In a board, the libraries node holds packages nodes directly; the XML
    // schema expects each wrapped in a library node. Wrap every bare packages
    // child.
    if( aLibraries == nullptr )
        return;

    if( aLibraries->FindChildById( EGKW_SECT_LIBRARY ) != nullptr )
        return; // already a proper library subtree

    std::vector<std::unique_ptr<EGB_NODE>> wrapped;

    for( auto& child : aLibraries->children )
    {
        if( child->id != EGKW_SECT_PACKAGES )
            continue;

        auto lib = std::make_unique<EGB_NODE>();
        lib->id = EGKW_SECT_LIBRARY;
        lib->name = wxS( "library" );
        lib->parent = aLibraries;
        child->parent = lib.get();
        lib->children.push_back( std::move( child ) );
        wrapped.push_back( std::move( lib ) );
    }

    if( !wrapped.empty() )
        aLibraries->children = std::move( wrapped );
}


void EAGLE_BIN_PARSER::postprocElements( EGB_NODE* aElements )
{
    if( aElements == nullptr )
        return;

    // Each element is followed (as a child) by an element2 record carrying its
    // name and value; merge those up onto the element.
    for( auto& el : aElements->children )
    {
        if( el->children.empty() || el->children.front()->id != EGKW_SECT_ELEMENT2 )
            continue;

        for( auto& el2 : el->children )
        {
            if( el2->id != EGKW_SECT_ELEMENT2 )
                continue;

            for( const auto& [key, value] : el2->props )
            {
                if( key == wxS( "name" ) )
                {
                    if( value == wxS( "-" ) )
                        el->props[wxS( "name" )] = wxS( "HYPHEN" );
                    else
                        el->props[wxS( "name" )] = value;
                }
                else if( key == wxS( "value" ) )
                {
                    el->props[wxS( "value" )] = value;
                }
            }
        }
    }
}


void EAGLE_BIN_PARSER::postprocNames( EGB_NODE* aLibraries, EGB_NODE* aElements )
{
    if( aLibraries == nullptr )
        return;

    // The binary references libraries and packages by 1-based ordinal, but the
    // XML schema (and KiCad's reader) resolve them by name. Give every library
    // and package a unique non-empty name, then rewrite each element's numeric
    // library/package references to those names so footprint lookup succeeds.

    for( size_t li = 0; li < aLibraries->children.size(); li++ )
    {
        EGB_NODE* lib = aLibraries->children[li].get();

        if( lib->id != EGKW_SECT_LIBRARY )
            continue;

        EGB_NODE* pkgs = lib->FindChildById( EGKW_SECT_PACKAGES );

        // The library name is carried on the inner packages node; fall back to
        // the ordinal when it is blank.
        wxString libName = pkgs ? pkgs->Prop( wxS( "library" ) ) : wxString();

        if( libName.IsEmpty() )
            libName = wxString::Format( wxS( "lib%zu" ), li + 1 );

        lib->props[wxS( "name" )] = libName;

        if( pkgs == nullptr )
            continue;

        std::map<wxString, int> seen;

        for( size_t pi = 0; pi < pkgs->children.size(); pi++ )
        {
            EGB_NODE* pkg = pkgs->children[pi].get();
            wxString  name = pkg->Prop( wxS( "name" ) );

            if( name.IsEmpty() )
                name = wxString::Format( wxS( "pkg%zu" ), pi + 1 );

            // Disambiguate repeated names so the per-library package map stays
            // unique.
            if( int& count = seen[name]; count++ > 0 )
                name = wxString::Format( wxS( "%s_%d" ), name, count );

            pkg->props[wxS( "name" )] = name;
        }
    }

    if( aElements == nullptr )
        return;

    auto nameByIdx = [&]( EGB_NODE* aParent, long aIdx ) -> wxString
    {
        if( aParent == nullptr || aIdx < 1 || aIdx > (long) aParent->children.size() )
            return wxString();

        return aParent->children[aIdx - 1]->Prop( wxS( "name" ) );
    };

    for( auto& el : aElements->children )
    {
        if( el->id != EGKW_SECT_ELEMENT )
            continue;

        long      libIdx = el->PropLong( wxS( "library" ) );
        EGB_NODE* lib = ( libIdx >= 1 && libIdx <= (long) aLibraries->children.size() )
                                ? aLibraries->children[libIdx - 1].get()
                                : nullptr;

        if( lib == nullptr )
            continue;

        el->props[wxS( "library" )] = lib->Prop( wxS( "name" ) );

        EGB_NODE* pkgs = lib->FindChildById( EGKW_SECT_PACKAGES );
        wxString  pkgName = nameByIdx( pkgs, el->PropLong( wxS( "package" ) ) );

        if( !pkgName.IsEmpty() )
            el->props[wxS( "package" )] = pkgName;
    }
}


void EAGLE_BIN_PARSER::postprocSignals( EGB_NODE* aSignals )
{
    if( aSignals == nullptr )
        return;

    // Flatten any nested signal so every signal sits directly under signals.
    // Connectivity of nested nets is not preserved, matching Eagle's own
    // binary-to-XML conversion. Iterate by index because nested signals are
    // appended to the same vector and must themselves be flattened, which
    // handles three or more levels of nesting.
    for( size_t i = 0; i < aSignals->children.size(); i++ )
    {
        EGB_NODE* sig = aSignals->children[i].get();

        if( sig->id != EGKW_SECT_SIGNAL )
            continue;

        std::vector<std::unique_ptr<EGB_NODE>> kept;
        std::vector<std::unique_ptr<EGB_NODE>> promoted;

        for( auto& inner : sig->children )
        {
            if( inner->id == EGKW_SECT_SIGNAL )
            {
                inner->parent = aSignals;
                promoted.push_back( std::move( inner ) );
            }
            else
            {
                kept.push_back( std::move( inner ) );
            }
        }

        sig->children = std::move( kept );

        // Append after rebuilding sig->children; this may reallocate, but sig
        // is only dereferenced above and i indexes the (stable) container.
        for( auto& p : promoted )
            aSignals->children.push_back( std::move( p ) );
    }
}


void EAGLE_BIN_PARSER::postprocContactRefs( EGB_NODE* aSignals, EGB_NODE* aElements, EGB_NODE* aLibraries )
{
    if( aSignals == nullptr || aElements == nullptr || aLibraries == nullptr )
        return;

    auto elemByIdx = [&]( long aIdx ) -> EGB_NODE*
    {
        if( aIdx < 1 || aIdx > (long) aElements->children.size() )
            return nullptr;

        return aElements->children[aIdx - 1].get();
    };

    auto libByIdx = [&]( long aIdx ) -> EGB_NODE*
    {
        if( aIdx < 1 || aIdx > (long) aLibraries->children.size() )
            return nullptr;

        return aLibraries->children[aIdx - 1].get();
    };

    auto pkgByIdx = [&]( EGB_NODE* aLib, long aIdx ) -> EGB_NODE*
    {
        if( aLib == nullptr )
            return nullptr;

        EGB_NODE* pkgs = aLib->FindChildById( EGKW_SECT_PACKAGES );

        if( pkgs == nullptr || aIdx < 1 || aIdx > (long) pkgs->children.size() )
            return nullptr;

        return pkgs->children[aIdx - 1].get();
    };

    for( auto& sig : aSignals->children )
    {
        // Resolve every contactref, regardless of sibling order; wires or
        // polygons may precede the contactrefs within a signal.
        for( auto& cr : sig->children )
        {
            if( cr->id != EGKW_SECT_CONTACTREF )
                continue;

            long      partNum = cr->PropLong( wxS( "partnumber" ) );
            EGB_NODE* elem = elemByIdx( partNum );

            if( elem == nullptr )
                continue;

            cr->props[wxS( "element" )] = elem->Prop( wxS( "name" ) );

            // Resolve the pad name by walking the package pads/pins/smd in order.
            EGB_NODE* lib = libByIdx( elem->PropLong( wxS( "library" ) ) );
            EGB_NODE* pkg = pkgByIdx( lib, elem->PropLong( wxS( "package" ) ) );

            if( pkg == nullptr )
            {
                cr->props[wxS( "pad" )] = wxS( "PIN_NOT_FOUND" );
                continue;
            }

            long      pinNum = cr->PropLong( wxS( "pin" ) );
            EGB_NODE* found = nullptr;

            for( const auto& child : pkg->children )
            {
                int kind = child->id & 0xFF00;

                if( kind == EGKW_SECT_PAD || kind == EGKW_SECT_SMD || kind == EGKW_SECT_PIN )
                {
                    if( --pinNum < 1 )
                    {
                        found = child.get();
                        break;
                    }
                }
            }

            if( found == nullptr )
                cr->props[wxS( "pad" )] = wxS( "PIN_NOT_FOUND" );
            else if( found->HasProp( wxS( "name" ) ) )
                cr->props[wxS( "pad" )] = found->Prop( wxS( "name" ) );
            else
                cr->props[wxS( "pad" )] = cr->Prop( wxS( "pin" ) );
        }
    }
}


void EAGLE_BIN_PARSER::postProcess( EGB_NODE* aRoot, const DRC_CTX& aDrc )
{
    EGB_NODE* drawing = aRoot->children.empty() ? nullptr : aRoot->children.front().get();

    if( drawing == nullptr )
        THROW_IO_ERROR( _( "Eagle binary file has no drawing section." ) );

    // KiCad's XML reader resolves the layer map from drawing/layers, so the
    // synthetic node must live under drawing, not the eagle root.
    EGB_NODE* layers = drawing->AddChild( EGKW_SECT_LAYERS, wxS( "layers" ) );

    EGB_NODE* board = drawing->FindChildById( EGKW_SECT_BOARD );
    EGB_NODE* drcNode = nullptr;
    EGB_NODE* libraries = nullptr;
    EGB_NODE* signals = nullptr;
    EGB_NODE* elements = nullptr;

    if( board != nullptr )
    {
        drcNode = board->AddChild( EGKW_SECT_DRC, wxS( "designrules" ) );
        libraries = board->FindChildByName( wxS( "libraries" ) );

        if( libraries == nullptr )
            THROW_IO_ERROR( _( "Eagle binary layout is missing a board/libraries node." ) );

        signals = board->FindChildByName( wxS( "signals" ) );
        elements = board->FindChildByName( wxS( "elements" ) );
    }

    postprocLayers( drawing, layers );

    if( drcNode != nullptr )
        postprocDrc( drcNode, aDrc );

    postprocLibs( libraries );
    postprocElements( elements );
    postprocSignals( signals );

    postprocWires( aRoot );
    postprocArcs( aRoot );
    postprocVias( aRoot );
    postprocCircles( aRoot );
    postprocSmd( aRoot );
    postprocDimensions( aRoot );

    // Resolve long-text names before contactrefs copy element and pad names,
    // and before postprocNames disambiguates package names.
    postprocFreeText( aRoot );

    // postprocContactRefs reads element library/package ordinals, so it must run
    // before postprocNames rewrites those ordinals into names.
    postprocContactRefs( signals, elements, libraries );
    postprocNames( libraries, elements );

    postprocRotation( aRoot );

    // Must run last so every dimensional attribute has its final value before
    // the decimicron-to-millimetre rewrite.
    postprocUnits( aRoot );
}


wxXmlNode* EAGLE_BIN_PARSER::toXml( const EGB_NODE* aNode ) const
{
    wxXmlNode* xml = new wxXmlNode( wxXML_ELEMENT_NODE, aNode->name );

    for( const auto& [key, value] : aNode->props )
        xml->AddAttribute( key, value );

    // wxXmlNode::AddChild appends to the end of the sibling chain, preserving
    // document order.
    for( const auto& child : aNode->children )
        xml->AddChild( toXml( child.get() ) );

    return xml;
}


std::unique_ptr<wxXmlDocument> EAGLE_BIN_PARSER::Parse( const std::vector<uint8_t>& aBytes )
{
    m_buf = &aBytes;
    m_pos = 0;

    if( aBytes.size() < 24 )
        THROW_IO_ERROR( _( "File is too small to be an Eagle binary board." ) );

    m_root = std::make_unique<EGB_NODE>();
    m_root->id = 0;
    m_root->name = wxS( "eagle" );

    long numBlocks = -1;
    readBlock( numBlocks, m_root.get() );

    DRC_CTX drc;

    // The trailing notes and DRC sections are present only in v4/v5; missing
    // sections are tolerated and fall back to defaults.
    readNotes();
    readDrc( drc );

    postProcess( m_root.get(), drc );

    auto doc = std::make_unique<wxXmlDocument>();
    doc->SetRoot( toXml( m_root.get() ) );

    m_buf = nullptr;
    return doc;
}
