/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful
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

#include "graphics_importer_sch.h"

#include <memory>
#include <tuple>

#include <sch_line.h>
#include <sch_shape.h>
#include <sch_text.h>


GRAPHICS_IMPORTER_SCH::GRAPHICS_IMPORTER_SCH()
{
    m_millimeterToIu = schIUScale.mmToIU( 1.0 );
}


VECTOR2I GRAPHICS_IMPORTER_SCH::MapCoordinate( const VECTOR2D& aCoordinate )
{
    VECTOR2D coord = aCoordinate;
    coord *= GetScale();
    coord += GetImportOffsetMM();
    coord *= GetMillimeterToIuFactor();

    return KiROUND( coord );
}


int GRAPHICS_IMPORTER_SCH::MapLineWidth( double aLineWidth )
{
    VECTOR2D factor = ImportScalingFactor();
    double   scale = ( std::abs( factor.x ) + std::abs( factor.y ) ) * 0.5;

    if( aLineWidth <= 0.0 )
        return int( GetLineWidthMM() * scale );

    // aLineWidth is in mm:
    return int( aLineWidth * scale );
}


STROKE_PARAMS GRAPHICS_IMPORTER_SCH::MapStrokeParams( const IMPORTED_STROKE& aStroke )
{
    // Historicaly -1 meant no-stroke in Eeschema.
    int width = ( aStroke.GetWidth() == -1 ) ? -1 : MapLineWidth( aStroke.GetWidth() );

    return STROKE_PARAMS( width, aStroke.GetPlotStyle(), aStroke.GetColor() );
}


void GRAPHICS_IMPORTER_SCH::AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                                     const IMPORTED_STROKE& aStroke )
{
    VECTOR2I pt0 = MapCoordinate( aStart );
    VECTOR2I pt1 = MapCoordinate( aEnd );

    // Skip 0 len lines:
    if( pt0 == pt1 )
        return;

    std::unique_ptr<SCH_LINE> line = std::make_unique<SCH_LINE>();
    line->SetStroke( MapStrokeParams( aStroke ) );

    line->SetStartPoint( pt0 );
    line->SetEndPoint( pt1 );

    addItem( std::move( line ) );
}


void GRAPHICS_IMPORTER_SCH::AddCircle( const VECTOR2D& aCenter, double aRadius,
                                       const IMPORTED_STROKE& aStroke, bool aFilled,
                                       const COLOR4D& aFillColor )
{
    std::unique_ptr<SCH_SHAPE> circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE );
    circle->SetFillColor( aFillColor );
    circle->SetFilled( aFilled );
    circle->SetStroke( MapStrokeParams( aStroke ) );
    circle->SetStart( MapCoordinate( aCenter ) );
    circle->SetEnd( MapCoordinate( VECTOR2D( aCenter.x + aRadius, aCenter.y ) ) );

    addItem( std::move( circle ) );
}


void GRAPHICS_IMPORTER_SCH::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart,
                                           const EDA_ANGLE& aAngle, const IMPORTED_STROKE& aStroke )
{
    std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC );

    /**
     * We need to perform the rotation/conversion here while still using floating point values
     * to avoid rounding errors when operating in integer space in KiCad
     */
    VECTOR2D end = aStart;
    VECTOR2D mid = aStart;

    RotatePoint( end, aCenter, -aAngle );
    RotatePoint( mid, aCenter, -aAngle / 2.0 );

    arc->SetArcGeometry( MapCoordinate( aStart ), MapCoordinate( mid ), MapCoordinate( end ) );

    // Ensure the arc can be handled by KiCad. Arcs with a too big radius cannot.
    // The criteria used here is radius < MAX_INT / 2.
    // this is not perfect, but we do not know the exact final position of the arc, so
    // we cannot test the coordinate values, because the arc can be moved before being placed.
    VECTOR2D         center = CalcArcCenter( arc->GetStart(), arc->GetEnd(), aAngle );
    double           radius = ( center - arc->GetStart() ).EuclideanNorm();
    constexpr double rd_max_value = std::numeric_limits<VECTOR2I::coord_type>::max() / 2.0;

    if( radius >= rd_max_value )
    {
        // Arc cannot be handled: convert it to a segment
        AddLine( aStart, end, aStroke );
        return;
    }

    arc->SetStroke( MapStrokeParams( aStroke ) );

    addItem( std::move( arc ) );
}


void GRAPHICS_IMPORTER_SCH::AddPolygon( const std::vector<VECTOR2D>& aVertices,
                                               const IMPORTED_STROKE& aStroke, bool aFilled,
                                               const COLOR4D& aFillColor )
{
    std::vector<VECTOR2I> convertedPoints;
    convertedPoints.reserve( aVertices.size() );

    for( const VECTOR2D& precisePoint : aVertices )
        convertedPoints.emplace_back( MapCoordinate( precisePoint ) );

    if( convertedPoints.empty() )
        return;

    std::unique_ptr<SCH_SHAPE> polygon = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY );

    if( aFilled )
    {
        polygon->SetFillMode( aFillColor != COLOR4D::UNSPECIFIED ? FILL_T::FILLED_WITH_COLOR
                                                                 : FILL_T::FILLED_SHAPE );
    }

    polygon->SetFillColor( aFillColor );
    polygon->SetPolyPoints( convertedPoints );
    polygon->AddPoint( convertedPoints[0] ); // Need to close last point for libedit

    polygon->SetStroke( MapStrokeParams( aStroke ) );

    addItem( std::move( polygon ) );
}


void GRAPHICS_IMPORTER_SCH::AddText( const VECTOR2D& aOrigin, const wxString& aText,
                                            double aHeight, double aWidth, double aThickness,
                                            double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                                            GR_TEXT_V_ALIGN_T aVJustify, const COLOR4D& aColor )
{
    std::unique_ptr<SCH_TEXT> textItem = std::make_unique<SCH_TEXT>();
    textItem->SetTextColor( aColor );
    textItem->SetTextThickness( MapLineWidth( aThickness ) );
    textItem->SetTextPos( MapCoordinate( aOrigin ) );
    textItem->SetTextAngle( EDA_ANGLE( aOrientation, DEGREES_T ) );
    textItem->SetTextWidth( aWidth * ImportScalingFactor().x );
    textItem->SetTextHeight( aHeight * ImportScalingFactor().y );
    textItem->SetVertJustify( aVJustify );
    textItem->SetHorizJustify( aHJustify );
    textItem->SetText( aText );

    addItem( std::move( textItem ) );
}


void GRAPHICS_IMPORTER_SCH::AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                                       const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                                       const IMPORTED_STROKE& aStroke )
{
    std::unique_ptr<SCH_SHAPE> spline = std::make_unique<SCH_SHAPE>( SHAPE_T::BEZIER );
    spline->SetStroke( MapStrokeParams( aStroke ) );
    spline->SetStart( MapCoordinate( aStart ) );
    spline->SetBezierC1( MapCoordinate( aBezierControl1 ) );
    spline->SetBezierC2( MapCoordinate( aBezierControl2 ) );
    spline->SetEnd( MapCoordinate( aEnd ) );

    if( setupSplineOrLine( *spline, aStroke.GetWidth() / 2 ) )
    {
        // SCH_LINES aren't SCH_SHAPES
        if( spline->GetShape() == SHAPE_T::SEGMENT )
            AddLine( aStart, aEnd, aStroke );
        else
            addItem( std::move( spline ) );
    }
}
