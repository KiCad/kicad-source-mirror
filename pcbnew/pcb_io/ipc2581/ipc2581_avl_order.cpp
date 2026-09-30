/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ipc2581_avl_order.h"

#include <algorithm>
#include <numeric>
#include <string_utils.h>


std::vector<size_t> OrderAvlRows( const std::vector<AVL_ROW>& aRows )
{
    std::vector<size_t> indices( aRows.size() );
    std::iota( indices.begin(), indices.end(), 0 );

    std::sort( indices.begin(), indices.end(),
               [&]( size_t aLeft, size_t aRight )
               {
                   int compare = aRows[aLeft].m_oemName.Cmp( aRows[aRight].m_oemName );

                   if( compare != 0 )
                       return compare < 0;

                   compare = StrNumCmp( aRows[aLeft].m_reference, aRows[aRight].m_reference, true );

                   return compare != 0 ? compare < 0 : aLeft < aRight;
               } );

    return indices;
}
