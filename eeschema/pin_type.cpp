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


struct pinTypeStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};

/*
* Conversion map between PLOT_DASH_TYPE values and style names displayed
*/
// clang-format off
const std::map<ELECTRICAL_PINTYPE, struct pinTypeStruct> pinTypes = {
    { ELECTRICAL_PINTYPE::INPUT,        { _( "Input" ), pintype_input_xpm } },
    { ELECTRICAL_PINTYPE::OUTPUT,       { _( "Output" ), pintype_output_xpm } },
    { ELECTRICAL_PINTYPE::BIDI,         { _( "Bidirectional" ), pintype_bidi_xpm } },
    { ELECTRICAL_PINTYPE::TRISTATE,     { _( "Tri-state" ), pintype_3states_xpm } },
    { ELECTRICAL_PINTYPE::PASSIVE,      { _( "Passive" ), pintype_passive_xpm } },
    { ELECTRICAL_PINTYPE::UNSPECIFIED,  { _( "Clock low" ), pintype_notspecif_xpm } },
    { ELECTRICAL_PINTYPE::POWER_IN,     { _( "Power input" ), pintype_powerinput_xpm } },
    { ELECTRICAL_PINTYPE::POWER_OUT,    { _( "Power output" ), pintype_poweroutput_xpm } },
    { ELECTRICAL_PINTYPE::OPENCOLLECTOR,{ _( "Open collector" ), pintype_opencoll_xpm } },
    { ELECTRICAL_PINTYPE::OPENEMITTER,  { _( "Open emitter" ), pintype_openemit_xpm } },
    { ELECTRICAL_PINTYPE::NC,           { _( "Not connected" ), pintype_noconnect_xpm } },
};
// clang-format on

wxString GetText( ELECTRICAL_PINTYPE aType )
{
    auto findIt = pinTypes.find( aType );

    wxCHECK_MSG( findIt != pinTypes.end(), wxT( "???" ), "Could not find pin type in lookup map" );

    return findIt->second.name;
}


BITMAP_DEF GetBitmap( ELECTRICAL_PINTYPE aType )
{
    auto findIt = pinTypes.find( aType );

    wxCHECK_MSG( findIt != pinTypes.end(), nullptr, "Could not find pin type in lookup map" );

    return findIt->second.bitmap;
}
