/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
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

/*
 * the class WS_DATA_ITEM (and WS_DATA_ITEM_TEXT) defines
 * a basic shape of a page layout ( frame references and title block )
 * Basic shapes are line, rect and texts
 * the WS_DATA_ITEM coordinates units is the mm, and are relative to
 * one of 4 page corners.
 *
 * These items cannot be drawn or plot "as this". they should be converted
 * to a "draw list" (WS_DRAW_ITEM_BASE and derived items)

 * The list of these items is stored in a WS_DATA_MODEL instance.
 *
 * When building the draw list:
 * the WS_DATA_MODEL is used to create a WS_DRAW_ITEM_LIST
 *  coordinates are converted to draw/plot coordinates.
 *  texts are expanded if they contain format symbols.
 *  Items with m_RepeatCount > 1 are created m_RepeatCount times
 *
 * the WS_DATA_MODEL is created only once.
 * the WS_DRAW_ITEM_LIST is created each time the page layout is plotted/drawn
 *
 * the WS_DATA_MODEL instance is created from a S expression which
 * describes the page layout (can be the default page layout or a custom file).
 */

#include <fctsys.h>
#include <eda_rect.h>
#include <gr_text.h>
#include <ws_draw_item.h>
#include <ws_data_model.h>
#include <base_units.h>
#include <page_info.h>
#include <layers_id_colors_and_visibility.h>

// ============================ BASE CLASS ==============================

void WS_DRAW_ITEM_BASE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;

    WS_DATA_ITEM* dataItem = GetPeer();

    if( !dataItem )     // No peer: this item is like a WS_DRAW_ITEM_PAGE
    {
        aLayers[0] = LAYER_WORKSHEET;
        return;
    }

    if( dataItem->GetPage1Option() == FIRST_PAGE_ONLY )
        aLayers[0] = LAYER_WORKSHEET_PAGE1;
    else if( dataItem->GetPage1Option() == SUBSEQUENT_PAGES )
        aLayers[0] = LAYER_WORKSHEET_PAGEn;
    else
        aLayers[0] = LAYER_WORKSHEET;
}


// A generic HitTest that can be used by some, but not all, sub-classes.
bool WS_DRAW_ITEM_BASE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    return sel.Intersects( GetBoundingBox() );
}


void WS_DRAW_ITEM_BASE::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    wxString            msg;
    WS_DATA_ITEM* dataItem = GetPeer();

    if( dataItem == nullptr )   // Is only a pure graphic item used in page layout editor
                                // to handle the page limits
        return;

    switch( dataItem->GetType() )
    {
    case WS_DATA_ITEM::WS_SEGMENT:
        aList.push_back( MSG_PANEL_ITEM( _( "Line" ), msg, DARKCYAN ) );
        break;

    case WS_DATA_ITEM::WS_RECT:
        aList.push_back( MSG_PANEL_ITEM( _( "Rectangle" ), msg, DARKCYAN ) );
        break;

    case WS_DATA_ITEM::WS_TEXT:
        msg = static_cast<WS_DRAW_ITEM_TEXT*>( this )->GetShownText();
        aList.push_back( MSG_PANEL_ITEM( _( "Text" ), msg, DARKCYAN ) );
        break;

    case WS_DATA_ITEM::WS_POLYPOLYGON:
        aList.push_back( MSG_PANEL_ITEM( _( "Imported Shape" ), msg, DARKCYAN ) );
        break;

    case WS_DATA_ITEM::WS_BITMAP:
        aList.push_back( MSG_PANEL_ITEM( _( "Image" ), msg, DARKCYAN ) );
        break;
    }

    switch( dataItem->GetPage1Option() )
    {
    case FIRST_PAGE_ONLY:  msg = _( "First Page Only" ); break;
    case SUBSEQUENT_PAGES: msg = _( "Subsequent Pages" ); break;
    default:               msg = _( "All Pages" ); break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "First Page Option" ), msg, BROWN ) );

    msg = MessageTextFromValue( UNSCALED_UNITS, dataItem->m_RepeatCount );
    aList.push_back( MSG_PANEL_ITEM( _( "Repeat Count" ), msg, BLUE ) );

    msg = MessageTextFromValue( UNSCALED_UNITS, dataItem->m_IncrementLabel );
    aList.push_back( MSG_PANEL_ITEM( _( "Repeat Label Increment" ), msg, DARKGRAY ) );

    msg.Printf( wxT( "(%s, %s)" ),
                MessageTextFromValue( aUnits, dataItem->m_IncrementVector.x ),
                MessageTextFromValue( aUnits, dataItem->m_IncrementVector.y ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Repeat Position Increment" ), msg, RED ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Comment" ), dataItem->m_Info, MAGENTA ) );
}


// ============================ TEXT ==============================

void WS_DRAW_ITEM_TEXT::PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor )
{
    Print( aDC, aOffset, aColor, FILLED );
}


const EDA_RECT WS_DRAW_ITEM_TEXT::GetBoundingBox() const
{
    return EDA_TEXT::GetTextBox( -1 );
}


bool WS_DRAW_ITEM_TEXT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return EDA_TEXT::TextHitTest( aPosition, aAccuracy );
}


bool WS_DRAW_ITEM_TEXT::HitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    return EDA_TEXT::TextHitTest( aRect, aContains, aAccuracy );
}


wxString WS_DRAW_ITEM_TEXT::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Text %s at (%s, %s)" ),
                             GetShownText(),
                             MessageTextFromValue( aUnits, GetTextPos().x ),
                             MessageTextFromValue( aUnits, GetTextPos().y ) );
}


// ============================ POLYGON =================================

void WS_DRAW_ITEM_POLYPOLYGONS::PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor )
{
    std::vector<wxPoint> points_moved;

    for( int idx = 0; idx < m_Polygons.OutlineCount(); ++idx )
    {
        points_moved.clear();
        SHAPE_LINE_CHAIN& outline = m_Polygons.Outline( idx );

        for( int ii = 0; ii < outline.PointCount(); ii++ )
            points_moved.push_back( wxPoint( outline.Point( ii ).x + aOffset.x,
                                             outline.Point( ii ).y + aOffset.y ) );

        GRPoly( nullptr, aDC, points_moved.size(), &points_moved[0], FILLED_SHAPE,
                GetPenWidth(), aColor, aColor );
    }
}


void WS_DRAW_ITEM_POLYPOLYGONS::SetPosition( wxPoint aPos )
{
    // Note: m_pos is the anchor point of the shape.
    wxPoint move_vect = aPos - m_pos;
    m_pos = aPos;

    // Move polygon corners to the new position:
    m_Polygons.Move( move_vect );
}


const EDA_RECT WS_DRAW_ITEM_POLYPOLYGONS::GetBoundingBox() const
{
    EDA_RECT rect;
    BOX2I box = m_Polygons.BBox();

    rect.SetX( box.GetX() );
    rect.SetY( box.GetY() );
    rect.SetWidth( box.GetWidth() );
    rect.SetHeight( box.GetHeight() );

    return rect;
}


bool WS_DRAW_ITEM_POLYPOLYGONS::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return m_Polygons.Collide( aPosition, aAccuracy );
}


bool WS_DRAW_ITEM_POLYPOLYGONS::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    // Fast test: if rect is outside the polygon bounding box, then they cannot intersect
    if( !sel.Intersects( GetBoundingBox() ) )
        return false;

    for( int idx = 0; idx < m_Polygons.OutlineCount(); ++idx )
    {
        const SHAPE_LINE_CHAIN& outline = m_Polygons.COutline( idx );

        for( int ii = 0; ii < outline.PointCount(); ii++ )
        {
            wxPoint corner( outline.CPoint( ii ).x, outline.CPoint( ii ).y );

            // Test if the point is within aRect
            if( sel.Contains( corner ) )
                return true;

            // Test if this edge intersects aRect
            int ii_next = (ii+1) % outline.PointCount();
            wxPoint next_corner( outline.CPoint( ii_next ).x, outline.CPoint( ii_next ).y );

            if( sel.Intersects( corner, next_corner ) )
                return true;
        }
    }

    return false;
}


wxString WS_DRAW_ITEM_POLYPOLYGONS::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Imported shape at (%s, %s)" ),
                             MessageTextFromValue( aUnits, GetPosition().x ),
                             MessageTextFromValue( aUnits, GetPosition().y ) );
}


// ============================ RECT ==============================

void WS_DRAW_ITEM_RECT::PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor )
{
    GRRect( nullptr, aDC, GetStart().x + aOffset.x, GetStart().y + aOffset.y,
            GetEnd().x + aOffset.x, GetEnd().y + aOffset.y, GetPenWidth(), aColor );
}


const EDA_RECT WS_DRAW_ITEM_RECT::GetBoundingBox() const
{
    return EDA_RECT( GetStart(), wxSize( GetEnd().x - GetStart().x, GetEnd().y - GetStart().y ) );
}


bool WS_DRAW_ITEM_RECT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int dist = aAccuracy + ( GetPenWidth() / 2 );
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


wxString WS_DRAW_ITEM_RECT::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Rectangle from (%s, %s) to (%s, %s)" ),
                             MessageTextFromValue( aUnits, GetStart().x ),
                             MessageTextFromValue( aUnits, GetStart().y ),
                             MessageTextFromValue( aUnits, GetEnd().x ),
                             MessageTextFromValue( aUnits, GetEnd().y ) );
}


// ============================ LINE ==============================

void WS_DRAW_ITEM_LINE::PrintWsItem(  wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor )
{
    GRLine( nullptr, aDC, GetStart() + aOffset, GetEnd() + aOffset, GetPenWidth(), aColor );
}


const EDA_RECT WS_DRAW_ITEM_LINE::GetBoundingBox() const
{
    return EDA_RECT( GetStart(), wxSize( GetEnd().x - GetStart().x, GetEnd().y - GetStart().y ) );
}


bool WS_DRAW_ITEM_LINE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int mindist = aAccuracy + ( GetPenWidth() / 2 );
    return TestSegmentHit( aPosition, GetStart(), GetEnd(), mindist );
}


wxString WS_DRAW_ITEM_LINE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Line from (%s, %s) to (%s, %s)" ),
                             MessageTextFromValue( aUnits, GetStart().x ),
                             MessageTextFromValue( aUnits, GetStart().y ),
                             MessageTextFromValue( aUnits, GetEnd().x ),
                             MessageTextFromValue( aUnits, GetEnd().y ) );
}


// ============== BITMAP ================

void WS_DRAW_ITEM_BITMAP::PrintWsItem( wxDC* aDC, const wxPoint& aOffset, COLOR4D aColor )
{
    WS_DATA_ITEM_BITMAP* bitmap = (WS_DATA_ITEM_BITMAP*) GetPeer();

    if( bitmap->m_ImageBitmap  )
        bitmap->m_ImageBitmap->DrawBitmap( aDC, m_pos + aOffset );
}


const EDA_RECT WS_DRAW_ITEM_BITMAP::GetBoundingBox() const
{
    auto*    bitmap = static_cast<const WS_DATA_ITEM_BITMAP*>( m_peer );
    wxSize bm_size = bitmap->m_ImageBitmap->GetSize();

    // Size is in mils, convert to iu (0.001 mm)
    bm_size.x *= 25.4;
    bm_size.y *= 25.4;

    EDA_RECT bbox;
    bbox.SetSize( bm_size );
    bbox.SetOrigin( m_pos.x - bm_size.x/2, m_pos.y - bm_size.y/2 );

    return bbox;
}


bool WS_DRAW_ITEM_BITMAP::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bbox = GetBoundingBox();
    bbox.Inflate( aAccuracy );

    return bbox.Contains( aPosition );
}


bool WS_DRAW_ITEM_BITMAP::HitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    return WS_DRAW_ITEM_BASE::HitTest( aRect, aContains, aAccuracy );
}


wxString WS_DRAW_ITEM_BITMAP::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Image at (%s, %s)" ),
                             MessageTextFromValue( aUnits, GetPosition().x ),
                             MessageTextFromValue( aUnits, GetPosition().y ) );
}


wxString WS_DRAW_ITEM_PAGE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    wxString txt( "Page limits" );
    return txt;
}


const EDA_RECT WS_DRAW_ITEM_PAGE::GetBoundingBox() const
{
    EDA_RECT dummy;

    // We want this graphic item alway visible. So gives the max size to the
    // bounding box to avoid any clamping:
    dummy.SetSize( wxSize( std::numeric_limits<int>::max(), std::numeric_limits<int>::max() ) );
    return dummy;
}

// ====================== WS_DRAW_ITEM_LIST ==============================

void WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList( const PAGE_INFO& aPageInfo,
                                                   const TITLE_BLOCK& aTitleBlock )
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();

    m_titleBlock = &aTitleBlock;
    m_paperFormat = &aPageInfo.GetType();

    // Build the basic layout shape, if the layout list is empty
    if( model.GetCount() == 0 && !model.VoidListAllowed() )
        model.SetPageLayout();

    model.SetupDrawEnvironment( aPageInfo, m_milsToIu );

    for( WS_DATA_ITEM* wsItem : model.GetItems() )
    {
        // Generate it only if the page option allows this
        if( wsItem->GetPage1Option() == FIRST_PAGE_ONLY && m_sheetNumber != 1 )
            continue;
        else if( wsItem->GetPage1Option() == SUBSEQUENT_PAGES && m_sheetNumber == 1 )
            continue;

        wsItem->SyncDrawItems( this, nullptr );
    }
}


/* Print the item list created by BuildWorkSheetGraphicList
 * aDC = the current Device Context
 * The not selected items are drawn first (most of items)
 * The selected items are drawn after (usually 0 or 1)
 * to be sure they are seen, even for overlapping items
 */
void WS_DRAW_ITEM_LIST::Print( wxDC* aDC, COLOR4D aColor )
{
    for( WS_DRAW_ITEM_BASE* item = GetFirst(); item; item = GetNext() )
        item->PrintWsItem( aDC, aColor );
}


