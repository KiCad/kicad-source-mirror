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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef __SPREAD_FOOTPRINTS_H
#define __SPREAD_FOOTPRINTS_H

#include <vector>

#include <base_units.h>
#include <math/vector2d.h>

class FOOTPRINT;


/**
 * Footprints (after loaded by reading a netlist for instance) are moved
 * to be in a small free area (outside the current board) without overlapping.
 * @param aBoard is the board to edit.
 * @param aFootprints: a list of footprints to be spread out.
 * @param aTargetBoxPosition the position of the upper left corner of the
 *        area allowed to spread footprints
 */
void SpreadFootprints( std::vector<FOOTPRINT*>* aFootprints, VECTOR2I aTargetBoxPosition,
                       bool aGroupBySheet = true, int aComponentGap = pcbIUScale.mmToIU( 1 ),
                       int aGroupGap = pcbIUScale.mmToIU( 1.5 ) );


#endif
