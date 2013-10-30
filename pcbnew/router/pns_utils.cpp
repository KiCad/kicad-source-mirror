/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#include "pns_utils.h"
#include "pns_line.h"
#include "pns_router.h"

const SHAPE_LINE_CHAIN OctagonalHull( const VECTOR2I& aP0,
        const VECTOR2I& aSize,
        int aClearance,
        int aChamfer )
{
    SHAPE_LINE_CHAIN s;

    s.SetClosed( true );

    s.Append( aP0.x - aClearance, aP0.y - aClearance + aChamfer );
    s.Append( aP0.x - aClearance + aChamfer, aP0.y - aClearance );
    s.Append( aP0.x + aSize.x + aClearance - aChamfer, aP0.y - aClearance );
    s.Append( aP0.x + aSize.x + aClearance, aP0.y - aClearance + aChamfer );
    s.Append( aP0.x + aSize.x + aClearance, aP0.y + aSize.y + aClearance - aChamfer );
    s.Append( aP0.x + aSize.x + aClearance - aChamfer, aP0.y + aSize.y + aClearance );
    s.Append( aP0.x - aClearance + aChamfer, aP0.y + aSize.y + aClearance );
    s.Append( aP0.x - aClearance, aP0.y + aSize.y + aClearance - aChamfer );

    return s;
}
