/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fp_shape.h>
#include <pcb_text.h>
#include <fp_text.h>
#include <memory>
#include <tuple>

#include "convert_to_biu.h"


GRAPHICS_IMPORTER_PCBNEW::GRAPHICS_IMPORTER_PCBNEW()
{
    m_layer = Dwgs_User;
    m_millimeterToIu = Millimeter2iu( 1.0 );
}


wxPoint GRAPHICS_IMPORTER_PCBNEW::MapCoordinate( const VECTOR2D& aCoordinate )
{
    VECTOR2D coord = ( aCoordinate + GetImportOffsetMM() ) * ImportScalingFactor();
    return wxPoint( (int) coord.x, (int) coord.y );
}


int GRAPHICS_IMPORTER_PCBNEW::MapLineWidth( double aLineWidth )
{
    if( aLineWidth <= 0.0 )
        return int( GetLineWidthMM() * ImportScalingFactor() );

    // aLineWidth is in mm:
    return int( aLineWidth * ImportScalingFactor() );
}


void GRAPHICS_IMPORTER_PCBNEW::AddLine( const VECTOR2D& aOrigin, const VECTOR2D& aEnd, double aWidth )
{
    std::unique_ptr<PCB_SHAPE> line( createDrawing() );
    line->SetShape( PCB_SHAPE_TYPE::SEGMENT );
    line->SetLayer( GetLayer() );
    line->SetWidth( MapLineWidth( aWidth ) );
    line->SetStart( MapCoordinate( aOrigin ) );
    line->SetEnd( MapCoordinate( aEnd ) );

    if( line->Type() == PCB_FP_SHAPE_T )
        static_cast<FP_SHAPE*>( line.get() )->SetLocalCoord();

    addItem( std::move( line ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddCircle( const VECTOR2D& aCenter, double aRadius, double aWidth, bool aFilled )
{
    std::unique_ptr<PCB_SHAPE> circle( createDrawing() );
    circle->SetShape( PCB_SHAPE_TYPE::CIRCLE );
    circle->SetFilled( aFilled );
    circle->SetLayer( GetLayer() );
    circle->SetWidth( MapLineWidth( aWidth ) );
    circle->SetCenter( MapCoordinate( aCenter ) );
    circle->SetArcStart( MapCoordinate( VECTOR2D( aCenter.x + aRadius, aCenter.y ) ) );

    if( circle->Type() == PCB_FP_SHAPE_T )
        static_cast<FP_SHAPE*>( circle.get() )->SetLocalCoord();

    addItem( std::move( circle ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart,
                                       double aAngle, double aWidth )
{
    std::unique_ptr<PCB_SHAPE> arc( createDrawing() );
    arc->SetShape( PCB_SHAPE_TYPE::ARC );
    arc->SetLayer( GetLayer() );
    arc->SetWidth( MapLineWidth( aWidth ) );
    arc->SetCenter( MapCoordinate( aCenter) );
    arc->SetArcStart( MapCoordinate( aStart ) );
    arc->SetAngle( aAngle * 10.0 );     // Pcbnew uses the decidegree

    if( arc->Type() == PCB_FP_SHAPE_T )
        static_cast<FP_SHAPE*>( arc.get() )->SetLocalCoord();

    addItem( std::move( arc ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddPolygon( const std::vector< VECTOR2D >& aVertices, double aWidth )
{
    std::vector< wxPoint > convertedPoints;
    convertedPoints.reserve( convertedPoints.size() );

    for( const VECTOR2D& precisePoint : aVertices )
        convertedPoints.emplace_back( MapCoordinate( precisePoint ) );

    std::unique_ptr<PCB_SHAPE> polygon( createDrawing() );
    polygon->SetShape( PCB_SHAPE_TYPE::POLYGON );
    polygon->SetFilled( GetLayer() != Edge_Cuts );
    polygon->SetLayer( GetLayer() );
    polygon->SetPolyPoints( convertedPoints );

    if( polygon->Type() == PCB_FP_SHAPE_T )
        static_cast<FP_SHAPE*>( polygon.get() )->SetLocalCoord();

    polygon->SetWidth( MapLineWidth( aWidth ) );
    addItem( std::move( polygon ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddText( const VECTOR2D& aOrigin, const wxString& aText,
        double aHeight, double aWidth, double aThickness, double aOrientation,
        EDA_TEXT_HJUSTIFY_T aHJustify, EDA_TEXT_VJUSTIFY_T aVJustify )
{
    std::unique_ptr<BOARD_ITEM> boardItem;
    EDA_TEXT* textItem;
    tie( boardItem, textItem ) = createText();
    boardItem->SetLayer( GetLayer() );
    textItem->SetTextThickness( MapLineWidth( aThickness ) );
    textItem->SetTextPos( MapCoordinate( aOrigin ) );
    textItem->SetTextAngle( aOrientation * 10.0 );      // Pcbnew uses the decidegree
    textItem->SetTextWidth( aWidth * ImportScalingFactor() );
    textItem->SetTextHeight( aHeight * ImportScalingFactor() );
    textItem->SetVertJustify( aVJustify );
    textItem->SetHorizJustify( aHJustify );
    textItem->SetText( aText );

    if( boardItem->Type() == PCB_FP_TEXT_T )
        static_cast<FP_TEXT*>( boardItem.get() )->SetLocalCoord();

    addItem( std::move( boardItem ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddSpline( const VECTOR2D& aStart, const VECTOR2D& BezierControl1,
                const VECTOR2D& BezierControl2, const VECTOR2D& aEnd, double aWidth )
{
    std::unique_ptr<PCB_SHAPE> spline( createDrawing() );
    spline->SetShape( PCB_SHAPE_TYPE::CURVE );
    spline->SetLayer( GetLayer() );
    spline->SetWidth( MapLineWidth( aWidth ) );
    spline->SetStart( MapCoordinate( aStart ) );
    spline->SetBezControl1( MapCoordinate( BezierControl1 ) );
    spline->SetBezControl2( MapCoordinate( BezierControl2 ) );
    spline->SetEnd( MapCoordinate( aEnd ) );
    spline->RebuildBezierToSegmentsPointsList( aWidth );

    if( spline->Type() == PCB_FP_SHAPE_T )
        static_cast<FP_SHAPE*>( spline.get() )->SetLocalCoord();

    addItem( std::move( spline ) );
}


std::unique_ptr<PCB_SHAPE> GRAPHICS_IMPORTER_BOARD::createDrawing()
{
    return std::make_unique<PCB_SHAPE>( m_board );
}


std::pair<std::unique_ptr<BOARD_ITEM>, EDA_TEXT*> GRAPHICS_IMPORTER_BOARD::createText()
{
    PCB_TEXT* text = new PCB_TEXT( m_board );
    return make_pair( std::unique_ptr<BOARD_ITEM>( text ), static_cast<EDA_TEXT*>( text ) );
}


std::unique_ptr<PCB_SHAPE> GRAPHICS_IMPORTER_FOOTPRINT::createDrawing()
{
    return std::make_unique<FP_SHAPE>( m_footprint );
}


std::pair<std::unique_ptr<BOARD_ITEM>, EDA_TEXT*> GRAPHICS_IMPORTER_FOOTPRINT::createText()
{
    FP_TEXT* text = new FP_TEXT( m_footprint );
    return make_pair( std::unique_ptr<BOARD_ITEM>( text ), static_cast<EDA_TEXT*>( text ) );
}
