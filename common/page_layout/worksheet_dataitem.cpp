/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * the class WORKSHEET_DATAITEM (and derived) defines
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
#include <draw_graphic_text.h>
#include <eda_rect.h>
#include <worksheet_painter.h>
#include <title_block.h>
#include <ws_draw_item.h>
#include <worksheet_dataitem.h>
#include <view/view.h>

using KIGFX::COLOR4D;


// Static members of class WORKSHEET_DATAITEM:
double WORKSHEET_DATAITEM::m_WSunits2Iu = 1000.0;
DPOINT WORKSHEET_DATAITEM::m_RB_Corner;
DPOINT WORKSHEET_DATAITEM::m_LT_Corner;
double WORKSHEET_DATAITEM::m_DefaultLineWidth = 0.0;
DSIZE  WORKSHEET_DATAITEM::m_DefaultTextSize( TB_DEFAULT_TEXTSIZE, TB_DEFAULT_TEXTSIZE );
double WORKSHEET_DATAITEM::m_DefaultTextThickness = 0.0;
bool WORKSHEET_DATAITEM::m_SpecialMode = false;


// The constructor:
WORKSHEET_DATAITEM::WORKSHEET_DATAITEM( WS_ITEM_TYPE aType )
{
    m_pageOption = ALL_PAGES;
    m_type = aType;
    m_RepeatCount = 1;
    m_IncrementLabel = 1;
    m_LineWidth = 0;
}


void WORKSHEET_DATAITEM::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    int pensize = GetPenSizeUi();

    if( pensize == 0 )
        pensize = aCollector ? aCollector->GetDefaultPenSize() : 0;

    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_BASE*          item = nullptr;

    for( size_t ii = 0; ii < m_drawItems.size(); ++ii )
    {
        item = m_drawItems[ ii ];
        itemFlags[ ii ] = item->GetFlags();

        if( aCollector )
            aCollector->Remove( item );

        if( aView )
            aView->Remove( item );

        delete item;
    }

    m_drawItems.clear();

    for( int jj = 0; jj < m_RepeatCount; jj++ )
    {
        if( jj && ! IsInsidePage( jj ) )
            continue;

        if( m_type == WS_SEGMENT )
            item = new WS_DRAW_ITEM_LINE( this, GetStartPosUi( jj ), GetEndPosUi( jj ), pensize );
        else if( m_type == WS_RECT )
            item = new WS_DRAW_ITEM_RECT( this, GetStartPosUi( jj ), GetEndPosUi( jj ), pensize );

        item->SetFlags( itemFlags[ jj ] );
        m_drawItems.push_back( item );

        if( aCollector )
            aCollector->Append( item );

        if( aView )
            aView->Add( item );
    }
}


// move item to aPosition
// starting point is moved to aPosition
// the Ending point is moved to a position which keeps the item size
// (if both coordinates have the same corner reference)
// MoveToUi and MoveTo takes the graphic position (i.e relative to the left top
// paper corner
void WORKSHEET_DATAITEM::MoveToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / m_WSunits2Iu;
    pos_mm.y = aPosition.y / m_WSunits2Iu;

    MoveTo( pos_mm );
}


void WORKSHEET_DATAITEM::MoveTo( DPOINT aPosition )
{
    DPOINT vector = aPosition - GetStartPos();
    DPOINT endpos = vector + GetEndPos();

    MoveStartPointTo( aPosition );
    MoveEndPointTo( endpos );

    for( int jj = 0; jj < m_RepeatCount; jj++ )
    {
        if( jj && !IsInsidePage( jj ) )
            continue;

        WS_DRAW_ITEM_BASE* drawItem = m_drawItems[ jj ];
        drawItem->SetPosition( GetStartPosUi( jj ) );
        drawItem->SetEnd( GetEndPosUi( jj ) );
    }
}


/* move the starting point of the item to a new position
 * aPosition = the new position of the starting point, in mm
 */
void WORKSHEET_DATAITEM::MoveStartPointTo( DPOINT aPosition )
{
    DPOINT position;

    // Calculate the position of the starting point
    // relative to the reference corner
    // aPosition is the position relative to the right top paper corner
    switch( m_Pos.m_Anchor )
    {
    case RB_CORNER:
        position = m_RB_Corner - aPosition;
        break;

    case RT_CORNER:
        position.x = m_RB_Corner.x - aPosition.x;
        position.y = aPosition.y - m_LT_Corner.y;
        break;

    case LB_CORNER:
        position.x = aPosition.x - m_LT_Corner.x;
        position.y = m_RB_Corner.y - aPosition.y;
        break;

    case LT_CORNER:
        position = aPosition - m_LT_Corner;
        break;
    }

    m_Pos.m_Pos = position;
}


/* move the starting point of the item to a new position
 * aPosition = the new position of the starting point in graphic units
 */
void WORKSHEET_DATAITEM::MoveStartPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / m_WSunits2Iu;
    pos_mm.y = aPosition.y / m_WSunits2Iu;

    MoveStartPointTo( pos_mm );
}


/**
 * move the ending point of the item to a new position
 * has meaning only for items defined by 2 points
 * (segments and rectangles)
 * aPosition = the new position of the ending point, in mm
 */
void WORKSHEET_DATAITEM::MoveEndPointTo( DPOINT aPosition )
{
    DPOINT position;

    // Calculate the position of the starting point
    // relative to the reference corner
    // aPosition is the position relative to the right top paper corner
    switch( m_End.m_Anchor )
    {
    case RB_CORNER:
        position = m_RB_Corner - aPosition;
        break;

    case RT_CORNER:
        position.x = m_RB_Corner.x - aPosition.x;
        position.y = aPosition.y - m_LT_Corner.y;
        break;

    case LB_CORNER:
        position.x = aPosition.x - m_LT_Corner.x;
        position.y = m_RB_Corner.y - aPosition.y;
        break;

    case LT_CORNER:
        position = aPosition - m_LT_Corner;
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
void WORKSHEET_DATAITEM::MoveEndPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / m_WSunits2Iu;
    pos_mm.y = aPosition.y / m_WSunits2Iu;

    MoveEndPointTo( pos_mm );
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
    return wxPoint( KiROUND( pos.x ), KiROUND( pos.y ) );
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
    return wxPoint( KiROUND( pos.x ), KiROUND( pos.y ) );
}


bool WORKSHEET_DATAITEM::IsInsidePage( int ii ) const
{
    DPOINT pos = GetStartPos( ii );

    for( int kk = 0; kk < 1; kk++ )
    {
        if( m_RB_Corner.x < pos.x || m_LT_Corner.x > pos.x )
            return false;

        if( m_RB_Corner.y < pos.y || m_LT_Corner.y > pos.y )
            return false;

        pos = GetEndPos( ii );
    }

    return true;
}


const wxString WORKSHEET_DATAITEM::GetClassName() const
{
    wxString name;

    switch( GetType() )
    {
        case WS_TEXT: name = wxT( "Text" ); break;
        case WS_SEGMENT: name = wxT( "Line" ); break;
        case WS_RECT: name = wxT( "Rectangle" ); break;
        case WS_POLYPOLYGON: name = wxT( "Imported Shape" ); break;
        case WS_BITMAP: name = wxT( "Image" ); break;
    }

    return name;
}


WORKSHEET_DATAITEM_POLYPOLYGON::WORKSHEET_DATAITEM_POLYPOLYGON() :
    WORKSHEET_DATAITEM( WS_POLYPOLYGON )
{
    m_Orient = 0.0;
}


void WORKSHEET_DATAITEM_POLYPOLYGON::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector,
                                                    KIGFX::VIEW* aView )
{
    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_BASE*          item = nullptr;

    for( size_t ii = 0; ii < m_drawItems.size(); ++ii )
    {
        item = m_drawItems[ ii ];
        itemFlags[ ii ] = item->GetFlags();

        if( aCollector )
            aCollector->Remove( item );

        if( aView )
            aView->Remove( item );

        delete item;
    }

    m_drawItems.clear();

    for( int jj = 0; jj < m_RepeatCount; jj++ )
    {
        if( jj && !IsInsidePage( jj ) )
            continue;

        for( int kk = 0; kk < GetPolyCount(); kk++ )
        {
            const bool fill = true;
            int pensize = GetPenSizeUi();
            auto poly = new WS_DRAW_ITEM_POLYGON( this, GetStartPosUi( jj ), fill, pensize );
            item->SetFlags( itemFlags[ jj ] );
            m_drawItems.push_back( poly );

            if( aCollector )
                aCollector->Append( poly );

            if( aView )
                aView->Add( poly );

            // Create polygon outline
            unsigned ist = GetPolyIndexStart( kk );
            unsigned iend = GetPolyIndexEnd( kk );

            while( ist <= iend )
                poly->m_Corners.push_back( GetCornerPositionUi( ist++, jj ) );

        }
    }
}


const DPOINT WORKSHEET_DATAITEM_POLYPOLYGON::GetCornerPosition( unsigned aIdx, int aRepeat ) const
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


WORKSHEET_DATAITEM_TEXT::WORKSHEET_DATAITEM_TEXT( const wxString& aTextBase ) :
    WORKSHEET_DATAITEM( WS_TEXT )
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


void WORKSHEET_DATAITEM_TEXT::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    int   pensize = GetPenSizeUi();
    bool  multilines = false;

    if( m_SpecialMode )
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

    textsize.x = KiROUND( m_ConstrainedTextSize.x * WORKSHEET_DATAITEM::m_WSunits2Iu );
    textsize.y = KiROUND( m_ConstrainedTextSize.y * WORKSHEET_DATAITEM::m_WSunits2Iu );

    if( m_Bold )
        pensize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );

    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_TEXT*          text = nullptr;

    for( size_t ii = 0; ii < m_drawItems.size(); ++ii )
    {
        text = (WS_DRAW_ITEM_TEXT*) m_drawItems[ ii ];
        itemFlags[ ii ] = text->GetFlags();

        if( aCollector )
            aCollector->Remove( text );

        if( aView )
            aView->Remove( text );

        delete text;
    }

    m_drawItems.clear();

    for( int jj = 0; jj < m_RepeatCount; ++jj )
    {
        if( jj > 0 && !IsInsidePage( jj ) )
            continue;

        text = new WS_DRAW_ITEM_TEXT( this, m_FullText, GetStartPosUi( jj ), textsize, pensize,
                                      m_Italic, m_Bold );
        text->SetFlags( itemFlags[ jj ] );
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
            IncrementLabel( ( jj + 1 ) * m_IncrementLabel );
    }
}


void WORKSHEET_DATAITEM_TEXT::IncrementLabel( int aIncr )
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
bool WORKSHEET_DATAITEM_TEXT::ReplaceAntiSlashSequence()
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


void WORKSHEET_DATAITEM_TEXT::SetConstrainedTextSize()
{
    m_ConstrainedTextSize = m_TextSize;

    if( m_ConstrainedTextSize.x == 0  )
        m_ConstrainedTextSize.x = m_DefaultTextSize.x;

    if( m_ConstrainedTextSize.y == 0 )
        m_ConstrainedTextSize.y = m_DefaultTextSize.y;

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
        WS_DRAW_ITEM_TEXT dummy( WS_DRAW_ITEM_TEXT( this, this->m_FullText, wxPoint( 0, 0 ),
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


void WORKSHEET_DATAITEM_BITMAP::SyncDrawItems( WS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    std::map<int, STATUS_FLAGS> itemFlags;
    WS_DRAW_ITEM_BASE*          item = nullptr;

    for( size_t ii = 0; ii < m_drawItems.size(); ++ii )
    {
        item = m_drawItems[ ii ];
        itemFlags[ ii ] = item->GetFlags();

        if( aCollector )
            aCollector->Remove( item );

        if( aView )
            aView->Remove( item );

        delete item;
    }

    m_drawItems.clear();

    for( int jj = 0; jj < m_RepeatCount; jj++ )
    {
        if( jj && !IsInsidePage( jj ) )
            continue;

        auto bitmap = new WS_DRAW_ITEM_BITMAP( this, GetStartPosUi( jj ) );
        item->SetFlags( itemFlags[ jj ] );
        m_drawItems.push_back( bitmap );

        if( aCollector )
            aCollector->Append( bitmap );

        if( aView )
            aView->Add( bitmap );
    }
}


/* set the pixel scale factor of the bitmap
 * this factor depend on the application internal unit
 * and the PPI bitmap factor
 * the pixel scale factor should be initialized before drawing the bitmap
 */
void WORKSHEET_DATAITEM_BITMAP::SetPixelScaleFactor()
{
    if( m_ImageBitmap )
    {
        // m_WSunits2Iu is the page layout unit to application internal unit
        // i.e. the mm to to application internal unit
        // however the bitmap definition is always known in pixels per inches
        double scale = m_WSunits2Iu * 25.4 / m_ImageBitmap->GetPPI();
        m_ImageBitmap->SetPixelScaleFactor( scale );
    }
}


/* return the PPI of the bitmap
 */
int WORKSHEET_DATAITEM_BITMAP::GetPPI() const
{
    if( m_ImageBitmap )
        return m_ImageBitmap->GetPPI() / m_ImageBitmap->GetScale();

    return 300;
}


/*adjust the PPI of the bitmap
 */
void WORKSHEET_DATAITEM_BITMAP::SetPPI( int aBitmapPPI )
{
    if( m_ImageBitmap )
        m_ImageBitmap->SetScale( (double) m_ImageBitmap->GetPPI() / aBitmapPPI );
}
