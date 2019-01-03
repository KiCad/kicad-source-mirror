/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Janito Vaqueiro Ferreira Filho <janito.vff@gmail.com>
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <list>

class IMPORTED_SHAPE
{
public:
    virtual ~IMPORTED_SHAPE() {}
    virtual void ImportTo( GRAPHICS_IMPORTER& aImporter ) const = 0;
};


class IMPORTED_LINE : public IMPORTED_SHAPE
{
public:
    IMPORTED_LINE( const VECTOR2D& aStart, const VECTOR2D& aEnd, double aWidth )
        : m_start( aStart ), m_end( aEnd ), m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddLine( m_start, m_end, m_width );
    }

private:
    const VECTOR2D m_start;
    const VECTOR2D m_end;
    double m_width;
};


class IMPORTED_CIRCLE : public IMPORTED_SHAPE
{
public:
    IMPORTED_CIRCLE( const VECTOR2D& aCenter, double aRadius, double aWidth )
        : m_center( aCenter ), m_radius( aRadius ), m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddCircle( m_center, m_radius, m_width );
    }

private:
    const VECTOR2D m_center;
    double m_radius;
    double m_width;
};


class IMPORTED_ARC : public IMPORTED_SHAPE
{
public:
    IMPORTED_ARC( const VECTOR2D& aCenter, const VECTOR2D& aStart, double aAngle, double aWidth )
        : m_center( aCenter ), m_start( aStart ), m_angle( aAngle ), m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddArc( m_center, m_start, m_angle, m_width );
    }

private:
    const VECTOR2D m_center;
    const VECTOR2D m_start;
    double m_angle;
    double m_width;
};


class IMPORTED_POLYGON : public IMPORTED_SHAPE
{
public:
    IMPORTED_POLYGON( const std::vector< VECTOR2D >& aVertices, double aWidth )
        : m_vertices( aVertices ), m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddPolygon( m_vertices, m_width );
    }

private:
    const std::vector< VECTOR2D > m_vertices;
    double m_width;
};


class IMPORTED_TEXT : public IMPORTED_SHAPE
{
public:
    IMPORTED_TEXT( const VECTOR2D& aOrigin, const wxString& aText,
            double aHeight, double aWidth, double aThickness, double aOrientation,
            EDA_TEXT_HJUSTIFY_T aHJustify, EDA_TEXT_VJUSTIFY_T aVJustify )
        : m_origin( aOrigin ), m_text( aText ),
            m_height( aHeight ), m_width( aWidth ), m_thickness( aThickness ),
            m_orientation( aOrientation ),
            m_hJustify( aHJustify ), m_vJustify( aVJustify )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddText( m_origin, m_text, m_height, m_width,
                    m_thickness, m_orientation, m_hJustify, m_vJustify );
    }

private:
    const VECTOR2D m_origin;
    const wxString m_text;
    double m_height;
    double m_width;
    double m_thickness;
    double m_orientation;
    EDA_TEXT_HJUSTIFY_T m_hJustify;
    EDA_TEXT_VJUSTIFY_T m_vJustify;
};


class IMPORTED_SPLINE : public IMPORTED_SHAPE
{
public:
    IMPORTED_SPLINE( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                     const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd, double aWidth )
        : m_start( aStart ), m_bezierControl1( aBezierControl1 ),
          m_bezierControl2( aBezierControl2 ), m_end( aEnd ), m_width( aWidth )
    {
    }

    void ImportTo( GRAPHICS_IMPORTER& aImporter ) const override
    {
        aImporter.AddSpline( m_start, m_bezierControl1, m_bezierControl2, m_end, m_width );
    }

private:
    const VECTOR2D m_start;
    const VECTOR2D m_bezierControl1;
    const VECTOR2D m_bezierControl2;
    const VECTOR2D m_end;
    double m_width;
};


class GRAPHICS_IMPORTER_BUFFER : public GRAPHICS_IMPORTER
{
public:
    void AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd, double aWidth ) override;

    void AddCircle( const VECTOR2D& aCenter, double aRadius, double aWidth ) override;

    void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, double aAngle, double aWidth ) override;

    void AddPolygon( const std::vector< VECTOR2D >& aVertices, double aWidth ) override;

    void AddText( const VECTOR2D& aOrigin, const wxString& aText,
            double aHeight, double aWidth, double aThickness, double aOrientation,
            EDA_TEXT_HJUSTIFY_T aHJustify, EDA_TEXT_VJUSTIFY_T aVJustify ) override;

    void AddSpline( const VECTOR2D& aStart, const VECTOR2D& BezierControl1,
                    const VECTOR2D& BezierControl2, const VECTOR2D& aEnd , double aWidth ) override;

    void ImportTo( GRAPHICS_IMPORTER& aImporter );

protected:
    ///> List of imported shapes
    std::list< std::unique_ptr< IMPORTED_SHAPE > > m_shapes;
};

#endif /* GRAPHICS_IMPORTER_BUFFER */
