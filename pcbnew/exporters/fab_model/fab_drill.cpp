/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software: you can redistribute it and/or
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

#include "fab_drill.h"

#include <algorithm>
#include <lseq.h>


PCB_LAYER_ID FabBackdrillMustNotCut( const LSEQ& aCopperStack, PCB_LAYER_ID aStart, PCB_LAYER_ID aMustCut )
{
    auto it = std::find( aCopperStack.begin(), aCopperStack.end(), aMustCut );

    if( it == aCopperStack.end() )
        return UNDEFINED_LAYER;

    if( aStart == F_Cu )
    {
        ++it;

        return it == aCopperStack.end() ? UNDEFINED_LAYER : *it;
    }

    if( aStart == B_Cu )
    {
        return it == aCopperStack.begin() ? UNDEFINED_LAYER : *( --it );
    }

    return UNDEFINED_LAYER;
}
