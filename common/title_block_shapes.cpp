/**
 * @file title_block_shape.cpp
 * @brief description of graphic items and texts to build a title block
 */

/*
 * This file creates a lot of structures which define the shape of a title block
 * and frame references
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

#include <fctsys.h>
#include <drawtxt.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <worksheet_shape_builder.h>

extern void SetDataList( WORKSHEET_LAYOUT& aDataList );


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
        dataList.SetDefaultLayout();

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
    bool bold;
    bool italic = false;
    EDA_COLOR_T color;

    for( unsigned ii = 0; ; ii++ )
    {
        WORKSHEET_DATAITEM*  wsItem = dataList.GetItem( ii );

        if( wsItem == NULL )
            break;

        switch( wsItem->m_Type )
        {
        case WORKSHEET_DATAITEM::WS_TEXT:
        {
            WORKSHEET_DATAITEM_TEXT * wsText = (WORKSHEET_DATAITEM_TEXT*)wsItem;
            wsText->m_FullText = BuildFullText( wsText->m_TextBase );
            if( wsText->m_FullText.IsEmpty() )
                break;

            bold = false;
            pensize = wsText->GetPenSizeUi();

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

            if( wsText->m_Flags & USE_BOLD )
            {
                bold = true;
                pensize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );
            }

            for( int jj = 0; jj < wsText->m_RepeatCount; )
            {
                if( ! wsText->IsInsidePage( jj ) )
                    break;
                Append( gtext = new WS_DRAW_ITEM_TEXT( wsText->m_FullText,
                                                       wsText->GetStartPosUi( jj ),
                                                       textsize,
                                                       pensize, color, italic, bold ) );
                wsText->TransfertSetupToGraphicText( gtext );

                jj++;
                if( wsText->m_RepeatCount > 1 )     // Try to increment label
                    wsText->IncrementLabel( jj );
            }
        }
            break;

        case WORKSHEET_DATAITEM::WS_SEGMENT:
            pensize = wsItem->GetPenSizeUi();

            if( pensize == 0 )
                pensize = m_penSize;

            for( int jj = 0; jj < wsItem->m_RepeatCount; jj++ )
            {
                if( ! wsItem->IsInsidePage( jj ) )
                    break;
                Append( new WS_DRAW_ITEM_LINE( wsItem->GetStartPosUi( jj ),
                                               wsItem->GetEndPosUi( jj ),
                                               pensize, aLineColor ) );
            }
            break;

        case WORKSHEET_DATAITEM::WS_RECT:
            pensize = wsItem->GetPenSizeUi();

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
        }
    }
}
