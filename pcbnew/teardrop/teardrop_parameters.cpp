/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include "teardrop_parameters.h"

#define TARGET_NAME_ROUND "td_round_shape"
#define TARGET_NAME_RECT "td_rect_shape"
#define TARGET_NAME_TRACK "td_track_end"

std::string GetTeardropTargetCanonicalName( TARGET_TD aTdType )
{
    // return the canonical name of the target aTdType
    std::string name;

    switch( aTdType )
    {
    case  TARGET_ROUND: name = TARGET_NAME_ROUND; break;
    case  TARGET_RECT:  name = TARGET_NAME_RECT;  break;
    case  TARGET_TRACK: name = TARGET_NAME_TRACK; break;
    default: break;
    }

    return name;
}


TARGET_TD GetTeardropTargetTypeFromCanonicalName( const std::string& aTargetName )
{
    // return the target type from the canonical name

    if( aTargetName == TARGET_NAME_ROUND )
        return TARGET_ROUND;

    if( aTargetName == TARGET_NAME_RECT )
        return TARGET_RECT;

    if( aTargetName == TARGET_NAME_TRACK )
        return TARGET_TRACK;

    return TARGET_UNKNOWN;
}
