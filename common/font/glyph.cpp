/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <font/glyph.h>
#include <trigo.h>

using namespace KIFONT;


STROKE_GLYPH::STROKE_GLYPH( const STROKE_GLYPH& aGlyph )
{
    reserve( aGlyph.size() );

    for( const std::vector<VECTOR2D>& pointList : aGlyph )
        push_back( pointList );

    m_boundingBox = aGlyph.m_boundingBox;
}


void STROKE_GLYPH::AddPoint( const VECTOR2D& aPoint )
{
    if( !m_penIsDown )
    {
        emplace_back();
        back().reserve( 16 );   // This handles all but 359 strokes (out of over 328,000)
        m_penIsDown = true;
    }

    back().push_back( aPoint );
}


void STROKE_GLYPH::RaisePen()
{
#if 0
    if( m_penIsDown )
        back().shrink_to_fit();
#endif

    m_penIsDown = false;
}


void STROKE_GLYPH::Finalize()
{
    // Shrinking the strokes saves a bit less than 512K of memory.  It's not worth it for the
    // performance hit of doing more than 328,000 reallocs.
#if 0
if( !empty() && !back().empty() )
        back().shrink_to_fit();
#endif
}


std::unique_ptr<GLYPH> STROKE_GLYPH::Transform( const VECTOR2D& aGlyphSize, const VECTOR2I& aOffset,
                                                double aTilt, const EDA_ANGLE& aAngle, bool aMirror,
                                                const VECTOR2I& aOrigin )
{
    std::unique_ptr<STROKE_GLYPH> glyph = std::make_unique<STROKE_GLYPH>( *this );

    VECTOR2D end = glyph->m_boundingBox.GetEnd();

    end.x *= aGlyphSize.x;
    end.y *= aGlyphSize.y;

    if( aTilt != 0.0 )
        end.x -= end.y * aTilt;

    glyph->m_boundingBox.SetEnd( end );
    glyph->m_boundingBox.Offset( aOffset );

    for( std::vector<VECTOR2D>& pointList : *glyph )
    {
        for( VECTOR2D& point : pointList )
        {
            point *= aGlyphSize;

            if( aTilt != 0.0 )
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


void STROKE_GLYPH::Move( const VECTOR2I& aOffset )
{
    m_boundingBox.Offset( aOffset );

    for( std::vector<VECTOR2D>& pointList : *this )
    {
        for( VECTOR2D& point : pointList )
            point += aOffset;
    }
}


BOX2D OUTLINE_GLYPH::BoundingBox()
{
    BOX2I bbox = BBox();
    return BOX2D( bbox.GetOrigin(), bbox.GetSize() );
}


void OUTLINE_GLYPH::Triangulate( std::function<void( const VECTOR2I& aPt1,
                                                     const VECTOR2I& aPt2,
                                                     const VECTOR2I& aPt3 )> aCallback ) const
{
    const_cast<OUTLINE_GLYPH*>( this )->CacheTriangulation( false );

    for( unsigned int i = 0; i < TriangulatedPolyCount(); i++ )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* polygon = TriangulatedPolygon( i );

        for( size_t j = 0; j < polygon->GetTriangleCount(); j++ )
        {
            VECTOR2I a, b, c;
            polygon->GetTriangle( j, a, b, c );
            aCallback( a, b, c );
        }
    }
}


void OUTLINE_GLYPH::CacheTriangulation( bool aPartition, bool aSimplify )
{
    // Only call CacheTriangulation if it has never been done before.  Otherwise we'll hash
    // the triangulation to see if it has been edited, and glyphs are invariant after creation.
    //
    // Also forces "partition" to false as we never want to partition a glyph.

    if( TriangulatedPolyCount() == 0 )
        SHAPE_POLY_SET::CacheTriangulation( false, aSimplify );
}


std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>>
OUTLINE_GLYPH::GetTriangulationData() const
{
    std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>> data;

    for( const std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>& poly : m_triangulatedPolys )
        data.push_back( std::make_unique<SHAPE_POLY_SET::TRIANGULATED_POLYGON>( *poly ) );

    return data;
}


void OUTLINE_GLYPH::CacheTriangulation(
        std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>>& aHintData )
{
    cacheTriangulation( false, false, &aHintData );
}
