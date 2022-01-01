/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <math/box2.h>
#include <geometry/shape_poly_set.h>
#include <wx/debug.h>

namespace KIFONT
{
class GLYPH
{
public:
    virtual ~GLYPH()
    {}

    virtual bool IsOutline() const { return false; }
    virtual bool IsStroke() const  { return false; }

    virtual BOX2D BoundingBox() = 0;

    virtual void Mirror( const VECTOR2D& aMirrorOrigin = { 0, 0 } ) = 0;
};


class OUTLINE_GLYPH : public GLYPH, public SHAPE_POLY_SET
{
public:
    OUTLINE_GLYPH() :
            SHAPE_POLY_SET()
    {}

    bool IsOutline() const override { return true; }

    BOX2D BoundingBox() override;

    void Mirror( const VECTOR2D& aMirrorOrigin = VECTOR2D( 0, 0 ) ) override;
};


class STROKE_GLYPH : public GLYPH, public std::vector<std::vector<VECTOR2D>>
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

    void Mirror( const VECTOR2D& aMirrorOrigin = { 0, 0 } ) override;

    std::unique_ptr<GLYPH> Transform( const VECTOR2D& aGlyphSize,  const VECTOR2D& aOffset,
                                      double aTilt );

private:
    bool  m_penIsDown = false;
    BOX2D m_boundingBox;
};


typedef std::vector<VECTOR2D>     GLYPH_POINTS;
typedef std::vector<GLYPH_POINTS> GLYPH_POINTS_LIST;
typedef std::vector<BOX2D>        GLYPH_BOUNDING_BOX_LIST;


} // namespace KIFONT

#endif  // GLYPH_H
