/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_pcb_text.cpp
 * @brief Class TEXTE_PCB texts on copper or technical layers implementation
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <gr_basic.h>
#include <base_struct.h>
#include <drawtxt.h>
#include <kicad_string.h>
#include <trigo.h>
#include <pcbcommon.h>
#include <colors_selection.h>
#include <richio.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <msgpanel.h>
#include <base_units.h>

#include <class_board.h>
#include <class_pcb_text.h>

#include <protos.h>


TEXTE_PCB::TEXTE_PCB( BOARD_ITEM* parent ) :
    BOARD_ITEM( parent, PCB_TEXT_T ),
    EDA_TEXT()
{
    m_MultilineAllowed = true;
}


TEXTE_PCB:: ~TEXTE_PCB()
{
}


void TEXTE_PCB::Copy( TEXTE_PCB* source )
{
    m_Parent    = source->m_Parent;
    Pback       = Pnext = NULL;
    m_Mirror    = source->m_Mirror;
    m_Size      = source->m_Size;
    m_Orient    = source->m_Orient;
    m_Pos       = source->m_Pos;
    m_Layer     = source->m_Layer;
    m_Thickness = source->m_Thickness;
    m_Attributs = source->m_Attributs;
    m_Italic    = source->m_Italic;
    m_Bold      = source->m_Bold;
    m_HJustify  = source->m_HJustify;
    m_VJustify  = source->m_VJustify;
    m_MultilineAllowed = m_MultilineAllowed;

    m_Text = source->m_Text;
}


void TEXTE_PCB::Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
                      GR_DRAWMODE DrawMode, const wxPoint& offset )
{
    BOARD* brd = GetBoard();

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    EDA_COLOR_T color = brd->GetLayerColor( m_Layer );

    EDA_DRAW_MODE_T fillmode = FILLED;

    if( DisplayOpt.DisplayDrawItems == SKETCH )
        fillmode = SKETCH;

    EDA_COLOR_T anchor_color = UNSPECIFIED_COLOR;

    if( brd->IsElementVisible( ANCHOR_VISIBLE ) )
        anchor_color = brd->GetVisibleElementColor( ANCHOR_VISIBLE );

    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
    EDA_TEXT::Draw( clipbox, DC, offset, color,
                    DrawMode, fillmode, anchor_color );
}


void TEXTE_PCB::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString    msg;

#if defined(__WXDEBUG__)
    BOARD_ITEM* parent = (BOARD_ITEM*) m_Parent;
    wxASSERT( parent );

    BOARD*      board;
    if( parent->Type() == PCB_DIMENSION_T )
        board = (BOARD*) parent->GetParent();
    else
        board = (BOARD*) parent;
    wxASSERT( board );
#endif

    if( m_Parent && m_Parent->Type() == PCB_DIMENSION_T )
        aList.push_back( MSG_PANEL_ITEM( _( "Dimension" ), m_Text, DARKGREEN ) );
    else
        aList.push_back( MSG_PANEL_ITEM( _( "PCB Text" ), m_Text, DARKGREEN ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), GetLayerName(), BLUE ) );

    if( !m_Mirror )
        aList.push_back( MSG_PANEL_ITEM( _( "Mirror" ), _( "No" ), DARKGREEN ) );
    else
        aList.push_back( MSG_PANEL_ITEM( _( "Mirror" ), _( "Yes" ), DARKGREEN ) );

    msg.Printf( wxT( "%.1f" ), m_Orient / 10.0 );
    aList.push_back( MSG_PANEL_ITEM( _( "Orientation" ), msg, DARKGREEN ) );

    msg = ::CoordinateToString( m_Thickness );
    aList.push_back( MSG_PANEL_ITEM( _( "Thickness" ), msg, MAGENTA ) );

    msg = ::CoordinateToString( m_Size.x );
    aList.push_back( MSG_PANEL_ITEM( _( "Size X" ), msg, RED ) );

    msg = ::CoordinateToString( m_Size.y );
    aList.push_back( MSG_PANEL_ITEM( _( "Size Y" ), msg, RED ) );
}


void TEXTE_PCB::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
    m_Orient += aAngle;
    NORMALIZE_ANGLE_360( m_Orient );
}


void TEXTE_PCB::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - ( m_Pos.y - aCentre.y );
    SetLayer( FlipLayer( GetLayer() ) );
    m_Mirror = !m_Mirror;
}


wxString TEXTE_PCB::GetSelectMenuText() const
{
    wxString text, shorttxt;

    if( m_Text.Len() < 12 )
        shorttxt << m_Text;
    else
        shorttxt += m_Text.Left( 10 ) + wxT( "..." );

    text.Printf( _( "Pcb Text \"%s\" on %s"),
                 GetChars ( shorttxt ), GetChars( GetLayerName() ) );

    return text;
}


EDA_ITEM* TEXTE_PCB::Clone() const
{
    return new TEXTE_PCB( *this );
}


const BOX2I TEXTE_PCB::ViewBBox() const
{
    EDA_RECT rect = GetTextBox();

    if( m_Orient != 0.0 )
    {
        // If the text is rotated, we need to take it into account
        wxPoint p1 = rect.GetOrigin();
        wxPoint p2 = wxPoint( p1.x + rect.GetWidth(), p1.y );
        wxPoint p3 = rect.GetEnd();
        wxPoint p4 = wxPoint( p1.x, p1.y + rect.GetHeight() );

        // Transform all the corners of the bounding box according to the rotation angle
        RotatePoint( &p1, m_Pos, -m_Orient );
        RotatePoint( &p2, m_Pos, -m_Orient );
        RotatePoint( &p3, m_Pos, -m_Orient );
        RotatePoint( &p4, m_Pos, -m_Orient );

        // Find the new bounding box origin and dimensions
        int minX = std::min( std::min( p1.x, p2.x ), std::min( p3.x, p4.x ) );
        int minY = std::min( std::min( p1.y, p2.y ), std::min( p3.y, p4.y ) );
        int maxX = std::max( std::max( p1.x, p2.x ), std::max( p3.x, p4.x ) );
        int maxY = std::max( std::max( p1.y, p2.y ), std::max( p3.y, p4.y ) );

        int width = maxX - minX;
        int height = maxY - minY;

        return BOX2I( VECTOR2I( minX, minY ), VECTOR2I( width, height ) );
    }
    else
    {
        return BOX2I( rect.GetOrigin(), rect.GetSize() );
    }
}

