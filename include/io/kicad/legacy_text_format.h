/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <eda_text.h>
#include <font/font.h>
#include <math/util.h>

namespace KICAD_FORMAT
{

inline int GetLegacyTextThickness( const EDA_TEXT& aText )
{
    const int thickness = aText.GetTextThickness();

    if( !aText.IsBold() || thickness <= 1 )
        return thickness;

    bool strokeFont = true;

    if( const KIFONT::FONT* font = aText.GetFont() )
        strokeFont = font->IsStroke();
    else if( !aText.GetUnresolvedFontName().IsEmpty() )
        strokeFont = KIFONT::FONT::IsStroke( aText.GetUnresolvedFontName() );

    return strokeFont ? KiROUND( thickness * 1.6 ) : thickness;
}

} // namespace KICAD_FORMAT
