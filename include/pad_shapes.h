/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * Enum PAD_SHAPE_T
 * is the set of pad shapes, used with D_PAD::{Set,Get}Shape()
 */
enum PAD_SHAPE_T
{
    PAD_SHAPE_CIRCLE,
    PAD_SHAPE_RECT,
    PAD_SHAPE_OVAL,
    PAD_SHAPE_TRAPEZOID,
    PAD_SHAPE_ROUNDRECT,
    PAD_SHAPE_CHAMFERED_RECT,   // Rectangle with a champered corner ( and with rounded other corners)
    PAD_SHAPE_CUSTOM            // A shape defined by user, using a set of basic shapes
                                // (thick segments, circles, arcs, polygons
};

static inline wxString PAD_SHAPE_T_asString( PAD_SHAPE_T a )
{
    switch( a )
    {
    case PAD_SHAPE_CIRCLE:         return "PAD_SHAPE_CIRCLE";
    case PAD_SHAPE_RECT:           return "PAD_SHAPE_RECT";
    case PAD_SHAPE_OVAL:           return "PAD_SHAPE_OVAL";
    case PAD_SHAPE_TRAPEZOID:      return "PAD_SHAPE_TRAPEZOID";
    case PAD_SHAPE_ROUNDRECT:      return "PAD_SHAPE_ROUNDRECT";
    case PAD_SHAPE_CHAMFERED_RECT: return "PAD_SHAPE_CHAMFERED_RECT";
    case PAD_SHAPE_CUSTOM:         return "PAD_SHAPE_CUSTOM";
    }
};


/**
 * Enum PAD_DRILL_SHAPE_T
 * is the set of pad drill shapes, used with D_PAD::{Set,Get}DrillShape()
 */
enum PAD_DRILL_SHAPE_T
{
    PAD_DRILL_SHAPE_CIRCLE,
    PAD_DRILL_SHAPE_OBLONG,
};


/**
 * Enum PAD_ATTR_T
 * is the set of pad shapes, used with D_PAD::{Set,Get}Attribute()
 * The double name is for convenience of Python devs
 */
enum PAD_ATTR_T
{
    PAD_ATTRIB_STANDARD,            ///< Usual pad
    PAD_ATTRIB_SMD,                 ///< Smd pad, appears on the solder paste layer (default)
    PAD_ATTRIB_CONN,                ///< Like smd, does not appear on the solder paste layer (default)
                                    ///< note also has a special attribute in Gerber X files
                                    ///< Used for edgecard connectors for instance
    PAD_ATTRIB_HOLE_NOT_PLATED,     ///< like PAD_STANDARD, but not plated
                                    ///< mechanical use only, no connection allowed
};


/**
 * Enum PAD_PROP_T
 * is the set of pad properties used in Gerber files (Draw files, and P&P files)
 * to define some properties in fabrication or test files
 */
enum PAD_PROP_T
{
    PAD_PROP_NONE,                  ///< no special fabrication property
    PAD_PROP_BGA,                   ///< Smd pad, used in BGA footprints
    PAD_PROP_FIDUCIAL_GLBL,         ///< a fiducial (usually a smd) for the full board
    PAD_PROP_FIDUCIAL_LOCAL,        ///< a fiducial (usually a smd) local to the parent footprint
    PAD_PROP_TESTPOINT,             ///< a test point pad
    PAD_PROP_HEATSINK,              ///< a pad used as heat sink, usually in SMD footprints
    PAD_PROP_CASTELLATED            ///< a pad with a castellated through hole
};


#endif  // PAD_SHAPES_H_
