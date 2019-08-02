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

#include <fctsys.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <gr_text.h>
#include <plotter.h>
#include <trigo.h>
#include <sch_edit_frame.h>
#include <bitmaps.h>
#include <general.h>
#include <sch_sheet.h>
#include <kicad_string.h>


SCH_SHEET_PIN::SCH_SHEET_PIN( SCH_SHEET* parent, const wxPoint& pos, const wxString& text ) :
    SCH_HIERLABEL( pos, text, SCH_SHEET_PIN_T ),
    m_edge( SHEET_UNDEFINED_SIDE )
{
    SetParent( parent );
    wxASSERT( parent );
    m_Layer = LAYER_SHEETLABEL;

    SetTextPos( pos );

    if( parent->IsVerticalOrientation() )
        SetEdge( SHEET_TOP_SIDE );
    else
        SetEdge( SHEET_LEFT_SIDE );

    m_shape = NET_INPUT;
    m_isDangling = true;
    m_number     = 2;
}


EDA_ITEM* SCH_SHEET_PIN::Clone() const
{
    return new SCH_SHEET_PIN( *this );
}


void SCH_SHEET_PIN::Print( wxDC* aDC, const wxPoint&  aOffset )
{
    // The icon selection is handle by the virtual method CreateGraphicShape called by ::Print
    SCH_HIERLABEL::Print( aDC, aOffset );
}


void SCH_SHEET_PIN::SwapData( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == SCH_SHEET_PIN_T,
                 wxString::Format( wxT( "SCH_SHEET_PIN object cannot swap data with %s object." ),
                                   GetChars( aItem->GetClass() ) ) );

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


int SCH_SHEET_PIN::GetPenSize() const
{
    return GetDefaultLineThickness();
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
    case SHEET_LEFT_SIDE:
        m_edge = aEdge;
        SetTextX( Sheet->m_pos.x );
        SetLabelSpinStyle( 2 ); // Orientation horiz inverse
        break;

    case SHEET_RIGHT_SIDE:
        m_edge = aEdge;
        SetTextX( Sheet->m_pos.x + Sheet->m_size.x );
        SetLabelSpinStyle( 0 ); // Orientation horiz normal
        break;

    case SHEET_TOP_SIDE:
        m_edge = aEdge;
        SetTextY( Sheet->m_pos.y );
        SetLabelSpinStyle( 3 ); // Orientation vert BOTTOM
        break;

    case SHEET_BOTTOM_SIDE:
        m_edge = aEdge;
        SetTextY( Sheet->m_pos.y + Sheet->m_size.y );
        SetLabelSpinStyle( 1 ); // Orientation vert UP
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

    wxPoint center = sheet->m_pos + ( sheet->m_size / 2 );

    if( m_edge == SHEET_LEFT_SIDE || m_edge == SHEET_RIGHT_SIDE )
    {
        if( Pos.x > center.x )
            SetEdge( SHEET_RIGHT_SIDE );
        else
            SetEdge( SHEET_LEFT_SIDE );

        SetTextY( Pos.y );

        if( GetTextPos().y < sheet->m_pos.y )
            SetTextY( sheet->m_pos.y );

        if( GetTextPos().y > (sheet->m_pos.y + sheet->m_size.y) )
            SetTextY( sheet->m_pos.y + sheet->m_size.y );
    }
    else
    {
        if( Pos.y > center.y )
            SetEdge( SHEET_BOTTOM_SIDE );
        else
            SetEdge( SHEET_TOP_SIDE );

        SetTextX( Pos.x );

        if( GetTextPos().x < sheet->m_pos.x )
            SetTextX( sheet->m_pos.x );

        if( GetTextPos().x > (sheet->m_pos.x + sheet->m_size.x) )
            SetTextX( sheet->m_pos.x + sheet->m_size.x );
    }
}


void SCH_SHEET_PIN::MirrorX( int aXaxis_position )
{
    int p = GetTextPos().y - aXaxis_position;

    SetTextY( aXaxis_position - p );

    switch( m_edge )
    {
    case SHEET_TOP_SIDE:    SetEdge( SHEET_BOTTOM_SIDE ); break;
    case SHEET_BOTTOM_SIDE: SetEdge( SHEET_TOP_SIDE );    break;
    default: break;
    }
}


void SCH_SHEET_PIN::MirrorY( int aYaxis_position )
{
    int p = GetTextPos().x - aYaxis_position;

    SetTextX( aYaxis_position - p );

    switch( m_edge )
    {
    case SHEET_LEFT_SIDE:  SetEdge( SHEET_RIGHT_SIDE ); break;
    case SHEET_RIGHT_SIDE: SetEdge( SHEET_LEFT_SIDE );  break;
    default: break;
    }
}


void SCH_SHEET_PIN::Rotate( wxPoint aPosition )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aPosition, 900 );
    SetTextPos( pt );

    switch( m_edge )
    {
    case SHEET_LEFT_SIDE:   SetEdge( SHEET_BOTTOM_SIDE ); break;
    case SHEET_RIGHT_SIDE:  SetEdge( SHEET_TOP_SIDE );    break;
    case SHEET_TOP_SIDE:    SetEdge( SHEET_LEFT_SIDE );   break;
    case SHEET_BOTTOM_SIDE: SetEdge( SHEET_RIGHT_SIDE );  break;
    default: break;
    }
}


void SCH_SHEET_PIN::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& aPos )
{
    /*
     * These are the same icon shapes as SCH_HIERLABEL but the graphic icon is slightly
     * different in 2 cases:
     * for INPUT type the icon is the OUTPUT shape of SCH_HIERLABEL
     * for OUTPUT type the icon is the INPUT shape of SCH_HIERLABEL
     */
    PINSHEETLABEL_SHAPE tmp = m_shape;

    switch( m_shape )
    {
    case NET_INPUT:  m_shape = NET_OUTPUT; break;
    case NET_OUTPUT: m_shape = NET_INPUT;  break;
    default:                               break;
    }

    SCH_HIERLABEL::CreateGraphicShape( aPoints, aPos );
    m_shape = tmp;
}


void SCH_SHEET_PIN::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( SHEET_LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


wxString SCH_SHEET_PIN::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _( "Hierarchical Sheet Pin %s" ), ShortenedShownText() );
}


BITMAP_DEF SCH_SHEET_PIN::GetMenuImage() const
{
    return add_hierar_pin_xpm;
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
