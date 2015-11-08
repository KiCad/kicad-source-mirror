/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file class_mire.cpp
 * MIRE class definition (targets for photo plots)
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <kicad_string.h>
#include <colors_selection.h>
#include <trigo.h>
#include <macros.h>
#include <richio.h>

#include <class_board.h>
#include <class_mire.h>
#include <base_units.h>


PCB_TARGET::PCB_TARGET( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_TARGET_T )
{
    m_Shape = 0;
    m_Size  = Millimeter2iu( 5 );       // Gives a decent size
    m_Width = Millimeter2iu( 0.15 );    // Gives a decent width
    m_Layer = Edge_Cuts;                   // a target is on all layers
}

PCB_TARGET::PCB_TARGET( BOARD_ITEM* aParent, int aShape, LAYER_ID aLayer,
        const wxPoint& aPos, int aSize, int aWidth ) :
    BOARD_ITEM( aParent, PCB_TARGET_T )
{
    m_Shape = aShape;
    m_Layer = aLayer;
    m_Pos   = aPos;
    m_Size  = aSize;
    m_Width = aWidth;
}


PCB_TARGET::~PCB_TARGET()
{
}

void PCB_TARGET::Copy( PCB_TARGET* source )
{
    m_Layer     = source->m_Layer;
    m_Width     = source->m_Width;
    m_Pos       = source->m_Pos;
    m_Shape     = source->m_Shape;
    m_Size      = source->m_Size;
    SetTimeStamp( GetNewTimeStamp() );
}


/* Draw PCB_TARGET object: 2 segments + 1 circle
 * The circle radius is half the radius of the target
 * 2 lines have length the diameter of the target
 */
void PCB_TARGET::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE mode_color,
                       const wxPoint& offset )
{
    int radius, ox, oy, width;
    int dx1, dx2, dy1, dy2;

    ox = m_Pos.x + offset.x;
    oy = m_Pos.y + offset.y;

    BOARD * brd =  GetBoard( );

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    EDA_COLOR_T gcolor = brd->GetLayerColor( m_Layer );

    GRSetDrawMode( DC, mode_color );
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)panel->GetDisplayOptions();
    bool filled = displ_opts ? displ_opts->m_DisplayDrawItemsFill : FILLED;
    width   = m_Width;

    radius = m_Size / 3;

    if( GetShape() )   // shape X
        radius = m_Size / 2;

    if( filled )
        GRCircle( panel->GetClipBox(), DC, ox, oy, radius, width, gcolor );
    else
    {
        GRCircle( panel->GetClipBox(), DC, ox, oy, radius + (width / 2), gcolor );
        GRCircle( panel->GetClipBox(), DC, ox, oy, radius - (width / 2), gcolor );
    }


    radius = m_Size / 2;
    dx1   = radius;
    dy1   = 0;
    dx2   = 0;
    dy2   = radius;

    if( GetShape() )   // shape X
    {
        dx1 = dy1 = radius;
        dx2 = dx1;
        dy2 = -dy1;
    }

    if( filled )
    {
        GRLine( panel->GetClipBox(), DC, ox - dx1, oy - dy1, ox + dx1, oy + dy1, width, gcolor );
        GRLine( panel->GetClipBox(), DC, ox - dx2, oy - dy2, ox + dx2, oy + dy2, width, gcolor );
    }
    else
    {
        GRCSegm( panel->GetClipBox(), DC, ox - dx1, oy - dy1, ox + dx1, oy + dy1, width, gcolor );
        GRCSegm( panel->GetClipBox(), DC, ox - dx2, oy - dy2, ox + dx2, oy + dy2, width, gcolor );
    }
}


bool PCB_TARGET::HitTest( const wxPoint& aPosition ) const
{
    int dX = aPosition.x - m_Pos.x;
    int dY = aPosition.y - m_Pos.y;
    int radius = m_Size / 2;
    return abs( dX ) <= radius && abs( dY ) <= radius;
}


bool PCB_TARGET::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( GetBoundingBox() );
    else
        return GetBoundingBox().Intersects( arect );
}


void PCB_TARGET::Rotate(const wxPoint& aRotCentre, double aAngle)
{
    RotatePoint( &m_Pos, aRotCentre, aAngle );
}


void PCB_TARGET::Flip(const wxPoint& aCentre )
{
    m_Pos.y  = aCentre.y - ( m_Pos.y - aCentre.y );
    SetLayer( FlipLayer( GetLayer() ) );
}


const EDA_RECT PCB_TARGET::GetBoundingBox() const
{
    EDA_RECT bBox;
    bBox.SetX( m_Pos.x - m_Size/2 );
    bBox.SetY( m_Pos.y - m_Size/2 );
    bBox.SetWidth( m_Size );
    bBox.SetHeight( m_Size );

    return bBox;
}


wxString PCB_TARGET::GetSelectMenuText() const
{
    wxString text;
    wxString msg;

    msg = ::CoordinateToString( m_Size );

    // Targets are on *every* layer by definition
    text.Printf( _( "Target size %s" ), GetChars( msg ) );

    return text;
}


EDA_ITEM* PCB_TARGET::Clone() const
{
    return new PCB_TARGET( *this );
}
