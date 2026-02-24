/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_OVERLAY_TYPES_H
#define DRC_RE_OVERLAY_TYPES_H

#include <wx/string.h>

// GTK renders native controls with slightly different baseline/padding than macOS Cocoa.
// These offsets shift all overlay field positions to compensate.
#ifdef __WXGTK__
constexpr int DRC_RE_OVERLAY_XO = -3;
constexpr int DRC_RE_OVERLAY_YO = -5;
#else
constexpr int DRC_RE_OVERLAY_XO = 0;
constexpr int DRC_RE_OVERLAY_YO = 0;
#endif


/**
 * Specifies the position of a label relative to its associated field.
 */
enum class LABEL_POSITION
{
    NONE,       ///< No label
    LEFT,       ///< Label to the left of the field
    RIGHT,      ///< Label to the right of the field
    TOP,        ///< Label above the field
    BOTTOM      ///< Label below the field
};


/**
 * Specifies the position and size of a field overlaid on a constraint bitmap.
 *
 * All coordinates are in 1x bitmap pixels. When the system selects a higher-resolution
 * bitmap (1.5x or 2x), these values are automatically scaled at runtime.
 */
struct DRC_RE_FIELD_POSITION
{
    int xStart;     ///< Left edge X coordinate where the field starts
    int xEnd;       ///< Right edge X coordinate where the field ends
    int yTop;       ///< Top edge Y coordinate of the field
    int tabOrder;   ///< Tab navigation order (1-based, lower numbers receive focus first)

    wxString       labelText;      ///< Optional label text (empty for no label)
    LABEL_POSITION labelPosition;  ///< Position of label relative to field

    DRC_RE_FIELD_POSITION() :
            xStart( 0 ),
            xEnd( 0 ),
            yTop( 0 ),
            tabOrder( 0 ),
            labelText(),
            labelPosition( LABEL_POSITION::NONE )
    {
    }

    DRC_RE_FIELD_POSITION( int aXStart, int aXEnd, int aYTop, int aTabOrder,
                           const wxString& aLabelText = wxEmptyString,
                           LABEL_POSITION aLabelPos = LABEL_POSITION::NONE ) :
            xStart( aXStart ),
            xEnd( aXEnd ),
            yTop( aYTop ),
            tabOrder( aTabOrder ),
            labelText( aLabelText ),
            labelPosition( aLabelPos )
    {
    }
};

#endif // DRC_RE_OVERLAY_TYPES_H
