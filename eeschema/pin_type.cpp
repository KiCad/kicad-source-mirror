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
 * @file pin_type.cpp
 * @brief Electrical pin type handling
 */

#include "pin_type.h"

#include <macros.h>

wxString GetText( ELECTRICAL_PINTYPE aType )
{
    switch( aType )
    {
    case PIN_INPUT:
        return _( "Input" );

    case PIN_OUTPUT:
        return _( "Output" );

    case PIN_BIDI:
        return _( "Bidirectional" );

    case PIN_TRISTATE:
        return _( "Tri-state" );

    case PIN_PASSIVE:
        return _( "Passive" );

    case PIN_UNSPECIFIED:
        return _( "Unspecified" );

    case PIN_POWER_IN:
        return _( "Power input" );

    case PIN_POWER_OUT:
        return _( "Power output" );

    case PIN_OPENCOLLECTOR:
        return _( "Open collector" );

    case PIN_OPENEMITTER:
        return _( "Open emitter" );

    case PIN_NC:
        return _( "Not connected" );
    };

    assert( !"invalid pin type" );
    return wxT( "???" );
}


BITMAP_DEF GetBitmap( ELECTRICAL_PINTYPE aType )
{
    switch( aType )
    {
    case PIN_INPUT:
        return pintype_input_xpm;

    case PIN_OUTPUT:
        return pintype_output_xpm;

    case PIN_BIDI:
        return pintype_bidi_xpm;

    case PIN_TRISTATE:
        return pintype_3states_xpm;

    case PIN_PASSIVE:
        return pintype_passive_xpm;

    case PIN_UNSPECIFIED:
        return pintype_notspecif_xpm;

    case PIN_POWER_IN:
        return pintype_powerinput_xpm;

    case PIN_POWER_OUT:
        return pintype_poweroutput_xpm;

    case PIN_OPENCOLLECTOR:
        return pintype_opencoll_xpm;

    case PIN_OPENEMITTER:
        return pintype_openemit_xpm;

    case PIN_NC:
        return pintype_noconnect_xpm;
    };

    assert( !"invalid pin type" );
    return NULL;
}
