/**
 * @file page_layout_graphic_items.cpp
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
 * the WS_DRAW_ITEM_LIST is created each time the page layout is plotted/drawn
 *
 * the WORKSHEET_LAYOUT instance is created from a S expression which
 * describes the page layout (can be the default page layout or a custom file).
 */

#include <fctsys.h>
#include <drawtxt.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>

/* a helper function to draw graphic symbols at start point or end point of
 * an item.
 * The start point symbol is a filled rectangle
 * The start point symbol is a filled circle
 */
inline void drawMarker( EDA_RECT* aClipBox, wxDC* aDC,
                        const wxPoint& aPos, int aSize, bool aEndPointShape = false )
{
    int markerHalfSize = aSize/2;

    if( aEndPointShape )
        GRFilledCircle( aClipBox, aDC, aPos.x, aPos.y, markerHalfSize,
                        0, GREEN, GREEN );
    else
        GRFilledRect( aClipBox, aDC,
                aPos.x - markerHalfSize, aPos.y - markerHalfSize,
                aPos.x + markerHalfSize, aPos.y + markerHalfSize,
                0, GREEN, GREEN );
}

/* Draws the item list created by BuildWorkSheetGraphicList
 * aClipBox = the clipping rect, or NULL if no clipping
 * aDC = the current Device Context
 * The not selected items are drawn first (most of items)
 * The selected items are drawn after (usually 0 or 1)
 * to be sure they are seen, even for overlapping items
 */
void WS_DRAW_ITEM_LIST::Draw( EDA_RECT* aClipBox, wxDC* aDC )
{
    // The not selected items are drawn first (most of items)
    for( WS_DRAW_ITEM_BASE* item = GetFirst(); item; item = GetNext() )
    {
        if( item->GetParent() && item->GetParent()->IsSelected() )
            continue;

        item->DrawWsItem( aClipBox, aDC );
    }

    // The selected items are drawn after (usually 0 or 1)
    int markerSize = WORKSHEET_DATAITEM::GetMarkerSizeUi();

    for( WS_DRAW_ITEM_BASE* item = GetFirst(); item; item = GetNext() )
    {
        if( !item->GetParent() || !item->GetParent()->IsSelected() )
            continue;

        item->DrawWsItem( aClipBox, aDC );

        switch( item->GetType() )
        {
        case WS_DRAW_ITEM_BASE::wsg_line:
            {
                WS_DRAW_ITEM_LINE* line = (WS_DRAW_ITEM_LINE*) item;

                if( markerSize )
                {
                    drawMarker( aClipBox, aDC, line->GetStart(), markerSize );
                    drawMarker( aClipBox, aDC, line->GetEnd(), markerSize, true );
                }
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_rect:
            {
                WS_DRAW_ITEM_RECT* rect = (WS_DRAW_ITEM_RECT*) item;

                if( markerSize )
                {
                    drawMarker( aClipBox, aDC, rect->GetStart(), markerSize );
                    drawMarker( aClipBox, aDC, rect->GetEnd(), markerSize, true );
                }
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_text:
            {
                WS_DRAW_ITEM_TEXT* text = (WS_DRAW_ITEM_TEXT*) item;

                if( markerSize )
                    drawMarker( aClipBox, aDC, text->GetTextPosition(),
                                markerSize );
               }
            break;

        case WS_DRAW_ITEM_BASE::wsg_poly:
            {
                WS_DRAW_ITEM_POLYGON* poly = (WS_DRAW_ITEM_POLYGON*) item;

                if( markerSize )
                {
                    drawMarker( aClipBox, aDC, poly->GetPosition(),
                                markerSize );
                }
            }
            break;

        case WS_DRAW_ITEM_BASE::wsg_bitmap:
            {
                WS_DRAW_ITEM_BITMAP* bitmap = (WS_DRAW_ITEM_BITMAP*) item;

                if( markerSize )
                {
                    drawMarker( aClipBox, aDC, bitmap->GetPosition(),
                                markerSize );
                }
            }
            break;
        }
    }
}

WS_DRAW_ITEM_TEXT::WS_DRAW_ITEM_TEXT( WORKSHEET_DATAITEM* aParent,
                   wxString& aText, wxPoint aPos, wxSize aSize,
                   int aPenWidth, EDA_COLOR_T aColor,
                   bool aItalic, bool aBold ) :
    WS_DRAW_ITEM_BASE( aParent, wsg_text, aColor ), EDA_TEXT( aText )
{
    SetTextPosition( aPos );
    SetSize( aSize );
    SetThickness( aPenWidth );
    SetItalic( aItalic );
    SetBold( aBold );
}

void WS_DRAW_ITEM_TEXT::DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC )
{
    Draw( aClipBox, aDC, wxPoint(0,0),
          GetColor(), UNSPECIFIED_DRAWMODE, FILLED, UNSPECIFIED_COLOR );
}

// return true if the point aPosition is on the text
bool WS_DRAW_ITEM_TEXT::HitTest( const wxPoint& aPosition) const
{
    return EDA_TEXT::TextHitTest( aPosition, 0 );
}

/* return true if the point aPosition is on the starting point of this item
 */
bool WS_DRAW_ITEM_TEXT::HitTestStartPoint( const wxPoint& aPosition)
{
    wxPoint pos = GetTextPosition();

    if( std::abs( pos.x - aPosition.x) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 &&
        std::abs( pos.y - aPosition.y) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 )
        return true;

    return false;
}

void WS_DRAW_ITEM_POLYGON::DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC )
{
    GRPoly( aClipBox, aDC,
            m_Corners.size(), &m_Corners[0],
            IsFilled() ? FILLED_SHAPE : NO_FILL,
            GetPenWidth(),
            GetColor(), GetColor() );
}

// return true if the point aPosition is inside one of polygons
#include <polygon_test_point_inside.h>
bool WS_DRAW_ITEM_POLYGON::HitTest( const wxPoint& aPosition) const
{
    return TestPointInsidePolygon( &m_Corners[0],
                                   m_Corners.size(), aPosition );
}

/* return true if the point aPosition is on the starting point of this item
 */
bool WS_DRAW_ITEM_POLYGON::HitTestStartPoint( const wxPoint& aPosition)
{
    wxPoint pos = GetPosition();

    if( std::abs( pos.x - aPosition.x) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 &&
        std::abs( pos.y - aPosition.y) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 )
        return true;

    return false;
}

void WS_DRAW_ITEM_RECT::DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC )
{
    GRRect( aClipBox, aDC,
            GetStart().x, GetStart().y,
            GetEnd().x, GetEnd().y,
            GetPenWidth(), GetColor() );
}

// return true if the point aPosition is on the rect outline
bool WS_DRAW_ITEM_RECT::HitTest( const wxPoint& aPosition) const
{
    int dist =  GetPenWidth()/2;
    wxPoint start = GetStart();
    wxPoint end;
    end.x = GetEnd().x;
    end.y = start.y;

    // Upper line
    if( TestSegmentHit( aPosition, start, end, dist ) )
        return true;

    // Right line
    start = end;
    end.y = GetEnd().y;
    if( TestSegmentHit( aPosition, start, end, dist ) )
        return true;

    // lower line
    start = end;
    end.x = GetStart().x;
    if( TestSegmentHit( aPosition, start, end, dist ) )
        return true;

    // left line
    start = end;
    end = GetStart();
    if( TestSegmentHit( aPosition, start, end, dist ) )
        return true;

    return false;
}

/* return true if the point aPosition is on the starting point of this item
 */
bool WS_DRAW_ITEM_RECT::HitTestStartPoint( const wxPoint& aPosition)
{
    wxPoint dist = GetStart() - aPosition;

    if( std::abs( dist.x) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 &&
        std::abs( dist.y) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 )
        return true;

    return false;
}

/* return true if the point aPosition is on the ending point of this item
 */
bool WS_DRAW_ITEM_RECT::HitTestEndPoint( const wxPoint& aPosition)
{
    wxPoint pos = GetEnd();

    int dist = (int) hypot( pos.x - aPosition.x, pos.y - aPosition.y );

    if( dist <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 )
        return true;

    return false;
}

void WS_DRAW_ITEM_LINE::DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC )
{
    GRLine( aClipBox, aDC, GetStart(), GetEnd(),
            GetPenWidth(), GetColor() );
}

// return true if the point aPosition is on the text
bool WS_DRAW_ITEM_LINE::HitTest( const wxPoint& aPosition) const
{
    return TestSegmentHit( aPosition, GetStart(), GetEnd(), GetPenWidth()/2 );
}

/* return true if the point aPosition is on the starting point of this item
 */
bool WS_DRAW_ITEM_LINE::HitTestStartPoint( const wxPoint& aPosition)
{
    wxPoint dist = GetStart() - aPosition;

    if( std::abs( dist.x) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 &&
        std::abs( dist.y) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 )
        return true;

    return false;
}

/* return true if the point aPosition is on the ending point of this item
 */
bool WS_DRAW_ITEM_LINE::HitTestEndPoint( const wxPoint& aPosition)
{
    wxPoint dist = GetEnd() - aPosition;

    if( std::abs( dist.x) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 &&
        std::abs( dist.y) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 )
        return true;

    return false;
}

/* Locate graphic items in m_graphicList at location aPosition
 * aList = the list of items found
 * aPosition is the position (in user units) to locate items
 */
void WS_DRAW_ITEM_LIST::Locate( std::vector <WS_DRAW_ITEM_BASE*>& aList,
                                const wxPoint& aPosition)
{
    for( WS_DRAW_ITEM_BASE* item = GetFirst(); item; item = GetNext() )
    {
        item->m_Flags &= ~(LOCATE_STARTPOINT|LOCATE_ENDPOINT);
        bool found = false;

        if( item->HitTestStartPoint ( aPosition ) )
        {
            item->m_Flags |= LOCATE_STARTPOINT;
            found = true;
        }

        if( item->HitTestEndPoint ( aPosition ) )
        {
            item->m_Flags |= LOCATE_ENDPOINT;
            found = true;
        }

        if( found || item->HitTest( aPosition ) )
        {
            aList.push_back( item );
        }
    }
}

/** The function to draw a WS_DRAW_ITEM_BITMAP
 */
void WS_DRAW_ITEM_BITMAP::DrawWsItem( EDA_RECT* aClipBox, wxDC* aDC )
{
    WORKSHEET_DATAITEM_BITMAP* parent = (WORKSHEET_DATAITEM_BITMAP*)GetParent();

    if( parent->m_ImageBitmap  )
    {
        parent->m_ImageBitmap->DrawBitmap( NULL, aDC, m_pos );
    }
}

/**
 * Virtual function
 * return true if the point aPosition is on bitmap
 */
bool WS_DRAW_ITEM_BITMAP::HitTest( const wxPoint& aPosition) const
{
    const WORKSHEET_DATAITEM_BITMAP* parent = static_cast<const WORKSHEET_DATAITEM_BITMAP*>( GetParent() );

    if( parent->m_ImageBitmap == NULL )
        return false;

    EDA_RECT rect = parent->m_ImageBitmap->GetBoundingBox();
    rect.Move( m_pos );
    return rect.Contains( aPosition );
}


/**
 * return true if the point aPosition is on the reference point of this item.
 */
bool WS_DRAW_ITEM_BITMAP::HitTestStartPoint( const wxPoint& aPosition)
{
    wxPoint dist = m_pos - aPosition;

    if( std::abs( dist.x) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 &&
        std::abs( dist.y) <= WORKSHEET_DATAITEM::GetMarkerSizeUi()/2 )
        return true;

    return false;
}

