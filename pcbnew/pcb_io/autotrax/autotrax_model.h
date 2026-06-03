/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Format interpretation derived from pcb-rnd src_plugins/io_autotrax:
 *   Copyright (C) 2016, 2017, 2018, 2020 Tibor 'Igor2' Palinkas
 *   Copyright (C) 2016, 2017 Erich S. Heinzle
 * Used under GPL v2-or-later.
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

#ifndef AUTOTRAX_MODEL_H_
#define AUTOTRAX_MODEL_H_

#include <vector>

#include <wx/string.h>

namespace AUTOTRAX
{

/**
 * Autotrax/Easytrax layer numbers as stored in the file. KiCad never sees these
 * directly; the builder maps them onto PCB_LAYER_ID. Numbers 2..5 are inner
 * copper, 9/10 are GND/Power planes, 11 is the board outline and 12 is keepout.
 */
enum LAYER
{
    LAYER_TOP_COPPER = 1,
    LAYER_MID1 = 2,
    LAYER_MID2 = 3,
    LAYER_MID3 = 4,
    LAYER_MID4 = 5,
    LAYER_BOTTOM_COPPER = 6,
    LAYER_TOP_SILK = 7,
    LAYER_BOTTOM_SILK = 8,
    LAYER_GND_PLANE = 9,
    LAYER_POWER_PLANE = 10,
    LAYER_BOARD = 11,
    LAYER_KEEPOUT = 12,
    LAYER_MULTI = 13
};


/// Free or component track segment (FT / CT). All coordinates are in mils.
struct TRACK
{
    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;
    double width = 0;
    int    layer = 0;
};


/// Free or component arc (FA / CA). Autotrax stores a center, radius and a
/// 4-bit quadrant mask in @ref segments rather than start/sweep angles.
struct ARC
{
    double centerX = 0;
    double centerY = 0;
    double radius = 0;
    int    segments = 15; ///< quadrant bitmask; 15 = full circle
    double width = 0;
    int    layer = 0;
};


/// Free or component via (FV / CV).
struct VIA
{
    double x = 0;
    double y = 0;
    double diameter = 0;
    double drill = 0;
};


/// Free or component pad/pin (FP / CP).
struct PAD
{
    double      x = 0;
    double      y = 0;
    double      xSize = 0;
    double      ySize = 0;
    int         shape = 0; ///< 1 round, 2 rect, 3 octagon, 4 round-rect
    double      drill = 0;
    int         planeFlags = 0;
    int         layer = 0;
    wxString    name;
};


/// Free or component rectangular fill (FF / CF), Autotrax's only pour.
struct FILL
{
    double x1 = 0;
    double y1 = 0;
    double x2 = 0;
    double y2 = 0;
    int    layer = 0;
};


/// Free or component string (FS / CS).
struct TEXT
{
    double      x = 0;
    double      y = 0;
    double      height = 0;
    int         direction = 0; ///< 0..3, multiplied by 90 degrees
    double      width = 0;
    int         layer = 0;
    wxString    text;
};


/// A placed component (COMP .. ENDCOMP) holding its own primitives.
struct COMPONENT
{
    wxString    refdes;
    wxString    name;
    wxString    value;
    double      x = 0;
    double      y = 0;

    std::vector<TRACK> tracks;
    std::vector<ARC>   arcs;
    std::vector<VIA>   vias;
    std::vector<PAD>   pads;
    std::vector<FILL>  fills;
    std::vector<TEXT>  texts;
};


/// One refdes -> net membership row collected from the NETDEF section.
struct NET_NODE
{
    wxString netName;
    wxString node; ///< "refdes-pad" connection token
};


/**
 * Everything parsed out of an Autotrax/Easytrax file, before any KiCad object
 * is created. The plugin builds a BOARD from this in a second pass.
 */
struct BOARD_DATA
{
    int version = 4; ///< 4 = Autotrax, 5 = Easytrax

    std::vector<TRACK>     tracks;
    std::vector<ARC>       arcs;
    std::vector<VIA>       vias;
    std::vector<PAD>       pads;
    std::vector<FILL>      fills;
    std::vector<TEXT>      texts;
    std::vector<COMPONENT> components;
    std::vector<NET_NODE>  netNodes;
};

} // namespace AUTOTRAX

#endif // AUTOTRAX_MODEL_H_
