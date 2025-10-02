/*
 * This program source code file is part of KiCad, a free EDA CAD application.

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

#include "tools/pcb_tool_utils.h"

#include <board.h>
#include <pcb_shape.h>
#include <pcb_track.h>

std::optional<int> GetBoardItemWidth( const BOARD_ITEM& aItem )
{
    switch( aItem.Type() )
    {
    case PCB_SHAPE_T:
    {
        const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( aItem );

        if( shape.GetShape() == SHAPE_T::SEGMENT )
            return shape.GetWidth();

        if( shape.GetWidth() && !shape.IsSolidFill() )
            return shape.GetWidth();

        break;
    }

    case PCB_TRACE_T:
    case PCB_ARC_T:
    {
        const PCB_TRACK& track = static_cast<const PCB_TRACK&>( aItem );
        return track.GetWidth();
    }

    default:
        break;
    }

    return std::nullopt;
}
