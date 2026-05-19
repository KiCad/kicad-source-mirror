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

#ifndef KICAD_PNS_HELPERS_H
#define KICAD_PNS_HELPERS_H

#include <geometry/shape_line_chain.h>
#include <math/vector2d.h>
#include <netinfo.h>
#include <pcb_track_types.h>
#include <router/pns_linked_item.h>
#include <router/pns_router.h>


class SHAPE_LINE_CHAIN;
namespace PNS
{

struct HELPERS
{
    static LINKED_ITEM* PickSegment( ROUTER* aRouter, const VECTOR2I& aWhere, int aLayer, VECTOR2I& aPointOut,
                                     const SHAPE_LINE_CHAIN& aBaseline = SHAPE_LINE_CHAIN() );

    static VECTOR2I SnapToNearestTrack( const VECTOR2I& aP, BOARD* aBoard, NETINFO_ITEM* aNet,
                                        PCB_TRACK** aNearestTrack );

    static VECTOR2I GetSnappedStartPoint( LINKED_ITEM* aStartItem, VECTOR2I aStartPoint );
};

} // namespace PNS

#endif //KICAD_PNS_HELPERS_H
