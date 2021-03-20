/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
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


#include <common.h>
#include <pgm_base.h>
#include <base_screen.h>
#include <eda_draw_frame.h>
#include <title_block.h>
#include <build_version.h>
#include <settings/color_settings.h>
#include <drawing_sheet/ds_draw_item.h>
#include <gal/graphics_abstraction_layer.h>

#include <drawing_sheet/ds_painter.h>
#include <drawing_sheet/ds_data_item.h>

#include <wx/app.h>

using namespace KIGFX;

static const wxString productName = wxT( "KiCad E.D.A.  " );

DS_RENDER_SETTINGS::DS_RENDER_SETTINGS()
{
    m_backgroundColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
    m_normalColor =     RED;
    m_selectedColor =   m_normalColor.Brightened( 0.5 );
    m_brightenedColor = COLOR4D( 0.0, 1.0, 0.0, 0.9 );
    m_pageBorderColor = COLOR4D( 0.4, 0.4, 0.4, 1.0 );

    update();
}


void DS_RENDER_SETTINGS::LoadColors( const COLOR_SETTINGS* aSettings )
{
    for( int layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = aSettings->GetColor( layer );

    for( int layer = GAL_LAYER_ID_START; layer < GAL_LAYER_ID_END; layer ++)
        m_layerColors[ layer ] = aSettings->GetColor( layer );

    m_backgroundColor = aSettings->GetColor( LAYER_SCHEMATIC_BACKGROUND );
    m_pageBorderColor = aSettings->GetColor( LAYER_SCHEMATIC_GRID );
    m_normalColor = aSettings->GetColor( LAYER_SCHEMATIC_DRAWINGSHEET );
}


COLOR4D DS_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    const EDA_ITEM* item = dynamic_cast<const EDA_ITEM*>( aItem );

    if( item )
    {
        // Selection disambiguation
        if( item->IsBrightened() )
            return m_brightenedColor;

        if( item->IsSelected() )
            return m_selectedColor;
    }

    return m_normalColor;
}


void DS_DRAW_ITEM_LIST::GetTextVars( wxArrayString* aVars )
{
    aVars->push_back( wxT( "KICAD_VERSION" ) );
    aVars->push_back( wxT( "#" ) );
    aVars->push_back( wxT( "##" ) );
    aVars->push_back( wxT( "SHEETNAME" ) );
    aVars->push_back( wxT( "FILENAME" ) );
    aVars->push_back( wxT( "PAPER" ) );
    aVars->push_back( wxT( "LAYER" ) );
    TITLE_BLOCK::GetContextualTextVars( aVars );
}


// returns the full text corresponding to the aTextbase,
// after replacing format symbols by the corresponding value
wxString DS_DRAW_ITEM_LIST::BuildFullText( const wxString& aTextbase )
{
    std::function<bool( wxString* )> wsResolver =
            [ this ]( wxString* token ) -> bool
            {
                bool tokenUpdated = false;

                if( token->IsSameAs( wxT( "KICAD_VERSION" ) ) && PgmOrNull() )
                {
                    // TODO: it'd be nice to get the Python script name/version here for when
                    // PgmOrNull() is null...

                    *token = wxString::Format( wxT( "%s%s %s" ),
                                               productName,
                                               Pgm().App().GetAppName(),
                                               GetBuildVersion() );
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "#" ) ) )
                {
                    *token = wxString::Format( wxT( "%s" ), m_pageNumber );
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "##" ) ) )
                {
                    *token = wxString::Format( wxT( "%d" ), m_sheetCount );
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "SHEETNAME" ) ) )
                {
                    *token = m_sheetFullName;
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "FILENAME" ) ) )
                {
                    wxFileName fn( m_fileName );
                    *token = fn.GetFullName();
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "PAPER" ) ) )
                {
                    *token = m_paperFormat ? *m_paperFormat : wxString( "" );
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "LAYER" ) ) )
                {
                    *token = m_sheetLayer ? *m_sheetLayer : wxString( "" );
                    tokenUpdated = true;
                }
                else if( m_titleBlock )
                {
                    m_titleBlock->TextVarResolver( token, m_project );
                    // no need for tokenUpdated; TextVarResolver() did a full resolve
                    return true;
                }

                if( tokenUpdated )
                {
                   *token = ExpandTextVars( *token, m_project );
                   return true;
                }

                return false;
            };

    return ExpandTextVars( aTextbase, &wsResolver, nullptr, m_project );
}


bool KIGFX::DS_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    auto item = dynamic_cast<const EDA_ITEM*>( aItem );

    if( !item )
        return false;

    switch( item->Type() )
    {
    case WSG_LINE_T:   draw( (DS_DRAW_ITEM_LINE*) item, aLayer );         break;
    case WSG_POLY_T:   draw( (DS_DRAW_ITEM_POLYPOLYGONS*) item, aLayer ); break;
    case WSG_RECT_T:   draw( (DS_DRAW_ITEM_RECT*) item, aLayer );         break;
    case WSG_TEXT_T:   draw( (DS_DRAW_ITEM_TEXT*) item, aLayer );         break;
    case WSG_BITMAP_T: draw( (DS_DRAW_ITEM_BITMAP*) item, aLayer );       break;
    case WSG_PAGE_T:   draw( (DS_DRAW_ITEM_PAGE*) item, aLayer );         break;
    default:           return false;
    }

    return true;
}


void KIGFX::DS_PAINTER::draw( const DS_DRAW_ITEM_LINE* aItem, int aLayer ) const
{
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetStrokeColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetLineWidth( std::max( aItem->GetPenWidth(), m_renderSettings.GetDefaultPenWidth() ) );
    m_gal->DrawLine( VECTOR2D( aItem->GetStart() ), VECTOR2D( aItem->GetEnd() ) );
}


void KIGFX::DS_PAINTER::draw( const DS_DRAW_ITEM_RECT* aItem, int aLayer ) const
{
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetStrokeColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetLineWidth( std::max( aItem->GetPenWidth(), m_renderSettings.GetDefaultPenWidth() ) );
    m_gal->DrawRectangle( VECTOR2D( aItem->GetStart() ), VECTOR2D( aItem->GetEnd() ) );
}


void KIGFX::DS_PAINTER::draw( const DS_DRAW_ITEM_POLYPOLYGONS* aItem, int aLayer ) const
{
    m_gal->SetFillColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );

    DS_DRAW_ITEM_POLYPOLYGONS* item =  (DS_DRAW_ITEM_POLYPOLYGONS*)aItem;

    for( int idx = 0; idx < item->GetPolygons().OutlineCount(); ++idx )
    {
        SHAPE_LINE_CHAIN& outline = item->GetPolygons().Outline( idx );
        m_gal->DrawPolygon( outline );
    }
}


void KIGFX::DS_PAINTER::draw( const DS_DRAW_ITEM_TEXT* aItem, int aLayer ) const
{
    VECTOR2D position( aItem->GetTextPos().x, aItem->GetTextPos().y );
    int      penWidth = std::max( aItem->GetEffectiveTextPenWidth(),
                                  m_renderSettings.GetDefaultPenWidth() );

    m_gal->Save();
    m_gal->Translate( position );
    m_gal->Rotate( -aItem->GetTextAngle() * M_PI / 1800.0 );
    m_gal->SetStrokeColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetLineWidth( penWidth );
    m_gal->SetTextAttributes( aItem );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->StrokeText( aItem->GetShownText(), VECTOR2D( 0, 0 ), 0.0 );
    m_gal->Restore();
}


void KIGFX::DS_PAINTER::draw( const DS_DRAW_ITEM_BITMAP* aItem, int aLayer ) const
{
    m_gal->Save();
    auto* bitmap = static_cast<DS_DATA_ITEM_BITMAP*>( aItem->GetPeer() );

    VECTOR2D position = aItem->GetPosition();
    m_gal->Translate( position );

    // When the image scale factor is not 1.0, we need to modify the actual scale
    // as the image scale factor is similar to a local zoom
    double img_scale = bitmap->m_ImageBitmap->GetScale();

    if( img_scale != 1.0 )
        m_gal->Scale( VECTOR2D( img_scale, img_scale ) );

    m_gal->DrawBitmap( *bitmap->m_ImageBitmap );

#if 0   // For bounding box debug purpose only
    EDA_RECT bbox = aItem->GetBoundingBox();
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( true );
    m_gal->SetFillColor( COLOR4D( 1, 1, 1, 0.4 ) );
    m_gal->SetStrokeColor( COLOR4D( 0, 0, 0, 1 ) );

    if( img_scale != 1.0 )
        m_gal->Scale( VECTOR2D( 1.0, 1.0 ) );

    m_gal->DrawRectangle( VECTOR2D( bbox.GetOrigin() ) - position,
                          VECTOR2D( bbox.GetEnd() ) - position );
#endif

    m_gal->Restore();
}


void KIGFX::DS_PAINTER::draw( const DS_DRAW_ITEM_PAGE* aItem, int aLayer ) const
{
    VECTOR2D origin = VECTOR2D( 0.0, 0.0 );
    VECTOR2D end = VECTOR2D( aItem->GetPageSize().x,
                             aItem->GetPageSize().y );

    m_gal->SetIsStroke( true );

    // Use a gray color for the border color
    m_gal->SetStrokeColor( m_renderSettings.m_pageBorderColor );
    m_gal->SetIsFill( false );
    m_gal->SetLineWidth(  m_renderSettings.GetDefaultPenWidth() );

    m_gal->DrawRectangle( origin, end );

    // Draw the corner marker
    double marker_size = aItem->GetMarkerSize();

    m_gal->SetStrokeColor( m_renderSettings.m_pageBorderColor );
    VECTOR2D pos = VECTOR2D( aItem->GetMarkerPos().x, aItem->GetMarkerPos().y );

    // Draw a circle and a X
    m_gal->DrawCircle( pos, marker_size );
    m_gal->DrawLine( VECTOR2D( pos.x - marker_size, pos.y - marker_size),
                     VECTOR2D( pos.x + marker_size, pos.y + marker_size ) );
    m_gal->DrawLine( VECTOR2D( pos.x + marker_size, pos.y - marker_size),
                     VECTOR2D( pos.x - marker_size, pos.y + marker_size ) );
}


void KIGFX::DS_PAINTER::DrawBorder( const PAGE_INFO* aPageInfo, int aScaleFactor ) const
{
    VECTOR2D origin = VECTOR2D( 0.0, 0.0 );
    VECTOR2D end = VECTOR2D( aPageInfo->GetWidthMils() * aScaleFactor,
                             aPageInfo->GetHeightMils() * aScaleFactor );

    m_gal->SetIsStroke( true );
    // Use a gray color for the border color
    m_gal->SetStrokeColor( m_renderSettings.m_pageBorderColor );
    m_gal->SetIsFill( false );
    m_gal->DrawRectangle( origin, end );
}
