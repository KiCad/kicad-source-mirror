/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <text_eval/text_eval_wrapper.h>

#include <drawing_sheet/ds_painter.h>
#include <drawing_sheet/ds_data_item.h>

#include <wx/app.h>

using namespace KIGFX;

static const wxString productName = wxT( "KiCad E.D.A." );

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
    for( int layer = SCH_LAYER_ID_START; layer < SCH_LAYER_ID_END; layer++ )
        m_layerColors[ layer ] = aSettings->GetColor( layer );

    for( int layer = GAL_LAYER_ID_START; layer < GAL_LAYER_ID_END; layer++ )
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

        if( item->Type() == WSG_TEXT_T )
        {
            COLOR4D color = static_cast<const DS_DRAW_ITEM_TEXT*>( item )->GetTextColor();

            if( color != COLOR4D::UNSPECIFIED )
                return color;
        }
    }

    return m_normalColor;
}


void DS_DRAW_ITEM_LIST::GetTextVars( wxArrayString* aVars )
{
    aVars->push_back( wxT( "KICAD_VERSION" ) );
    aVars->push_back( wxT( "#" ) );
    aVars->push_back( wxT( "##" ) );
    aVars->push_back( wxT( "SHEETNAME" ) );
    aVars->push_back( wxT( "SHEETPATH" ) );
    aVars->push_back( wxT( "FILENAME" ) );
    aVars->push_back( wxT( "FILEPATH" ) );
    aVars->push_back( wxT( "PROJECTNAME" ) );
    aVars->push_back( wxT( "PAPER" ) );
    aVars->push_back( wxT( "LAYER" ) );
    TITLE_BLOCK::GetContextualTextVars( aVars );
}


wxString DS_DRAW_ITEM_LIST::BuildFullText( const wxString& aTextbase )
{
    std::function<bool( wxString* )> wsResolver =
            [&]( wxString* token ) -> bool
            {
                bool tokenUpdated = false;

                if( token->IsSameAs( wxT( "KICAD_VERSION" ) ) && PgmOrNull() )
                {
                    *token = wxString::Format( wxT( "%s %s" ), productName, GetBaseVersion() );
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
                    *token = m_sheetName;
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "SHEETPATH" ) ) )
                {
                    *token = m_sheetPath;
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "FILENAME" ) ) )
                {
                    wxFileName fn( m_fileName );
                    *token = fn.GetFullName();
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "FILEPATH" ) ) )
                {
                    wxFileName fn( m_fileName );
                    *token = fn.GetFullPath();
                    return true;
                }
                else if( token->IsSameAs( wxT( "PAPER" ) ) )
                {
                    *token = m_paperFormat;
                    tokenUpdated = true;
                }
                else if( token->IsSameAs( wxT( "LAYER" ) ) )
                {
                    *token = m_sheetLayer;
                    tokenUpdated = true;
                }
                else if( m_titleBlock )
                {
                    if( m_titleBlock->TextVarResolver( token, m_project, m_flags ) )
                    {
                        // No need for tokenUpdated; TITLE_BLOCK::TextVarResolver() already goes
                        // up to the project.
                        //
                        // However, the title block may have variables in it itself, so we need
                        // to run the worksheet resolver again.
                        //
                        const TITLE_BLOCK* savedTitleBlock = m_titleBlock;

                        m_titleBlock = nullptr;
                        {
                            *token = ExpandTextVars( *token, &wsResolver, m_flags );
                        }
                        m_titleBlock = savedTitleBlock;

                        return true;
                    }
                }
                else if( m_properties && m_properties->count( *token ) )
                {
                    *token = m_properties->at( *token );
                    tokenUpdated = true;
                }

                if( tokenUpdated )
                {
                   *token = ExpandTextVars( *token, m_project, m_flags );
                   return true;
                }

                if( m_project && m_project->TextVarResolver( token ) )
                    return true;

                return false;
            };

    wxString retv = ExpandTextVars( aTextbase, &wsResolver, m_flags );

    static EXPRESSION_EVALUATOR evaluator;

    if( retv.Contains( wxS( "@{" ) ) )
        retv = evaluator.Evaluate( retv );

    return retv;
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
    KIFONT::FONT* font = aItem->GetFont();

    if( !font )
    {
        font = KIFONT::FONT::GetFont( m_renderSettings.GetDefaultFont(), aItem->IsBold(),
                                      aItem->IsItalic(), nullptr, true );
    }

    const COLOR4D& color = m_renderSettings.GetColor( aItem, aLayer );

    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );

    TEXT_ATTRIBUTES attrs = aItem->GetAttributes();
    attrs.m_StrokeWidth = std::max( aItem->GetEffectiveTextPenWidth(),
                                    m_renderSettings.GetDefaultPenWidth() );

    font->Draw( m_gal, aItem->GetShownText( true ), aItem->GetTextPos(), attrs,
                aItem->GetFontMetrics() );
}


void KIGFX::DS_PAINTER::draw( const DS_DRAW_ITEM_BITMAP* aItem, int aLayer ) const
{
    m_gal->Save();
    auto* bitmap = static_cast<DS_DATA_ITEM_BITMAP*>( aItem->GetPeer() );

    VECTOR2D position = aItem->GetPosition();
    m_gal->Translate( position );

    // If we've failed to read the bitmap data, don't try to draw it
    if( !( bitmap && bitmap->m_ImageBitmap
                    && bitmap->m_ImageBitmap->GetImageData() ) )
    {
        return;
    }

    // When the image scale factor is not 1.0, we need to modify the actual scale
    // as the image scale factor is similar to a local zoom
    double img_scale = bitmap->m_ImageBitmap->GetScale();

    if( img_scale != 1.0 )
        m_gal->Scale( VECTOR2D( img_scale, img_scale ) );

    m_gal->DrawBitmap( *bitmap->m_ImageBitmap );

#if 0   // For bounding box debug purpose only
    BOX2I bbox = aItem->GetBoundingBox();
    m_canvas->SetIsFill( true );
    m_canvas->SetIsStroke( true );
    m_canvas->SetFillColor( COLOR4D( 1, 1, 1, 0.4 ) );
    m_canvas->SetStrokeColor( COLOR4D( 0, 0, 0, 1 ) );

    if( img_scale != 1.0 )
        m_canvas->Scale( VECTOR2D( 1.0, 1.0 ) );

    m_canvas->DrawRectangle( VECTOR2D( bbox.GetOrigin() ) - position,
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
    m_gal->SetLineWidth( m_renderSettings.GetDefaultPenWidth() );
    m_gal->DrawRectangle( origin, end );
}
