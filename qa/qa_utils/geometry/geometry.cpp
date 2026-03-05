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


#include "qa_utils/geometry/geometry.h"

#include <geometry/shape_line_chain.h>


std::ostream& boost_test_print_type( std::ostream& os, const SHAPE_LINE_CHAIN& c )
{
    os << "SHAPE_LINE_CHAIN: " << c.PointCount() << " points: [\n";

    for( int i = 0; i < c.PointCount(); ++i )
    {
        os << "   " << i << ": " << c.CPoint( i ) << "\n";
    }

    os << "]";
    return os;
}


bool KI_TEST::ChainsAreCyclicallyEqual( const SHAPE_LINE_CHAIN& aChainA, const SHAPE_LINE_CHAIN& aChainB, int aTol )
{
    return aChainA.CompareGeometry( aChainB, true, aTol );
}
