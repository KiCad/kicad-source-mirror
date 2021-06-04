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

#include <pcad/pcb_arc.h>

#include <board.h>
#include <footprint.h>
#include <fp_shape.h>
#include <math/util.h>      // for KiROUND
#include <pcb_shape.h>
#include <trigo.h>
#include <xnode.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCB_ARC::PCB_ARC( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCB_COMPONENT( aCallbacks, aBoard )
{
    m_objType    = wxT( 'A' );
    m_StartX     = 0;
    m_StartY     = 0;
    m_Angle      = 0;
    m_Width      = 0;
}


PCB_ARC::~PCB_ARC()
{
}


void PCB_ARC::Parse( XNODE*          aNode,
                     int             aLayer,
                     const wxString& aDefaultMeasurementUnit,
                     const wxString& aActualConversion )
{
    XNODE*      lNode;
    double      a = 0.0;
    int         r = 0;
    int         endX = 0;
    int         endY = 0;

    m_PCadLayer     = aLayer;
    m_KiCadLayer    = GetKiCadLayer();

    if( FindNode( aNode, wxT( "width" ) ) )
        SetWidth( FindNode( aNode, wxT( "width" ) )->GetNodeContent(),
                  aDefaultMeasurementUnit, &m_Width, aActualConversion );

    if( aNode->GetName() == wxT( "triplePointArc" ) )
    {
        // center point
        lNode = FindNode( aNode, wxT( "pt" ) );

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &m_positionX, &m_positionY, aActualConversion );

        // start point
        if( lNode )
            lNode = lNode->GetNext();

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &m_StartX, &m_StartY, aActualConversion );

        // end point
        if( lNode )
            lNode = lNode->GetNext();

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &endX, &endY, aActualConversion );

        if( m_StartX == endX && m_StartY == endY )
        {
            m_Angle = 3600;
        }
        else
        {
            double alpha1  = ArcTangente( m_StartY - m_positionY, m_StartX - m_positionX );
            double alpha2  = ArcTangente( endY - m_positionY, endX - m_positionX );
            m_Angle = alpha1 - alpha2;

            NORMALIZE_ANGLE_POS( m_Angle );
        }
    }
    else if( aNode->GetName() == wxT( "arc" ) )
    {
        lNode = FindNode( aNode, wxT( "pt" ) );

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &m_positionX, &m_positionY, aActualConversion );

        lNode   = FindNode( aNode, wxT( "radius" ) );
        if( lNode)
            SetWidth( FindNode( aNode, wxT( "radius" ) )->GetNodeContent(),
                      aDefaultMeasurementUnit, &r, aActualConversion );


        lNode   = FindNode( aNode, wxT( "startAngle" ) );
        if( lNode )
            a = StrToInt1Units( lNode->GetNodeContent() );

        lNode   = FindNode( aNode, wxT( "sweepAngle" ) );
        if( lNode )
            m_Angle = StrToInt1Units( lNode->GetNodeContent() );

        m_StartX = m_positionX + KiROUND( cosdecideg( r, a ) );
        m_StartY = m_positionY - KiROUND( sindecideg( r, a ) );
    }
}


void PCB_ARC::SetPosOffset( int aX_offs, int aY_offs )
{
    PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

    m_StartX    += aX_offs;
    m_StartY    += aY_offs;
}


void PCB_ARC::Flip()
{
    PCB_COMPONENT::Flip();

    m_StartX = -m_StartX;
    m_Angle = -m_Angle;

    m_KiCadLayer = FlipLayer( m_KiCadLayer );
}


void PCB_ARC::AddToFootprint( FOOTPRINT* aFootprint )
{
    if( IsNonCopperLayer( m_KiCadLayer ) )
    {
        FP_SHAPE* arc = new FP_SHAPE(
                aFootprint, ( IsCircle() ? PCB_SHAPE_TYPE::CIRCLE : PCB_SHAPE_TYPE::ARC ) );
        aFootprint->Add( arc );

        arc->m_Start0   = wxPoint( m_positionX, m_positionY );
        arc->m_End0     = wxPoint( m_StartX, m_StartY );

        // Setting angle will set m_ThirdPoint0, so must be done after setting
        // m_Start0 and m_End0
        arc->SetAngle( -m_Angle );

        arc->SetWidth( m_Width );
        arc->SetLayer( m_KiCadLayer );

        arc->SetDrawCoord();
    }
}


void PCB_ARC::AddToBoard()
{
    PCB_SHAPE* arc = new PCB_SHAPE( m_board );

    m_board->Add( arc, ADD_MODE::APPEND );

    arc->SetShape( IsCircle() ? PCB_SHAPE_TYPE::CIRCLE : PCB_SHAPE_TYPE::ARC );
    arc->SetFilled( false );
    arc->SetLayer( m_KiCadLayer );
    arc->SetStart( wxPoint( m_positionX, m_positionY ) );
    arc->SetEnd( wxPoint( m_StartX, m_StartY ) );
    arc->SetAngle( -m_Angle );
    arc->SetWidth( m_Width );
}


bool PCB_ARC::IsCircle()
{
    return ( m_Angle == 3600 );
}

} // namespace PCAD2KICAD
