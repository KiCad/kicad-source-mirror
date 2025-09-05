/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers
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
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */


#pragma once

#include <wx/string.h>
#include <wx/arrstr.h>
#include <math/vector2d.h>
#include <font/text_attributes.h>

struct MULTILINE_PIN_TEXT_LAYOUT
{
    bool        m_IsMultiLine = false;     // true if brace-wrapped multi-line stacked list
    wxArrayString m_Lines;                 // individual numbered lines (trimmed)
    VECTOR2D    m_StartPos;                // position used for line index 0 after alignment shift
    int         m_LineSpacing = 0;         // inter-line spacing in IU (along secondary axis)
};

// Compute layout for a (possibly) multi-line stacked pin number string.  If not multi-line, the
// returned layout has m_IsMultiLine=false and no further adjustments are required.
MULTILINE_PIN_TEXT_LAYOUT ComputeMultiLinePinNumberLayout( const wxString& aText,
        const VECTOR2D& aAnchorPos, const TEXT_ATTRIBUTES& aAttrs );
