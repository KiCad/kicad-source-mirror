/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012 Alexander Lunev <al.lunev@yahoo.com>
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
 * @file pcb_arc.cpp
 */

#include <wx/wx.h>
#include <wx/config.h>

#include <common.h>
#include <trigo.h>

#include <pcb_arc.h>

namespace PCAD2KICAD {

PCB_ARC::PCB_ARC( PCB_CALLBACKS* aCallbacks, BOARD* aBoard ) : PCB_COMPONENT( aCallbacks, aBoard )
{
    m_objType   = wxT( 'A' );
    m_startX    = 0;
    m_startY    = 0;
    m_angle     = 0;
    m_width     = 0;
}


PCB_ARC::~PCB_ARC()
{
}


void PCB_ARC::Parse( XNODE*     aNode,
                     int        aLayer,
                     wxString   aDefaultMeasurementUnit,
                     wxString   aActualConversion )
{
    XNODE*      lNode;
    double      r = 0.0, a = 0.0;
    int         endPointX, endPointY;

    m_PCadLayer     = aLayer;
    m_KiCadLayer    = GetKiCadLayer();
    if( FindNode( aNode, wxT( "width" ) ) )
        SetWidth( FindNode( aNode, wxT( "width" ) )->GetNodeContent(),
                  aDefaultMeasurementUnit, &m_width, aActualConversion );

    if( aNode->GetName() == wxT( "triplePointArc" ) )
    {
        // center point
        lNode = FindNode( aNode, wxT( "pt" ) );

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &m_positionX, &m_positionY, aActualConversion );

        // start point
        lNode = lNode->GetNext();

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &m_startX, &m_startY, aActualConversion );

        // end point
        lNode = lNode->GetNext();

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &endPointX, &endPointY, aActualConversion );

        int alpha1  = ArcTangente( m_startY - m_positionY, m_startX - m_positionX );
        int alpha2  = ArcTangente( endPointY - m_positionY, endPointX - m_positionX );
        m_angle = alpha1 - alpha2;

        if( m_angle < 0 )
            m_angle = 3600 + m_angle;
    }

    if( aNode->GetName() == wxT( "arc" ) )
    {
        lNode = FindNode( aNode, wxT( "pt" ) );

        if( lNode )
            SetPosition( lNode->GetNodeContent(), aDefaultMeasurementUnit,
                         &m_positionX, &m_positionY, aActualConversion );

        lNode   = FindNode( aNode, wxT( "radius" ) );
        if( lNode)
            r = StrToIntUnits( lNode->GetNodeContent(), wxT( ' ' ), aActualConversion );

        lNode   = FindNode( aNode, wxT( "startAngle" ) );
        if( lNode )
            a = StrToInt1Units( lNode->GetNodeContent() );

        m_startX    = KiROUND( m_positionX + r * sin( (a - 900.0) * M_PI / 1800.0 ) );
        m_startY    = KiROUND( m_positionY - r * cos( (a - 900.0) * M_PI / 1800.0 ) );

        lNode   = FindNode( aNode, wxT( "sweepAngle" ) );
        if( lNode )
            m_angle = StrToInt1Units( lNode->GetNodeContent() );
    }
}


void PCB_ARC::SetPosOffset( int aX_offs, int aY_offs )
{
    PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

    m_startX    += aX_offs;
    m_startY    += aY_offs;
}


void PCB_ARC::AddToModule( MODULE* aModule )
{
    if( IsValidNonCopperLayerIndex( m_KiCadLayer ) )
    {
        EDGE_MODULE* arc = new EDGE_MODULE( aModule, S_ARC );
        aModule->m_Drawings.PushBack( arc );

        arc->SetAngle( -m_angle );
        arc->m_Start0   = wxPoint( m_positionX, m_positionY );
        arc->m_End0     = wxPoint( m_startX, m_startY );

        arc->SetWidth( m_width );
        arc->SetLayer( m_KiCadLayer );

        arc->SetDrawCoord();
    }
}


void PCB_ARC::AddToBoard()
{
    DRAWSEGMENT* dseg = new DRAWSEGMENT( m_board );

    m_board->Add( dseg, ADD_APPEND );

    dseg->SetShape( S_ARC );
    dseg->SetTimeStamp( m_timestamp );
    dseg->SetLayer( m_KiCadLayer );
    dseg->SetStart( wxPoint( m_positionX, m_positionY ) );
    dseg->SetEnd( wxPoint( m_startX, m_startY ) );
    dseg->SetAngle( -m_angle );
    dseg->SetWidth( m_width );
}

} // namespace PCAD2KICAD
