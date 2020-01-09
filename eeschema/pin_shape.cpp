/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file pin_shape.cpp
 * @brief Pin shape handling
 */

#include "pin_shape.h"

#include <macros.h>


struct pinShapeStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};

/*
* Conversion map between PLOT_DASH_TYPE values and style names displayed
*/
// clang-format off
const std::map<GRAPHIC_PINSHAPE, struct pinShapeStruct> pinShapes = {
    { GRAPHIC_PINSHAPE::LINE,               { _( "Line" ), pinshape_normal_xpm } },
    { GRAPHIC_PINSHAPE::INVERTED,           { _( "Inverted" ), pinshape_invert_xpm } },
    { GRAPHIC_PINSHAPE::CLOCK,              { _( "Clock" ), pinshape_clock_normal_xpm } },
    { GRAPHIC_PINSHAPE::INVERTED_CLOCK,     { _( "Inverted clock" ), pinshape_clock_invert_xpm } },
    { GRAPHIC_PINSHAPE::INPUT_LOW,          { _( "Input low" ), pinshape_active_low_input_xpm } },
    { GRAPHIC_PINSHAPE::CLOCK_LOW,          { _( "Clock low" ), pinshape_clock_active_low_xpm } },
    { GRAPHIC_PINSHAPE::OUTPUT_LOW,         { _( "Output low" ), pinshape_active_low_output_xpm } },
    { GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK, { _( "Falling edge clock" ), pinshape_clock_fall_xpm } },
    { GRAPHIC_PINSHAPE::NONLOGIC,           { _( "NonLogic" ), pinshape_nonlogic_xpm } },
};
// clang-format on


wxString GetText( GRAPHIC_PINSHAPE aShape )
{
    auto findIt = pinShapes.find( aShape );

    wxCHECK_MSG( findIt != pinShapes.end(), wxT( "?" ), "Could not find pinshape in lookup map" );

    return findIt->second.name;
}


BITMAP_DEF GetBitmap( GRAPHIC_PINSHAPE aShape )
{
    auto findIt = pinShapes.find( aShape );

    wxCHECK_MSG( findIt != pinShapes.end(), nullptr, "Could not find pinshape in lookup map" );

    return findIt->second.bitmap;
}
