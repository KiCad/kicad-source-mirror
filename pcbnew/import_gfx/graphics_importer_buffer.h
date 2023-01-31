/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2018-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <list>

class IMPORTED_SHAPE
{
public:
    virtual ~IMPORTED_SHAPE() {}
    virtual void ImportTo( GRAPHICS_IMPORTER& aImporter ) const = 0;

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const = 0;

    virtual void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) = 0;

    void SetParentShapeIndex( int aIndex ) { m_parentShapeIndex = aIndex; }
    int GetParentShapeIndex() const { return m_parentShapeIndex; }

protected:
    int m_parentShapeIndex = -1;
};


class IMPORTED_LINE : public IMPORTED_SHAPE
{
public:
    IMPORTED_LINE( const VECTOR2D& aStart, const VECTOR2D& aEnd, double aWidth ) :
            m_start( aStart ),
            m_end( aEnd ),
            m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddLine( m_start, m_end, m_width );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_LINE>( *this );
    }

    void Transform(const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation) override
    {
        m_start = aTransform * m_start + aTranslation;
        m_end = aTransform * m_end + aTranslation;
    }

private:
    VECTOR2D m_start;
    VECTOR2D m_end;
    double   m_width;
};


class IMPORTED_CIRCLE : public IMPORTED_SHAPE
{
public:
    IMPORTED_CIRCLE( const VECTOR2D& aCenter, double aRadius, double aWidth, bool aFilled ) :
            m_center( aCenter ),
            m_radius( aRadius ),
            m_width( aWidth ),
            m_filled( aFilled )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddCircle( m_center, m_radius, m_width, m_filled );
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

private:
    VECTOR2D m_center;
    double   m_radius;
    double   m_width;
    bool     m_filled;
};


class IMPORTED_ARC : public IMPORTED_SHAPE
{
public:
    IMPORTED_ARC( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                  double aWidth )  :
            m_center( aCenter ),
            m_start( aStart ),
            m_angle( aAngle ),
            m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddArc( m_center, m_start, m_angle, m_width );
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

private:
    VECTOR2D  m_center;
    VECTOR2D  m_start;
    EDA_ANGLE m_angle;
    double    m_width;
};


class IMPORTED_POLYGON : public IMPORTED_SHAPE
{
public:
    IMPORTED_POLYGON( const std::vector< VECTOR2D >& aVertices, double aWidth ) :
            m_vertices( aVertices ),
            m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddPolygon( m_vertices, m_width );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_POLYGON>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        for(VECTOR2D& vert : m_vertices )
        {
            vert = aTransform * vert + aTranslation;
        }
    }

    std::vector<VECTOR2D>& Vertices() { return m_vertices; }

    double GetWidth() const { return m_width; }

private:
    std::vector<VECTOR2D> m_vertices;
    double                m_width;
};


class IMPORTED_TEXT : public IMPORTED_SHAPE
{
public:
    IMPORTED_TEXT( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth,
                   double aThickness, double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                   GR_TEXT_V_ALIGN_T aVJustify ) :
        m_origin( aOrigin ),
        m_text( aText ),
        m_height( aHeight ),
        m_width( aWidth ),
        m_thickness( aThickness ),
        m_orientation( aOrientation ),
        m_hJustify( aHJustify ),
        m_vJustify( aVJustify )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddText( m_origin, m_text, m_height, m_width,
                    m_thickness, m_orientation, m_hJustify, m_vJustify );
    }

    virtual std::unique_ptr<IMPORTED_SHAPE> clone() const override
    {
        return std::make_unique<IMPORTED_TEXT>( *this );
    }

    void Transform( const MATRIX3x3D& aTransform, const VECTOR2D& aTranslation ) override
    {
        m_origin = aTransform * m_origin + aTranslation;
    }

private:
    VECTOR2D            m_origin;
    const wxString      m_text;
    double              m_height;
    double              m_width;
    double              m_thickness;
    double              m_orientation;
    GR_TEXT_H_ALIGN_T   m_hJustify;
    GR_TEXT_V_ALIGN_T   m_vJustify;
};


class IMPORTED_SPLINE : public IMPORTED_SHAPE
{
public:
    IMPORTED_SPLINE( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                     const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd, double aWidth ) :
        m_start( aStart ),
        m_bezierControl1( aBezierControl1 ),
        m_bezierControl2( aBezierControl2 ),
        m_end( aEnd ),
        m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddSpline( m_start, m_bezierControl1, m_bezierControl2, m_end, m_width );
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

private:
    VECTOR2D m_start;
    VECTOR2D m_bezierControl1;
    VECTOR2D m_bezierControl2;
    VECTOR2D m_end;
    double   m_width;
};


class GRAPHICS_IMPORTER_BUFFER : public GRAPHICS_IMPORTER
{
public:
    void AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd, double aWidth ) override;

    void AddCircle( const VECTOR2D& aCenter, double aRadius, double aWidth, bool aFilled ) override;

    void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                 double aWidth ) override;

    void AddPolygon( const std::vector< VECTOR2D >& aVertices, double aWidth ) override;

    void AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth,
                  double aThickness, double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                  GR_TEXT_V_ALIGN_T aVJustify ) override;

    void AddSpline( const VECTOR2D& aStart, const VECTOR2D& BezierControl1,
                    const VECTOR2D& BezierControl2, const VECTOR2D& aEnd , double aWidth ) override;

    void ImportTo( GRAPHICS_IMPORTER& aImporter );
    void AddShape( std::unique_ptr<IMPORTED_SHAPE>& aShape );

    std::list<std::unique_ptr<IMPORTED_SHAPE>>& GetShapes()
    {
        return m_shapes;
    }

    void ClearShapes()
    {
        m_shapes.clear();
    }

    void PostprocessNestedPolygons();

protected:
    ///< List of imported shapes
    std::list< std::unique_ptr< IMPORTED_SHAPE > > m_shapes;
};

#endif /* GRAPHICS_IMPORTER_BUFFER */
