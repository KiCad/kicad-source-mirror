/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Janito Vaqueiro Ferreira Filho <janito.vff@gmail.com>
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

#ifndef GRAPHICS_IMPORTER_BUFFER_H
#define GRAPHICS_IMPORTER_BUFFER_H

#include "graphics_importer.h"

#include <math/matrix3x3.h>
#include <math/box2.h>
#include <list>


class IMPORTED_SHAPE
{
public:
    virtual ~IMPORTED_SHAPE() {}
    virtual void ImportTo( GRAPHICS_IMPORTER& aImporter ) const = 0;

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const = 0;

    virtual void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) = 0;

    void SetParentShapeIndex( int aIndex ) { m_parentShapeIndex = aIndex; }
    int  GetParentShapeIndex() const { return m_parentShapeIndex; }

    virtual BOX2D GetBoundingBox() const = 0;

protected:
    int m_parentShapeIndex = -1;
};


class IMPORTED_LINE : public IMPORTED_SHAPE
{
public:
    IMPORTED_LINE( const VECTOR2D& aStart, const VECTOR2D& aEnd, const IMPORTED_STROKE& aStroke ) :
            m_start( aStart ), m_end( aEnd ), m_stroke( aStroke )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddLine( m_start, m_end, m_stroke );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_LINE>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        m_start = aTransform * m_start + aTranslation;
        m_end = aTransform * m_end + aTranslation;
    }

    BOX2D GetBoundingBox() const override
    {
        BOX2D box;
        box.Merge( m_start );
        box.Merge( m_end );
        return box;
    }

private:
    VECTOR2D        m_start;
    VECTOR2D        m_end;
    IMPORTED_STROKE m_stroke;
};


class IMPORTED_CIRCLE : public IMPORTED_SHAPE
{
public:
    IMPORTED_CIRCLE( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke,
                     bool aFilled, const COLOR4D& aFillColor ) :
            m_center( aCenter ),
            m_radius( aRadius ), m_stroke( aStroke ), m_filled( aFilled ), m_fillColor( aFillColor )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddCircle( m_center, m_radius, m_stroke, m_filled, m_fillColor );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_CIRCLE>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        VECTOR2D newCenter = ( aTransform * m_center ) + aTranslation;

        VECTOR2D newRadius = VECTOR2D( m_radius, 0 );
        newRadius = aTransform * newRadius;

        m_center = newCenter;
        m_radius = newRadius.EuclideanNorm();
    }

    BOX2D GetBoundingBox() const override
    {
        BOX2D box;
        box.Merge( m_center - VECTOR2D( m_radius, m_radius ) );
        box.Merge( m_center + VECTOR2D( m_radius, m_radius ) );
        return box;
    }

private:
    VECTOR2D        m_center;
    double          m_radius;
    IMPORTED_STROKE m_stroke;
    bool            m_filled;
    COLOR4D         m_fillColor;
};


class IMPORTED_ARC : public IMPORTED_SHAPE
{
public:
    IMPORTED_ARC( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                  const IMPORTED_STROKE& aStroke ) :
            m_center( aCenter ),
            m_start( aStart ), m_angle( aAngle ), m_stroke( aStroke )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddArc( m_center, m_start, m_angle, m_stroke );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_ARC>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        m_start = aTransform * m_start + aTranslation;
        m_center = aTransform * m_center + aTranslation;
    }

    BOX2D GetBoundingBox() const override
    {
        BOX2D box;

        box.Merge( m_start + m_stroke.GetWidth() );
        box.Merge( m_start - m_stroke.GetWidth() );

        for( double angle = 0; angle < m_angle.AsDegrees(); angle += 5 )
        {
            EDA_ANGLE ang = EDA_ANGLE( angle );
            VECTOR2D start = m_center + m_start;
            VECTOR2D pt = {start.x * ang.Cos() + start.y * ang.Sin(),
                           start.x * ang.Sin() - start.y * ang.Cos()};

            box.Merge( pt );
        }

        return box;
    }

private:
    VECTOR2D        m_center;
    VECTOR2D        m_start;
    EDA_ANGLE       m_angle;
    IMPORTED_STROKE m_stroke;
};


class IMPORTED_POLYGON : public IMPORTED_SHAPE
{
public:
    IMPORTED_POLYGON( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke,
                      bool aFilled, const COLOR4D& aFillColor ) :
            m_vertices( aVertices ),
            m_stroke( aStroke ),
            m_filled( aFilled ),
            m_fillColor( aFillColor )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddPolygon( m_vertices, m_stroke, m_filled, m_fillColor );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_POLYGON>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        for( VECTOR2D& vert : m_vertices )
        {
            vert = aTransform * vert + aTranslation;
        }
    }

    std::vector<VECTOR2D>& Vertices() { return m_vertices; }

    bool IsFilled() const { return m_filled; }

    const COLOR4D& GetFillColor() const { return m_fillColor; }

    const IMPORTED_STROKE& GetStroke() const { return m_stroke; }

    BOX2D GetBoundingBox() const override
    {
        BOX2D box;

        for( const VECTOR2D& vert : m_vertices )
            box.Merge( vert );

        return box;
    }

private:
    std::vector<VECTOR2D> m_vertices;
    IMPORTED_STROKE       m_stroke;
    bool                  m_filled;
    COLOR4D               m_fillColor;
};


class IMPORTED_TEXT : public IMPORTED_SHAPE
{
public:
    IMPORTED_TEXT( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth,
                   double aThickness, double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                   GR_TEXT_V_ALIGN_T aVJustify, const COLOR4D& aColor ) :
            m_origin( aOrigin ),
            m_text( aText ), m_height( aHeight ), m_width( aWidth ), m_thickness( aThickness ),
            m_orientation( aOrientation ), m_hJustify( aHJustify ), m_vJustify( aVJustify ),
            m_color( aColor )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddText( m_origin, m_text, m_height, m_width, m_thickness, m_orientation,
                           m_hJustify, m_vJustify, m_color );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_TEXT>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        m_origin = aTransform * m_origin + aTranslation;

        VECTOR2D textSize = aTransform * VECTOR2D( m_width, m_height );
        m_width = textSize.x;
        m_height = textSize.y;
    }

    BOX2D GetBoundingBox() const override
    {
        BOX2D box;
        box.Merge( m_origin );
        box.Merge( m_origin + VECTOR2D( m_width * m_text.length(), m_height ) );

        return box;
    }

private:
    VECTOR2D          m_origin;
    const wxString    m_text;
    double            m_height;
    double            m_width;
    double            m_thickness;
    double            m_orientation;
    GR_TEXT_H_ALIGN_T m_hJustify;
    GR_TEXT_V_ALIGN_T m_vJustify;
    COLOR4D           m_color;
};


class IMPORTED_SPLINE : public IMPORTED_SHAPE
{
public:
    IMPORTED_SPLINE( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                     const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                     const IMPORTED_STROKE& aStroke ) :
            m_start( aStart ),
            m_bezierControl1( aBezierControl1 ), m_bezierControl2( aBezierControl2 ), m_end( aEnd ),
            m_stroke( aStroke )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddSpline( m_start, m_bezierControl1, m_bezierControl2, m_end, m_stroke );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_SPLINE>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        m_start = aTransform * m_start + aTranslation;
        m_bezierControl1 = aTransform * m_bezierControl1 + aTranslation;
        m_bezierControl2 = aTransform * m_bezierControl2 + aTranslation;
        m_end = aTransform * m_end + aTranslation;
    }

    BOX2D GetBoundingBox() const override
    {
        BOX2D box;
        box.Merge( m_start );
        box.Merge( m_end );
        return box;
    }

private:
    VECTOR2D        m_start;
    VECTOR2D        m_bezierControl1;
    VECTOR2D        m_bezierControl2;
    VECTOR2D        m_end;
    IMPORTED_STROKE m_stroke;
};


class GRAPHICS_IMPORTER_BUFFER : public GRAPHICS_IMPORTER
{
public:
    void AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                  const IMPORTED_STROKE& aStroke ) override;

    void AddCircle( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke,
                    bool aFilled, const COLOR4D& aFillColor = COLOR4D::UNSPECIFIED ) override;

    void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                 const IMPORTED_STROKE& aStroke ) override;

    void AddPolygon( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke,
                     bool aFilled, const COLOR4D& aFillColor = COLOR4D::UNSPECIFIED ) override;

    void AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth,
                  double aThickness, double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                  GR_TEXT_V_ALIGN_T aVJustify,
                  const COLOR4D&    aColor = COLOR4D::UNSPECIFIED ) override;

    void AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                    const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                    const IMPORTED_STROKE& aStroke ) override;

    void ImportTo( GRAPHICS_IMPORTER& aImporter );
    void AddShape( std::unique_ptr<IMPORTED_SHAPE>& aShape );

    std::list<std::unique_ptr<IMPORTED_SHAPE>>& GetShapes() { return m_shapes; }

    void ClearShapes() { m_shapes.clear(); }

    void PostprocessNestedPolygons();

protected:
    /// List of imported shapes.
    std::list<std::unique_ptr<IMPORTED_SHAPE>> m_shapes;
};

#endif /* GRAPHICS_IMPORTER_BUFFER */
