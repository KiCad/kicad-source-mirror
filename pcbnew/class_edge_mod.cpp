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
 * @file class_edge_mod.cpp
 * @brief EDGE_MODULE class definition.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <wxstruct.h>
#include <trigo.h>
#include <class_drawpanel.h>
#include <class_pcb_screen.h>
#include <confirm.h>
#include <kicad_string.h>
#include <colors_selection.h>
#include <richio.h>
#include <macros.h>
#include <math_for_graphics.h>
#include <wxBasePcbFrame.h>
#include <msgpanel.h>
#include <base_units.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <stdio.h>

EDGE_MODULE::EDGE_MODULE( MODULE* parent, STROKE_T aShape ) :
    DRAWSEGMENT( parent, PCB_MODULE_EDGE_T )
{
    m_Shape = aShape;
    m_Angle = 0;
    m_Layer = F_SilkS;
}


EDGE_MODULE::~EDGE_MODULE()
{
}


const EDGE_MODULE& EDGE_MODULE::operator = ( const EDGE_MODULE& rhs )
{
    if( &rhs == this )
        return *this;

    DRAWSEGMENT::operator=( rhs );

    m_Start0 = rhs.m_Start0;
    m_End0   = rhs.m_End0;

    m_PolyPoints = rhs.m_PolyPoints;    // std::vector copy
    return *this;
}


void EDGE_MODULE::Copy( EDGE_MODULE* source )
{
    if( source == NULL )
        return;

    *this = *source;
}


void EDGE_MODULE::SetLocalCoord()
{
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )
    {
        m_Start0 = m_Start;
        m_End0 = m_End;
        return;
    }

    m_Start0 = m_Start - module->GetPosition();
    m_End0 = m_End - module->GetPosition();
    double angle = module->GetOrientation();
    RotatePoint( &m_Start0.x, &m_Start0.y, -angle );
    RotatePoint( &m_End0.x, &m_End0.y, -angle );
}


void EDGE_MODULE::SetDrawCoord()
{
    MODULE* module = (MODULE*) m_Parent;

    m_Start = m_Start0;
    m_End   = m_End0;

    if( module )
    {
        RotatePoint( &m_Start.x, &m_Start.y, module->GetOrientation() );
        RotatePoint( &m_End.x,   &m_End.y,   module->GetOrientation() );

        m_Start += module->GetPosition();
        m_End   += module->GetPosition();
    }
}


void EDGE_MODULE::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE draw_mode,
                        const wxPoint& offset )
{
    int         ux0, uy0, dx, dy, radius, StAngle, EndAngle;
    LAYER_ID    curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    MODULE* module = (MODULE*) m_Parent;

    if( !module )
        return;

    BOARD* brd = GetBoard( );

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    EDA_COLOR_T color = brd->GetLayerColor( m_Layer );
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)panel->GetDisplayOptions();

    if(( draw_mode & GR_ALLOW_HIGHCONTRAST ) && displ_opts && displ_opts->m_ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
            ColorTurnToDarkDarkGray( &color );
    }

    ux0 = m_Start.x - offset.x;
    uy0 = m_Start.y - offset.y;

    dx = m_End.x - offset.x;
    dy = m_End.y - offset.y;

    GRSetDrawMode( DC, draw_mode );
    bool filled = displ_opts ? displ_opts->m_DisplayModEdgeFill : FILLED;

    if( IsCopperLayer( m_Layer ) )
        filled = displ_opts ? displ_opts->m_DisplayPcbTrackFill : FILLED;

    switch( m_Shape )
    {
    case S_SEGMENT:
        if( filled )
            GRLine( panel->GetClipBox(), DC, ux0, uy0, dx, dy, m_Width, color );
        else
            // SKETCH Mode
            GRCSegm( panel->GetClipBox(), DC, ux0, uy0, dx, dy, m_Width, color );

        break;

    case S_CIRCLE:
        radius = KiROUND( Distance( ux0, uy0, dx, dy ) );

        if( filled )
        {
            GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius, m_Width, color );
        }
        else        // SKETCH Mode
        {
            GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius + (m_Width / 2), color );
            GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius - (m_Width / 2), color );
        }

        break;

    case S_ARC:
        radius   = KiROUND( Distance( ux0, uy0, dx, dy ) );
        StAngle  = ArcTangente( dy - uy0, dx - ux0 );
        EndAngle = StAngle + m_Angle;

        if( !panel->GetPrintMirrored() )
        {
            if( StAngle > EndAngle )
                std::swap( StAngle, EndAngle );
        }
        else    // Mirrored mode: arc orientation is reversed
        {
            if( StAngle < EndAngle )
                std::swap( StAngle, EndAngle );
        }

        if( filled )
        {
            GRArc( panel->GetClipBox(), DC, ux0, uy0, StAngle, EndAngle, radius, m_Width, color );
        }
        else        // SKETCH Mode
        {
            GRArc( panel->GetClipBox(), DC, ux0, uy0, StAngle, EndAngle,
                   radius + (m_Width / 2), color );
            GRArc( panel->GetClipBox(), DC, ux0, uy0, StAngle, EndAngle,
                   radius - (m_Width / 2), color );
        }
        break;

    case S_POLYGON:
        {
        // We must compute true coordinates from m_PolyPoints
        // which are relative to module position, orientation 0
        std::vector<wxPoint> points = m_PolyPoints;

        for( unsigned ii = 0; ii < points.size(); ii++ )
        {
            wxPoint& pt = points[ii];

            RotatePoint( &pt.x, &pt.y, module->GetOrientation() );
            pt += module->GetPosition() - offset;
        }

        GRPoly( panel->GetClipBox(), DC, points.size(), &points[0], true, m_Width, color, color );
        }
        break;

    default:
        break;
    }
}


// see class_edge_mod.h
void EDGE_MODULE::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;

    MODULE*  module = (MODULE*) m_Parent;

    if( !module )
        return;

    BOARD* board = (BOARD*) module->GetParent();

    if( !board )
        return;

    aList.push_back( MSG_PANEL_ITEM( _( "Footprint" ), module->GetReference(), DARKCYAN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Value" ), module->GetValue(), BLUE ) );
    msg.Printf( wxT( "%8.8lX" ), module->GetTimeStamp() );
    aList.push_back( MSG_PANEL_ITEM( _( "TimeStamp" ), msg, BROWN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Footprint Layer" ),
                     module->GetLayerName(), RED ) );

    // append the features shared with the base class
    DRAWSEGMENT::GetMsgPanelInfo( aList );
}



wxString EDGE_MODULE::GetSelectMenuText() const
{
    wxString text;
    text.Printf( _( "Graphic (%s) on %s of %s" ),
            GetChars( ShowShape( (STROKE_T) m_Shape ) ),
            GetChars( GetLayerName() ),
            GetChars( ((MODULE*) GetParent())->GetReference() ) );

    return text;
}


EDA_ITEM* EDGE_MODULE::Clone() const
{
    return new EDGE_MODULE( *this );
}


void EDGE_MODULE::Flip( const wxPoint& aCentre )
{
    wxPoint pt;

    switch( GetShape() )
    {
    case S_ARC:
        SetAngle( -GetAngle() );
        //Fall through
    default:
    case S_SEGMENT:
        pt = GetStart();
        pt.y -= aCentre.y;
        pt.y  = -pt.y;
        pt.y += aCentre.y;
        SetStart( pt );

        pt = GetEnd();
        pt.y -= aCentre.y;
        pt.y  = -pt.y;
        pt.y += aCentre.y;
        SetEnd( pt );

        m_Start0.y = -m_Start0.y;
        m_End0.y = -m_End0.y;
        break;

    case S_POLYGON:
        // polygon corners coordinates are always relative to the
        // footprint position, orientation 0
        for( unsigned ii = 0; ii < m_PolyPoints.size(); ii++ )
            m_PolyPoints[ii].y = -m_PolyPoints[ii].y;
    }

    SetLayer( FlipLayer( GetLayer() ) );
}

void EDGE_MODULE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // do the base class rotation
    DRAWSEGMENT::Rotate( aRotCentre, aAngle );

    // and now work out the new offset
    SetLocalCoord();
}
