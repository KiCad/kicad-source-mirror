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
 * the class WS_DATA_ITEM (and derived) defines
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
 * the WS_DRAW_ITEM_LIST is created each time the page layout is plot/drawn
 *
 * the WS_DATA_MODEL instance is created from a S expression which
 * describes the page layout (can be the default page layout or a custom file).
 */

#include <fctsys.h>
#include <gr_text.h>
#include <eda_rect.h>
#include <view/view.h>
#include <ws_painter.h>
#include <title_block.h>
#include <ws_draw_item.h>
#include <ws_data_model.h>

using KIGFX::COLOR4D;


// The constructor:
WS_DATA_ITEM::WS_DATA_ITEM( WS_ITEM_TYPE aType )
{
    m_pageOption = ALL_PAGES;
    m_type = aType;
    m_RepeatCount = 1;
    m_IncrementLabel = 1;
    m_LineWidth = 0;
}


void WS_DATA_ITEM::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    int pensize = GetPenSizeUi();

    if( pensize == 0 )
        pensize = aCollector ? aCollector->GetDefaultPenSize() : 0;

    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_BASE*          item = nullptr;

    for( size_t i = 0; i < m_drawItems.size(); ++i )
    {
        item = m_drawItems[ i ];
        itemFlags[ i ] = item->GetFlags();

        if( aCollector )
            aCollector->Remove( item );

        if( aView )
            aView->Remove( item );

        delete item;
    }

    m_drawItems.clear();

    for( int j = 0; j < m_RepeatCount; j++ )
    {
        if( j && ! IsInsidePage( j ) )
            continue;

        if( m_type == WS_SEGMENT )
            item = new WS_DRAW_ITEM_LINE( this, j, GetStartPosUi( j ), GetEndPosUi( j ), pensize );
        else if( m_type == WS_RECT )
            item = new WS_DRAW_ITEM_RECT( this, j, GetStartPosUi( j ), GetEndPosUi( j ), pensize );

        item->SetFlags( itemFlags[ j ] );
        m_drawItems.push_back( item );

        if( aCollector )
            aCollector->Append( item );

        if( aView )
            aView->Add( item );
    }
}


int WS_DATA_ITEM::GetPenSizeUi()
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();

    if( m_LineWidth != 0 )
        return KiROUND( m_LineWidth * model.m_WSunits2Iu );
    else
        return KiROUND( model.m_DefaultLineWidth * model.m_WSunits2Iu );
}


// move item to aPosition
// starting point is moved to aPosition
// the Ending point is moved to a position which keeps the item size
// (if both coordinates have the same corner reference)
// MoveToUi and MoveTo takes the graphic position (i.e relative to the left top
// paper corner
void WS_DATA_ITEM::MoveToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    pos_mm.y = aPosition.y / WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;

    MoveTo( pos_mm );
}


void WS_DATA_ITEM::MoveTo( DPOINT aPosition )
{
    DPOINT vector = aPosition - GetStartPos();
    DPOINT endpos = vector + GetEndPos();

    MoveStartPointTo( aPosition );
    MoveEndPointTo( endpos );

    for( WS_DRAW_ITEM_BASE* drawItem : m_drawItems )
    {
        drawItem->SetPosition( GetStartPosUi( drawItem->GetIndexInPeer() ) );
        drawItem->SetEnd( GetEndPosUi( drawItem->GetIndexInPeer() ) );
    }
}


/* move the starting point of the item to a new position
 * aPosition = the new position of the starting point, in mm
 */
void WS_DATA_ITEM::MoveStartPointTo( DPOINT aPosition )
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();
    DPOINT         position;

    // Calculate the position of the starting point
    // relative to the reference corner
    // aPosition is the position relative to the right top paper corner
    switch( m_Pos.m_Anchor )
    {
    case RB_CORNER:
        position = model.m_RB_Corner - aPosition;
        break;

    case RT_CORNER:
        position.x = model.m_RB_Corner.x - aPosition.x;
        position.y = aPosition.y - model.m_LT_Corner.y;
        break;

    case LB_CORNER:
        position.x = aPosition.x - model.m_LT_Corner.x;
        position.y = model.m_RB_Corner.y - aPosition.y;
        break;

    case LT_CORNER:
        position = aPosition - model.m_LT_Corner;
        break;
    }

    m_Pos.m_Pos = position;
}


/* move the starting point of the item to a new position
 * aPosition = the new position of the starting point in graphic units
 */
void WS_DATA_ITEM::MoveStartPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm( aPosition.x / WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu,
                   aPosition.y / WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );

    MoveStartPointTo( pos_mm );
}


/**
 * move the ending point of the item to a new position
 * has meaning only for items defined by 2 points
 * (segments and rectangles)
 * aPosition = the new position of the ending point, in mm
 */
void WS_DATA_ITEM::MoveEndPointTo( DPOINT aPosition )
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();
    DPOINT         position;

    // Calculate the position of the starting point
    // relative to the reference corner
    // aPosition is the position relative to the right top paper corner
    switch( m_End.m_Anchor )
    {
    case RB_CORNER:
        position = model.m_RB_Corner - aPosition;
        break;

    case RT_CORNER:
        position.x = model.m_RB_Corner.x - aPosition.x;
        position.y = aPosition.y - model.m_LT_Corner.y;
        break;

    case LB_CORNER:
        position.x = aPosition.x - model.m_LT_Corner.x;
        position.y = model.m_RB_Corner.y - aPosition.y;
        break;

    case LT_CORNER:
        position = aPosition - model.m_LT_Corner;
        break;
    }

    // Modify m_End only for items having 2 coordinates
    switch( GetType() )
    {
    case WS_SEGMENT:
    case WS_RECT:
        m_End.m_Pos = position;
        break;

    default:
        break;
    }
}


/* move the ending point of the item to a new position
 * has meaning only for items defined by 2 points
 * (segments and rectangles)
 * aPosition = the new position of the ending point in graphic units
 */
void WS_DATA_ITEM::MoveEndPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    pos_mm.y = aPosition.y / WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;

    MoveEndPointTo( pos_mm );
}


const DPOINT WS_DATA_ITEM::GetStartPos( int ii ) const
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();
    DPOINT         pos( m_Pos.m_Pos.x + ( m_IncrementVector.x * ii ),
                        m_Pos.m_Pos.y + ( m_IncrementVector.y * ii ) );

    switch( m_Pos.m_Anchor )
    {
        case RB_CORNER:      // right bottom corner
            pos = model.m_RB_Corner - pos;
            break;

        case RT_CORNER:      // right top corner
            pos.x = model.m_RB_Corner.x - pos.x;
            pos.y = model.m_LT_Corner.y + pos.y;
            break;

        case LB_CORNER:      // left bottom corner
            pos.x = model.m_LT_Corner.x + pos.x;
            pos.y = model.m_RB_Corner.y - pos.y;
            break;

        case LT_CORNER:      // left top corner
            pos = model.m_LT_Corner + pos;
            break;
    }

    return pos;
}


const wxPoint WS_DATA_ITEM::GetStartPosUi( int ii ) const
{
    DPOINT pos = GetStartPos( ii ) * WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    return wxPoint( KiROUND( pos.x ), KiROUND( pos.y ) );
}


const DPOINT WS_DATA_ITEM::GetEndPos( int ii ) const
{
    DPOINT pos( m_End.m_Pos.x + ( m_IncrementVector.x * ii ),
                m_End.m_Pos.y + ( m_IncrementVector.y * ii ) );

    switch( m_End.m_Anchor )
    {
    case RB_CORNER:      // right bottom corner
        pos = WS_DATA_MODEL::GetTheInstance().m_RB_Corner - pos;
        break;

    case RT_CORNER:      // right top corner
        pos.x = WS_DATA_MODEL::GetTheInstance().m_RB_Corner.x - pos.x;
        pos.y = WS_DATA_MODEL::GetTheInstance().m_LT_Corner.y + pos.y;
        break;

    case LB_CORNER:      // left bottom corner
        pos.x = WS_DATA_MODEL::GetTheInstance().m_LT_Corner.x + pos.x;
        pos.y = WS_DATA_MODEL::GetTheInstance().m_RB_Corner.y - pos.y;
        break;

    case LT_CORNER:      // left top corner
        pos = WS_DATA_MODEL::GetTheInstance().m_LT_Corner + pos;
        break;
    }

    return pos;
}


const wxPoint WS_DATA_ITEM::GetEndPosUi( int ii ) const
{
    DPOINT pos = GetEndPos( ii );
    pos = pos * WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    return wxPoint( KiROUND( pos.x ), KiROUND( pos.y ) );
}


bool WS_DATA_ITEM::IsInsidePage( int ii ) const
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();

    DPOINT pos = GetStartPos( ii );

    for( int kk = 0; kk < 1; kk++ )
    {
        if( model.m_RB_Corner.x < pos.x || model.m_LT_Corner.x > pos.x )
            return false;

        if( model.m_RB_Corner.y < pos.y || model.m_LT_Corner.y > pos.y )
            return false;

        pos = GetEndPos( ii );
    }

    return true;
}


const wxString WS_DATA_ITEM::GetClassName() const
{
    wxString name;

    switch( GetType() )
    {
        case WS_TEXT:        name = wxT( "Text" );           break;
        case WS_SEGMENT:     name = wxT( "Line" );           break;
        case WS_RECT:        name = wxT( "Rectangle" );      break;
        case WS_POLYPOLYGON: name = wxT( "Imported Shape" ); break;
        case WS_BITMAP:      name = wxT( "Image" );          break;
    }

    return name;
}


WS_DATA_ITEM_POLYGONS::WS_DATA_ITEM_POLYGONS() :
    WS_DATA_ITEM( WS_POLYPOLYGON )
{
    m_Orient = 0.0;
}


void WS_DATA_ITEM_POLYGONS::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_BASE*          item = nullptr;

    for( size_t i = 0; i < m_drawItems.size(); ++i )
    {
        item = m_drawItems[ i ];
        itemFlags[ i ] = item->GetFlags();

        if( aCollector )
            aCollector->Remove( item );

        if( aView )
            aView->Remove( item );

        delete item;
    }

    m_drawItems.clear();

    for( int j = 0; j < m_RepeatCount; j++ )
    {
        if( j && !IsInsidePage( j ) )
            continue;

        int pensize = GetPenSizeUi();
        auto poly_shape = new WS_DRAW_ITEM_POLYPOLYGONS( this, j, GetStartPosUi( j ), pensize );
        poly_shape->SetFlags( itemFlags[ j ] );
        m_drawItems.push_back( poly_shape );

        // Transfer all outlines (basic polygons)
        SHAPE_POLY_SET& polygons = poly_shape->GetPolygons();
        for( int kk = 0; kk < GetPolyCount(); kk++ )
        {
            // Create new outline
            unsigned ist = GetPolyIndexStart( kk );
            unsigned iend = GetPolyIndexEnd( kk );

            polygons.NewOutline();

            while( ist <= iend )
                polygons.Append( GetCornerPositionUi( ist++, j ) );
        }

        if( aCollector )
            aCollector->Append( poly_shape );

        if( aView )
            aView->Add( poly_shape );
    }
}


int WS_DATA_ITEM_POLYGONS::GetPenSizeUi()
{
    return KiROUND( m_LineWidth * WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );
}


const DPOINT WS_DATA_ITEM_POLYGONS::GetCornerPosition( unsigned aIdx, int aRepeat ) const
{
    DPOINT pos = m_Corners[aIdx];

    // Rotation:
    RotatePoint( &pos.x, &pos.y, m_Orient * 10 );
    pos += GetStartPos( aRepeat );
    return pos;
}


void WS_DATA_ITEM_POLYGONS::SetBoundingBox()
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


bool WS_DATA_ITEM_POLYGONS::IsInsidePage( int ii ) const
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();

    DPOINT pos = GetStartPos( ii );
    pos += m_minCoord;  // left top pos of bounding box

    if( model.m_LT_Corner.x > pos.x || model.m_LT_Corner.y > pos.y )
        return false;

    pos = GetStartPos( ii );
    pos += m_maxCoord;  // rignt bottom pos of bounding box

    if( model.m_RB_Corner.x < pos.x || model.m_RB_Corner.y < pos.y )
        return false;

    return true;
}


const wxPoint WS_DATA_ITEM_POLYGONS::GetCornerPositionUi( unsigned aIdx, int aRepeat ) const
{
    DPOINT pos = GetCornerPosition( aIdx, aRepeat );
    pos = pos * WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    return wxPoint( int(pos.x), int(pos.y) );
}


WS_DATA_ITEM_TEXT::WS_DATA_ITEM_TEXT( const wxString& aTextBase ) :
        WS_DATA_ITEM( WS_TEXT )
{
    m_TextBase = aTextBase;
    m_IncrementLabel = 1;
    m_Hjustify = GR_TEXT_HJUSTIFY_LEFT;
    m_Vjustify = GR_TEXT_VJUSTIFY_CENTER;
    m_Italic = false;
    m_Bold = false;
    m_Orient = 0.0;
    m_LineWidth = 0.0;      // 0 means use default value
}


void WS_DATA_ITEM_TEXT::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    int   pensize = GetPenSizeUi();
    bool  multilines = false;

    if( WS_DATA_MODEL::GetTheInstance().m_EditMode )
        m_FullText = m_TextBase;
    else
    {
        m_FullText = aCollector ? aCollector->BuildFullText( m_TextBase ) : wxString();
        multilines = ReplaceAntiSlashSequence();
    }

    if( pensize == 0 )
        pensize = aCollector ? aCollector->GetDefaultPenSize() : 1;

    SetConstrainedTextSize();
    wxSize textsize;

    textsize.x = KiROUND( m_ConstrainedTextSize.x * WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );
    textsize.y = KiROUND( m_ConstrainedTextSize.y * WS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );

    if( m_Bold )
        pensize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );

    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_TEXT*          text = nullptr;

    for( size_t i = 0; i < m_drawItems.size(); ++i )
    {
        text = (WS_DRAW_ITEM_TEXT*) m_drawItems[ i ];
        itemFlags[ i ] = text->GetFlags();

        if( aCollector )
            aCollector->Remove( text );

        if( aView )
            aView->Remove( text );

        delete text;
    }

    m_drawItems.clear();

    for( int j = 0; j < m_RepeatCount; ++j )
    {
        if( j > 0 && !IsInsidePage( j ) )
            continue;

        text = new WS_DRAW_ITEM_TEXT( this, j, m_FullText, GetStartPosUi( j ), textsize, pensize,
                                      m_Italic, m_Bold );
        text->SetFlags( itemFlags[ j ] );
        m_drawItems.push_back( text );

        if( aCollector )
            aCollector->Append( text );

        if( aView )
            aView->Add( text );

        text->SetHorizJustify( m_Hjustify ) ;
        text->SetVertJustify( m_Vjustify );
        text->SetTextAngle( m_Orient * 10 );    // graphic text orient unit = 0.1 degree
        text->SetMultilineAllowed( multilines );

        // Increment label for the next text (has no meaning for multiline texts)
        if( m_RepeatCount > 1 && !multilines )
            IncrementLabel(( j + 1 ) * m_IncrementLabel );
    }
}


int WS_DATA_ITEM_TEXT::GetPenSizeUi()
{
    WS_DATA_MODEL& model = WS_DATA_MODEL::GetTheInstance();

    if( m_LineWidth != 0 )
        return KiROUND( m_LineWidth * model.m_WSunits2Iu );
    else
        return KiROUND( model.m_DefaultTextThickness * model.m_WSunits2Iu );
}


void WS_DATA_ITEM_TEXT::IncrementLabel( int aIncr )
{
    int last = m_TextBase.Len() -1;

    wxChar lbchar = m_TextBase[last];
    m_FullText = m_TextBase;
    m_FullText.RemoveLast();

    if( lbchar >= '0' &&  lbchar <= '9' )
        // A number is expected:
        m_FullText << (int)( aIncr + lbchar - '0' );
    else
        m_FullText << (wxChar) ( aIncr + lbchar );
}


// Replace the '\''n' sequence by EOL
// and the sequence  '\''\' by only one '\' in m_FullText
// if m_FullText is a multiline text (i.e.contains '\n') return true
bool WS_DATA_ITEM_TEXT::ReplaceAntiSlashSequence()
{
    bool multiline = false;

    for( unsigned ii = 0; ii < m_FullText.Len(); ii++ )
    {
        if( m_FullText[ii] == '\n' )
            multiline = true;

        else if( m_FullText[ii] == '\\' )
        {
            if( ++ii >= m_FullText.Len() )
                break;

            if( m_FullText[ii] == '\\' )
            {
                // a double \\ sequence is replaced by a single \ char
                m_FullText.Remove(ii, 1);
                ii--;
            }
            else if( m_FullText[ii] == 'n' )
            {
                // Replace the "\n" sequence by a EOL char
                multiline = true;
                m_FullText[ii] = '\n';
                m_FullText.Remove(ii-1, 1);
                ii--;
            }
        }
    }

    return multiline;
}


void WS_DATA_ITEM_TEXT::SetConstrainedTextSize()
{
    m_ConstrainedTextSize = m_TextSize;

    if( m_ConstrainedTextSize.x == 0  )
        m_ConstrainedTextSize.x = WS_DATA_MODEL::GetTheInstance().m_DefaultTextSize.x;

    if( m_ConstrainedTextSize.y == 0 )
        m_ConstrainedTextSize.y = WS_DATA_MODEL::GetTheInstance().m_DefaultTextSize.y;

    if( m_BoundingBoxSize.x || m_BoundingBoxSize.y )
    {
        // to know the X and Y size of the line, we should use
        // EDA_TEXT::GetTextBox()
        // but this function uses integers
        // So, to avoid truncations with our unit in mm, use microns.
        wxSize size_micron;
        #define FSCALE 1000.0
        int linewidth = 0;
        size_micron.x = KiROUND( m_ConstrainedTextSize.x * FSCALE );
        size_micron.y = KiROUND( m_ConstrainedTextSize.y * FSCALE );
        WS_DRAW_ITEM_TEXT dummy( WS_DRAW_ITEM_TEXT( this, 0, this->m_FullText, wxPoint( 0, 0 ),
                                 size_micron, linewidth, m_Italic, m_Bold ) );
        dummy.SetMultilineAllowed( true );
        dummy.SetHorizJustify( m_Hjustify ) ;
        dummy.SetVertJustify( m_Vjustify );
        dummy.SetTextAngle( m_Orient * 10 );

        EDA_RECT rect = dummy.GetTextBox();
        DSIZE size;
        size.x = rect.GetWidth() / FSCALE;
        size.y = rect.GetHeight() / FSCALE;

        if( m_BoundingBoxSize.x && size.x > m_BoundingBoxSize.x )
            m_ConstrainedTextSize.x *= m_BoundingBoxSize.x / size.x;

        if( m_BoundingBoxSize.y &&  size.y > m_BoundingBoxSize.y )
            m_ConstrainedTextSize.y *= m_BoundingBoxSize.y / size.y;
    }
}


void WS_DATA_ITEM_BITMAP::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_BASE*          item = nullptr;

    for( size_t i = 0; i < m_drawItems.size(); ++i )
    {
        item = m_drawItems[ i ];
        itemFlags[ i ] = item->GetFlags();

        if( aCollector )
            aCollector->Remove( item );

        if( aView )
            aView->Remove( item );

        delete item;
    }

    m_drawItems.clear();

    for( int j = 0; j < m_RepeatCount; j++ )
    {
        if( j && !IsInsidePage( j ) )
            continue;

        auto bitmap = new WS_DRAW_ITEM_BITMAP( this, j, GetStartPosUi( j ) );
        bitmap->SetFlags( itemFlags[ j ] );
        m_drawItems.push_back( bitmap );

        if( aCollector )
            aCollector->Append( bitmap );

        if( aView )
            aView->Add( bitmap );
    }
}


int WS_DATA_ITEM_BITMAP::GetPPI() const
{
    if( m_ImageBitmap )
        return m_ImageBitmap->GetPPI() / m_ImageBitmap->GetScale();

    return 300;
}


void WS_DATA_ITEM_BITMAP::SetPPI( int aBitmapPPI )
{
    if( m_ImageBitmap )
        m_ImageBitmap->SetScale( (double) m_ImageBitmap->GetPPI() / aBitmapPPI );
}
