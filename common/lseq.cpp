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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <layer_ids.h>
#include <lseq.h>

#include <algorithm>
#include <vector>


int LSEQ::TestLayers( PCB_LAYER_ID aRhs, PCB_LAYER_ID aLhs ) const
{
    if( aRhs == aLhs )
        return 0;

    auto itRhs = std::find( begin(), end(), aRhs );
    auto itLhs = std::find( begin(), end(), aLhs );

    return std::distance( itRhs, itLhs );
}