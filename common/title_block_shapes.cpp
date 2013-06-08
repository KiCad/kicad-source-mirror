/**
 * @file title_block_shape.cpp
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
#include <worksheet.h>
#include <class_title_block.h>
#include <worksheet_shape_builder.h>

WORKSHEET_DATAITEM_POLYPOLYGON::WORKSHEET_DATAITEM_POLYPOLYGON() :
    WORKSHEET_DATAITEM( WS_POLYPOLYGON )
{
    m_Orient = 0.0;
}

const DPOINT WORKSHEET_DATAITEM_POLYPOLYGON::GetCornerPosition( unsigned aIdx,
                                                         int aRepeat ) const
{
    DPOINT pos = m_Corners[aIdx];

    // Rotation:
    RotatePoint( &pos.x, &pos.y, m_Orient * 10 );
    pos += GetStartPos( aRepeat );
    return pos;
}

void WORKSHEET_DATAITEM_POLYPOLYGON::SetBoundingBox()
{
    if( m_Corners.size() == 0 )
    {
        m_minCoord.x = m_maxCoord.x = 0.0;
        m_minCoord.y = m_maxCoord.y = 0.0;
        return;
    }

    DPOINT pos;
    pos = m_Corners[0];
    RotatePoint( &pos.x, &pos.y, m_Orient * 10 );
    m_minCoord = m_maxCoord = pos;

    for( unsigned ii = 1; ii < m_Corners.size(); ii++ )
    {
        pos = m_Corners[ii];
        RotatePoint( &pos.x, &pos.y, m_Orient * 10 );

        if( m_minCoord.x > pos.x )
            m_minCoord.x = pos.x;

        if( m_minCoord.y > pos.y )
            m_minCoord.y = pos.y;

        if( m_maxCoord.x < pos.x )
            m_maxCoord.x = pos.x;

        if( m_maxCoord.y < pos.y )
            m_maxCoord.y = pos.y;
    }
}

bool WORKSHEET_DATAITEM_POLYPOLYGON::IsInsidePage( int ii ) const
{
    DPOINT pos = GetStartPos( ii );
    pos += m_minCoord;  // left top pos of bounding box

    if( m_LT_Corner.x > pos.x || m_LT_Corner.y > pos.y )
        return false;

    pos = GetStartPos( ii );
    pos += m_maxCoord;  // rignt bottom pos of bounding box

    if( m_RB_Corner.x < pos.x || m_RB_Corner.y < pos.y )
        return false;

    return true;
}

const wxPoint WORKSHEET_DATAITEM_POLYPOLYGON::GetCornerPositionUi( unsigned aIdx,
                                                            int aRepeat ) const
{
    DPOINT pos = GetCornerPosition( aIdx, aRepeat );
    pos = pos * m_WSunits2Iu;
    return wxPoint( int(pos.x), int(pos.y) );
}

WORKSHEET_DATAITEM_TEXT::WORKSHEET_DATAITEM_TEXT( const wxChar* aTextBase ) :
    WORKSHEET_DATAITEM( WS_TEXT )
{
    m_TextBase = aTextBase;
    m_IncrementLabel = 1;
    m_Hjustify = GR_TEXT_HJUSTIFY_LEFT;
    m_Vjustify = GR_TEXT_VJUSTIFY_CENTER;
    m_Orient = 0.0;
    m_TextSize.x = m_TextSize.y = TB_DEFAULT_TEXTSIZE;
}

void WORKSHEET_DATAITEM_TEXT::TransfertSetupToGraphicText( WS_DRAW_ITEM_TEXT* aGText )
{
    aGText->SetHorizJustify( m_Hjustify ) ;
    aGText->SetVertJustify( m_Vjustify );
    aGText->SetOrientation( m_Orient * 10 );    // graphic text orient unit = 0.1 degree
}

void WORKSHEET_DATAITEM_TEXT::IncrementLabel( int aIncr )
{
    wxChar lbchar = m_TextBase[0];
    if( lbchar >= '0' &&  lbchar <= '9' )
        // A number is expected:
        m_FullText.Printf( wxT("%d"), aIncr + lbchar - '0' );
    else
        m_FullText.Printf( wxT("%c"), aIncr + lbchar );
}

void WORKSHEET_DATAITEM_TEXT::SetConstrainedTextSize()
{
    m_ConstrainedTextSize = m_TextSize;

    if( m_BoundingBoxSize.x )
    {
        bool italic = (m_Flags & USE_ITALIC) != 0;
        int linewidth = 0;
        int lenMsg   = ReturnGraphicTextWidth( m_FullText, m_TextSize.x, italic, linewidth );
        if( lenMsg > m_BoundingBoxSize.x )
            m_ConstrainedTextSize.x = m_TextSize.x * m_BoundingBoxSize.x / lenMsg;
    }

    if( m_BoundingBoxSize.y )
    {
        if( m_ConstrainedTextSize.y > m_BoundingBoxSize.y )
            m_ConstrainedTextSize.y = m_BoundingBoxSize.y;
    }
}

const DPOINT WORKSHEET_DATAITEM::GetStartPos( int ii ) const
{
    DPOINT pos;
    pos.x = m_Pos.m_Pos.x + ( m_IncrementVector.x * ii );
    pos.y = m_Pos.m_Pos.y + ( m_IncrementVector.y * ii );

    switch( m_Pos.m_Anchor )
    {
        case RB_CORNER:      // right bottom corner
            pos = m_RB_Corner - pos;
            break;

        case RT_CORNER:      // right top corner
            pos.x = m_RB_Corner.x - pos.x;
            pos.y = m_LT_Corner.y + pos.y;
            break;

        case LB_CORNER:      // left bottom corner
            pos.x = m_LT_Corner.x + pos.x;
            pos.y = m_RB_Corner.y - pos.y;
            break;

        case LT_CORNER:      // left top corner
            pos = m_LT_Corner + pos;
            break;
    }

    return pos;
}

const wxPoint WORKSHEET_DATAITEM::GetStartPosUi( int ii ) const
{
    DPOINT pos = GetStartPos( ii );
    pos = pos * m_WSunits2Iu;
    return wxPoint( int(pos.x), int(pos.y) );
}

const DPOINT WORKSHEET_DATAITEM::GetEndPos( int ii ) const
{
    DPOINT pos;
    pos.x = m_End.m_Pos.x + ( m_IncrementVector.x * ii );
    pos.y = m_End.m_Pos.y + ( m_IncrementVector.y * ii );
    switch( m_End.m_Anchor )
    {
        case RB_CORNER:      // right bottom corner
            pos = m_RB_Corner - pos;
            break;

        case RT_CORNER:      // right top corner
            pos.x = m_RB_Corner.x - pos.x;
            pos.y = m_LT_Corner.y + pos.y;
            break;

        case LB_CORNER:      // left bottom corner
            pos.x = m_LT_Corner.x + pos.x;
            pos.y = m_RB_Corner.y - pos.y;
            break;

        case LT_CORNER:      // left top corner
            pos = m_LT_Corner + pos;
            break;
    }

    return pos;
}

const wxPoint WORKSHEET_DATAITEM::GetEndPosUi( int ii ) const
{
    DPOINT pos = GetEndPos( ii );
    pos = pos * m_WSunits2Iu;
    return wxPoint( int(pos.x), int(pos.y) );
}


bool WORKSHEET_DATAITEM::IsInsidePage( int ii ) const
{
    DPOINT pos = GetStartPos( ii );

    if( m_RB_Corner.x < pos.x || m_LT_Corner.x > pos.x )
        return false;

    if( m_RB_Corner.y < pos.y || m_LT_Corner.y > pos.y )
        return false;

    pos = GetEndPos( ii );

    if( m_RB_Corner.x < pos.x || m_LT_Corner.x > pos.x )
        return false;

    if( m_RB_Corner.y < pos.y || m_LT_Corner.y > pos.y )
        return false;

    return true;
}

double WORKSHEET_DATAITEM::m_WSunits2Iu = 1.0;
DPOINT WORKSHEET_DATAITEM::m_RB_Corner;
DPOINT WORKSHEET_DATAITEM::m_LT_Corner;

WORKSHEET_LAYOUT dataList;  // The layout shape

void WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList(
                       const wxString& aPaperFormat,
                       const wxString& aFileName,
                       const wxString& aSheetPathHumanReadable,
                       const TITLE_BLOCK& aTitleBlock,
                       EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
{
    #define milsTomm (25.4/1000)

    m_titleBlock = &aTitleBlock,
    m_paperFormat = &aPaperFormat,
    m_fileName = &aFileName,
    m_sheetFullName = &aSheetPathHumanReadable;

    // Build the basic layout shape, if the layout list is empty
    if( dataList.GetCount() == 0 )
        dataList.SetLayout();

    WORKSHEET_DATAITEM::m_WSunits2Iu = m_milsToIu / milsTomm;

    // Left top corner position
    DPOINT lt_corner;
    lt_corner.x = m_LTmargin.x;
    lt_corner.y = m_LTmargin.y;
    WORKSHEET_DATAITEM::m_LT_Corner = lt_corner * milsTomm;

    // Right bottom corner position
    DPOINT rb_corner;
    rb_corner.x = m_pageSize.x - m_RBmargin.x;
    rb_corner.y = m_pageSize.y - m_RBmargin.y;
    WORKSHEET_DATAITEM::m_RB_Corner = rb_corner * milsTomm;

    WS_DRAW_ITEM_TEXT* gtext;
    int pensize;
    EDA_COLOR_T color;

    for( unsigned ii = 0; ; ii++ )
    {
        WORKSHEET_DATAITEM*  wsItem = dataList.GetItem( ii );

        if( wsItem == NULL )
            break;

        pensize = wsItem->GetPenSizeUi();

        switch( wsItem->m_Type )
        {
        case WORKSHEET_DATAITEM::WS_TEXT:
        {
            WORKSHEET_DATAITEM_TEXT * wsText = (WORKSHEET_DATAITEM_TEXT*)wsItem;
            wsText->m_FullText = BuildFullText( wsText->m_TextBase );
            if( wsText->m_FullText.IsEmpty() )
                break;

            if( pensize == 0 )
                pensize = m_penSize;

            color = aLineColor;

            if( wsText->m_Flags & USE_TEXT_COLOR )
                color = aTextColor;

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
                if( ! wsText->IsInsidePage( jj ) )
                    continue;

                Append( gtext = new WS_DRAW_ITEM_TEXT( wsText->m_FullText,
                                                       wsText->GetStartPosUi( jj ),
                                                       textsize,
                                                       pensize, color,
                                                       wsText->IsItalic(),
                                                       wsText->IsBold() ) );
                wsText->TransfertSetupToGraphicText( gtext );

                // Increment label for the next text
                if( wsText->m_RepeatCount > 1 )
                    wsText->IncrementLabel( jj+1 );
            }
        }
            break;

        case WORKSHEET_DATAITEM::WS_SEGMENT:
            if( pensize == 0 )
                pensize = m_penSize;

            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( ! wsItem->IsInsidePage( jj ) )
                    continue;
                Append( new WS_DRAW_ITEM_LINE( wsItem->GetStartPosUi( jj ),
                                               wsItem->GetEndPosUi( jj ),
                                               pensize, aLineColor ) );
            }
            break;

        case WORKSHEET_DATAITEM::WS_RECT:
            if( pensize == 0 )
                pensize = m_penSize;

            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( ! wsItem->IsInsidePage( jj ) )
                    break;

                Append( new WS_DRAW_ITEM_RECT( wsItem->GetStartPosUi( jj ),
                                               wsItem->GetEndPosUi( jj ),
                                               pensize, aLineColor ) );
            }
            break;

        case WORKSHEET_DATAITEM::WS_POLYPOLYGON:
        {
            WORKSHEET_DATAITEM_POLYPOLYGON * wspoly =
                (WORKSHEET_DATAITEM_POLYPOLYGON*) wsItem;
            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( ! wsItem->IsInsidePage( jj ) )
                    continue;

                for( int kk = 0; kk < wspoly->GetPolyCount(); kk++ )
                {
                    const bool fill = true;
                    WS_DRAW_ITEM_POLYGON* poly = new WS_DRAW_ITEM_POLYGON( fill,
                                                   pensize, aLineColor );
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
        }
    }
}
