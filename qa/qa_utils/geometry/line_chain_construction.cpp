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

#include <qa_utils/geometry/line_chain_construction.h>

namespace KI_TEST
{

SHAPE_LINE_CHAIN BuildRectChain( const VECTOR2I& aSize, const VECTOR2I& aCentre )
{
    const std::vector<VECTOR2I> pts = {
        { aCentre.x - aSize.x / 2, aCentre.y - aSize.y / 2 },
        { aCentre.x - aSize.x / 2, aCentre.y + aSize.y / 2 },
        { aCentre.x + aSize.x / 2, aCentre.y + aSize.y / 2 },
        { aCentre.x + aSize.x / 2, aCentre.y - aSize.y / 2 },
    };

    SHAPE_LINE_CHAIN chain( pts );
    chain.SetClosed( true );

    return chain;
}

SHAPE_LINE_CHAIN BuildSquareChain( int aSize, const VECTOR2I& aCentre )
{
    return BuildRectChain( { aSize, aSize }, aCentre );
}

} // namespace KI_TEST
