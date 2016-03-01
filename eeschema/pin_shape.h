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
 * @file pin_shape.h
 * @brief Pin shape handling
 */

#ifndef _PIN_SHAPE_H_
#define _PIN_SHAPE_H_

#include <wx/string.h>
#include <bitmaps.h>

enum GRAPHIC_PINSHAPE
{
    PINSHAPE_LINE,
    PINSHAPE_INVERTED,
    PINSHAPE_CLOCK,
    PINSHAPE_INVERTED_CLOCK,
    PINSHAPE_INPUT_LOW,
    PINSHAPE_CLOCK_LOW,
    PINSHAPE_OUTPUT_LOW,
    PINSHAPE_FALLING_EDGE_CLOCK,
    PINSHAPE_NONLOGIC
};

enum
{
    PINSHAPE_COUNT = PINSHAPE_NONLOGIC + 1
};

// UI
wxString    GetText( GRAPHIC_PINSHAPE shape );
BITMAP_DEF  GetBitmap( GRAPHIC_PINSHAPE shape );

#endif
