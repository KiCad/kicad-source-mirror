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

#include <general.h>
#include <sch_sheet.h>
#include <kicad_string.h>


SCH_SHEET_PIN::SCH_SHEET_PIN( SCH_SHEET* parent, const wxPoint& pos, const wxString& text ) :
    SCH_HIERLABEL( pos, text, SCH_SHEET_PIN_T )
{
    SetParent( parent );
    wxASSERT( parent );
    m_Layer = LAYER_SHEETLABEL;
    m_Pos   = pos;

    if( parent->IsVerticalOrientation() )
        SetEdge( 2 );
    else
        SetEdge( 0 );

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
                          EDA_COLOR_T     aColor )
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
    int tmp;
    tmp = pin->GetNumber();
    pin->SetNumber( GetNumber() );
    SetNumber( tmp );
    tmp = pin->GetEdge();
    pin->SetEdge( GetEdge() );
    SetEdge( tmp );
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


void SCH_SHEET_PIN::SetEdge( int aEdge )
{
    SCH_SHEET* Sheet = (SCH_SHEET*) GetParent();

    /* use -1 to adjust text orientation without changing edge*/
    if( aEdge > -1 )
        m_edge = aEdge;

    switch( m_edge )
    {
    case 0:        /* pin on left side*/
        m_Pos.x = Sheet->m_pos.x;
        SetOrientation( 2 ); /* Orientation horiz inverse */
        break;

    case 1:        /* pin on right side*/
        m_Pos.x = Sheet->m_pos.x + Sheet->m_size.x;
        SetOrientation( 0 ); /* Orientation horiz normal */
        break;

    case 2:        /* pin on top side*/
        m_Pos.y = Sheet->m_pos.y;
        SetOrientation( 3 ); /* Orientation vert BOTTOM  */
        break;

    case 3:        /* pin on bottom side*/
        m_Pos.y = Sheet->m_pos.y + Sheet->m_size.y;
        SetOrientation( 1 ); /* Orientation vert UP  */
        break;
    }
}


int SCH_SHEET_PIN::GetEdge() const
{
    return m_edge;
}


void SCH_SHEET_PIN::ConstrainOnEdge( wxPoint Pos )
{
    SCH_SHEET* Sheet = (SCH_SHEET*) GetParent();

    if( Sheet == NULL )
        return;

    if( m_edge<2 ) /*horizontal sheetpin*/
    {
        if( Pos.x > ( Sheet->m_pos.x + ( Sheet->m_size.x / 2 ) ) )
        {
            SetEdge( 1 );
        }
        else
        {
            SetEdge( 0 );
        }

        m_Pos.y = Pos.y;

        if( m_Pos.y < Sheet->m_pos.y )
            m_Pos.y = Sheet->m_pos.y;

        if( m_Pos.y > (Sheet->m_pos.y + Sheet->m_size.y) )
            m_Pos.y = Sheet->m_pos.y + Sheet->m_size.y;
    }
    else /* vertical sheetpin*/
    {
        if( Pos.y > ( Sheet->m_pos.y + ( Sheet->m_size.y / 2 ) ) )
        {
            SetEdge( 3 ); //bottom
        }
        else
        {
            SetEdge( 2 ); //top
        }

        m_Pos.x = Pos.x;

        if( m_Pos.x < Sheet->m_pos.x )
            m_Pos.x = Sheet->m_pos.x;

        if( m_Pos.x > (Sheet->m_pos.x + Sheet->m_size.x) )
            m_Pos.x = Sheet->m_pos.x + Sheet->m_size.x;
    }
}


bool SCH_SHEET_PIN::Save( FILE* aFile ) const
{
    int type = 'U', side = 'L';

    if( m_Text.IsEmpty() )
        return true;

    switch( m_edge )
    {
    case 0:     //pin on left side
        side = 'L';
        break;

    case 1:     //pin on right side
        side = 'R';
        break;

    case 2:      //pin on top side
        side = 'T';
        break;

    case 3:     //pin on bottom side
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
                 type, side, m_Pos.x, m_Pos.y,
                 m_Size.x ) == EOF )
    {
        return false;
    }

    return true;
}


bool SCH_SHEET_PIN::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    int     size;
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

    int r = sscanf( cp, "%d %d %d", &m_Pos.x, &m_Pos.y, &size );
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

    m_Size.x = m_Size.y = size;

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
    case 'R' : /* pin on right side */
        SetEdge( 1 );
        break;

    case 'T' : /* pin on top side */
        SetEdge( 2 );
        break;

    case 'B' : /* pin on bottom side */
        SetEdge( 3 );
        break;

    case 'L' : /* pin on left side */
    default  :
        SetEdge( 0 );
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
    int p = m_Pos.y - aXaxis_position;

    m_Pos.y = aXaxis_position - p;

    switch( m_edge )
    {
    case 2:
        SetEdge( 3 );
        break;

    case 3:
        SetEdge( 2 );
        break;
    }
}


void SCH_SHEET_PIN::MirrorY( int aYaxis_position )
{
    int p = m_Pos.x - aYaxis_position;

    m_Pos.x = aYaxis_position - p;

    switch( m_edge )
    {
    case 0:
        SetEdge( 1 );
        break;

    case 1:
        SetEdge( 0 );
        break;
    }
}


void SCH_SHEET_PIN::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_Pos, aPosition, 900 );

    switch( m_edge )
    {
    case 0:     //pin on left side
        SetEdge( 3 );
        break;

    case 1:     //pin on right side
        SetEdge( 2 );
        break;

    case 2:      //pin on top side
        SetEdge( 0 );
        break;

    case 3:     //pin on bottom side
        SetEdge( 1 );
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
    int tmp = m_shape;

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
    DANGLING_END_ITEM item( SHEET_LABEL_END, this, m_Pos );
    aItemList.push_back( item );
}


wxString SCH_SHEET_PIN::GetSelectMenuText() const
{
    wxString tmp;
    tmp.Printf( _( "Hierarchical Sheet Pin %s" ), GetChars( ShortenedShownText() ) );
    return tmp;
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
