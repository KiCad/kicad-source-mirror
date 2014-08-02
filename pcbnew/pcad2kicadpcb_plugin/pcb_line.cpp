/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file pcb_line.cpp
 */

#include <wx/wx.h>
#include <wx/config.h>

#include <common.h>

#include <pcb_line.h>

namespace PCAD2KICAD {

PCB_LINE::PCB_LINE( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCB_COMPONENT( aCallbacks,
                                                                                aBoard )
{
    m_width     = 0;
    m_toX       = 0;
    m_toY       = 0;
    m_objType   = wxT( 'L' );
}


PCB_LINE::~PCB_LINE()
{
}


void PCB_LINE::Parse( XNODE*        aNode,
                      int           aLayer,
                      wxString      aDefaultMeasurementUnit,
                      wxString      aActualConversion )
{
    XNODE*      lNode;
    wxString    propValue;

    m_PCadLayer     = aLayer;
    m_KiCadLayer    = GetKiCadLayer();
    m_positionX     = 0;
    m_positionY     = 0;
    m_toX   = 0;
    m_toY   = 0;
    m_width = 0;
    lNode   = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
        SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                     &m_positionX, &m_positionY, aActualConversion );

    lNode = lNode->GetNext();

    if( lNode )
        SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                     &m_toX, &m_toY, aActualConversion );

    lNode = FindNode( aNode, wxT( "width" ) );

    if( lNode )
        SetWidth( lNode->GetNodeContent(), aDefaultMeasurementUnit, &m_width, aActualConversion );

    lNode = FindNode( aNode, wxT( "netNameRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_net = propValue;
        m_netCode = GetNetCode( m_net );
    }
}


void PCB_LINE::SetPosOffset( int aX_offs, int aY_offs )
{
    PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

    m_toX   += aX_offs;
    m_toY   += aY_offs;
}


void PCB_LINE::Flip()
{
    PCB_COMPONENT::Flip();

    m_toX = -m_toX;
    m_KiCadLayer = FlipLayer( m_KiCadLayer );
}


void PCB_LINE::AddToModule( MODULE* aModule )
{
    if( IsNonCopperLayer( m_KiCadLayer ) )
    {
        EDGE_MODULE* segment = new EDGE_MODULE( aModule, S_SEGMENT );
        aModule->GraphicalItems().PushBack( segment );

        segment->m_Start0   = wxPoint( m_positionX, m_positionY );
        segment->m_End0     = wxPoint( m_toX, m_toY );

        segment->SetWidth( m_width );
        segment->SetLayer( m_KiCadLayer );

        segment->SetDrawCoord();
    }
}


void PCB_LINE::AddToBoard()
{
    if( IsCopperLayer( m_KiCadLayer ) )
    {
        TRACK* track = new TRACK( m_board );
        m_board->m_Track.Append( track );

        track->SetTimeStamp( m_timestamp );

        track->SetPosition( wxPoint( m_positionX, m_positionY ) );
        track->SetEnd( wxPoint( m_toX, m_toY ) );

        track->SetWidth( m_width );

        track->SetLayer( m_KiCadLayer );
        track->SetNetCode( m_netCode );
    }
    else
    {
        DRAWSEGMENT* dseg = new DRAWSEGMENT( m_board );
        m_board->Add( dseg, ADD_APPEND );

        dseg->SetTimeStamp( m_timestamp );
        dseg->SetLayer( m_KiCadLayer );
        dseg->SetStart( wxPoint( m_positionX, m_positionY ) );
        dseg->SetEnd( wxPoint( m_toX, m_toY ) );
        dseg->SetWidth( m_width );
    }
}

} // namespace PCAD2KICAD
