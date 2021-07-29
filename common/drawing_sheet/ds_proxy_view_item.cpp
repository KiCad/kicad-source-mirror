/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2020 CERN
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

#include <layer_ids.h>
#include <page_info.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_painter.h>
#include <project.h>
#include <view/view.h>

using namespace KIGFX;

DS_PROXY_VIEW_ITEM::DS_PROXY_VIEW_ITEM( int aMils2IUscalefactor, const PAGE_INFO* aPageInfo,
                                        const PROJECT* aProject, const TITLE_BLOCK* aTitleBlock ) :
        EDA_ITEM( NOT_USED ), // this item is never added to a BOARD so it needs no type
        m_mils2IUscalefactor( aMils2IUscalefactor ),
        m_titleBlock( aTitleBlock ),
        m_pageInfo( aPageInfo ),
        m_pageNumber( "1" ),
        m_sheetCount( 1 ),
        m_isFirstPage( false ),
        m_project( aProject ),
        m_colorLayer( LAYER_DRAWINGSHEET ),
        m_pageBorderColorLayer( LAYER_GRID )
{
}


const BOX2I DS_PROXY_VIEW_ITEM::ViewBBox() const
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


void DS_PROXY_VIEW_ITEM::buildDrawList( VIEW* aView, DS_DRAW_ITEM_LIST* aDrawList ) const
{
    RENDER_SETTINGS* settings = aView->GetPainter()->GetSettings();
    wxString         fileName( m_fileName.c_str(), wxConvUTF8 );
    wxString         sheetName( m_sheetName.c_str(), wxConvUTF8 );

    aDrawList->SetDefaultPenSize( (int) settings->GetDrawingSheetLineWidth() );
    // Adjust the scaling factor: drawing sheet item coordinates and sizes are stored in mils,
    // and must be scaled to the same units as the caller
    aDrawList->SetMilsToIUfactor( m_mils2IUscalefactor );
    aDrawList->SetIsFirstPage( m_isFirstPage );
    aDrawList->SetPageNumber( m_pageNumber );
    aDrawList->SetSheetCount( m_sheetCount );
    aDrawList->SetFileName( fileName );
    aDrawList->SetSheetName( sheetName );
    aDrawList->SetProject( m_project );

    aDrawList->BuildDrawItemsList( *m_pageInfo, *m_titleBlock );
}


void DS_PROXY_VIEW_ITEM::ViewDraw( int aLayer, VIEW* aView ) const
{
    GAL*              gal = aView->GetGAL();
    RENDER_SETTINGS*  settings = aView->GetPainter()->GetSettings();
    DS_DRAW_ITEM_LIST drawList;

    buildDrawList( aView, &drawList );

    // Draw the title block normally even if the view is flipped
    bool flipped = gal->IsFlippedX();

    if( flipped )
    {
        gal->Save();
        gal->Translate( VECTOR2D( m_pageInfo->GetWidthMils() * m_mils2IUscalefactor, 0 ) );
        gal->Scale( VECTOR2D( -1.0, 1.0 ) );
    }

    DS_PAINTER ws_painter( gal );
    auto       ws_settings = static_cast<DS_RENDER_SETTINGS*>( ws_painter.GetSettings() );

    ws_settings->SetNormalColor( settings->GetLayerColor( m_colorLayer ) );
    ws_settings->SetSelectedColor( settings->GetLayerColor( LAYER_SELECT_OVERLAY ) );
    ws_settings->SetBrightenedColor( settings->GetLayerColor( LAYER_BRIGHTENED ) );
    ws_settings->SetPageBorderColor( settings->GetLayerColor( m_pageBorderColorLayer ) );

    // Draw all the components that make the drawing sheet
    for( DS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item; item = drawList.GetNext() )
        ws_painter.Draw( item, LAYER_DRAWINGSHEET );

    // Draw gray line that outlines the sheet size
    if( settings->GetShowPageLimits() )
        ws_painter.DrawBorder( m_pageInfo, m_mils2IUscalefactor );

    if( flipped )
        gal->Restore();
}


void DS_PROXY_VIEW_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = LAYER_DRAWINGSHEET;
}


bool DS_PROXY_VIEW_ITEM::HitTestDrawingSheetItems( VIEW* aView, const wxPoint& aPosition )
{
    int               accuracy = (int) aView->ToWorld( 5.0 );   // five pixels at current zoom
    DS_DRAW_ITEM_LIST drawList;

    buildDrawList( aView, &drawList );

    for( DS_DRAW_ITEM_BASE* item = drawList.GetFirst(); item; item = drawList.GetNext() )
    {
        if( item->HitTest( aPosition, accuracy ) )
            return true;
    }

    return false;
}
