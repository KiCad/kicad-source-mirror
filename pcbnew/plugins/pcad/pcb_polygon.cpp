/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcb_polygon.cpp
 */

#include <wx/wx.h>

#include <common.h>
#include <math/util.h>      // for KiROUND
#include <fp_shape.h>
#include <pcb_polygon.h>

namespace PCAD2KICAD {

PCB_POLYGON::PCB_POLYGON( PCB_CALLBACKS* aCallbacks, BOARD* aBoard, int aPCadLayer ) :
    PCB_COMPONENT( aCallbacks, aBoard )
{
    m_width      = 0;

    // P-CAD polygons are similar to zones (and we're going to convert them as such), except
    // that they don't avoid other copper pours.  So effectively they're very-high-priority
    // zones.
    m_priority   = 100000;

    m_objType    = wxT( 'Z' );
    m_PCadLayer  = aPCadLayer;
    m_KiCadLayer = GetKiCadLayer();
    m_filled     = true;
}


PCB_POLYGON::~PCB_POLYGON()
{
    int i, island;

    for( i = 0; i < (int) m_outline.GetCount(); i++ )
    {
        delete m_outline[i];
    }

    for( island = 0; island < (int) m_cutouts.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_cutouts[island]->GetCount(); i++ )
        {
            delete (*m_cutouts[island])[i];
        }

        delete m_cutouts[island];
    }

    for( island = 0; island < (int) m_islands.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_islands[island]->GetCount(); i++ )
        {
            delete (*m_islands[island])[i];
        }

        delete m_islands[island];
    }
}

void PCB_POLYGON::AssignNet( const wxString& aNetName )
{
    m_net = aNetName;
    m_netCode = GetNetCode( m_net );
}

void PCB_POLYGON::SetOutline( VERTICES_ARRAY* aOutline )
{
    int i;

    m_outline.Empty();

    for( i = 0; i < (int) aOutline->GetCount(); i++ )
        m_outline.Add( new wxRealPoint( (*aOutline)[i]->x, (*aOutline)[i]->y ) );

    if( m_outline.Count() > 0 )
    {
        m_positionX = m_outline[0]->x;
        m_positionY = m_outline[0]->y;
    }
}

void PCB_POLYGON::FormPolygon( XNODE*   aNode, VERTICES_ARRAY* aPolygon,
                               const wxString& aDefaultMeasurementUnit,
                               const wxString& aActualConversion )
{
    XNODE*      lNode;
    double      x, y;

    lNode = FindNode( aNode, wxT( "pt" ) );

    while( lNode )
    {
        if( lNode->GetName() == wxT( "pt" ) )
        {
            SetDoublePrecisionPosition(
                lNode->GetNodeContent(), aDefaultMeasurementUnit, &x, &y, aActualConversion );
            aPolygon->Add( new wxRealPoint( x, y ) );
        }

        lNode = lNode->GetNext();
    }
}


bool PCB_POLYGON::Parse( XNODE*          aNode,
                         const wxString& aDefaultMeasurementUnit,
                         const wxString& aActualConversion )
{
    XNODE*      lNode;
    wxString    propValue;

    lNode = FindNode( aNode, wxT( "netNameRef" ) );

    if( lNode )
    {
        lNode->GetAttribute( wxT( "Name" ), &propValue );
        propValue.Trim( false );
        propValue.Trim( true );
        m_net = propValue;
        m_netCode = GetNetCode( m_net );
    }

    // retrieve polygon outline
    FormPolygon( aNode, &m_outline, aDefaultMeasurementUnit, aActualConversion );

    m_positionX = m_outline[0]->x;
    m_positionY = m_outline[0]->y;

    // fill the polygon with the same contour as its outline is
    m_islands.Add( new VERTICES_ARRAY );
    FormPolygon( aNode, m_islands[0], aDefaultMeasurementUnit, aActualConversion );

    return true;
}


void PCB_POLYGON::AddToFootprint( FOOTPRINT* aFootprint )
{
    if( IsNonCopperLayer( m_KiCadLayer ) )
    {
        FP_SHAPE* dwg = new FP_SHAPE( aFootprint, PCB_SHAPE_TYPE::POLYGON );
        aFootprint->Add( dwg );

        dwg->SetWidth( 0 );
        dwg->SetLayer( m_KiCadLayer );

        auto outline = new std::vector<wxPoint>;
        for( auto point : m_outline )
            outline->push_back( wxPoint( point->x, point->y ) );

        dwg->SetPolyPoints( *outline );
        dwg->SetStart0( *outline->begin() );
        dwg->SetEnd0( outline->back() );
        dwg->SetDrawCoord();

        delete( outline );
    }
}


void PCB_POLYGON::AddToBoard()
{
    int i = 0;

    if( m_outline.GetCount() > 0 )
    {
        ZONE* zone = new ZONE( m_board );
        m_board->Add( zone, ADD_MODE::APPEND );

        zone->SetLayer( m_KiCadLayer );
        zone->SetNetCode( m_netCode );

        // add outline
        for( i = 0; i < (int) m_outline.GetCount(); i++ )
        {
            zone->AppendCorner( wxPoint( KiROUND( m_outline[i]->x ),
                                         KiROUND( m_outline[i]->y ) ), -1 );
        }

        zone->SetLocalClearance( m_width );

        zone->SetPriority( m_priority );

        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                     zone->GetDefaultHatchPitch(), true );

        if ( m_objType == wxT( 'K' ) )
        {
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowTracks( true );
            zone->SetDoNotAllowVias( true );
            zone->SetDoNotAllowPads( true );
            zone->SetDoNotAllowCopperPour( true );
            zone->SetDoNotAllowFootprints( false );
        }
        else if( m_objType == wxT( 'C' ) )
        {
            // convert cutouts to keepouts because standalone cutouts are not supported in KiCad
            zone->SetIsRuleArea( true );
            zone->SetDoNotAllowCopperPour( true );
            zone->SetDoNotAllowTracks( false );
            zone->SetDoNotAllowVias( false );
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );
        }

        //if( m_filled )
        //    zone->BuildFilledPolysListData( m_board );
    }
}


void PCB_POLYGON::Flip()
{
    PCB_COMPONENT::Flip();

    m_KiCadLayer = FlipLayer( m_KiCadLayer );
}


void PCB_POLYGON::SetPosOffset( int aX_offs, int aY_offs )
{
    int i, island;

    PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

    for( i = 0; i < (int) m_outline.GetCount(); i++ )
    {
        m_outline[i]->x += aX_offs;
        m_outline[i]->y += aY_offs;
    }

    for( island = 0; island < (int) m_islands.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_islands[island]->GetCount(); i++ )
        {
            (*m_islands[island])[i]->x  += aX_offs;
            (*m_islands[island])[i]->y  += aY_offs;
        }
    }

    for( island = 0; island < (int) m_cutouts.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_cutouts[island]->GetCount(); i++ )
        {
            (*m_cutouts[island])[i]->x  += aX_offs;
            (*m_cutouts[island])[i]->y  += aY_offs;
        }
    }
}

} // namespace PCAD2KICAD
