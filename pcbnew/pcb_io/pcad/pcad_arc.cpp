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

#include <pcad/pcad_arc.h>

#include <board.h>
#include <footprint.h>
#include <math/util.h>      // for KiROUND
#include <pcb_shape.h>
#include <trigo.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_ARC::PCAD_ARC( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard ) :
        PCAD_PCB_COMPONENT( aCallbacks, aBoard )
{
    m_ObjType = wxT( 'A' );
    m_StartX     = 0;
    m_StartY     = 0;
    m_Angle      = ANGLE_0;
    m_Width      = 0;
}


PCAD_ARC::~PCAD_ARC()
{
}


void PCAD_ARC::Parse( XNODE* aNode, int aLayer, const wxString& aDefaultUnits,
                      const wxString& aActualConversion )
{
    XNODE*      lNode;
    int         r = 0;
    VECTOR2I    end;

    m_PCadLayer     = aLayer;
    m_KiCadLayer    = GetKiCadLayer();

    if( FindNode( aNode, wxT( "width" ) ) )
    {
        SetWidth( FindNode( aNode, wxT( "width" ) )->GetNodeContent(), aDefaultUnits, &m_Width,
                  aActualConversion );
    }

    if( aNode->GetName() == wxT( "triplePointArc" ) )
    {
        // center point
        lNode = FindNode( aNode, wxT( "pt" ) );

        if( lNode )
        {
            SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_PositionX, &m_PositionY,
                         aActualConversion );
        }

        // start point
        if( lNode )
            lNode = lNode->GetNext();

        if( lNode )
        {
            SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_StartX, &m_StartY,
                         aActualConversion );
        }

        // end point
        if( lNode )
            lNode = lNode->GetNext();

        if( lNode )
        {
            SetPosition( lNode->GetNodeContent(), aDefaultUnits, &end.x, &end.y,
                         aActualConversion );
        }

        VECTOR2I position( m_PositionX, m_PositionY );
        VECTOR2I start( m_StartX, m_StartY );

        if( start == end )
        {
            m_Angle = ANGLE_360;
        }
        else
        {
            EDA_ANGLE alpha1( start - position );
            EDA_ANGLE alpha2( end - position );
            m_Angle = alpha1 - alpha2;

            m_Angle.Normalize();
        }
    }
    else if( aNode->GetName() == wxT( "arc" ) )
    {
        lNode = FindNode( aNode, wxT( "pt" ) );

        if( lNode )
        {
            SetPosition( lNode->GetNodeContent(), aDefaultUnits, &m_PositionX, &m_PositionY,
                         aActualConversion );
        }

        lNode   = FindNode( aNode, wxT( "radius" ) );

        if( lNode)
        {
            SetWidth( FindNode( aNode, wxT( "radius" ) )->GetNodeContent(), aDefaultUnits, &r,
                      aActualConversion );
        }


        lNode   = FindNode( aNode, wxT( "startAngle" ) );

        EDA_ANGLE a = ANGLE_0;

        if( lNode )
            a = EDA_ANGLE( StrToInt1Units( lNode->GetNodeContent() ), TENTHS_OF_A_DEGREE_T );

        lNode   = FindNode( aNode, wxT( "sweepAngle" ) );

        if( lNode )
            m_Angle = EDA_ANGLE( StrToInt1Units( lNode->GetNodeContent() ), TENTHS_OF_A_DEGREE_T );

        m_StartX = m_PositionX + KiROUND( r * a.Cos() );
        m_StartY = m_PositionY - KiROUND( r * a.Sin() );
    }
}


void PCAD_ARC::SetPosOffset( int aX_offs, int aY_offs )
{
    PCAD_PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

    m_StartX    += aX_offs;
    m_StartY    += aY_offs;
}


void PCAD_ARC::Flip()
{
    PCAD_PCB_COMPONENT::Flip();

    m_StartX = -m_StartX;
    m_Angle = -m_Angle;

    m_KiCadLayer = m_board->FlipLayer( m_KiCadLayer );
}


void PCAD_ARC::AddToBoard( FOOTPRINT* aFootprint )
{
    PCB_SHAPE* arc = new PCB_SHAPE( aFootprint, IsCircle() ? SHAPE_T::CIRCLE : SHAPE_T::ARC );

    arc->SetCenter( VECTOR2I( m_PositionX, m_PositionY ) );
    arc->SetStart( VECTOR2I( m_StartX, m_StartY ) );
    arc->SetArcAngleAndEnd( -m_Angle, true );

    arc->SetStroke( STROKE_PARAMS( m_Width, LINE_STYLE::SOLID ) );
    arc->SetLayer( m_KiCadLayer );

    if( aFootprint )
    {
        aFootprint->Add( arc );
        arc->Rotate( { 0, 0 }, aFootprint->GetOrientation() );
        arc->Move( aFootprint->GetPosition() );
    }
    else
        m_board->Add( arc );
}


bool PCAD_ARC::IsCircle()
{
    return ( m_Angle == ANGLE_360 );
}

} // namespace PCAD2KICAD
