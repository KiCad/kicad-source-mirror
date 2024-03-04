/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef PAD_SHAPES_H_
#define PAD_SHAPES_H_

#include <string>

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
enum PAD_DRILL_SHAPE_T
{
    PAD_DRILL_SHAPE_CIRCLE,
    PAD_DRILL_SHAPE_OBLONG,
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


#endif  // PAD_SHAPES_H_
