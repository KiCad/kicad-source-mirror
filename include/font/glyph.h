/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GLYPH_H
#define GLYPH_H

#include <gal/gal.h>
#include <memory>
#include <math/box2.h>
#include <geometry/shape_poly_set.h>
#include <geometry/eda_angle.h>
#include <wx/debug.h>

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4275 )
#endif

namespace KIFONT
{


class GAL_API GLYPH
{
public:
    virtual ~GLYPH()
    {}

    virtual bool IsOutline() const { return false; }
    virtual bool IsStroke() const  { return false; }

    virtual BOX2D BoundingBox() = 0;

    bool IsHover() const { return m_isHover; }
    void SetIsHover( bool aIsHover ) { m_isHover = aIsHover; }

private:
    bool m_isHover = false;
};


class GAL_API OUTLINE_GLYPH : public GLYPH, public SHAPE_POLY_SET
{
public:
    OUTLINE_GLYPH() :
            SHAPE_POLY_SET()
    {}

    OUTLINE_GLYPH( const OUTLINE_GLYPH& aGlyph ) :
            SHAPE_POLY_SET( aGlyph )
    {}

    OUTLINE_GLYPH( const SHAPE_POLY_SET& aPoly ) :
            SHAPE_POLY_SET( aPoly )
    {}

    bool IsOutline() const override { return true; }

    BOX2D BoundingBox() override;

    void Triangulate( std::function<void( const VECTOR2I& aPt1,
                                          const VECTOR2I& aPt2,
                                          const VECTOR2I& aPt3 )> aCallback ) const;

    void CacheTriangulation( bool aPartition = true, bool aSimplify = false ) override;

    /**
     * @return a set of triangulated polygons from the glyph.  CacheTriangulation() will use this
     * data as hint data the next time around.
     */
    std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>> GetTriangulationData() const;

    /**
     * Cache the triangulation for the glyph from a known set of triangle indexes.
     * (See GetTriangulationData() above for more info.)
     */
    void CacheTriangulation( std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>>& aHintData );
};


class GAL_API STROKE_GLYPH : public GLYPH, public std::vector<std::vector<VECTOR2D>>
{
public:
    STROKE_GLYPH()
    {}

    STROKE_GLYPH( const STROKE_GLYPH& aGlyph );

    bool IsStroke() const override { return true; }

    void AddPoint( const VECTOR2D& aPoint );
    void RaisePen();
    void Finalize();

    BOX2D BoundingBox() override { return m_boundingBox; }
    void SetBoundingBox( const BOX2D& bbox ) { m_boundingBox = bbox; }

    std::unique_ptr<GLYPH> Transform( const VECTOR2D& aGlyphSize,  const VECTOR2I& aOffset,
                                      double aTilt, const EDA_ANGLE& aAngle, bool aMirror,
                                      const VECTOR2I& aOrigin  );

    void Move( const VECTOR2I& aOffset );

private:
    bool  m_penIsDown = false;
    BOX2D m_boundingBox;
};



} // namespace KIFONT

#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#endif  // GLYPH_H
