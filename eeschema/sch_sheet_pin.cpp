/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>

#include <bitmaps.h>
#include <general.h>
#include <geometry/shape_line_chain.h>
#include <gr_text.h>
#include <kicad_string.h>
#include <plotter.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_painter.h>
#include <trigo.h>


SCH_SHEET_PIN::SCH_SHEET_PIN( SCH_SHEET* parent, const wxPoint& pos, const wxString& text ) :
    SCH_HIERLABEL( pos, text, SCH_SHEET_PIN_T ),
    m_edge( SHEET_SIDE::UNDEFINED )
{
    SetParent( parent );
    wxASSERT( parent );
    m_layer = LAYER_SHEETLABEL;

    SetTextPos( pos );

    if( parent->IsVerticalOrientation() )
        SetEdge( SHEET_SIDE::TOP );
    else
        SetEdge( SHEET_SIDE::LEFT );

    m_shape      = PINSHEETLABEL_SHAPE::PS_INPUT;
    m_isDangling = true;
    m_number     = 2;
}


EDA_ITEM* SCH_SHEET_PIN::Clone() const
{
    return new SCH_SHEET_PIN( *this );
}


void SCH_SHEET_PIN::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    // The icon selection is handle by the virtual method CreateGraphicShape called by ::Print
    SCH_HIERLABEL::Print( aSettings, aOffset );
}


void SCH_SHEET_PIN::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == SCH_SHEET_PIN_T,
            wxString::Format( wxT( "SCH_SHEET_PIN object cannot swap data with %s object." ),
                    aItem->GetClass() ) );

    SCH_SHEET_PIN* pin = ( SCH_SHEET_PIN* ) aItem;
    SCH_TEXT::SwapData( (SCH_TEXT*) pin );
    int tmp = pin->GetNumber();
    pin->SetNumber( GetNumber() );
    SetNumber( tmp );
    SHEET_SIDE stmp = pin->GetEdge();
    pin->SetEdge( GetEdge() );
    SetEdge( stmp );
}


bool SCH_SHEET_PIN::operator==( const SCH_SHEET_PIN* aPin ) const
{
    return aPin == this;
}


int SCH_SHEET_PIN::GetPenWidth() const
{
    return 1;
}


void SCH_SHEET_PIN::SetNumber( int aNumber )
{
    wxASSERT( aNumber >= 2 );

    m_number = aNumber;
}


void SCH_SHEET_PIN::SetEdge( SHEET_SIDE aEdge )
{
    SCH_SHEET* Sheet = GetParent();

    // use SHEET_UNDEFINED_SIDE to adjust text orientation without changing edge

    switch( aEdge )
    {
    case SHEET_SIDE::LEFT:
        m_edge = aEdge;
        SetTextX( Sheet->m_pos.x );
        SetLabelSpinStyle( LABEL_SPIN_STYLE::RIGHT ); // Orientation horiz inverse
        break;

    case SHEET_SIDE::RIGHT:
        m_edge = aEdge;
        SetTextX( Sheet->m_pos.x + Sheet->m_size.x );
        SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT ); // Orientation horiz normal
        break;

    case SHEET_SIDE::TOP:
        m_edge = aEdge;
        SetTextY( Sheet->m_pos.y );
        SetLabelSpinStyle( LABEL_SPIN_STYLE::BOTTOM ); // Orientation vert BOTTOM
        break;

    case SHEET_SIDE::BOTTOM:
        m_edge = aEdge;
        SetTextY( Sheet->m_pos.y + Sheet->m_size.y );
        SetLabelSpinStyle( LABEL_SPIN_STYLE::UP ); // Orientation vert UP
        break;

    default:
        break;
    }
}


enum SHEET_SIDE SCH_SHEET_PIN::GetEdge() const
{
    return m_edge;
}


void SCH_SHEET_PIN::ConstrainOnEdge( wxPoint Pos )
{
    SCH_SHEET* sheet = GetParent();

    if( sheet == NULL )
        return;

    int leftSide  = sheet->m_pos.x;
    int rightSide = sheet->m_pos.x + sheet->m_size.x;
    int topSide   = sheet->m_pos.y;
    int botSide   = sheet->m_pos.y + sheet->m_size.y;

    SHAPE_LINE_CHAIN sheetEdge;

    sheetEdge.Append( leftSide,  topSide );
    sheetEdge.Append( rightSide, topSide );
    sheetEdge.Append( rightSide, botSide );
    sheetEdge.Append( leftSide,  botSide );
    sheetEdge.Append( leftSide,  topSide );

    switch( sheetEdge.NearestSegment( Pos ) )
    {
    case 0: SetEdge( SHEET_SIDE::TOP ); break;
    case 1: SetEdge( SHEET_SIDE::RIGHT ); break;
    case 2: SetEdge( SHEET_SIDE::BOTTOM ); break;
    case 3: SetEdge( SHEET_SIDE::LEFT ); break;
    default: wxASSERT( "Invalid segment number" );
    }

    switch( GetEdge() )
    {
    case SHEET_SIDE::RIGHT:
    case SHEET_SIDE::LEFT:
        SetTextY( Pos.y );

        if( GetTextPos().y < topSide )
            SetTextY( topSide );

        if( GetTextPos().y > botSide )
            SetTextY( botSide );

        break;

    case SHEET_SIDE::BOTTOM:
    case SHEET_SIDE::TOP:
        SetTextX( Pos.x );

        if( GetTextPos().x < leftSide )
            SetTextX( leftSide );

        if( GetTextPos().x > rightSide )
            SetTextX( rightSide );

        break;

    case SHEET_SIDE::UNDEFINED:
        wxASSERT( "Undefined sheet side" );
    }
}


void SCH_SHEET_PIN::MirrorVertically( int aCenter )
{
    int p = GetTextPos().y - aCenter;

    SetTextY( aCenter - p );

    switch( m_edge )
    {
    case SHEET_SIDE::TOP: SetEdge( SHEET_SIDE::BOTTOM ); break;
    case SHEET_SIDE::BOTTOM: SetEdge( SHEET_SIDE::TOP ); break;
    default: break;
    }
}


void SCH_SHEET_PIN::MirrorHorizontally( int aCenter )
{
    int p = GetTextPos().x - aCenter;

    SetTextX( aCenter - p );

    switch( m_edge )
    {
    case SHEET_SIDE::LEFT: SetEdge( SHEET_SIDE::RIGHT ); break;
    case SHEET_SIDE::RIGHT: SetEdge( SHEET_SIDE::LEFT ); break;
    default: break;
    }
}


void SCH_SHEET_PIN::Rotate( wxPoint aCenter )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aCenter, 900 );
    ConstrainOnEdge( pt );
}


void SCH_SHEET_PIN::CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                        std::vector<wxPoint>& aPoints, const wxPoint& aPos ) const
{
    /*
     * These are the same icon shapes as SCH_HIERLABEL but the graphic icon is slightly
     * different in 2 cases:
     * for INPUT type the icon is the OUTPUT shape of SCH_HIERLABEL
     * for OUTPUT type the icon is the INPUT shape of SCH_HIERLABEL
     */
    PINSHEETLABEL_SHAPE shape = m_shape;

    switch( shape )
    {
    case PINSHEETLABEL_SHAPE::PS_INPUT:  shape = PINSHEETLABEL_SHAPE::PS_OUTPUT; break;
    case PINSHEETLABEL_SHAPE::PS_OUTPUT: shape = PINSHEETLABEL_SHAPE::PS_INPUT;  break;
    default:                                                                     break;
    }

    SCH_HIERLABEL::CreateGraphicShape( aSettings, aPoints, aPos, shape );
}


void SCH_SHEET_PIN::GetEndPoints( std::vector<DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( SHEET_LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


wxString SCH_SHEET_PIN::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Hierarchical Sheet Pin %s" ), ShortenedShownText() );
}


BITMAPS SCH_SHEET_PIN::GetMenuImage() const
{
    return BITMAPS::add_hierar_pin;
}


bool SCH_SHEET_PIN::HitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPoint );
}


#if defined(DEBUG)

void SCH_SHEET_PIN::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">"
                                 << " pin_name=\"" << TO_UTF8( GetText() )
                                 << '"' << "/>\n" << std::flush;
}

#endif
