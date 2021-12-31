/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
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

    virtual void AddPoint( const VECTOR2D& aPoint ) = 0;

    virtual void RaisePen() = 0;

    virtual void Finalize() = 0;

    virtual BOX2D BoundingBox() = 0;

    virtual std::shared_ptr<GLYPH> Resize( const VECTOR2D& aGlyphSize ) const = 0;

    virtual std::shared_ptr<GLYPH> Translate( const VECTOR2D& aOffset ) const = 0;

    virtual std::shared_ptr<GLYPH> Mirror( bool aMirror,
                                           const VECTOR2D& aMirrorOrigin = { 0, 0 } ) const = 0;

    virtual void Mirror( const VECTOR2D& aMirrorOrigin = { 0, 0 } ) = 0;

    virtual const SHAPE_POLY_SET& GetPolylist() const = 0;

    virtual const std::vector<std::vector<VECTOR2D>>& GetPoints() const = 0;

    virtual bool IsOutline() const { return false; }
    virtual bool IsStroke() const { return false; }
};

typedef std::vector<std::shared_ptr<GLYPH>> GLYPH_LIST;


class OUTLINE_GLYPH : public GLYPH
{
public:
    OUTLINE_GLYPH( const SHAPE_POLY_SET& poly )
    {
        m_polySet = poly;
    }

    const SHAPE_POLY_SET& GetPolylist() const override { return m_polySet; }

    bool IsOutline() const override
    {
        return true;
    }

    //
    void AddPoint( const VECTOR2D& aPoint ) override
    {
        wxFAIL_MSG( "unimplemented" );
    }

    void  RaisePen() override
    {
        wxFAIL_MSG( "unimplemented" );
    }

    void  Finalize() override
    {
        wxFAIL_MSG( "unimplemented" );
    }

    BOX2D BoundingBox() override
    {
        wxFAIL_MSG( "unimplemented" );
        return BOX2D();
    }

    std::shared_ptr<GLYPH> Resize( const VECTOR2D& aGlyphSize ) const override
    {
        wxFAIL_MSG( "unimplemented" );
        return nullptr;
    }

    std::shared_ptr<GLYPH> Translate( const VECTOR2D& aOffset ) const override
    {
        wxFAIL_MSG( "unimplemented" );
        return nullptr;
    }

    std::shared_ptr<GLYPH> Mirror( bool aMirror,
                                   const VECTOR2D& aMirrorOrigin = { 0, 0 } ) const override
    {
        wxFAIL_MSG( "unimplemented" );
        return nullptr;
    }

    void Mirror( const VECTOR2D& aMirrorOrigin = VECTOR2D( 0, 0 ) ) override
    {
        wxFAIL_MSG( "unimplemented" );
    }

    const std::vector<std::vector<VECTOR2D>>& GetPoints() const override
    {
        wxFAIL_MSG( "unimplemented" );
        return m_dummy;
    }

private:
    SHAPE_POLY_SET                     m_polySet;

    // For unimplemented return values
    std::vector<std::vector<VECTOR2D>> m_dummy;
};


class STROKE_GLYPH : public GLYPH
{
public:
    STROKE_GLYPH()
    {}

    STROKE_GLYPH( const STROKE_GLYPH& aGlyph );

    void AddPoint( const VECTOR2D& aPoint ) override;

    void RaisePen() override;

    void Finalize() override;

    BOX2D BoundingBox() override;

    std::shared_ptr<GLYPH> Resize( const VECTOR2D& aGlyphSize ) const override;

    std::shared_ptr<GLYPH> Translate( const VECTOR2D& aOffset ) const override;

    std::shared_ptr<GLYPH> Transform( const VECTOR2D& aGlyphSize,  const VECTOR2D& aOffset,
                                      double aTilt );

    std::shared_ptr<GLYPH> Mirror( bool aMirror,
                                   const VECTOR2D& aMirrorOrigin = { 0, 0 } ) const override;

    void Mirror( const VECTOR2D& aMirrorOrigin = { 0, 0 } ) override;

    bool IsStroke() const override { return true; }

    //
    const SHAPE_POLY_SET& GetPolylist() const override
    {
        wxFAIL_MSG( "unimplemented" );
        return m_dummy;
    }

    const std::vector<std::vector<VECTOR2D>>& GetPoints() const override { return m_pointLists; }

private:
    void clearBoundingBox()
    {
        m_boundingBox.SetOrigin( 0, 0 );
        m_boundingBox.SetSize( 0, 0 );
    }

private:
    bool                               m_penIsDown = false;
    std::vector<std::vector<VECTOR2D>> m_pointLists;
    BOX2D                              m_boundingBox;

    // For unimplemented return values
    SHAPE_POLY_SET                     m_dummy;
};


typedef std::vector<VECTOR2D>     GLYPH_POINTS;
typedef std::vector<GLYPH_POINTS> GLYPH_POINTS_LIST;
typedef std::vector<BOX2D>        GLYPH_BOUNDING_BOX_LIST;


} // namespace KIFONT

#endif  // GLYPH_H
