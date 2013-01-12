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
#include <wxBasePcbFrame.h>
#include <pcbcommon.h>
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
    m_Width = 120;
}


EDGE_MODULE::~EDGE_MODULE()
{
}


void EDGE_MODULE::Copy( EDGE_MODULE* source )
{
    if( source == NULL )
        return;

    DRAWSEGMENT::Copy( source );

    m_Start0 = source->m_Start0;
    m_End0   = source->m_End0;

    m_PolyPoints = source->m_PolyPoints;    // std::vector copy
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

        m_Start += module->m_Pos;
        m_End   += module->m_Pos;
    }
}


void EDGE_MODULE::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, GR_DRAWMODE draw_mode,
                        const wxPoint& offset )
{
    int             ux0, uy0, dx, dy, radius, StAngle, EndAngle;
    int             type_trace;
    int             typeaff;
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    PCB_BASE_FRAME* frame;
    MODULE* module = (MODULE*) m_Parent;

    if( module == NULL )
        return;


    BOARD * brd = GetBoard( );

    if( brd->IsLayerVisible( m_Layer ) == false )
        return;

    EDA_COLOR_T color = brd->GetLayerColor( m_Layer );

    if(( draw_mode & GR_ALLOW_HIGHCONTRAST ) && DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
            ColorTurnToDarkDarkGray( &color );
    }


    frame = (PCB_BASE_FRAME*) panel->GetParent();

    type_trace = m_Shape;

    ux0 = m_Start.x - offset.x;
    uy0 = m_Start.y - offset.y;

    dx = m_End.x - offset.x;
    dy = m_End.y - offset.y;

    GRSetDrawMode( DC, draw_mode );
    typeaff = frame->m_DisplayModEdge;

    if( m_Layer <= LAST_COPPER_LAYER )
    {
        typeaff = frame->m_DisplayPcbTrackFill;

        if( !typeaff )
            typeaff = SKETCH;
    }

    if( DC->LogicalToDeviceXRel( m_Width ) < MIN_DRAW_WIDTH )
        typeaff = LINE;

    switch( type_trace )
    {
    case S_SEGMENT:
        if( typeaff == LINE )
            GRLine( panel->GetClipBox(), DC, ux0, uy0, dx, dy, 0, color );
        else if( typeaff == FILLED )
            GRLine( panel->GetClipBox(), DC, ux0, uy0, dx, dy, m_Width, color );
        else
            // SKETCH Mode
            GRCSegm( panel->GetClipBox(), DC, ux0, uy0, dx, dy, m_Width, color );

        break;

    case S_CIRCLE:
        radius = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );

        if( typeaff == LINE )
        {
            GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius, color );
        }
        else
        {
            if( typeaff == FILLED )
            {
                GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius, m_Width, color );
            }
            else        // SKETCH Mode
            {
                GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius + (m_Width / 2), color );
                GRCircle( panel->GetClipBox(), DC, ux0, uy0, radius - (m_Width / 2), color );
            }
        }

        break;

    case S_ARC:
        radius   = (int) hypot( (double) (dx - ux0), (double) (dy - uy0) );
        StAngle  = (int) ArcTangente( dy - uy0, dx - ux0 );
        EndAngle = StAngle + m_Angle;

        if( StAngle > EndAngle )
            EXCHG( StAngle, EndAngle );

        if( typeaff == LINE )
        {
            GRArc( panel->GetClipBox(), DC, ux0, uy0, StAngle, EndAngle, radius, color );
        }
        else if( typeaff == FILLED )
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

        // We must compute true coordinates from m_PolyPoints
        // which are relative to module position, orientation 0

        std::vector<wxPoint> points = m_PolyPoints;

        for( unsigned ii = 0; ii < points.size(); ii++ )
        {
            wxPoint& pt = points[ii];

            RotatePoint( &pt.x, &pt.y, module->GetOrientation() );
            pt += module->m_Pos - offset;
        }

        GRPoly( panel->GetClipBox(), DC, points.size(), &points[0], true, m_Width, color, color );
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

    aList.push_back( MSG_PANEL_ITEM( _( "Graphic Item" ), wxEmptyString, DARKCYAN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Module" ), module->m_Reference->m_Text, DARKCYAN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Value" ), module->m_Value->m_Text, BLUE ) );
    msg.Printf( wxT( "%8.8lX" ), module->GetTimeStamp() );
    aList.push_back( MSG_PANEL_ITEM( _( "TimeStamp" ), msg, BROWN ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Mod Layer" ), board->GetLayerName( module->GetLayer() ),
                                     RED ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Seg Layer" ), board->GetLayerName( GetLayer() ), RED ) );
    msg = ::CoordinateToString( m_Width );
    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, BLUE ) );
}



wxString EDGE_MODULE::GetSelectMenuText() const
{
    wxString text;

    text << _( "Graphic" ) << wxT( " " ) << ShowShape( (STROKE_T) m_Shape );
    text << wxT( " (" ) << GetLayerName() << wxT( ")" );
    text << _( " of " ) << ( (MODULE*) GetParent() )->GetReference();

    return text;
}


EDA_ITEM* EDGE_MODULE::Clone() const
{
    return new EDGE_MODULE( *this );
}


#if defined(DEBUG)

void EDGE_MODULE::Show( int nestLevel, std::ostream& os ) const
{
    wxString shape = ShowShape( (STROKE_T) m_Shape );

    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " type=\"" << TO_UTF8( shape ) << "\">";

    os << " <start" << m_Start0 << "/>";
    os << " <end" << m_End0 << "/>";

    os << " </" << GetClass().Lower().mb_str() << ">\n";
}


#endif
