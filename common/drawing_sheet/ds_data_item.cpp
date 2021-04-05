/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * the class DS_DATA_ITEM (and derived) defines
 * a basic shape of a page layout ( frame references and title block )
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
 * the DS_DRAW_ITEM_LIST is created each time the page layout is plot/drawn
 *
 * the DS_DATA_MODEL instance is created from a S expression which
 * describes the page layout (can be the default page layout or a custom file).
 */

#include <gr_text.h>
#include <eda_rect.h>
#include <math/util.h>      // for KiROUND
#include <view/view.h>
#include <title_block.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_painter.h>
#include <trigo.h>

using KIGFX::COLOR4D;


// The constructor:
DS_DATA_ITEM::DS_DATA_ITEM( DS_ITEM_TYPE aType )
{
    m_pageOption = ALL_PAGES;
    m_type = aType;
    m_RepeatCount = 1;
    m_IncrementLabel = 1;
    m_LineWidth = 0;
}


DS_DATA_ITEM::~DS_DATA_ITEM()
{
    for( DS_DRAW_ITEM_BASE* item : m_drawItems )
        delete item;
}


void DS_DATA_ITEM::SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    int pensize = GetPenSizeUi();

    if( pensize == 0 )
        pensize = aCollector ? aCollector->GetDefaultPenSize() : 0;

    std::map<int, STATUS_FLAGS> itemFlags;
    DS_DRAW_ITEM_BASE*          item = nullptr;

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

        if( m_type == DS_SEGMENT )
            item = new DS_DRAW_ITEM_LINE( this, j, GetStartPosUi( j ), GetEndPosUi( j ), pensize );
        else if( m_type == DS_RECT )
            item = new DS_DRAW_ITEM_RECT( this, j, GetStartPosUi( j ), GetEndPosUi( j ), pensize );
        else
        {
            wxFAIL_MSG( "Unknown drawing sheet item type" );
            continue;
        }

        item->SetFlags( itemFlags[ j ] );
        m_drawItems.push_back( item );

        if( aCollector )
            aCollector->Append( item );

        if( aView )
            aView->Add( item );
    }
}


int DS_DATA_ITEM::GetPenSizeUi()
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

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
void DS_DATA_ITEM::MoveToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    pos_mm.y = aPosition.y / DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;

    MoveTo( pos_mm );
}


void DS_DATA_ITEM::MoveTo( DPOINT aPosition )
{
    DPOINT vector = aPosition - GetStartPos();
    DPOINT endpos = vector + GetEndPos();

    MoveStartPointTo( aPosition );
    MoveEndPointTo( endpos );

    for( DS_DRAW_ITEM_BASE* drawItem : m_drawItems )
    {
        drawItem->SetPosition( GetStartPosUi( drawItem->GetIndexInPeer() ) );
        drawItem->SetEnd( GetEndPosUi( drawItem->GetIndexInPeer() ) );
    }
}


/* move the starting point of the item to a new position
 * aPosition = the new position of the starting point, in mm
 */
void DS_DATA_ITEM::MoveStartPointTo( DPOINT aPosition )
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();
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
void DS_DATA_ITEM::MoveStartPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm( aPosition.x / DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu,
                   aPosition.y / DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );

    MoveStartPointTo( pos_mm );
}


/**
 * move the ending point of the item to a new position
 * has meaning only for items defined by 2 points
 * (segments and rectangles)
 * aPosition = the new position of the ending point, in mm
 */
void DS_DATA_ITEM::MoveEndPointTo( DPOINT aPosition )
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();
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
    case DS_SEGMENT:
    case DS_RECT:
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
void DS_DATA_ITEM::MoveEndPointToUi( wxPoint aPosition )
{
    DPOINT pos_mm;
    pos_mm.x = aPosition.x / DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    pos_mm.y = aPosition.y / DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;

    MoveEndPointTo( pos_mm );
}


const DPOINT DS_DATA_ITEM::GetStartPos( int ii ) const
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();
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


const wxPoint DS_DATA_ITEM::GetStartPosUi( int ii ) const
{
    DPOINT pos = GetStartPos( ii ) * DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    return wxPoint( KiROUND( pos.x ), KiROUND( pos.y ) );
}


const DPOINT DS_DATA_ITEM::GetEndPos( int ii ) const
{
    DPOINT pos( m_End.m_Pos.x + ( m_IncrementVector.x * ii ),
                m_End.m_Pos.y + ( m_IncrementVector.y * ii ) );

    switch( m_End.m_Anchor )
    {
    case RB_CORNER:      // right bottom corner
        pos = DS_DATA_MODEL::GetTheInstance().m_RB_Corner - pos;
        break;

    case RT_CORNER:      // right top corner
        pos.x = DS_DATA_MODEL::GetTheInstance().m_RB_Corner.x - pos.x;
        pos.y = DS_DATA_MODEL::GetTheInstance().m_LT_Corner.y + pos.y;
        break;

    case LB_CORNER:      // left bottom corner
        pos.x = DS_DATA_MODEL::GetTheInstance().m_LT_Corner.x + pos.x;
        pos.y = DS_DATA_MODEL::GetTheInstance().m_RB_Corner.y - pos.y;
        break;

    case LT_CORNER:      // left top corner
        pos = DS_DATA_MODEL::GetTheInstance().m_LT_Corner + pos;
        break;
    }

    return pos;
}


const wxPoint DS_DATA_ITEM::GetEndPosUi( int ii ) const
{
    DPOINT pos = GetEndPos( ii );
    pos = pos * DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    return wxPoint( KiROUND( pos.x ), KiROUND( pos.y ) );
}


bool DS_DATA_ITEM::IsInsidePage( int ii ) const
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

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


const wxString DS_DATA_ITEM::GetClassName() const
{
    wxString name;

    switch( GetType() )
    {
        case DS_TEXT:        name = _( "Text" );           break;
        case DS_SEGMENT:     name = _( "Line" );           break;
        case DS_RECT:        name = _( "Rectangle" );      break;
        case DS_POLYPOLYGON: name = _( "Imported Shape" ); break;
        case DS_BITMAP:      name = _( "Image" );          break;
    }

    return name;
}


DS_DATA_ITEM_POLYGONS::DS_DATA_ITEM_POLYGONS() :
        DS_DATA_ITEM( DS_POLYPOLYGON )
{
    m_Orient = 0.0;
}


void DS_DATA_ITEM_POLYGONS::SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    std::map<int, STATUS_FLAGS> itemFlags;
    DS_DRAW_ITEM_BASE*          item = nullptr;

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
        auto poly_shape = new DS_DRAW_ITEM_POLYPOLYGONS( this, j, GetStartPosUi( j ), pensize );
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


int DS_DATA_ITEM_POLYGONS::GetPenSizeUi()
{
    return KiROUND( m_LineWidth * DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );
}


const DPOINT DS_DATA_ITEM_POLYGONS::GetCornerPosition( unsigned aIdx, int aRepeat ) const
{
    DPOINT pos = m_Corners[aIdx];

    // Rotation:
    RotatePoint( &pos.x, &pos.y, m_Orient * 10 );
    pos += GetStartPos( aRepeat );
    return pos;
}


void DS_DATA_ITEM_POLYGONS::SetBoundingBox()
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


bool DS_DATA_ITEM_POLYGONS::IsInsidePage( int ii ) const
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

    DPOINT pos = GetStartPos( ii );
    pos += m_minCoord;  // left top pos of bounding box

    if( model.m_LT_Corner.x > pos.x || model.m_LT_Corner.y > pos.y )
        return false;

    pos = GetStartPos( ii );
    pos += m_maxCoord;  // right bottom pos of bounding box

    if( model.m_RB_Corner.x < pos.x || model.m_RB_Corner.y < pos.y )
        return false;

    return true;
}


const wxPoint DS_DATA_ITEM_POLYGONS::GetCornerPositionUi( unsigned aIdx, int aRepeat ) const
{
    DPOINT pos = GetCornerPosition( aIdx, aRepeat );
    pos = pos * DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu;
    return wxPoint( int(pos.x), int(pos.y) );
}


DS_DATA_ITEM_TEXT::DS_DATA_ITEM_TEXT( const wxString& aTextBase ) :
        DS_DATA_ITEM( DS_TEXT )
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


void DS_DATA_ITEM_TEXT::SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    int   pensize = GetPenSizeUi();
    bool  multilines = false;

    if( DS_DATA_MODEL::GetTheInstance().m_EditMode )
    {
        m_FullText = m_TextBase;
    }
    else
    {
        m_FullText = aCollector ? aCollector->BuildFullText( m_TextBase ) : wxString();
        multilines = ReplaceAntiSlashSequence();
    }

    if( pensize == 0 )
        pensize = aCollector ? aCollector->GetDefaultPenSize() : 1;

    SetConstrainedTextSize();
    wxSize textsize;

    textsize.x = KiROUND( m_ConstrainedTextSize.x * DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );
    textsize.y = KiROUND( m_ConstrainedTextSize.y * DS_DATA_MODEL::GetTheInstance().m_WSunits2Iu );

    if( m_Bold )
        pensize = GetPenSizeForBold( std::min( textsize.x, textsize.y ) );

    std::map<int, STATUS_FLAGS> itemFlags;
    DS_DRAW_ITEM_TEXT*          text = nullptr;

    for( size_t i = 0; i < m_drawItems.size(); ++i )
    {
        text = (DS_DRAW_ITEM_TEXT*) m_drawItems[ i ];
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

        text = new DS_DRAW_ITEM_TEXT( this, j, m_FullText, GetStartPosUi( j ), textsize, pensize,
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


int DS_DATA_ITEM_TEXT::GetPenSizeUi()
{
    DS_DATA_MODEL& model = DS_DATA_MODEL::GetTheInstance();

    if( m_LineWidth != 0 )
        return KiROUND( m_LineWidth * model.m_WSunits2Iu );
    else
        return KiROUND( model.m_DefaultTextThickness * model.m_WSunits2Iu );
}


void DS_DATA_ITEM_TEXT::IncrementLabel( int aIncr )
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
bool DS_DATA_ITEM_TEXT::ReplaceAntiSlashSequence()
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


void DS_DATA_ITEM_TEXT::SetConstrainedTextSize()
{
    m_ConstrainedTextSize = m_TextSize;

    if( m_ConstrainedTextSize.x == 0  )
        m_ConstrainedTextSize.x = DS_DATA_MODEL::GetTheInstance().m_DefaultTextSize.x;

    if( m_ConstrainedTextSize.y == 0 )
        m_ConstrainedTextSize.y = DS_DATA_MODEL::GetTheInstance().m_DefaultTextSize.y;

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
        DS_DRAW_ITEM_TEXT dummy( DS_DRAW_ITEM_TEXT( this, 0, m_FullText, wxPoint( 0, 0 ),
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


void DS_DATA_ITEM_BITMAP::SyncDrawItems( DS_DRAW_ITEM_LIST* aCollector, KIGFX::VIEW* aView )
{
    std::map<int, STATUS_FLAGS> itemFlags;
    DS_DRAW_ITEM_BASE*          item = nullptr;

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

    if( aCollector )
    {
        double pix_size_iu = aCollector->GetMilsToIUfactor() * 1000 / m_ImageBitmap->GetPPI();
        m_ImageBitmap->SetPixelSizeIu( pix_size_iu );
    }

    m_drawItems.clear();

    for( int j = 0; j < m_RepeatCount; j++ )
    {
        if( j && !IsInsidePage( j ) )
            continue;

        DS_DRAW_ITEM_BITMAP* bitmap = new DS_DRAW_ITEM_BITMAP( this, j, GetStartPosUi( j ) );

        bitmap->SetFlags( itemFlags[ j ] );
        m_drawItems.push_back( bitmap );

        if( aCollector )
            aCollector->Append( bitmap );

        if( aView )
            aView->Add( bitmap );
    }
}


int DS_DATA_ITEM_BITMAP::GetPPI() const
{
    if( m_ImageBitmap )
        return m_ImageBitmap->GetPPI() / m_ImageBitmap->GetScale();

    return 300;
}


void DS_DATA_ITEM_BITMAP::SetPPI( int aBitmapPPI )
{
    if( m_ImageBitmap )
        m_ImageBitmap->SetScale( (double) m_ImageBitmap->GetPPI() / aBitmapPPI );
}
