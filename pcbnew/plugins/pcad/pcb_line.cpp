/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2012-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <common.h>
#include <fp_shape.h>
#include <pcb_shape.h>
#include <pcb_line.h>

namespace PCAD2KICAD {

PCB_LINE::PCB_LINE( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCB_COMPONENT( aCallbacks,
                                                                                aBoard )
{
    m_Width     = 0;
    m_ToX       = 0;
    m_ToY       = 0;
    m_objType   = wxT( 'L' );
}


PCB_LINE::~PCB_LINE()
{
}


void PCB_LINE::Parse( XNODE*          aNode,
                      int             aLayer,
                      const wxString& aDefaultMeasurementUnit,
                      const wxString& aActualConversion )
{
    XNODE*      lNode;
    wxString    propValue;

    m_PCadLayer     = aLayer;
    m_KiCadLayer    = GetKiCadLayer();
    m_positionX     = 0;
    m_positionY     = 0;
    m_ToX   = 0;
    m_ToY   = 0;
    m_Width = 0;
    lNode   = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
        SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                     &m_positionX, &m_positionY, aActualConversion );

    if( lNode )
        lNode = lNode->GetNext();

    if( lNode )
        SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                     &m_ToX, &m_ToY, aActualConversion );

    lNode = FindNode( aNode, wxT( "width" ) );

    if( lNode )
        SetWidth( lNode->GetNodeContent(), aDefaultMeasurementUnit, &m_Width, aActualConversion );

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

    m_ToX   += aX_offs;
    m_ToY   += aY_offs;
}


void PCB_LINE::Flip()
{
    PCB_COMPONENT::Flip();

    m_ToX = -m_ToX;
    m_KiCadLayer = FlipLayer( m_KiCadLayer );
}


void PCB_LINE::AddToFootprint( FOOTPRINT* aFootprint )
{
    if( IsNonCopperLayer( m_KiCadLayer ) )
    {
        FP_SHAPE* segment = new FP_SHAPE( aFootprint, PCB_SHAPE_TYPE::SEGMENT );
        aFootprint->Add( segment );

        segment->m_Start0   = wxPoint( m_positionX, m_positionY );
        segment->m_End0     = wxPoint( m_ToX, m_ToY );

        segment->SetWidth( m_Width );
        segment->SetLayer( m_KiCadLayer );

        segment->SetDrawCoord();
    }
}


void PCB_LINE::AddToBoard()
{
    if( IsCopperLayer( m_KiCadLayer ) )
    {
        TRACK* track = new TRACK( m_board );
        m_board->Add( track );

        track->SetPosition( wxPoint( m_positionX, m_positionY ) );
        track->SetEnd( wxPoint( m_ToX, m_ToY ) );

        track->SetWidth( m_Width );

        track->SetLayer( m_KiCadLayer );
        track->SetNetCode( m_netCode );
    }
    else
    {
        PCB_SHAPE* segment = new PCB_SHAPE( m_board );
        m_board->Add( segment, ADD_MODE::APPEND );

        segment->SetLayer( m_KiCadLayer );
        segment->SetStart( wxPoint( m_positionX, m_positionY ) );
        segment->SetEnd( wxPoint( m_ToX, m_ToY ) );
        segment->SetWidth( m_Width );
    }
}

} // namespace PCAD2KICAD
