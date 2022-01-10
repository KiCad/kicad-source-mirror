/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <plotters/plotter.h>
#include <trigo.h>

#include <callback_gal.h>


using namespace KIGFX;


void CALLBACK_GAL::DrawGlyph( const KIFONT::GLYPH& aGlyph, int aNth, int aTotal )
{
    if( aGlyph.IsStroke() )
    {
        const KIFONT::STROKE_GLYPH& glyph = static_cast<const KIFONT::STROKE_GLYPH&>( aGlyph );

        for( const std::vector<VECTOR2D>& pointList : glyph )
        {
            for( size_t ii = 1; ii < pointList.size(); ii++ )
                m_strokeCallback( pointList[ ii - 1], pointList[ii] );
        }
    }
    else if( aGlyph.IsOutline() )
    {
        const KIFONT::OUTLINE_GLYPH& glyph = static_cast<const KIFONT::OUTLINE_GLYPH&>( aGlyph );

        glyph.Triangulate( m_triangleCallback );
    }
}


