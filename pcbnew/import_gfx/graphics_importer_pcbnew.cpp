/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


GRAPHICS_IMPORTER_PCBNEW::GRAPHICS_IMPORTER_PCBNEW()
{
    m_layer = Dwgs_User;
    m_millimeterToIu = pcbIUScale.mmToIU( 1.0 );
}


VECTOR2I GRAPHICS_IMPORTER_PCBNEW::MapCoordinate( const VECTOR2D& aCoordinate )
{
    VECTOR2D coord = ( aCoordinate + GetImportOffsetMM() ) * ImportScalingFactor();
    return VECTOR2I( KiROUND( coord.x ), KiROUND( coord.y ) );
}


int GRAPHICS_IMPORTER_PCBNEW::MapLineWidth( double aLineWidth )
{
    if( aLineWidth <= 0.0 )
        return int( GetLineWidthMM() * ImportScalingFactor() );

    // aLineWidth is in mm:
    return int( aLineWidth * ImportScalingFactor() );
}


void GRAPHICS_IMPORTER_PCBNEW::AddLine( const VECTOR2D& aOrigin, const VECTOR2D& aEnd,
                                        double aWidth )
{
    std::unique_ptr<PCB_SHAPE> line( createDrawing() );
    line->SetShape( SHAPE_T::SEGMENT );
    line->SetLayer( GetLayer() );
    line->SetStroke( STROKE_PARAMS( MapLineWidth( aWidth ), PLOT_DASH_TYPE::SOLID ) );
    line->SetStart( MapCoordinate( aOrigin ) );
    line->SetEnd( MapCoordinate( aEnd ) );

    // Skip 0 len lines:
    if( line->GetStart() == line->GetEnd() )
        return;

    addItem( std::move( line ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddCircle( const VECTOR2D& aCenter, double aRadius, double aWidth,
                                          bool aFilled )
{
    std::unique_ptr<PCB_SHAPE> circle( createDrawing() );
    circle->SetShape( SHAPE_T::CIRCLE );
    circle->SetFilled( aFilled );
    circle->SetLayer( GetLayer() );
    circle->SetStroke( STROKE_PARAMS( MapLineWidth( aWidth ), PLOT_DASH_TYPE::SOLID ) );
    circle->SetStart( MapCoordinate( aCenter ));
    circle->SetEnd( MapCoordinate( VECTOR2D( aCenter.x + aRadius, aCenter.y ) ) );

    addItem( std::move( circle ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart,
                                       const EDA_ANGLE& aAngle, double aWidth )
{
    std::unique_ptr<PCB_SHAPE> arc( createDrawing() );
    arc->SetShape( SHAPE_T::ARC );
    arc->SetLayer( GetLayer() );

    /**
     * We need to perform the rotation/conversion here while still using floating point values
     * to avoid rounding errors when operating in integer space in pcbnew
     */
    VECTOR2D end = aStart;
    VECTOR2D mid = aStart;

    RotatePoint( end, aCenter, -aAngle );
    RotatePoint( mid, aCenter, -aAngle / 2.0 );

    arc->SetArcGeometry( MapCoordinate( aStart ), MapCoordinate( mid ), MapCoordinate( end ) );

    arc->SetStroke( STROKE_PARAMS( MapLineWidth( aWidth ), PLOT_DASH_TYPE::SOLID ) );

    addItem( std::move( arc ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddPolygon( const std::vector<VECTOR2D>& aVertices, double aWidth )
{
    std::vector<VECTOR2I> convertedPoints;
    convertedPoints.reserve( aVertices.size() );

    for( const VECTOR2D& precisePoint : aVertices )
        convertedPoints.emplace_back( MapCoordinate( precisePoint ) );

    std::unique_ptr<PCB_SHAPE> polygon( createDrawing() );
    polygon->SetShape( SHAPE_T::POLY );
    polygon->SetFilled( GetLayer() != Edge_Cuts );
    polygon->SetLayer( GetLayer() );
    polygon->SetPolyPoints( convertedPoints );

    if( FOOTPRINT* parentFP = polygon->GetParentFootprint() )
    {
        polygon->Rotate( { 0, 0 }, parentFP->GetOrientation() );
        polygon->Move( parentFP->GetPosition() );
    }

    polygon->SetStroke( STROKE_PARAMS( MapLineWidth( aWidth ), PLOT_DASH_TYPE::SOLID ) );
    addItem( std::move( polygon ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddText( const VECTOR2D& aOrigin, const wxString& aText,
                                        double aHeight, double aWidth, double aThickness,
                                        double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                                        GR_TEXT_V_ALIGN_T aVJustify )
{
    std::unique_ptr<PCB_TEXT> textItem = createText();
    textItem->SetLayer( GetLayer() );
    textItem->SetTextThickness( MapLineWidth( aThickness ) );
    textItem->SetTextPos( MapCoordinate( aOrigin ) );
    textItem->SetTextAngle( EDA_ANGLE( aOrientation, DEGREES_T ) );
    textItem->SetTextWidth( aWidth * ImportScalingFactor() );
    textItem->SetTextHeight( aHeight * ImportScalingFactor() );
    textItem->SetVertJustify( aVJustify );
    textItem->SetHorizJustify( aHJustify );
    textItem->SetText( aText );

    addItem( std::move( textItem ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddSpline( const VECTOR2D& aStart, const VECTOR2D& BezierControl1,
                                          const VECTOR2D& BezierControl2, const VECTOR2D& aEnd,
                                          double aWidth )
{
    std::unique_ptr<PCB_SHAPE> spline( createDrawing() );
    spline->SetShape( SHAPE_T::BEZIER );
    spline->SetLayer( GetLayer() );
    spline->SetStroke( STROKE_PARAMS( MapLineWidth( aWidth ), PLOT_DASH_TYPE::SOLID ) );
    spline->SetStart( MapCoordinate( aStart ) );
    spline->SetBezierC1( MapCoordinate( BezierControl1 ));
    spline->SetBezierC2( MapCoordinate( BezierControl2 ));
    spline->SetEnd( MapCoordinate( aEnd ) );
    spline->RebuildBezierToSegmentsPointsList( aWidth );

    // If the spline is degenerated (i.e. a segment) add it as segment or discard it if
    // null (i.e. very small) length
    if( spline->GetBezierPoints().size() <= 2 )
    {
        spline->SetShape( SHAPE_T::SEGMENT );
        int dist = VECTOR2I(spline->GetStart()- spline->GetEnd()).EuclideanNorm();

        // segment smaller than MIN_SEG_LEN_ACCEPTABLE_NM nanometers are skipped.
        #define MIN_SEG_LEN_ACCEPTABLE_NM 20
        if( dist < MIN_SEG_LEN_ACCEPTABLE_NM )
            return;
    }

    addItem( std::move( spline ) );
}


std::unique_ptr<PCB_SHAPE> GRAPHICS_IMPORTER_BOARD::createDrawing()
{
    return std::make_unique<PCB_SHAPE>( m_board );
}


std::unique_ptr<PCB_TEXT> GRAPHICS_IMPORTER_BOARD::createText()
{
    return std::make_unique<PCB_TEXT>( m_board );
}


std::unique_ptr<PCB_SHAPE> GRAPHICS_IMPORTER_FOOTPRINT::createDrawing()
{
    return std::make_unique<PCB_SHAPE>( m_footprint );
}


std::unique_ptr<PCB_TEXT> GRAPHICS_IMPORTER_FOOTPRINT::createText()
{
    return std::make_unique<PCB_TEXT>( m_footprint, PCB_TEXT::TEXT_is_DIVERS );
}
