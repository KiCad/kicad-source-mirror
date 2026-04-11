/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/geometry/poly_set_construction.h>

#include <qa_utils/geometry/line_chain_construction.h>

namespace KI_TEST
{

SHAPE_POLY_SET BuildPolyset( const std::vector<SHAPE_LINE_CHAIN>& aOutlines )
{
    SHAPE_POLY_SET polyset;

    for( const auto& outline : aOutlines )
    {
        polyset.AddOutline( outline );
    }

    return polyset;
}


SHAPE_POLY_SET BuildHollowSquare( int aOuterSize, int aInnerSize, const VECTOR2I& aCentre )
{
    SHAPE_POLY_SET polyset;

    polyset.AddOutline( BuildRectChain( { aOuterSize, aOuterSize }, aCentre ) );
    polyset.AddHole( BuildRectChain( { aInnerSize, aInnerSize }, aCentre ) );

    return polyset;
}

} // namespace KI_TEST