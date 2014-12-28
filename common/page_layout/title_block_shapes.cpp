/**
 * @file title_block_shapes.cpp
 * @brief description of graphic items and texts to build a title block
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
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


/*
 * the class WORKSHEET_DATAITEM (and WORKSHEET_DATAITEM_TEXT) defines
 * a basic shape of a page layout ( frame references and title block )
 * Basic shapes are line, rect and texts
 * the WORKSHEET_DATAITEM coordinates units is the mm, and are relative to
 * one of 4 page corners.
 *
 * These items cannot be drawn or plot "as this". they should be converted
 * to a "draw list" (WS_DRAW_ITEM_BASE and derived items)

 * The list of these items is stored in a WORKSHEET_LAYOUT instance.
 *
 * When building the draw list:
 * the WORKSHEET_LAYOUT is used to create a WS_DRAW_ITEM_LIST
 *  coordinates are converted to draw/plot coordinates.
 *  texts are expanded if they contain format symbols.
 *  Items with m_RepeatCount > 1 are created m_RepeatCount times
 *
 * the WORKSHEET_LAYOUT is created only once.
 * the WS_DRAW_ITEM_LIST is created each time the page layout is plot/drawn
 *
 * the WORKSHEET_LAYOUT instance is created from a S expression which
 * describes the page layout (can be the default page layout or a custom file).
 */

#include <fctsys.h>
#include <drawtxt.h>
#include <class_page_info.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>


void WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList(
                       const PAGE_INFO& aPageInfo,
                       const TITLE_BLOCK& aTitleBlock,
                       EDA_COLOR_T aColor, EDA_COLOR_T aAltColor )
{
    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();

    #define milsTomm (25.4/1000)

    m_titleBlock = &aTitleBlock;
    m_paperFormat = &aPageInfo.GetType();

    wxPoint LTmargin( Mm2mils( pglayout.GetLeftMargin() ),
                      Mm2mils( pglayout.GetTopMargin() ) );
    wxPoint RBmargin( Mm2mils( pglayout.GetRightMargin() ),
                      Mm2mils( pglayout.GetBottomMargin() ) );

    SetMargins( LTmargin, RBmargin );
    SetPageSize( aPageInfo.GetSizeMils() );

    // Build the basic layout shape, if the layout list is empty
    if( pglayout.GetCount() == 0 && !pglayout.VoidListAllowed() )
        pglayout.SetPageLayout();

    WORKSHEET_DATAITEM::m_WSunits2Iu = m_milsToIu / milsTomm;
    WORKSHEET_DATAITEM::m_Color = aColor;       // the default color to draw items
    WORKSHEET_DATAITEM::m_AltColor = aAltColor; // an alternate color to draw items

    // Left top corner position
    DPOINT lt_corner;
    lt_corner.x = pglayout.GetLeftMargin();
    lt_corner.y = pglayout.GetTopMargin();
    WORKSHEET_DATAITEM::m_LT_Corner = lt_corner;

    // Right bottom corner position
    DPOINT rb_corner;
    rb_corner.x = (m_pageSize.x*milsTomm) - pglayout.GetRightMargin();
    rb_corner.y = (m_pageSize.y*milsTomm) - pglayout.GetBottomMargin();
    WORKSHEET_DATAITEM::m_RB_Corner = rb_corner;

    WS_DRAW_ITEM_TEXT* gtext;
    int pensize;

    for( unsigned ii = 0; ; ii++ )
    {
        WORKSHEET_DATAITEM*  wsItem = pglayout.GetItem( ii );

        if( wsItem == NULL )
            break;

        // Generate it only if the page option allows this
        if( wsItem->GetPage1Option() < 0    // Not on page 1
            && m_sheetNumber <= 1 )
            continue;

        if( wsItem->GetPage1Option() > 0    // Only on page 1
            && m_sheetNumber > 1 )
            continue;

        EDA_COLOR_T color = wsItem->GetItemColor();

        pensize = wsItem->GetPenSizeUi();

        switch( wsItem->GetType() )
        {
        case WORKSHEET_DATAITEM::WS_TEXT:
        {
            WORKSHEET_DATAITEM_TEXT * wsText = (WORKSHEET_DATAITEM_TEXT*)wsItem;
            bool multilines = false;

            if( wsText->m_SpecialMode )
                wsText->m_FullText = wsText->m_TextBase;
            else
            {
                wsText->m_FullText = BuildFullText( wsText->m_TextBase );
                multilines = wsText->ReplaceAntiSlashSequence();
            }

            if( wsText->m_FullText.IsEmpty() )
                break;

            if( pensize == 0 )
                pensize = m_penSize;

            wsText->SetConstrainedTextSize();
            wxSize textsize;

            textsize.x = KiROUND( wsText->m_ConstrainedTextSize.x
                                  * WORKSHEET_DATAITEM::m_WSunits2Iu );
            textsize.y = KiROUND( wsText->m_ConstrainedTextSize.y
                                  * WORKSHEET_DATAITEM::m_WSunits2Iu );

            if( wsText->IsBold())
                pensize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );

            for( int jj = 0; jj < wsText->m_RepeatCount; jj++)
            {
                if( jj && ! wsText->IsInsidePage( jj ) )
                    continue;

                Append( gtext = new WS_DRAW_ITEM_TEXT( wsText, wsText->m_FullText,
                                                       wsText->GetStartPosUi( jj ),
                                                       textsize,
                                                       pensize, color,
                                                       wsText->IsItalic(),
                                                       wsText->IsBold() ) );
                gtext->SetMultilineAllowed( multilines );
                wsText->TransfertSetupToGraphicText( gtext );

                // Increment label for the next text
                // (has no meaning for multiline texts)
                if( wsText->m_RepeatCount > 1 && !multilines )
                    wsText->IncrementLabel( (jj+1)*wsText->m_IncrementLabel);
            }
        }
            break;

        case WORKSHEET_DATAITEM::WS_SEGMENT:
            if( pensize == 0 )
                pensize = m_penSize;

            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( jj && ! wsItem->IsInsidePage( jj ) )
                    continue;
                Append( new WS_DRAW_ITEM_LINE( wsItem, wsItem->GetStartPosUi( jj ),
                                               wsItem->GetEndPosUi( jj ),
                                               pensize, color ) );
            }
            break;

        case WORKSHEET_DATAITEM::WS_RECT:
            if( pensize == 0 )
                pensize = m_penSize;

            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( jj && ! wsItem->IsInsidePage( jj ) )
                    break;

                Append( new WS_DRAW_ITEM_RECT( wsItem, wsItem->GetStartPosUi( jj ),
                                               wsItem->GetEndPosUi( jj ),
                                               pensize, color ) );
            }
            break;

        case WORKSHEET_DATAITEM::WS_POLYPOLYGON:
        {
            WORKSHEET_DATAITEM_POLYPOLYGON * wspoly =
                (WORKSHEET_DATAITEM_POLYPOLYGON*) wsItem;
            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( jj && ! wsItem->IsInsidePage( jj ) )
                    continue;

                for( int kk = 0; kk < wspoly->GetPolyCount(); kk++ )
                {
                    const bool fill = true;
                    WS_DRAW_ITEM_POLYGON* poly = new WS_DRAW_ITEM_POLYGON( wspoly,
                                                wspoly->GetStartPosUi( jj ),
                                                fill, pensize, color );
                    Append( poly );

                    // Create polygon outline
                    unsigned ist = wspoly->GetPolyIndexStart( kk );
                    unsigned iend = wspoly->GetPolyIndexEnd( kk );
                    while( ist <= iend )
                        poly->m_Corners.push_back(
                            wspoly->GetCornerPositionUi( ist++, jj ) );

                }
            }
        }
            break;

        case WORKSHEET_DATAITEM::WS_BITMAP:

            ((WORKSHEET_DATAITEM_BITMAP*)wsItem)->SetPixelScaleFactor();

            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( jj && ! wsItem->IsInsidePage( jj ) )
                    continue;

                Append( new WS_DRAW_ITEM_BITMAP( wsItem,
                    wsItem->GetStartPosUi( jj ) ) );
            }
            break;

        }
    }
}

