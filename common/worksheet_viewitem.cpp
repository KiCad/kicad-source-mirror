/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

/**
 * @file worksheet_viewitem.cpp
 * @brief Class that handles properties and drawing of worksheet layout.
 */

#include <worksheet_viewitem.h>
#include <worksheet_shape_builder.h>
#include <gal/graphics_abstraction_layer.h>
#include <painter.h>
#include <layers_id_colors_and_visibility.h>
#include <boost/foreach.hpp>
#include <class_page_info.h>

using namespace KIGFX;

WORKSHEET_VIEWITEM::WORKSHEET_VIEWITEM( const PAGE_INFO* aPageInfo, const TITLE_BLOCK* aTitleBlock ) :
    EDA_ITEM( NOT_USED ), // this item is never added to a BOARD so it needs no type
    m_titleBlock( aTitleBlock ), m_pageInfo( aPageInfo ), m_sheetNumber( 1 ), m_sheetCount( 1 ) {}


void WORKSHEET_VIEWITEM::SetPageInfo( const PAGE_INFO* aPageInfo )
{
    m_pageInfo = aPageInfo;
    ViewUpdate( GEOMETRY );
}


void WORKSHEET_VIEWITEM::SetTitleBlock( const TITLE_BLOCK* aTitleBlock )
{
    m_titleBlock = aTitleBlock;
    ViewUpdate( GEOMETRY );
}


const BOX2I WORKSHEET_VIEWITEM::ViewBBox() const
{
    BOX2I bbox;

    if( m_pageInfo != NULL )
    {
        bbox.SetOrigin( VECTOR2I( 0, 0 ) );
        bbox.SetEnd( VECTOR2I( m_pageInfo->GetWidthMils() * 25400,
                               m_pageInfo->GetHeightMils() * 25400 ) );
    }
    else
    {
        bbox.SetMaximum();
    }

    return bbox;
}


void WORKSHEET_VIEWITEM::ViewDraw( int aLayer, GAL* aGal ) const
{
    RENDER_SETTINGS* settings = m_view->GetPainter()->GetSettings();
    wxString fileName( m_fileName.c_str(), wxConvUTF8 );
    wxString sheetName( m_sheetName.c_str(), wxConvUTF8 );
    WS_DRAW_ITEM_LIST drawList;

    drawList.SetPenSize( settings->GetWorksheetLineWidth() );
    // Sorry, but I don't get this multi #ifdef from include/convert_to_biu.h, so here goes a magic
    // number. IU_PER_MILS should be 25400 (as in a different compilation unit), but somehow
    // it equals 1 in this case..
    drawList.SetMilsToIUfactor( 25400 /* IU_PER_MILS */ );
    drawList.SetSheetNumber( m_sheetNumber );
    drawList.SetSheetCount( m_sheetCount );
    drawList.SetFileName( fileName );
    drawList.SetSheetName( sheetName );

    COLOR4D color = settings->GetColor( this, aLayer );
    EDA_COLOR_T edaColor = ColorFindNearest( color.r * 255, color.g * 255, color.b * 255 );
    drawList.BuildWorkSheetGraphicList( *m_pageInfo, *m_titleBlock, edaColor, edaColor );

    // Draw all the components that make the page layout
    WS_DRAW_ITEM_BASE* item = drawList.GetFirst();
    while( item )
    {
        switch( item->GetType() )
        {
        case WS_DRAW_ITEM_BASE::wsg_line:
            draw( static_cast<const WS_DRAW_ITEM_LINE*>( item ), aGal );
            break;

        case WS_DRAW_ITEM_BASE::wsg_rect:
            draw( static_cast<const WS_DRAW_ITEM_RECT*>( item ), aGal );
            break;

        case WS_DRAW_ITEM_BASE::wsg_poly:
            draw( static_cast<const WS_DRAW_ITEM_POLYGON*>( item ), aGal );
            break;

        case WS_DRAW_ITEM_BASE::wsg_text:
            draw( static_cast<const WS_DRAW_ITEM_TEXT*>( item ), aGal );
            break;

        case WS_DRAW_ITEM_BASE::wsg_bitmap:
            break;
        }

        item = drawList.GetNext();
    }

    // Draw gray line that outlines the sheet size
    drawBorder( aGal );
}


void WORKSHEET_VIEWITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = ITEM_GAL_LAYER( WORKSHEET );
}


void WORKSHEET_VIEWITEM::draw( const WS_DRAW_ITEM_LINE* aItem, GAL* aGal ) const
{
    aGal->SetIsStroke( true );
    aGal->SetIsFill( false );
    aGal->SetStrokeColor( COLOR4D( aItem->GetColor() ) );
    aGal->SetLineWidth( aItem->GetPenWidth() );
    aGal->DrawLine( VECTOR2D( aItem->GetStart() ), VECTOR2D( aItem->GetEnd() ) );
}


void WORKSHEET_VIEWITEM::draw( const WS_DRAW_ITEM_RECT* aItem, GAL* aGal ) const
{
    aGal->SetIsStroke( true );
    aGal->SetIsFill( false );
    aGal->SetStrokeColor( COLOR4D( aItem->GetColor() ) );
    aGal->SetLineWidth( aItem->GetPenWidth() );
    aGal->DrawRectangle( VECTOR2D( aItem->GetStart() ), VECTOR2D( aItem->GetEnd() ) );
}


void WORKSHEET_VIEWITEM::draw( const WS_DRAW_ITEM_POLYGON* aItem, GAL* aGal ) const
{
    std::deque<VECTOR2D> corners;
    BOOST_FOREACH( wxPoint point, aItem->m_Corners )
    {
        corners.push_back( VECTOR2D( point ) );
    }

    if( aItem->IsFilled() )
    {
        aGal->SetFillColor( COLOR4D( aItem->GetColor() ) );
        aGal->SetIsFill( true );
        aGal->SetIsStroke( false );
        aGal->DrawPolygon( corners );
    }
    else
    {
        aGal->SetStrokeColor( COLOR4D( aItem->GetColor() ) );
        aGal->SetIsFill( false );
        aGal->SetIsStroke( true );
        aGal->SetLineWidth( aItem->GetPenWidth() );
        aGal->DrawPolyline( corners );
    }
}


void WORKSHEET_VIEWITEM::draw( const WS_DRAW_ITEM_TEXT* aItem, GAL* aGal ) const
{
    VECTOR2D position( aItem->GetTextPosition().x, aItem->GetTextPosition().y );

    aGal->Save();
    aGal->Translate( position );
    aGal->Rotate( -aItem->GetOrientation() * M_PI / 1800.0 );
    aGal->SetStrokeColor( COLOR4D( aItem->GetColor() ) );
    aGal->SetLineWidth( aItem->GetThickness() );
    aGal->SetTextAttributes( aItem );
    aGal->StrokeText( aItem->GetShownText(), VECTOR2D( 0, 0 ), 0.0 );
    aGal->Restore();
}


void WORKSHEET_VIEWITEM::drawBorder( GAL* aGal ) const
{
    VECTOR2D origin = VECTOR2D( 0.0, 0.0 );
    VECTOR2D end = VECTOR2D( m_pageInfo->GetWidthMils() * 25400,
                             m_pageInfo->GetHeightMils() * 25400 );

    aGal->SetIsStroke( true );
    aGal->SetIsFill( false );
    aGal->DrawRectangle( origin, end );
}
