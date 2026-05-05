/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PADS_PCB_SHAPES_H_
#define PADS_PCB_SHAPES_H_

#include <string>

#include <padstack.h>

namespace PADS_PCB
{

/**
 * Map a PADS pad-stack shape code to a KiCad pad shape, shared by both PADS PCB importers.
 *
 * The code lives in pcbnew (PAD_SHAPE is a pcbnew type) rather than the format-neutral
 * pads_common. The "T" finger and untrimmed variants resolve to their base shape; any
 * unrecognized code falls back to a round pad. The caller sets the size, which depends on the
 * code beyond just the shape.
 */
inline PAD_SHAPE PadsShapeToKiCad( const std::string& aShape )
{
    if( aShape == "S" || aShape == "ST" || aShape == "RF" )
        return PAD_SHAPE::RECTANGLE;

    if( aShape == "O" || aShape == "OT" || aShape == "OF" )
        return PAD_SHAPE::OVAL;

    if( aShape == "RC" || aShape == "OC" )
        return PAD_SHAPE::ROUNDRECT;

    return PAD_SHAPE::CIRCLE;
}

} // namespace PADS_PCB

#endif // PADS_PCB_SHAPES_H_
