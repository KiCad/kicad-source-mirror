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
 * @file pin_type.h
 * @brief Electrical pin type handling
 */
#ifndef PIN_TYPE_H_
#define PIN_TYPE_H_

#include <wx/string.h>
#include <bitmaps.h>

/**
 * The component library pin object electrical types used in ERC tests.
 */
enum class ELECTRICAL_PINTYPE
{
    PT_INPUT,         ///< usual pin input: must be connected
    PT_OUTPUT,        ///< usual output
    PT_BIDI,          ///< input or output (like port for a microprocessor)
    PT_TRISTATE,      ///< tris state bus pin
    PT_PASSIVE,       ///< pin for passive components: must be connected, and can be connected to any pin
    PT_UNSPECIFIED,   ///< unknown electrical properties: creates always a warning when connected
    PT_POWER_IN,      ///< power input (GND, VCC for ICs). Must be connected to a power output.
    PT_POWER_OUT,     ///< output of a regulator: intended to be connected to power input pins
    PT_OPENCOLLECTOR, ///< pin type open collector
    PT_OPENEMITTER,   ///< pin type open emitter
    PT_NC             ///< not connected (must be left open)
};

enum
{
    PINTYPE_COUNT = static_cast<int>( ELECTRICAL_PINTYPE::PT_NC ) + 1
};

// UI
wxString GetText( ELECTRICAL_PINTYPE );
BITMAP_DEF GetBitmap( ELECTRICAL_PINTYPE );

#endif
