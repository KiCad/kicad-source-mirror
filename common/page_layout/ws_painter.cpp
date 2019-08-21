/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <common.h>
#include <base_screen.h>
#include <eda_draw_frame.h>
#include <title_block.h>
#include <build_version.h>
#include <ws_draw_item.h>
#include <gal/graphics_abstraction_layer.h>

#include <ws_painter.h>
#include <ws_data_item.h>

using namespace KIGFX;

static const wxString productName = wxT( "KiCad E.D.A.  " );

WS_RENDER_SETTINGS::WS_RENDER_SETTINGS()
{
    m_backgroundColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
    m_normalColor =     RED;
    m_selectedColor =   m_normalColor.Brightened( 0.5 );
    m_brightenedColor = COLOR4D( 0.0, 1.0, 0.0, 0.9 );

    update();
}


const COLOR4D& WS_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
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


// returns the full text corresponding to the aTextbase,
// after replacing format symbols by the corresponding value
wxString WS_DRAW_ITEM_LIST::BuildFullText( const wxString& aTextbase )
{
    wxString msg;

    /* Known formats
     * %% = replaced by %
     * %K = Kicad version
     * %Z = paper format name (A4, USLetter)
     * %Y = company name
     * %D = date
     * %R = revision
     * %S = sheet number
     * %N = number of sheets
     * %L = layer name
     * %Cx = comment (x = 0 to 9 to identify the comment)
     * %F = filename
     * %P = sheet path (sheet full name)
     * %T = title
     */

    for( unsigned ii = 0; ii < aTextbase.Len(); ii++ )
    {
        if( aTextbase[ii] != '%' )
        {
            msg << aTextbase[ii];
            continue;
        }

        if( ++ii >= aTextbase.Len() )
            break;

        wxChar format = aTextbase[ii];
        switch( format )
        {
            case '%':
                msg += '%';
                break;

            case 'D':
                if( m_titleBlock )
                    msg += m_titleBlock->GetDate();
                break;

            case 'R':
                if( m_titleBlock )
                    msg += m_titleBlock->GetRevision();
                break;

            case 'K':
                msg += productName + Pgm().App().GetAppName();
                msg += wxT( " " ) + GetBuildVersion();
                break;

            case 'Z':
                if( m_paperFormat )
                    msg += *m_paperFormat;
                break;

            case 'S':
                msg << m_sheetNumber;
                break;

            case 'N':
                msg << m_sheetCount;
                break;

            case 'F':
                {
                    wxFileName fn( m_fileName );
                    msg += fn.GetFullName();
                }
                break;

            case 'L':
                if( m_sheetLayer )
                    msg += *m_sheetLayer;
                break;

            case 'P':
                msg += m_sheetFullName;
                break;

            case 'Y':
                if( m_titleBlock )
                    msg += m_titleBlock->GetCompany();
                break;

            case 'T':
                if( m_titleBlock )
                    msg += m_titleBlock->GetTitle();
                break;

            case 'C':
                format = aTextbase[++ii];
                switch( format )
                {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if( m_titleBlock )
                        msg += m_titleBlock->GetComment( format - '0');
                    break;

                default:
                    break;
                }

            default:
                break;
        }
    }

    return msg;
}


void TITLE_BLOCK::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const

{
    // Don't write the title block information if there is nothing to write.
    bool isempty = true;
    for( unsigned idx = 0; idx < m_tbTexts.GetCount(); idx++ )
    {
        if( ! m_tbTexts[idx].IsEmpty() )
        {
            isempty = false;
            break;
        }
    }

    if( !isempty  )
    {
        aFormatter->Print( aNestLevel, "(title_block\n" );

        if( !GetTitle().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(title %s)\n",
                               aFormatter->Quotew( GetTitle() ).c_str() );

        if( !GetDate().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(date %s)\n",
                               aFormatter->Quotew( GetDate() ).c_str() );

        if( !GetRevision().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(rev %s)\n",
                               aFormatter->Quotew( GetRevision() ).c_str() );

        if( !GetCompany().IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(company %s)\n",
                               aFormatter->Quotew( GetCompany() ).c_str() );

        for( int ii = 0; ii < 9; ii++ )
        {
            if( !GetComment(ii).IsEmpty() )
                aFormatter->Print( aNestLevel+1, "(comment %d %s)\n", ii+1,
                                  aFormatter->Quotew( GetComment(ii) ).c_str() );
        }

        aFormatter->Print( aNestLevel, ")\n\n" );
    }
}


bool KIGFX::WS_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    auto item = static_cast<const EDA_ITEM*>( aItem );

    switch( item->Type() )
    {
    case WSG_LINE_T:   draw( (WS_DRAW_ITEM_LINE*) item, aLayer );         break;
    case WSG_POLY_T:   draw( (WS_DRAW_ITEM_POLYPOLYGONS*) item, aLayer );      break;
    case WSG_RECT_T:   draw( (WS_DRAW_ITEM_RECT*) item, aLayer );         break;
    case WSG_TEXT_T:   draw( (WS_DRAW_ITEM_TEXT*) item, aLayer );         break;
    case WSG_BITMAP_T: draw( (WS_DRAW_ITEM_BITMAP*) item, aLayer );       break;
    case WSG_PAGE_T:   draw( (WS_DRAW_ITEM_PAGE*) item, aLayer );       break;
    default:           return false;
    }

    return true;
}


void KIGFX::WS_PAINTER::draw( const WS_DRAW_ITEM_LINE* aItem, int aLayer ) const
{
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetStrokeColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetLineWidth( aItem->GetPenWidth() );
    m_gal->DrawLine( VECTOR2D( aItem->GetStart() ), VECTOR2D( aItem->GetEnd() ) );
}


void KIGFX::WS_PAINTER::draw( const WS_DRAW_ITEM_RECT* aItem, int aLayer ) const
{
    m_gal->SetIsStroke( true );
    m_gal->SetIsFill( false );
    m_gal->SetStrokeColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetLineWidth( aItem->GetPenWidth() );
    m_gal->DrawRectangle( VECTOR2D( aItem->GetStart() ), VECTOR2D( aItem->GetEnd() ) );
}


void KIGFX::WS_PAINTER::draw( const WS_DRAW_ITEM_POLYPOLYGONS* aItem, int aLayer ) const
{
    m_gal->SetFillColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );

    WS_DRAW_ITEM_POLYPOLYGONS* item =  (WS_DRAW_ITEM_POLYPOLYGONS*)aItem;

    for( int idx = 0; idx < item->GetPolygons().OutlineCount(); ++idx )
    {
        SHAPE_LINE_CHAIN& outline = item->GetPolygons().Outline( idx );
        m_gal->DrawPolygon( outline );
    }
}


void KIGFX::WS_PAINTER::draw( const WS_DRAW_ITEM_TEXT* aItem, int aLayer ) const
{
    VECTOR2D position( aItem->GetTextPos().x, aItem->GetTextPos().y );

    m_gal->Save();
    m_gal->Translate( position );
    m_gal->Rotate( -aItem->GetTextAngle() * M_PI / 1800.0 );
    m_gal->SetStrokeColor( m_renderSettings.GetColor( aItem, aLayer ) );
    m_gal->SetLineWidth( aItem->GetThickness() );
    m_gal->SetTextAttributes( aItem );
    m_gal->SetIsFill( false );
    m_gal->SetIsStroke( true );
    m_gal->StrokeText( aItem->GetShownText(), VECTOR2D( 0, 0 ), 0.0 );
    m_gal->Restore();
}


void KIGFX::WS_PAINTER::draw( const WS_DRAW_ITEM_BITMAP* aItem, int aLayer ) const
{
    m_gal->Save();
    VECTOR2D position = aItem->GetPosition();
    m_gal->Translate( position );
    auto* bitmap = static_cast<WS_DATA_ITEM_BITMAP*>( aItem->GetPeer() );

    // When the image scale factor is not 1.0, we need to modify the actual scale
    // as the image scale factor is similar to a local zoom
    double img_scale = bitmap->m_ImageBitmap->GetScale();

    if( img_scale != 1.0 )
        m_gal->Scale( VECTOR2D( img_scale, img_scale ) );

    m_gal->DrawBitmap( *bitmap->m_ImageBitmap );
    m_gal->Restore();
}


void KIGFX::WS_PAINTER::draw( const WS_DRAW_ITEM_PAGE* aItem, int aLayer ) const
{
    VECTOR2D origin = VECTOR2D( 0.0, 0.0 );
    VECTOR2D end = VECTOR2D( aItem->GetPageSize().x,
                             aItem->GetPageSize().y );

    m_gal->SetIsStroke( true );

    // Use a gray color for the border color
    m_gal->SetStrokeColor( COLOR4D( 0.4, 0.4, 0.4, 1.0 ) );
    m_gal->SetIsFill( false );
    m_gal->DrawRectangle( origin, end );

    // Draw the corner marker
    double marker_size = aItem->GetMarkerSize();

    m_gal->SetStrokeColor( COLOR4D( 0.4, 0.4, 1.0, 1.0 ) );
    VECTOR2D pos = VECTOR2D( aItem->GetMarkerPos().x, aItem->GetMarkerPos().y );

    // Draw a cirle and a X
    m_gal->DrawCircle( pos, marker_size );
    m_gal->DrawLine( VECTOR2D( pos.x - marker_size, pos.y - marker_size),
                     VECTOR2D( pos.x + marker_size, pos.y + marker_size ) );
    m_gal->DrawLine( VECTOR2D( pos.x + marker_size, pos.y - marker_size),
                     VECTOR2D( pos.x - marker_size, pos.y + marker_size ) );
}


void KIGFX::WS_PAINTER::DrawBorder( const PAGE_INFO* aPageInfo, int aScaleFactor ) const
{
    VECTOR2D origin = VECTOR2D( 0.0, 0.0 );
    VECTOR2D end = VECTOR2D( aPageInfo->GetWidthMils() * aScaleFactor,
                             aPageInfo->GetHeightMils() * aScaleFactor );

    m_gal->SetIsStroke( true );
    // Use a gray color for the border color
    m_gal->SetStrokeColor( COLOR4D( 0.4, 0.4, 0.4, 1.0 ) );
    m_gal->SetIsFill( false );
    m_gal->DrawRectangle( origin, end );
}
