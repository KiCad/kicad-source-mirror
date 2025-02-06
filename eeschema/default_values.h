/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef DEFAULT_VALUES_H
#define DEFAULT_VALUES_H


/// The size of the rectangle indicating an unconnected wire or label
#define DANGLING_SYMBOL_SIZE 12

///< The size of the rectangle indicating a connected, unselected wire end
#define UNSELECTED_END_SIZE 4

///< The size of the rectangle indicating the anchor of a text object (including fields)
#define TEXT_ANCHOR_SIZE 8

///< The default pin len value when creating pins(can be changed in preference menu)
#define DEFAULT_PIN_LENGTH 100

///< The default pin number size when creating pins(can be changed in preference menu)
#define DEFAULT_PINNUM_SIZE 50

///< The default pin name size when creating pins(can be changed in preference menu)
#define DEFAULT_PINNAME_SIZE 50

///< The default selection highlight thickness (can be changed in preference menu)
#define DEFAULTSELECTIONTHICKNESS 3

///< The default line width in mils. (can be changed in preference menu)
#define DEFAULT_LINE_WIDTH_MILS 6

///< The default wire width in mils. (can be changed in preference menu)
#define DEFAULT_WIRE_WIDTH_MILS 6

///< The default bus width in mils. (can be changed in preference menu)
#define DEFAULT_BUS_WIDTH_MILS 12

///< The default noconnect size in mils.
#define DEFAULT_NOCONNECT_SIZE 48

///< The default junction diameter in mils. (can be changed in preference menu)
#define DEFAULT_JUNCTION_DIAM 36

///< The default bus and wire entry size in mils.
#define DEFAULT_SCH_ENTRY_SIZE 100

///< The default text size in mils. (can be changed in preference menu)
#define DEFAULT_TEXT_SIZE 50

///< Ratio of the font height to the baseline of the text above the wire.
#define DEFAULT_TEXT_OFFSET_RATIO 0.15

///< Ratio of the font height to space around global labels
#define DEFAULT_LABEL_SIZE_RATIO 0.375

///< The offset of the pin name string from the end of the pin in mils.
#define DEFAULT_PIN_NAME_OFFSET 20

///< The intersheets references prefix string
#define DEFAULT_IREF_PREFIX "["

///< The intersheets references suffix string
#define DEFAULT_IREF_SUFFIX "]"

///< Radius of snap "gravity well"
#define SNAP_RANGE 55

#endif
