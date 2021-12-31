/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

using namespace KIFONT;


STROKE_GLYPH::STROKE_GLYPH( const STROKE_GLYPH& aGlyph )
{
    for( const std::vector<VECTOR2D>& pointList : aGlyph.m_pointLists )
        m_pointLists.push_back( pointList );
}


void STROKE_GLYPH::AddPoint( const VECTOR2D& aPoint )
{
    if( !m_penIsDown )
    {
        std::vector<VECTOR2D> v;
        m_pointLists.push_back( v );
        m_penIsDown = true;
    }

    m_pointLists.back().push_back( aPoint );
}


void STROKE_GLYPH::RaisePen()
{
    if( m_penIsDown )
        m_pointLists.back().shrink_to_fit();

    m_penIsDown = false;
}


void STROKE_GLYPH::Finalize()
{
    if( !m_pointLists.empty() && !m_pointLists.back().empty() )
        m_pointLists.back().shrink_to_fit();
}


BOX2D STROKE_GLYPH::BoundingBox()
{
    if( m_boundingBox.GetWidth() == 0 && m_boundingBox.GetHeight() == 0 )
    {
        bool first = true;

        for( const std::vector<VECTOR2D>& pointList : m_pointLists )
        {
            for( const VECTOR2D& point : pointList )
            {
                if( first )
                {
                    m_boundingBox.SetOrigin( point );
                    first = false;
                }
                else
                {
                    m_boundingBox.Merge( point );
                }
            }
        }
    }

    return m_boundingBox;
}


std::shared_ptr<GLYPH> STROKE_GLYPH::Resize( const VECTOR2D& aGlyphSize ) const
{
    std::shared_ptr<STROKE_GLYPH> glyph = std::make_shared<STROKE_GLYPH>( *this );

    glyph->clearBoundingBox();

    bool first = true;

    for( std::vector<VECTOR2D>& pointList : glyph->m_pointLists )
    {
        for( VECTOR2D& point : pointList )
        {
            point.x = point.x * aGlyphSize.x;
            point.y = point.y * aGlyphSize.y;

            if( first )
            {
                glyph->m_boundingBox.SetOrigin( point );
                first = false;
            }
            else
            {
                glyph->m_boundingBox.Merge( point );
            }
        }
    }

    return glyph;
}


std::shared_ptr<GLYPH> STROKE_GLYPH::Translate( const VECTOR2D& aOffset ) const
{
    auto glyph = std::make_shared<STROKE_GLYPH>( *this );

    glyph->clearBoundingBox();

    bool first = true;

    for( std::vector<VECTOR2D>& pointList : glyph->m_pointLists )
    {
        for( VECTOR2D& point : pointList )
        {
            point.x += aOffset.x;
            point.y += aOffset.y;

            if( first )
            {
                glyph->m_boundingBox.SetOrigin( point );
                first = false;
            }
            else
            {
                glyph->m_boundingBox.Merge( point );
            }
        }
    }

    return glyph;
}


std::shared_ptr<GLYPH> STROKE_GLYPH::Transform( const VECTOR2D& aGlyphSize, const VECTOR2D& aOffset,
                                                double aTilt )
{
    std::shared_ptr<STROKE_GLYPH> glyph = std::make_shared<STROKE_GLYPH>( *this );

    glyph->clearBoundingBox();

    bool first = true;

    for( std::vector<VECTOR2D>& pointList : glyph->m_pointLists )
    {
        for( VECTOR2D& point : pointList )
        {
            point.x *= aGlyphSize.x;
            point.y *= aGlyphSize.y;

            if( aTilt )
                point.x -= point.y * aTilt;

            point.x += aOffset.x;
            point.y += aOffset.y;

            if( first )
            {
                glyph->m_boundingBox.SetOrigin( point );
                first = false;
            }
            else
            {
                glyph->m_boundingBox.Merge( point );
            }
        }
    }

    return glyph;
}


std::shared_ptr<GLYPH> STROKE_GLYPH::Mirror( bool aMirror, const VECTOR2D& aMirrorOrigin ) const
{
    // TODO figure out a way to not make a copy if aMirror is false
    auto glyph = std::make_shared<STROKE_GLYPH>( *this );

    if( aMirror )
        glyph->Mirror( aMirrorOrigin );

    return glyph;
}


void STROKE_GLYPH::Mirror( const VECTOR2D& aMirrorOrigin )
{
    double originX = aMirrorOrigin.x;

    clearBoundingBox();

    bool first = true;

    for( std::vector<VECTOR2D>& pointList : m_pointLists )
    {
        for( VECTOR2D& point : pointList )
        {
            if( first )
            {
                //originX = point.x;
                point.x = originX - ( point.x - originX );
                m_boundingBox.SetOrigin( point );
                first = false;
            }
            else
            {
                point.x = originX - ( point.x - originX );
                m_boundingBox.Merge( point );
            }
        }
    }
}
