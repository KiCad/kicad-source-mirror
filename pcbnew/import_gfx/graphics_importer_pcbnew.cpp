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

#include <class_board.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
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
    unique_ptr<DRAWSEGMENT> line( createDrawing() );
    line->SetShape( S_SEGMENT );
    line->SetLayer( GetLayer() );
    line->SetWidth( MapLineWidth( aWidth ) );
    line->SetStart( MapCoordinate( aOrigin ) );
    line->SetEnd( MapCoordinate( aEnd ) );

    if( line->Type() == PCB_MODULE_EDGE_T )
        static_cast<EDGE_MODULE*>( line.get() )->SetLocalCoord();

    addItem( std::move( line ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddCircle( const VECTOR2D& aCenter, double aRadius, double aWidth )
{
    unique_ptr<DRAWSEGMENT> circle( createDrawing() );
    circle->SetShape( S_CIRCLE );
    circle->SetLayer( GetLayer() );
    circle->SetWidth( MapLineWidth( aWidth ) );
    circle->SetCenter( MapCoordinate( aCenter ) );
    circle->SetArcStart( MapCoordinate( VECTOR2D( aCenter.x + aRadius, aCenter.y ) ) );

    if( circle->Type() == PCB_MODULE_EDGE_T )
        static_cast<EDGE_MODULE*>( circle.get() )->SetLocalCoord();

    addItem( std::move( circle ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart,
                                       double aAngle, double aWidth )
{
    unique_ptr<DRAWSEGMENT> arc( createDrawing() );
    arc->SetShape( S_ARC );
    arc->SetLayer( GetLayer() );
    arc->SetWidth( MapLineWidth( aWidth ) );
    arc->SetCenter( MapCoordinate( aCenter) );
    arc->SetArcStart( MapCoordinate( aStart ) );
    arc->SetAngle( aAngle * 10.0 );     // Pcbnew uses the decidegree

    if( arc->Type() == PCB_MODULE_EDGE_T )
        static_cast<EDGE_MODULE*>( arc.get() )->SetLocalCoord();

    addItem( std::move( arc ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddPolygon( const std::vector< VECTOR2D >& aVertices, double aWidth )
{
    std::vector< wxPoint > convertedPoints;
    convertedPoints.reserve( convertedPoints.size() );

    for( const VECTOR2D& precisePoint : aVertices )
        convertedPoints.emplace_back( MapCoordinate( precisePoint ) );

    unique_ptr<DRAWSEGMENT> polygon( createDrawing() );
    polygon->SetShape( S_POLYGON );
    polygon->SetLayer( GetLayer() );
    polygon->SetPolyPoints( convertedPoints );

    if( polygon->Type() == PCB_MODULE_EDGE_T )
        static_cast<EDGE_MODULE*>( polygon.get() )->SetLocalCoord();

    polygon->SetWidth( MapLineWidth( aWidth ) );
    addItem( std::move( polygon ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddText( const VECTOR2D& aOrigin, const wxString& aText,
        double aHeight, double aWidth, double aThickness, double aOrientation,
        EDA_TEXT_HJUSTIFY_T aHJustify, EDA_TEXT_VJUSTIFY_T aVJustify )
{
    unique_ptr<BOARD_ITEM> boardItem;
    EDA_TEXT* textItem;
    tie( boardItem, textItem ) = createText();
    boardItem->SetLayer( GetLayer() );
    textItem->SetThickness( MapLineWidth( aThickness ) );
    textItem->SetTextPos( MapCoordinate( aOrigin ) );
    textItem->SetTextAngle( aOrientation * 10.0 );      // Pcbnew uses the decidegree
    textItem->SetTextWidth( aWidth * ImportScalingFactor() );
    textItem->SetTextHeight( aHeight * ImportScalingFactor() );
    textItem->SetVertJustify( aVJustify );
    textItem->SetHorizJustify( aHJustify );
    textItem->SetText( aText );

    if( boardItem->Type() == PCB_MODULE_TEXT_T )
        static_cast<TEXTE_MODULE*>( boardItem.get() )->SetLocalCoord();

    addItem( std::move( boardItem ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddSpline( const VECTOR2D& aStart, const VECTOR2D& BezierControl1,
                const VECTOR2D& BezierControl2, const VECTOR2D& aEnd, double aWidth )
{
    unique_ptr<DRAWSEGMENT> spline( createDrawing() );
    spline->SetShape( S_CURVE );
    spline->SetLayer( GetLayer() );
    spline->SetWidth( MapLineWidth( aWidth ) );
    spline->SetStart( MapCoordinate( aStart ) );
    spline->SetBezControl1( MapCoordinate( BezierControl1 ) );
    spline->SetBezControl2( MapCoordinate( BezierControl2 ) );
    spline->SetEnd( MapCoordinate( aEnd ) );
    spline->RebuildBezierToSegmentsPointsList( aWidth );

    if( spline->Type() == PCB_MODULE_EDGE_T )
        static_cast<EDGE_MODULE*>( spline.get() )->SetLocalCoord();

    addItem( std::move( spline ) );
}


unique_ptr<DRAWSEGMENT> GRAPHICS_IMPORTER_BOARD::createDrawing()
{
    return std::make_unique<DRAWSEGMENT>( m_board );
}


std::pair<unique_ptr<BOARD_ITEM>, EDA_TEXT*> GRAPHICS_IMPORTER_BOARD::createText()
{
    TEXTE_PCB* text = new TEXTE_PCB( m_board );
    return make_pair( unique_ptr<BOARD_ITEM>( text ), static_cast<EDA_TEXT*>( text ) );
}


unique_ptr<DRAWSEGMENT> GRAPHICS_IMPORTER_MODULE::createDrawing()
{
    return unique_ptr<DRAWSEGMENT>( new EDGE_MODULE( m_module ) );
}


std::pair<unique_ptr<BOARD_ITEM>, EDA_TEXT*> GRAPHICS_IMPORTER_MODULE::createText()
{
    TEXTE_MODULE* text = new TEXTE_MODULE( m_module );
    return make_pair( unique_ptr<BOARD_ITEM>( text ), static_cast<EDA_TEXT*>( text ) );
}
