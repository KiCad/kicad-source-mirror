/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <font/glyph.h>
#include <trigo.h>

using namespace KIFONT;


STROKE_GLYPH::STROKE_GLYPH( const STROKE_GLYPH& aGlyph )
{
    for( const std::vector<VECTOR2D>& pointList : aGlyph )
        push_back( pointList );

    m_boundingBox = aGlyph.m_boundingBox;
}


void STROKE_GLYPH::AddPoint( const VECTOR2D& aPoint )
{
    if( !m_penIsDown )
    {
        std::vector<VECTOR2D> v;
        push_back( v );
        m_penIsDown = true;
    }

    back().push_back( aPoint );
}


void STROKE_GLYPH::RaisePen()
{
    if( m_penIsDown )
        back().shrink_to_fit();

    m_penIsDown = false;
}


void STROKE_GLYPH::Finalize()
{
    if( !empty() && !back().empty() )
        back().shrink_to_fit();
}


std::unique_ptr<GLYPH> STROKE_GLYPH::Transform( const VECTOR2D& aGlyphSize, const VECTOR2I& aOffset,
                                                double aTilt, const EDA_ANGLE& aAngle, bool aMirror,
                                                const VECTOR2I& aOrigin )
{
    std::unique_ptr<STROKE_GLYPH> glyph = std::make_unique<STROKE_GLYPH>( *this );

    VECTOR2D end = glyph->m_boundingBox.GetEnd();

    end.x *= aGlyphSize.x;
    end.y *= aGlyphSize.y;

    if( aTilt )
        end.x -= end.y * aTilt;

    glyph->m_boundingBox.SetEnd( end );
    glyph->m_boundingBox.Offset( aOffset );

    for( std::vector<VECTOR2D>& pointList : *glyph.get() )
    {
        for( VECTOR2D& point : pointList )
        {
            point *= aGlyphSize;

            if( aTilt )
                point.x -= point.y * aTilt;

            point += aOffset;

            if( aMirror )
                point.x = aOrigin.x - ( point.x - aOrigin.x );

            if( !aAngle.IsZero() )
                RotatePoint( point, aOrigin, aAngle );
        }
    }

    return glyph;
}


BOX2D OUTLINE_GLYPH::BoundingBox()
{
    BOX2I bbox = BBox();
    return BOX2D( bbox.GetOrigin(), bbox.GetSize() );
}
