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

wxString GetText( GRAPHIC_PINSHAPE shape )
{
    switch( shape )
    {
    case PINSHAPE_LINE:
        return _( "Line" );

    case PINSHAPE_INVERTED:
        return _( "Inverted" );

    case PINSHAPE_CLOCK:
        return _( "Clock" );

    case PINSHAPE_INVERTED_CLOCK:
        return _( "Inverted clock" );

    case PINSHAPE_INPUT_LOW:
        return _( "Input low" );

    case PINSHAPE_CLOCK_LOW:
        return _( "Clock low" );

    case PINSHAPE_OUTPUT_LOW:
        return _( "Output low" );

    case PINSHAPE_FALLING_EDGE_CLOCK:
        return _( "Falling edge clock" );

    case PINSHAPE_NONLOGIC:
        return _( "NonLogic" );
    }

    assert( !"Invalid pin shape" );
    return wxT( "?" );
}


BITMAP_DEF GetBitmap( GRAPHIC_PINSHAPE shape )
{
    switch( shape )
    {
    case PINSHAPE_LINE:
        return pinshape_normal_xpm;

    case PINSHAPE_INVERTED:
        return pinshape_invert_xpm;

    case PINSHAPE_CLOCK:
        return pinshape_clock_normal_xpm;

    case PINSHAPE_INVERTED_CLOCK:
        return pinshape_clock_invert_xpm;

    case PINSHAPE_INPUT_LOW:
        return pinshape_active_low_input_xpm;

    case PINSHAPE_CLOCK_LOW:
        return pinshape_clock_active_low_xpm;

    case PINSHAPE_OUTPUT_LOW:
        return pinshape_active_low_output_xpm;

    case PINSHAPE_FALLING_EDGE_CLOCK:
        return pinshape_clock_fall_xpm;

    case PINSHAPE_NONLOGIC:
        return pinshape_nonlogic_xpm;
    }

    assert( !"Invalid pin shape" );
    return 0;
};
