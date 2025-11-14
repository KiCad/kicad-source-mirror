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

#include "graphics_importer_pcbnew.h"

#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <memory>
#include <tuple>


GRAPHICS_IMPORTER_PCBNEW::GRAPHICS_IMPORTER_PCBNEW( BOARD_ITEM_CONTAINER* aParent ) :
        m_parent( aParent )
{
    m_layer = Dwgs_User;
    m_millimeterToIu = pcbIUScale.mmToIU( 1.0 );
}


VECTOR2I GRAPHICS_IMPORTER_PCBNEW::MapCoordinate( const VECTOR2D& aCoordinate )
{
    VECTOR2D coord = aCoordinate;
    coord *= GetScale();
    coord += GetImportOffsetMM();
    coord *= GetMillimeterToIuFactor();

    return KiROUND( coord.x, coord.y );
}


int GRAPHICS_IMPORTER_PCBNEW::MapLineWidth( double aLineWidth )
{
    VECTOR2D factor = ImportScalingFactor();
    double   scale = ( factor.x + factor.y ) * 0.5;

    if( aLineWidth <= 0.0 )
        return int( GetLineWidthMM() * scale );

    // aLineWidth is in mm:
    return int( aLineWidth * scale );
}


STROKE_PARAMS GRAPHICS_IMPORTER_PCBNEW::MapStrokeParams( const IMPORTED_STROKE& aStroke )
{
    // Historicaly -1 meant no-stroke in Eeschema, but this has never been the case for
    // PCBNew.  (The importer, which doesn't know which program it's creating content for,
    // also uses -1 for no-stroke.)
    int width = ( aStroke.GetWidth() == -1 ) ? 0 : MapLineWidth( aStroke.GetWidth() );

    return STROKE_PARAMS( width, aStroke.GetPlotStyle(), aStroke.GetColor() );
}


void GRAPHICS_IMPORTER_PCBNEW::AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                                        const IMPORTED_STROKE& aStroke )
{
    std::unique_ptr<PCB_SHAPE> line = std::make_unique<PCB_SHAPE>( m_parent );
    line->SetShape( SHAPE_T::SEGMENT );
    line->SetLayer( GetLayer() );
    line->SetStroke( MapStrokeParams( aStroke ) );
    line->SetStart( MapCoordinate( aStart ) );
    line->SetEnd( MapCoordinate( aEnd ) );

    // Skip 0 len lines:
    if( line->GetStart() == line->GetEnd() )
        return;

    addItem( std::move( line ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddCircle( const VECTOR2D& aCenter, double aRadius,
                                          const IMPORTED_STROKE& aStroke, bool aFilled,
                                          const COLOR4D& aFillColor )
{
    std::unique_ptr<PCB_SHAPE> circle = std::make_unique<PCB_SHAPE>( m_parent );
    circle->SetShape( SHAPE_T::CIRCLE );
    circle->SetFilled( aFilled );
    circle->SetLayer( GetLayer() );
    circle->SetStroke( MapStrokeParams( aStroke ) );
    circle->SetStart( MapCoordinate( aCenter ) );
    circle->SetEnd( MapCoordinate( VECTOR2D( aCenter.x + aRadius, aCenter.y ) ) );

    addItem( std::move( circle ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart,
                                       const EDA_ANGLE& aAngle, const IMPORTED_STROKE& aStroke )
{
    /**
     * We need to perform the rotation/conversion here while still using floating point values
     * to avoid rounding errors when operating in integer space in pcbnew
     */
    VECTOR2D end = aStart;
    VECTOR2D mid = aStart;

    RotatePoint( end, aCenter, -aAngle );
    RotatePoint( mid, aCenter, -aAngle / 2.0 );

    // Ensure the arc can be handled by Pcbnew. Arcs with a too big radius cannot.
    // The criteria used here is radius < MAX_INT / 2.
    // this is not perfect, but we do not know the exact final position of the arc, so
    // we cannot test the coordinate values, because the arc can be moved before being placed.
    VECTOR2D center = MapCoordinate( aCenter );
    double radius = ( center - MapCoordinate( aStart ) ).EuclideanNorm();
    double rd_max_value = std::numeric_limits<VECTOR2I::coord_type>::max() / 2.0;

    if( radius >= rd_max_value )
    {
        // Arc cannot be handled: convert it to a segment
        AddLine( aStart, end, aStroke );
        return;
    }

    std::unique_ptr<PCB_SHAPE> arc = std::make_unique<PCB_SHAPE>( m_parent );
    arc->SetShape( SHAPE_T::ARC );
    arc->SetLayer( GetLayer() );

    arc->SetArcGeometry( MapCoordinate( aStart ), MapCoordinate( mid ), MapCoordinate( end ) );
    arc->SetStroke( MapStrokeParams( aStroke ) );

    addItem( std::move( arc ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddPolygon( const std::vector<VECTOR2D>& aVertices,
                                           const IMPORTED_STROKE& aStroke, bool aFilled,
                                           const COLOR4D& aFillColor )
{
    std::vector<VECTOR2I> convertedPoints;
    convertedPoints.reserve( aVertices.size() );

    for( const VECTOR2D& precisePoint : aVertices )
        convertedPoints.emplace_back( MapCoordinate( precisePoint ) );

    std::unique_ptr<PCB_SHAPE> polygon = std::make_unique<PCB_SHAPE>( m_parent );
    polygon->SetShape( SHAPE_T::POLY );
    polygon->SetFilled( aFilled );
    polygon->SetLayer( GetLayer() );
    polygon->SetPolyPoints( convertedPoints );

    if( FOOTPRINT* parentFP = polygon->GetParentFootprint() )
    {
        polygon->Rotate( { 0, 0 }, parentFP->GetOrientation() );
        polygon->Move( parentFP->GetPosition() );
    }

    polygon->SetStroke( MapStrokeParams( aStroke ) );

    if( polygon->IsPolyShapeValid() )
        addItem( std::move( polygon ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddText( const VECTOR2D& aOrigin, const wxString& aText,
                                        double aHeight, double aWidth, double aThickness,
                                        double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                                        GR_TEXT_V_ALIGN_T aVJustify, const COLOR4D& aColor )
{
    std::unique_ptr<PCB_TEXT> textItem = std::make_unique<PCB_TEXT>( m_parent );
    textItem->SetLayer( GetLayer() );
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


void GRAPHICS_IMPORTER_PCBNEW::AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                                          const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                                          const IMPORTED_STROKE& aStroke )
{
    std::unique_ptr<PCB_SHAPE> spline = std::make_unique<PCB_SHAPE>( m_parent );

    spline->SetLayer( GetLayer() );
    spline->SetStroke( MapStrokeParams( aStroke ) );
    spline->SetStart( MapCoordinate( aStart ) );
    spline->SetBezierC1( MapCoordinate( aBezierControl1 ));
    spline->SetBezierC2( MapCoordinate( aBezierControl2 ));
    spline->SetEnd( MapCoordinate( aEnd ) );

    if( setupSplineOrLine( *spline, ARC_HIGH_DEF ) )
    {
        addItem( std::move( spline ) );
    }
}
