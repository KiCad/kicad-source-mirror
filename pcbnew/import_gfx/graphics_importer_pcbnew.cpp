/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "graphics_importer_pcbnew.h"

#include <class_board.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include <tuple>

using namespace std;

static std::vector<wxPoint> convertPoints( const std::vector<VECTOR2D>& aPoints,
        double aScaleFactor );


static wxPoint Round( const VECTOR2D& aVec )
{
    return wxPoint( (int) aVec.x, (int) aVec.y );
}


void GRAPHICS_IMPORTER_PCBNEW::AddLine( const VECTOR2D& aOrigin, const VECTOR2D& aEnd )
{
    unique_ptr<DRAWSEGMENT> line( createDrawing() );
    line->SetShape( S_SEGMENT );
    line->SetLayer( GetLayer() );
    line->SetWidth( GetLineWidth() );
    line->SetStart( Round ( aOrigin * GetScale() ) );
    line->SetEnd( Round ( aEnd * GetScale() ) );
    addItem( std::move( line ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddCircle( const VECTOR2D& aCenter, double aRadius )
{
    unique_ptr<DRAWSEGMENT> circle( createDrawing() );
    circle->SetShape( S_CIRCLE );
    circle->SetLayer( GetLayer() );
    circle->SetWidth( GetLineWidth() );
    circle->SetCenter( Round ( aCenter * GetScale() ) );
    circle->SetArcStart( Round ( VECTOR2D( aCenter.x + aRadius, aCenter.y ) * GetScale() ) );
    addItem( std::move( circle ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, double aAngle )
{
    unique_ptr<DRAWSEGMENT> arc( createDrawing() );
    arc->SetShape( S_ARC );
    arc->SetLayer( GetLayer() );
    arc->SetWidth( GetLineWidth() );
    arc->SetCenter( Round ( aCenter * GetScale() ) );
    arc->SetArcStart( Round ( aStart * GetScale() ) );
    arc->SetAngle( aAngle );
    addItem( std::move( arc ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddPolygon( const std::vector< VECTOR2D >& aVertices )
{
    std::vector< wxPoint > convertedVertices = convertPoints( aVertices, GetScale() );
    unique_ptr<DRAWSEGMENT> polygon( createDrawing() );
    polygon->SetShape( S_POLYGON );
    polygon->SetLayer( GetLayer() );
    polygon->SetPolyPoints( convertedVertices );
    addItem( std::move( polygon ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddText( const VECTOR2D& aOrigin, const wxString& aText,
        double aHeight, double aWidth, double aOrientation,
        EDA_TEXT_HJUSTIFY_T aHJustify, EDA_TEXT_VJUSTIFY_T aVJustify )
{
    unique_ptr<BOARD_ITEM> boardItem;
    EDA_TEXT* textItem;
    tie( boardItem, textItem ) = createText();
    boardItem->SetLayer( GetLayer() );
    textItem->SetThickness( GetLineWidth() );
    textItem->SetTextPos( Round( aOrigin * GetScale() ) );
    textItem->SetTextAngle( aOrientation );
    textItem->SetTextWidth( aWidth * GetScale() );
    textItem->SetTextHeight( aHeight * GetScale() );
    textItem->SetVertJustify( aVJustify );
    textItem->SetHorizJustify( aHJustify );
    textItem->SetText( aText );
    addItem( std::move( boardItem ) );
}


void GRAPHICS_IMPORTER_PCBNEW::AddSpline( const VECTOR2D& aStart, const VECTOR2D& BezierControl1,
                const VECTOR2D& BezierControl2, const VECTOR2D& aEnd, double aWidth )
{
    unique_ptr<DRAWSEGMENT> spline( createDrawing() );
    aWidth = GetLineWidth();    // To do: use dxf line thickness if defined
    spline->SetShape( S_CURVE );
    spline->SetLayer( GetLayer() );
    spline->SetWidth( aWidth );
    spline->SetStart( Round( aStart * GetScale() ) );
    spline->SetBezControl1( Round( BezierControl1 * GetScale() ) );
    spline->SetBezControl2( Round( BezierControl2 * GetScale() ) );
    spline->SetEnd( Round( aEnd * GetScale() ) );
    spline->RebuildBezierToSegmentsPointsList( aWidth );
    addItem( std::move( spline ) );
}


unique_ptr<DRAWSEGMENT> GRAPHICS_IMPORTER_BOARD::createDrawing()
{
    return unique_ptr<DRAWSEGMENT>( new DRAWSEGMENT( m_board ) );
}


pair<unique_ptr<BOARD_ITEM>, EDA_TEXT*> GRAPHICS_IMPORTER_BOARD::createText()
{
    TEXTE_PCB* text = new TEXTE_PCB( m_board );
    return make_pair( unique_ptr<BOARD_ITEM>( text ), static_cast<EDA_TEXT*>( text ) );
}


unique_ptr<DRAWSEGMENT> GRAPHICS_IMPORTER_MODULE::createDrawing()
{
    return unique_ptr<DRAWSEGMENT>( new EDGE_MODULE( m_module ) );
}


pair<unique_ptr<BOARD_ITEM>, EDA_TEXT*> GRAPHICS_IMPORTER_MODULE::createText()
{
    TEXTE_MODULE* text = new TEXTE_MODULE( m_module );
    return make_pair( unique_ptr<BOARD_ITEM>( text ), static_cast<EDA_TEXT*>( text ) );
}


static std::vector< wxPoint > convertPoints( const std::vector<VECTOR2D>& aPoints,
        double aScaleFactor )
{
    std::vector<wxPoint> convertedPoints;
    convertedPoints.reserve( aPoints.size() );

    for( const VECTOR2D& precisePoint : aPoints )
    {
        auto scaledX = precisePoint.x * aScaleFactor;
        auto scaledY = precisePoint.y * aScaleFactor;

        convertedPoints.emplace_back( scaledX, scaledY );
    }

    return convertedPoints;
}
