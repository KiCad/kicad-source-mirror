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


#include "multiline_pin_text.h"

#include <wx/tokenzr.h>

MULTILINE_PIN_TEXT_LAYOUT ComputeMultiLinePinNumberLayout( const wxString& aText,
        const VECTOR2D& aAnchorPos, const TEXT_ATTRIBUTES& aAttrs )
{
    MULTILINE_PIN_TEXT_LAYOUT layout;
    layout.m_StartPos = aAnchorPos;

    if( !( aText.StartsWith( "[" ) && aText.EndsWith( "]" ) && aText.Contains( "\n" ) ) )
        return layout; // not multi-line stacked

    wxString content = aText.Mid( 1, aText.Length() - 2 );
    wxArrayString lines; wxStringSplit( content, lines, '\n' );
    if( lines.size() <= 1 )
        return layout;

    layout.m_IsMultiLine = true;

    layout.m_Lines = lines;
    for( size_t i = 0; i < layout.m_Lines.size(); ++i )
        layout.m_Lines[i].Trim( true ).Trim( false );

    layout.m_LineSpacing = KiROUND( aAttrs.m_Size.y * 1.3 );

    // Apply alignment-dependent origin shift identical to sch_painter logic
    if( aAttrs.m_Angle == ANGLE_VERTICAL )
    {
        int totalWidth = ( (int) layout.m_Lines.size() - 1 ) * layout.m_LineSpacing;
        if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_RIGHT )
            layout.m_StartPos.x -= totalWidth;
        else if( aAttrs.m_Halign == GR_TEXT_H_ALIGN_CENTER )
            layout.m_StartPos.x -= totalWidth / 2;
    }
    else
    {
        int totalHeight = ( (int) layout.m_Lines.size() - 1 ) * layout.m_LineSpacing;
        if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_BOTTOM )
            layout.m_StartPos.y -= totalHeight;
        else if( aAttrs.m_Valign == GR_TEXT_V_ALIGN_CENTER )
            layout.m_StartPos.y -= totalHeight / 2;
    }

    return layout;
}
