/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2019 CERN
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

#include <ws_proxy_view_item.h>
#include <ws_draw_item.h>
#include <ws_data_item.h>
#include <gal/graphics_abstraction_layer.h>
#include <painter.h>
#include <layers_id_colors_and_visibility.h>
#include <page_info.h>
#include <view/view.h>
#include <ws_painter.h>

using namespace KIGFX;

WS_PROXY_VIEW_ITEM::WS_PROXY_VIEW_ITEM( int aMils2IUscalefactor, const PAGE_INFO* aPageInfo,
                                        const TITLE_BLOCK* aTitleBlock ) :
        EDA_ITEM( NOT_USED ), // this item is never added to a BOARD so it needs no type
        m_mils2IUscalefactor( aMils2IUscalefactor ),
        m_titleBlock( aTitleBlock ),
        m_pageInfo( aPageInfo ),
        m_sheetNumber( 1 ),
        m_sheetCount( 1 )
{
}


void WS_PROXY_VIEW_ITEM::SetPageInfo( const PAGE_INFO* aPageInfo )
{
    m_pageInfo = aPageInfo;
}


void WS_PROXY_VIEW_ITEM::SetTitleBlock( const TITLE_BLOCK* aTitleBlock )
{
    m_titleBlock = aTitleBlock;
}


const BOX2I WS_PROXY_VIEW_ITEM::ViewBBox() const
{
    BOX2I bbox;

    if( m_pageInfo != NULL )
    {
        bbox.SetOrigin( VECTOR2I( 0, 0 ) );
        bbox.SetEnd( VECTOR2I( m_pageInfo->GetWidthMils() * m_mils2IUscalefactor,
                               m_pageInfo->GetHeightMils() * m_mils2IUscalefactor ) );
    }
    else
    {
        bbox.SetMaximum();
    }

    return bbox;
}


void WS_PROXY_VIEW_ITEM::ViewDraw( int aLayer, VIEW* aView ) const
{
    auto gal = aView->GetGAL();
    auto settings = aView->GetPainter()->GetSettings();
    wxString fileName( m_fileName.c_str(), wxConvUTF8 );
    wxString sheetName( m_sheetName.c_str(), wxConvUTF8 );
    WS_DRAW_ITEM_LIST drawList;

    drawList.SetDefaultPenSize( (int) settings->GetWorksheetLineWidth() );
    // Adjust the scaling factor for worksheet items:
    // worksheet items coordinates and sizes are stored in mils,
    // and must be scaled to the same units as the caller
    drawList.SetMilsToIUfactor( m_mils2IUscalefactor );
    drawList.SetSheetNumber( m_sheetNumber );
    drawList.SetSheetCount( m_sheetCount );
    drawList.SetFileName( fileName );
    drawList.SetSheetName( sheetName );

    drawList.BuildWorkSheetGraphicList( *m_pageInfo, *m_titleBlock );

    // Draw the title block normally even if the view is flipped
    bool flipped = gal->IsFlippedX();

    if( flipped )
    {
        gal->Save();
        gal->Translate( VECTOR2D( m_pageInfo->GetWidthMils() * m_mils2IUscalefactor, 0 ) );
        gal->Scale( VECTOR2D( -1.0, 1.0 ) );
    }

    WS_PAINTER   ws_painter( gal );
    WS_RENDER_SETTINGS* ws_settings =static_cast<WS_RENDER_SETTINGS*>( ws_painter.GetSettings() );

    ws_settings->SetNormalColor( settings->GetLayerColor( LAYER_WORKSHEET ) );
    ws_settings->SetSelectedColor( settings->GetLayerColor( LAYER_SELECT_OVERLAY ) );
    ws_settings->SetBrightenedColor( settings->GetLayerColor( LAYER_BRIGHTENED ) );

    // Draw all the components that make the page layout
    for( WS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item; item = drawList.GetNext() )
        ws_painter.Draw( item, LAYER_WORKSHEET );

    // Draw gray line that outlines the sheet size
    if( settings->GetShowPageLimits() )
        ws_painter.DrawBorder( m_pageInfo, m_mils2IUscalefactor );

    if( flipped )
        gal->Restore();
}


void WS_PROXY_VIEW_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = LAYER_WORKSHEET;
}


