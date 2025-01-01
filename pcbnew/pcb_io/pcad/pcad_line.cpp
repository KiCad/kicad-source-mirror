/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <pcad/pcad_line.h>

#include <board.h>
#include <common.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_LINE::PCAD_LINE( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCAD_PCB_COMPONENT( aCallbacks, aBoard )
{
    m_Width     = 0;
    m_ToX       = 0;
    m_ToY       = 0;
    m_ObjType = wxT( 'L' );
}


PCAD_LINE::~PCAD_LINE()
{
}


void PCAD_LINE::Parse( XNODE* aNode, int aLayer, const wxString& aDefaultUnits,
                       const wxString& aActualConversion )
{
    XNODE*      lNode;
    wxString    propValue;

    m_PCadLayer  = aLayer;
    m_KiCadLayer = GetKiCadLayer();
    m_PositionX  = 0;
    m_PositionY  = 0;
    m_ToX        = 0;
    m_ToY        = 0;
    m_Width      = 0;
    lNode        = FindNode( aNode, wxT( "pt" ) );

    if( lNode )
    {
        SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_PositionX, &m_PositionY,
                     aActualConversion );
    }

    if( lNode )
        lNode = lNode->GetNext();

    if( lNode )
        SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_ToX, &m_ToY, aActualConversion );

    lNode = FindNode( aNode, wxT( "width" ) );

    if( lNode )
        SetWidth( lNode->GetNodeContent(), aDefaultUnits, &m_Width, aActualConversion );

    lNode = FindNode( aNode, wxT( "netNameRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_Net = propValue;
        m_NetCode = GetNetCode( m_Net );
    }
}


void PCAD_LINE::SetPosOffset( int aX_offs, int aY_offs )
{
    PCAD_PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

    m_ToX   += aX_offs;
    m_ToY   += aY_offs;
}


void PCAD_LINE::Flip()
{
    PCAD_PCB_COMPONENT::Flip();

    m_ToX = -m_ToX;
    m_KiCadLayer = m_board->FlipLayer( m_KiCadLayer );
}


void PCAD_LINE::AddToBoard( FOOTPRINT* aFootprint )
{
    if( IsCopperLayer( m_KiCadLayer ) && !aFootprint )
    {
        PCB_TRACK* track = new PCB_TRACK( m_board );
        m_board->Add( track );

        track->SetPosition( VECTOR2I( m_PositionX, m_PositionY ) );
        track->SetEnd( VECTOR2I( m_ToX, m_ToY ) );

        track->SetWidth( m_Width );

        track->SetLayer( m_KiCadLayer );
        track->SetNetCode( m_NetCode );
    }
    else
    {
        PCB_SHAPE* segment = new PCB_SHAPE( m_board, SHAPE_T::SEGMENT );
        m_board->Add( segment, ADD_MODE::APPEND );

        segment->SetLayer( m_KiCadLayer );
        segment->SetStart( VECTOR2I( m_PositionX, m_PositionY ) );
        segment->SetEnd( VECTOR2I( m_ToX, m_ToY ) );
        segment->SetStroke( STROKE_PARAMS( m_Width, LINE_STYLE::SOLID ) );

        if( aFootprint )
        {
            segment->Rotate( { 0, 0 }, aFootprint->GetOrientation() );
            segment->Move( aFootprint->GetPosition() );
        }
    }
}

} // namespace PCAD2KICAD
