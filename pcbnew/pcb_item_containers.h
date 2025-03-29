/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef PCB_ITEM_CONTAINERS_H_
#define PCB_ITEM_CONTAINERS_H_

#include <macros_swig.h>

// Board-level items
class FOOTPRINT;
class PCB_TRACK;
class PCB_GROUP;
class PCB_GENERATOR;
class PCB_POINT;
class PCB_MARKER;
class ZONE;

DECL_VEC_FOR_SWIG( MARKERS, PCB_MARKER* )
DECL_VEC_FOR_SWIG( ZONES, ZONE* )
DECL_DEQ_FOR_SWIG( TRACKS, PCB_TRACK* )
DECL_DEQ_FOR_SWIG( FOOTPRINTS, FOOTPRINT* )
// Dequeue rather than Vector just so we can use moveUnflaggedItems in pcbnew_control.cpp
DECL_DEQ_FOR_SWIG( GROUPS, PCB_GROUP* )
DECL_DEQ_FOR_SWIG( GENERATORS, PCB_GENERATOR* )
DECL_DEQ_FOR_SWIG( PCB_POINTS, PCB_POINT* )


// Shared with board and footprint
class BOARD_ITEM;

DECL_DEQ_FOR_SWIG( DRAWINGS, BOARD_ITEM* )


// Footprint-level items
class PAD;
class PCB_FIELD;

DECL_DEQ_FOR_SWIG( PADS, PAD* )
DECL_DEQ_FOR_SWIG( PCB_FIELDS, PCB_FIELD* )
DECL_VEC_FOR_SWIG( PCB_FIELD_VEC, PCB_FIELD* )

#endif // PCB_ITEM_CONTAINERS_H_
