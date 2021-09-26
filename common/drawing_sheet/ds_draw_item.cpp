/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
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

/*
 * the class DS_DATA_ITEM (and DS_DATA_ITEM_TEXT) defines
 * a basic shape of a drawing sheet (frame references and title block)
 * Basic shapes are line, rect and texts
 * the DS_DATA_ITEM coordinates units is the mm, and are relative to
 * one of 4 page corners.
 *
 * These items cannot be drawn or plot "as this". they should be converted
 * to a "draw list" (DS_DRAW_ITEM_BASE and derived items)

 * The list of these items is stored in a DS_DATA_MODEL instance.
 *
 * When building the draw list:
 * the DS_DATA_MODEL is used to create a DS_DRAW_ITEM_LIST
 *  coordinates are converted to draw/plot coordinates.
 *  texts are expanded if they contain format symbols.
 *  Items with m_RepeatCount > 1 are created m_RepeatCount times
 *
 * the DS_DATA_MODEL is created only once.
 * the DS_DRAW_ITEM_LIST is created each time the drawing sheet is plotted/drawn
 *
 * the DS_DATA_MODEL instance is created from a S expression which
 * describes the drawing sheet (can be the default drawing sheet or a custom file).
 */

#include <eda_rect.h>
#include <eda_draw_frame.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <base_units.h>
#include <page_info.h>
#include <layer_ids.h>
#include <gr_basic.h>
#include <trigo.h>
#include <render_settings.h>

// ============================ BASE CLASS ==============================

void DS_DRAW_ITEM_BASE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;

    DS_DATA_ITEM* dataItem = GetPeer();

    if( !dataItem )     // No peer: this item is like a DS_DRAW_ITEM_PAGE
    {
        aLayers[0] = LAYER_DRAWINGSHEET;
        return;
    }

    if( dataItem->GetPage1Option() == FIRST_PAGE_ONLY )
        aLayers[0] = LAYER_DRAWINGSHEET_PAGE1;
    else if( dataItem->GetPage1Option() == SUBSEQUENT_PAGES )
        aLayers[0] = LAYER_DRAWINGSHEET_PAGEn;
    else
        aLayers[0] = LAYER_DRAWINGSHEET;
}


// A generic HitTest that can be used by some, but not all, sub-classes.
bool DS_DRAW_ITEM_BASE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    return sel.Intersects( GetBoundingBox() );
}


void DS_DRAW_ITEM_BASE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                         std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString      msg;
    DS_DATA_ITEM* dataItem = GetPeer();

    if( dataItem == nullptr )   // Is only a pure graphic item used in drawing sheet editor to
                                // handle the page limits
        return;

    switch( dataItem->GetType() )
    {
    case DS_DATA_ITEM::DS_SEGMENT:
        aList.push_back( MSG_PANEL_ITEM( _( "Line" ), msg ) );
        break;

    case DS_DATA_ITEM::DS_RECT:
        aList.push_back( MSG_PANEL_ITEM( _( "Rectangle" ), msg ) );
        break;

    case DS_DATA_ITEM::DS_TEXT:
        msg = static_cast<DS_DRAW_ITEM_TEXT*>( this )->GetShownText();
        aList.push_back( MSG_PANEL_ITEM( _( "Text" ), msg ) );
        break;

    case DS_DATA_ITEM::DS_POLYPOLYGON:
        aList.push_back( MSG_PANEL_ITEM( _( "Imported Shape" ), msg ) );
        break;

    case DS_DATA_ITEM::DS_BITMAP:
        aList.push_back( MSG_PANEL_ITEM( _( "Image" ), msg ) );
        break;
    }

    switch( dataItem->GetPage1Option() )
    {
    case FIRST_PAGE_ONLY:  msg = _( "First Page Only" );  break;
    case SUBSEQUENT_PAGES: msg = _( "Subsequent Pages" ); break;
    default:               msg = _( "All Pages" );        break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "First Page Option" ), msg ) );

    msg = MessageTextFromValue( EDA_UNITS::UNSCALED, dataItem->m_RepeatCount );
    aList.push_back( MSG_PANEL_ITEM( _( "Repeat Count" ), msg ) );

    msg = MessageTextFromValue( EDA_UNITS::UNSCALED, dataItem->m_IncrementLabel );
    aList.push_back( MSG_PANEL_ITEM( _( "Repeat Label Increment" ), msg ) );

    msg.Printf( wxT( "(%s, %s)" ),
                MessageTextFromValue( aFrame->GetUserUnits(), dataItem->m_IncrementVector.x ),
                MessageTextFromValue( aFrame->GetUserUnits(), dataItem->m_IncrementVector.y ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Repeat Position Increment" ), msg ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Comment" ), dataItem->m_Info ) );
}


// ============================ TEXT ==============================

void DS_DRAW_ITEM_TEXT::PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    Print( aSettings, aOffset, aSettings->GetLayerColor( LAYER_DRAWINGSHEET ), FILLED );
}


const EDA_RECT DS_DRAW_ITEM_TEXT::GetBoundingBox() const
{
    return EDA_TEXT::GetTextBox();
}


bool DS_DRAW_ITEM_TEXT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return EDA_TEXT::TextHitTest( aPosition, aAccuracy );
}


bool DS_DRAW_ITEM_TEXT::HitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    return EDA_TEXT::TextHitTest( aRect, aContains, aAccuracy );
}


wxString DS_DRAW_ITEM_TEXT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Text '%s'" ),
                             GetShownText() );
}


void DS_DRAW_ITEM_TEXT::SetTextAngle( double aAngle )
{
    EDA_TEXT::SetTextAngle( NormalizeAngle360Min( aAngle ) );
}

// ============================ POLYGON =================================

void DS_DRAW_ITEM_POLYPOLYGONS::PrintWsItem( const RENDER_SETTINGS* aSettings,
                                             const wxPoint& aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( LAYER_DRAWINGSHEET );
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    std::vector<wxPoint> points_moved;

    for( int idx = 0; idx < m_Polygons.OutlineCount(); ++idx )
    {
        points_moved.clear();
        SHAPE_LINE_CHAIN& outline = m_Polygons.Outline( idx );

        for( int ii = 0; ii < outline.PointCount(); ii++ )
        {
            points_moved.emplace_back( outline.CPoint( ii ).x + aOffset.x,
                                       outline.CPoint( ii ).y + aOffset.y );
        }

        GRPoly( nullptr, DC, points_moved.size(), &points_moved[0], true, penWidth,
                color, color );
    }
}


void DS_DRAW_ITEM_POLYPOLYGONS::SetPosition( const wxPoint& aPos )
{
    // Note: m_pos is the anchor point of the shape.
    wxPoint move_vect = aPos - m_pos;
    m_pos = aPos;

    // Move polygon corners to the new position:
    m_Polygons.Move( move_vect );
}


const EDA_RECT DS_DRAW_ITEM_POLYPOLYGONS::GetBoundingBox() const
{
    EDA_RECT rect;
    BOX2I box = m_Polygons.BBox();

    rect.SetX( box.GetX() );
    rect.SetY( box.GetY() );
    rect.SetWidth( box.GetWidth() );
    rect.SetHeight( box.GetHeight() );

    return rect;
}


bool DS_DRAW_ITEM_POLYPOLYGONS::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return m_Polygons.Collide( aPosition, aAccuracy );
}


bool DS_DRAW_ITEM_POLYPOLYGONS::HitTest( const EDA_RECT& aRect, bool aContained,
                                         int aAccuracy ) const
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


wxString DS_DRAW_ITEM_POLYPOLYGONS::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Imported Shape" ) );
}


// ============================ RECT ==============================

void DS_DRAW_ITEM_RECT::PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( LAYER_DRAWINGSHEET );
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    GRRect( nullptr, DC, GetStart().x + aOffset.x, GetStart().y + aOffset.y,
            GetEnd().x + aOffset.x, GetEnd().y + aOffset.y, penWidth, color );
}


const EDA_RECT DS_DRAW_ITEM_RECT::GetBoundingBox() const
{
    return EDA_RECT( GetStart(), wxSize( GetEnd().x - GetStart().x, GetEnd().y - GetStart().y ) );
}


bool DS_DRAW_ITEM_RECT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
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


bool DS_DRAW_ITEM_RECT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT sel = aRect;

    if ( aAccuracy )
        sel.Inflate( aAccuracy );

    if( aContained )
        return sel.Contains( GetBoundingBox() );

    // For greedy we need to check each side of the rect as we're pretty much always inside the
    // rect which defines the drawing-sheet frame.
    EDA_RECT side = GetBoundingBox();
    side.SetHeight( 0 );

    if( sel.Intersects( side ) )
        return true;

    side.SetY( GetBoundingBox().GetBottom() );

    if( sel.Intersects( side ) )
        return true;

    side = GetBoundingBox();
    side.SetWidth( 0 );

    if( sel.Intersects( side ) )
        return true;

    side.SetX( GetBoundingBox().GetRight() );

    if( sel.Intersects( side ) )
        return true;

    return false;
}


wxString DS_DRAW_ITEM_RECT::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Rectangle, width %s height %s" ),
                             MessageTextFromValue( aUnits, std::abs( GetStart().x - GetEnd().x ) ),
                             MessageTextFromValue( aUnits, std::abs( GetStart().y - GetEnd().y ) ) );
}


// ============================ LINE ==============================

void DS_DRAW_ITEM_LINE::PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = aSettings->GetLayerColor( LAYER_DRAWINGSHEET );
    int     penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    GRLine( nullptr, DC, GetStart() + aOffset, GetEnd() + aOffset, penWidth, color );
}


const EDA_RECT DS_DRAW_ITEM_LINE::GetBoundingBox() const
{
    return EDA_RECT( GetStart(), wxSize( GetEnd().x - GetStart().x, GetEnd().y - GetStart().y ) );
}


bool DS_DRAW_ITEM_LINE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int mindist = aAccuracy + ( GetPenWidth() / 2 ) + 1;
    return TestSegmentHit( aPosition, GetStart(), GetEnd(), mindist );
}


wxString DS_DRAW_ITEM_LINE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Line, length %s" ),
                             MessageTextFromValue( aUnits, EuclideanNorm( GetStart() - GetEnd() ) ) );
}


// ============== BITMAP ================

void DS_DRAW_ITEM_BITMAP::PrintWsItem( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    DS_DATA_ITEM_BITMAP* bitmap = (DS_DATA_ITEM_BITMAP*) GetPeer();

    if( !bitmap->m_ImageBitmap )
        return;

    bitmap->m_ImageBitmap->DrawBitmap( aSettings->GetPrintDC(), m_pos + aOffset );
}


const EDA_RECT DS_DRAW_ITEM_BITMAP::GetBoundingBox() const
{
    auto*    bitmap = static_cast<const DS_DATA_ITEM_BITMAP*>( m_peer );
    wxSize bm_size = bitmap->m_ImageBitmap->GetSize();

    EDA_RECT bbox;
    bbox.SetSize( bm_size );
    bbox.SetOrigin( m_pos.x - bm_size.x/2, m_pos.y - bm_size.y/2 );

    return bbox;
}


bool DS_DRAW_ITEM_BITMAP::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT bbox = GetBoundingBox();
    bbox.Inflate( aAccuracy );

    return bbox.Contains( aPosition );
}


bool DS_DRAW_ITEM_BITMAP::HitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    return DS_DRAW_ITEM_BASE::HitTest( aRect, aContains, aAccuracy );
}


wxString DS_DRAW_ITEM_BITMAP::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return _( "Image" );
}


wxString DS_DRAW_ITEM_PAGE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return _( "Page Limits" );
}


const EDA_RECT DS_DRAW_ITEM_PAGE::GetBoundingBox() const
{
    EDA_RECT dummy;

    // We want this graphic item always visible. So gives the max size to the
    // bounding box to avoid any clamping:
    dummy.SetSize( wxSize( std::numeric_limits<int>::max(), std::numeric_limits<int>::max() ) );
    return dummy;
}


// ====================== DS_DRAW_ITEM_LIST ==============================

void DS_DRAW_ITEM_LIST::BuildDrawItemsList( const PAGE_INFO& aPageInfo,
                                            const TITLE_BLOCK& aTitleBlock )
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

    m_titleBlock = &aTitleBlock;
    m_paperFormat = &aPageInfo.GetType();

    // Build the basic layout shape, if the layout list is empty
    if( model.GetCount() == 0 && !model.VoidListAllowed() )
        model.LoadDrawingSheet();

    model.SetupDrawEnvironment( aPageInfo, m_milsToIu );

    for( DS_DATA_ITEM* wsItem : model.GetItems() )
    {
        // Generate it only if the page option allows this
        if( wsItem->GetPage1Option() == FIRST_PAGE_ONLY && !m_isFirstPage )
            continue;
        else if( wsItem->GetPage1Option() == SUBSEQUENT_PAGES && m_isFirstPage )
            continue;

        wsItem->SyncDrawItems( this, nullptr );
    }
}


/* Print the item list created by BuildDrawItemsList
 * aDC = the current Device Context
 * The not selected items are drawn first (most of items)
 * The selected items are drawn after (usually 0 or 1)
 * to be sure they are seen, even for overlapping items
 */
void DS_DRAW_ITEM_LIST::Print( const RENDER_SETTINGS* aSettings )
{
    std::vector<DS_DRAW_ITEM_BASE*> second_items;

    for( DS_DRAW_ITEM_BASE* item = GetFirst(); item; item = GetNext() )
    {
        if( item->Type() == WSG_BITMAP_T )
            item->PrintWsItem( aSettings );
        else
            second_items.push_back( item );
    }

    for( auto item : second_items )
        item->PrintWsItem( aSettings );
}


