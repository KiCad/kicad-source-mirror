/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007, 2008 Lubo Racko <developer@lura.sk>
 * Copyright (C) 2007, 2008, 2012-2013 Alexander Lunev <al.lunev@yahoo.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcad/pcad_polygon.h>

#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <math/util.h>      // for KiROUND
#include <xnode.h>
#include <zone.h>

#include <wx/string.h>

namespace PCAD2KICAD {

PCAD_POLYGON::PCAD_POLYGON( PCAD_CALLBACKS* aCallbacks, BOARD* aBoard, int aPCadLayer ) :
        PCAD_PCB_COMPONENT( aCallbacks, aBoard )
{
    m_Width      = 0;

    // P-CAD polygons are similar to zones (and we're going to convert them as such), except
    // that they don't avoid other copper pours.  So effectively they're very-high-priority
    // zones.
    m_Priority   = 100000;

    m_ObjType = wxT( 'Z' );
    m_PCadLayer  = aPCadLayer;
    m_KiCadLayer = GetKiCadLayer();
    m_filled     = true;
}


PCAD_POLYGON::~PCAD_POLYGON()
{
    int i, island;

    for( i = 0; i < (int) m_Outline.GetCount(); i++ )
        delete m_Outline[i];

    for( island = 0; island < (int) m_Cutouts.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_Cutouts[island]->GetCount(); i++ )
            delete (*m_Cutouts[island])[i];

        delete m_Cutouts[island];
    }

    for( island = 0; island < (int) m_Islands.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_Islands[island]->GetCount(); i++ )
            delete (*m_Islands[island])[i];

        delete m_Islands[island];
    }
}

void PCAD_POLYGON::AssignNet( const wxString& aNetName )
{
    m_Net = aNetName;
    m_NetCode = GetNetCode( m_Net );
}

void PCAD_POLYGON::SetOutline( VERTICES_ARRAY* aOutline )
{
    int i;

    m_Outline.Empty();

    for( i = 0; i < (int) aOutline->GetCount(); i++ )
        m_Outline.Add( new wxRealPoint( (*aOutline)[i]->x, (*aOutline)[i]->y ) );

    if( m_Outline.Count() > 0 )
    {
        m_PositionX = m_Outline[0]->x;
        m_PositionY = m_Outline[0]->y;
    }
}

void PCAD_POLYGON::FormPolygon( XNODE* aNode, VERTICES_ARRAY* aPolygon,
                                const wxString& aDefaultUnits, const wxString& aActualConversion )
{
    XNODE*      lNode;
    double      x, y;

    lNode = FindNode( aNode, wxT( "pt" ) );

    while( lNode )
    {
        if( lNode->GetName() == wxT( "pt" ) )
        {
            SetDoublePrecisionPosition( lNode->GetNodeContent(), aDefaultUnits, &x, &y,
                                        aActualConversion );
            aPolygon->Add( new wxRealPoint( x, y ) );
        }

        lNode = lNode->GetNext();
    }
}


bool PCAD_POLYGON::Parse( XNODE* aNode, const wxString& aDefaultUnits,
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
        m_Net = propValue;
        m_NetCode = GetNetCode( m_Net );
    }

    // retrieve polygon outline
    FormPolygon( aNode, &m_Outline, aDefaultUnits, aActualConversion );

    m_PositionX = m_Outline[0]->x;
    m_PositionY = m_Outline[0]->y;

    // fill the polygon with the same contour as its outline is
    m_Islands.Add( new VERTICES_ARRAY );
    FormPolygon( aNode, m_Islands[0], aDefaultUnits, aActualConversion );

    return true;
}


void PCAD_POLYGON::AddToBoard( FOOTPRINT* aFootprint )
{
    if( m_Outline.GetCount() > 0 )
    {
        if( aFootprint )
        {
            PCB_SHAPE* dwg = new PCB_SHAPE( aFootprint, SHAPE_T::POLY );
            aFootprint->Add( dwg );

            dwg->SetStroke( STROKE_PARAMS( 0 ) );
            dwg->SetLayer( m_KiCadLayer );

            auto outline = new std::vector<VECTOR2I>;

            for( auto point : m_Outline )
                outline->push_back( VECTOR2I( point->x, point->y ) );

            dwg->SetPolyPoints( *outline );
            dwg->SetStart( *outline->begin() );
            dwg->SetEnd( outline->back() );
            dwg->Rotate( { 0, 0 }, aFootprint->GetOrientation() );
            dwg->Move( aFootprint->GetPosition() );

            delete( outline );
        }
        else
        {
            ZONE* zone = new ZONE( m_board );
            m_board->Add( zone, ADD_MODE::APPEND );

            zone->SetLayer( m_KiCadLayer );
            zone->SetNetCode( m_NetCode );

            // add outline
            for( int i = 0; i < (int) m_Outline.GetCount(); i++ )
            {
                zone->AppendCorner( KiROUND( m_Outline[i]->x, m_Outline[i]->y ), -1 );
            }

            zone->SetLocalClearance( m_Width );

            zone->SetAssignedPriority( m_Priority );

            zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE,
                                         zone->GetDefaultHatchPitch(), true );

            if ( m_ObjType == wxT( 'K' ) )
            {
                zone->SetIsRuleArea( true );
                zone->SetDoNotAllowTracks( true );
                zone->SetDoNotAllowVias( true );
                zone->SetDoNotAllowPads( true );
                zone->SetDoNotAllowZoneFills( true );
                zone->SetDoNotAllowFootprints( false );
            }
            else if( m_ObjType == wxT( 'C' ) )
            {
                // convert cutouts to keepouts because standalone cutouts are not supported in KiCad
                zone->SetIsRuleArea( true );
                zone->SetDoNotAllowZoneFills( true );
                zone->SetDoNotAllowTracks( false );
                zone->SetDoNotAllowVias( false );
                zone->SetDoNotAllowPads( false );
                zone->SetDoNotAllowFootprints( false );
            }

            //if( m_filled )
            //    zone->BuildFilledPolysListData( m_board );
        }
    }
}


void PCAD_POLYGON::Flip()
{
    PCAD_PCB_COMPONENT::Flip();

    m_KiCadLayer = m_board->FlipLayer( m_KiCadLayer );
}


void PCAD_POLYGON::SetPosOffset( int aX_offs, int aY_offs )
{
    int i, island;

    PCAD_PCB_COMPONENT::SetPosOffset( aX_offs, aY_offs );

    for( i = 0; i < (int) m_Outline.GetCount(); i++ )
    {
        m_Outline[i]->x += aX_offs;
        m_Outline[i]->y += aY_offs;
    }

    for( island = 0; island < (int) m_Islands.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_Islands[island]->GetCount(); i++ )
        {
            (*m_Islands[island])[i]->x  += aX_offs;
            (*m_Islands[island])[i]->y  += aY_offs;
        }
    }

    for( island = 0; island < (int) m_Cutouts.GetCount(); island++ )
    {
        for( i = 0; i < (int) m_Cutouts[island]->GetCount(); i++ )
        {
            (*m_Cutouts[island])[i]->x  += aX_offs;
            (*m_Cutouts[island])[i]->y  += aY_offs;
        }
    }
}

} // namespace PCAD2KICAD
