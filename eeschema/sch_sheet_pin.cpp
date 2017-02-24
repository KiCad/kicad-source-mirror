/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_sheet_pin.cpp
 * @brief Implementation of the SCH_SHEET_PIN class.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <plot_common.h>
#include <trigo.h>
#include <richio.h>
#include <schframe.h>
#include <bitmaps.h>

#include <general.h>
#include <sch_sheet.h>
#include <kicad_string.h>


SCH_SHEET_PIN::SCH_SHEET_PIN( SCH_SHEET* parent, const wxPoint& pos, const wxString& text ) :
    SCH_HIERLABEL( pos, text, SCH_SHEET_PIN_T )
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


void SCH_SHEET_PIN::Draw( EDA_DRAW_PANEL* aPanel,
                          wxDC*           aDC,
                          const wxPoint&  aOffset,
                          GR_DRAWMODE     aDraw_mode,
                          COLOR4D         aColor )
{
    // The icon selection is handle by the virtual method CreateGraphicShape
    // called by ::Draw
    SCH_HIERLABEL::Draw( aPanel, aDC, aOffset, aDraw_mode, aColor );
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


void SCH_SHEET_PIN::SetEdge( SCH_SHEET_PIN::SHEET_SIDE aEdge )
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


enum SCH_SHEET_PIN::SHEET_SIDE SCH_SHEET_PIN::GetEdge() const
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
        {
            SetEdge( SHEET_RIGHT_SIDE );
        }
        else
        {
            SetEdge( SHEET_LEFT_SIDE );
        }

        SetTextY( Pos.y );

        if( GetTextPos().y < sheet->m_pos.y )
            SetTextY( sheet->m_pos.y );

        if( GetTextPos().y > (sheet->m_pos.y + sheet->m_size.y) )
            SetTextY( sheet->m_pos.y + sheet->m_size.y );
    }
    else // vertical sheetpin
    {
        if( Pos.y > center.y )
        {
            SetEdge( SHEET_BOTTOM_SIDE ); //bottom
        }
        else
        {
            SetEdge( SHEET_TOP_SIDE ); //top
        }

        SetTextX( Pos.x );

        if( GetTextPos().x < sheet->m_pos.x )
            SetTextX( sheet->m_pos.x );

        if( GetTextPos().x > (sheet->m_pos.x + sheet->m_size.x) )
            SetTextX( sheet->m_pos.x + sheet->m_size.x );
    }
}


bool SCH_SHEET_PIN::Save( FILE* aFile ) const
{
    int type = 'U', side = 'L';

    if( m_Text.IsEmpty() )
        return true;

    switch( m_edge )
    {
    default:
    case SHEET_LEFT_SIDE:     //pin on left side
        side = 'L';
        break;

    case SHEET_RIGHT_SIDE:     //pin on right side
        side = 'R';
        break;

    case SHEET_TOP_SIDE:      //pin on top side
        side = 'T';
        break;

    case SHEET_BOTTOM_SIDE:     //pin on bottom side
        side = 'B';
        break;
    }

    switch( m_shape )
    {
    case NET_INPUT:
        type = 'I'; break;

    case NET_OUTPUT:
        type = 'O'; break;

    case NET_BIDI:
        type = 'B'; break;

    case NET_TRISTATE:
        type = 'T'; break;

    case NET_UNSPECIFIED:
        type = 'U'; break;
    }

    if( fprintf( aFile, "F%d %s %c %c %-3d %-3d %-3d\n", m_number,
                 EscapedUTF8( m_Text ).c_str(),     // supplies wrapping quotes
                 type, side, GetTextPos().x, GetTextPos().y,
                 GetTextWidth() ) == EOF )
    {
        return false;
    }

    return true;
}


bool SCH_SHEET_PIN::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    int     x, y, size;
    char    number[256];
    char    name[256];
    char    connectType[256];
    char    sheetSide[256];
    char*   line = aLine.Line();
    char*   cp;

    static const char delims[] = " \t";

    // Read coordinates.
    // D( printf( "line: \"%s\"\n", line );)

    cp = strtok( line, delims );

    strncpy( number, cp, sizeof(number) );
    number[sizeof(number)-1] = 0;

    cp += strlen( number ) + 1;

    cp += ReadDelimitedText( name, cp, sizeof(name) );

    cp = strtok( cp, delims );
    strncpy( connectType, cp, sizeof(connectType) );
    connectType[sizeof(connectType)-1] = 0;

    cp = strtok( NULL, delims );
    strncpy( sheetSide, cp, sizeof(sheetSide) );
    sheetSide[sizeof(sheetSide)-1] = 0;

    cp += strlen( sheetSide ) + 1;

    int r = sscanf( cp, "%d %d %d", &x, &y, &size );
    if( r != 3 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file sheet hierarchical label error at line %d.\n" ),
                          aLine.LineNumber() );

        aErrorMsg << FROM_UTF8( line );
        return false;
    }

    m_Text = FROM_UTF8( name );

    if( size == 0 )
        size = GetDefaultTextSize();

    SetTextSize( wxSize( size, size ) );
    SetTextPos( wxPoint( x, y ) );

    switch( connectType[0] )
    {
    case 'I':
        m_shape = NET_INPUT;
        break;

    case 'O':
        m_shape = NET_OUTPUT;
        break;

    case 'B':
        m_shape = NET_BIDI;
        break;

    case 'T':
        m_shape = NET_TRISTATE;
        break;

    case 'U':
        m_shape = NET_UNSPECIFIED;
        break;
    }

    switch( sheetSide[0] )
    {
    case 'R' : // pin on right side
        SetEdge( SHEET_RIGHT_SIDE );
        break;

    case 'T' : // pin on top side
        SetEdge( SHEET_TOP_SIDE );
        break;

    case 'B' : // pin on bottom side
        SetEdge( SHEET_BOTTOM_SIDE );
        break;

    case 'L' : // pin on left side
    default  :
        SetEdge( SHEET_LEFT_SIDE );
        break;
    }

    return true;
}


bool SCH_SHEET_PIN::Matches( wxFindReplaceData& aSearchData,
                             void* aAuxData, wxPoint* aFindLocation )
{
    wxCHECK_MSG( GetParent() != NULL, false,
                 wxT( "Sheet pin " ) + m_Text + wxT( " does not have a parent sheet!" ) );

    wxLogTrace( traceFindItem, wxT( "    child item " ) + GetSelectMenuText() );

    if( SCH_ITEM::Matches( m_Text, aSearchData ) )
    {
        if( aFindLocation )
            *aFindLocation = GetBoundingBox().Centre();

        return true;
    }

    return false;
}


void SCH_SHEET_PIN::MirrorX( int aXaxis_position )
{
    int p = GetTextPos().y - aXaxis_position;

    SetTextY( aXaxis_position - p );

    switch( m_edge )
    {
    case SHEET_TOP_SIDE:
        SetEdge( SHEET_BOTTOM_SIDE );
        break;

    case SHEET_BOTTOM_SIDE:
        SetEdge( SHEET_TOP_SIDE );
        break;

    default:
        break;
    }
}


void SCH_SHEET_PIN::MirrorY( int aYaxis_position )
{
    int p = GetTextPos().x - aYaxis_position;

    SetTextX( aYaxis_position - p );

    switch( m_edge )
    {
    case SHEET_LEFT_SIDE:
        SetEdge( SHEET_RIGHT_SIDE );
        break;

    case SHEET_RIGHT_SIDE:
        SetEdge( SHEET_LEFT_SIDE );
        break;

    default:
        break;
    }
}


void SCH_SHEET_PIN::Rotate( wxPoint aPosition )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aPosition, 900 );
    SetTextPos( pt );

    switch( m_edge )
    {
    case SHEET_LEFT_SIDE:     //pin on left side
        SetEdge( SHEET_BOTTOM_SIDE );
        break;

    case SHEET_RIGHT_SIDE:     //pin on right side
        SetEdge( SHEET_TOP_SIDE );
        break;

    case SHEET_TOP_SIDE:      //pin on top side
        SetEdge( SHEET_LEFT_SIDE );
        break;

    case SHEET_BOTTOM_SIDE:     //pin on bottom side
        SetEdge( SHEET_RIGHT_SIDE );
        break;

    default:
        break;
    }
}


void SCH_SHEET_PIN::CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& aPos )
{
     /* This is the same icon shapes as SCH_HIERLABEL
     * but the graphic icon is slightly different in 2 cases:
     * for INPUT type the icon is the OUTPUT shape of SCH_HIERLABEL
     * for OUTPUT type the icon is the INPUT shape of SCH_HIERLABEL
     */
    PINSHEETLABEL_SHAPE tmp = m_shape;

    switch( m_shape )
    {
    case NET_INPUT:
        m_shape = NET_OUTPUT;
        break;

    case NET_OUTPUT:
        m_shape = NET_INPUT;
        break;

    default:
        break;
    }

    SCH_HIERLABEL::CreateGraphicShape( aPoints, aPos );
    m_shape = tmp;
}


void SCH_SHEET_PIN::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_ITEM item( SHEET_LABEL_END, this, GetTextPos() );
    aItemList.push_back( item );
}


wxString SCH_SHEET_PIN::GetSelectMenuText() const
{
    wxString tmp;
    tmp.Printf( _( "Hierarchical Sheet Pin %s" ), GetChars( ShortenedShownText() ) );
    return tmp;
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
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">"
                                 << " pin_name=\"" << TO_UTF8( m_Text )
                                 << '"' << "/>\n" << std::flush;

//    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n";
}

#endif
