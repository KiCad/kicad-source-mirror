/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <colors_selection.h>
#include <richio.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <wxBasePcbFrame.h>
#include <msgpanel.h>
#include <base_units.h>
#include <bitmaps.h>

#include <class_board.h>
#include <class_pcb_text.h>


TEXTE_PCB::TEXTE_PCB( BOARD_ITEM* parent ) :
    BOARD_ITEM( parent, PCB_TEXT_T ),
    EDA_TEXT()
{
    SetMultilineAllowed( true );
}


TEXTE_PCB::~TEXTE_PCB()
{
}


void TEXTE_PCB::SetTextAngle( double aAngle )
{
    EDA_TEXT::SetTextAngle( NormalizeAngle360( aAngle ) );
}


void TEXTE_PCB::Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
                      GR_DRAWMODE DrawMode, const wxPoint& offset )
{
    BOARD* brd = GetBoard();

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    COLOR4D color = brd->GetLayerColor( m_Layer );

    EDA_DRAW_MODE_T fillmode = FILLED;
    DISPLAY_OPTIONS* displ_opts =
        panel ? (DISPLAY_OPTIONS*)panel->GetDisplayOptions() : NULL;

    if( displ_opts && displ_opts->m_DisplayDrawItemsFill == SKETCH )
        fillmode = SKETCH;

    // shade text if high contrast mode is active
    if( ( DrawMode & GR_ALLOW_HIGHCONTRAST ) && displ_opts && displ_opts->m_ContrastModeDisplay )
    {
        PCB_LAYER_ID curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

        if( !IsOnLayer( curr_layer ) )
            color = COLOR4D( DARKDARKGRAY );
    }

    COLOR4D anchor_color = COLOR4D::UNSPECIFIED;

    if( brd->IsElementVisible( LAYER_ANCHOR ) )
        anchor_color = brd->GetVisibleElementColor( LAYER_ANCHOR );

    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
    EDA_TEXT::Draw( clipbox, DC, offset, color,
                    DrawMode, fillmode, anchor_color );

    // Enable these line to draw the bounding box (debug tests purposes only)
#if 0
    {
        EDA_RECT BoundaryBox = GetBoundingBox();
        GRRect( clipbox, DC, BoundaryBox, 0, BROWN );
    }
#endif
}


void TEXTE_PCB::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString    msg;

    wxCHECK_RET( m_Parent != NULL, wxT( "TEXTE_PCB::GetMsgPanelInfo() m_Parent is NULL." ) );

    if( m_Parent->Type() == PCB_DIMENSION_T )
        aList.push_back( MSG_PANEL_ITEM( _( "Dimension" ), GetShownText(), DARKGREEN ) );
    else
        aList.push_back( MSG_PANEL_ITEM( _( "PCB Text" ), GetShownText(), DARKGREEN ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), GetLayerName(), BLUE ) );

    if( !IsMirrored() )
        aList.push_back( MSG_PANEL_ITEM( _( "Mirror" ), _( "No" ), DARKGREEN ) );
    else
        aList.push_back( MSG_PANEL_ITEM( _( "Mirror" ), _( "Yes" ), DARKGREEN ) );

    msg.Printf( wxT( "%.1f" ), GetTextAngle() / 10.0 );
    aList.push_back( MSG_PANEL_ITEM( _( "Angle" ), msg, DARKGREEN ) );

    msg = ::CoordinateToString( GetThickness() );
    aList.push_back( MSG_PANEL_ITEM( _( "Thickness" ), msg, MAGENTA ) );

    msg = ::CoordinateToString( GetTextWidth() );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, RED ) );

    msg = ::CoordinateToString( GetTextHeight() );
    aList.push_back( MSG_PANEL_ITEM( _( "Height" ), msg, RED ) );
}


const EDA_RECT TEXTE_PCB::GetBoundingBox() const
{
    EDA_RECT rect = GetTextBox( -1, -1 );

    if( GetTextAngle() )
        rect = rect.GetBoundingBoxRotated( GetTextPos(), GetTextAngle() );

    return rect;
}


void TEXTE_PCB::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    wxPoint pt = GetTextPos();
    RotatePoint( &pt, aRotCentre, aAngle );
    SetTextPos( pt );

    SetTextAngle( GetTextAngle() + aAngle );
}


void TEXTE_PCB::Flip( const wxPoint& aCentre )
{
    SetTextY( aCentre.y - ( GetTextPos().y - aCentre.y ) );

    int copperLayerCount = GetBoard()->GetCopperLayerCount();

    SetLayer( FlipLayer( GetLayer(), copperLayerCount ) );
    SetMirrored( !IsMirrored() );
}


wxString TEXTE_PCB::GetSelectMenuText() const
{
    wxString text;

    text.Printf( _( "Pcb Text \"%s\" on %s"),
                 GetChars ( ShortenedShownText() ), GetChars( GetLayerName() ) );

    return text;
}


BITMAP_DEF TEXTE_PCB::GetMenuImage() const
{
    return add_text_xpm;
}


EDA_ITEM* TEXTE_PCB::Clone() const
{
    return new TEXTE_PCB( *this );
}
