/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tools/constraint_endpoint_overlay.h>

#include <algorithm>

#include <board.h>
#include <pcb_shape.h>
#include <tool/edit_points.h>
#include <view/view.h>
#include <view/view_overlay.h>
#include <gal/color4d.h>

#include <constraints/constraint_builder.h>

using KIGFX::COLOR4D;


bool CONSTRAINT_ENDPOINT_OVERLAY::inPointSet( const CONSTRAINT_MEMBER& aMember ) const
{
    return std::find( m_pointSet.begin(), m_pointSet.end(), aMember ) != m_pointSet.end();
}


std::vector<PCB_SHAPE*> CONSTRAINT_ENDPOINT_OVERLAY::resolveShapes() const
{
    std::vector<PCB_SHAPE*> shapes;

    if( !m_board )
        return shapes;

    for( const KIID& id : m_shapeIds )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( m_board->ResolveItem( id, true ) ) )
            shapes.push_back( shape );
    }

    return shapes;
}


void CONSTRAINT_ENDPOINT_OVERLAY::SetShapes( const std::vector<PCB_SHAPE*>& aShapes )
{
    m_shapeIds.clear();

    for( const PCB_SHAPE* shape : aShapes )
        m_shapeIds.push_back( shape->m_Uuid );

    // Drop picked anchors whose shape is no longer offered so the set matches what is visible.
    std::erase_if( m_pointSet,
                   [&]( const CONSTRAINT_MEMBER& aMember )
                   {
                       return std::find( m_shapeIds.begin(), m_shapeIds.end(), aMember.m_item )
                              == m_shapeIds.end();
                   } );
}


bool CONSTRAINT_ENDPOINT_OVERLAY::ToggleNearest( const VECTOR2I& aPos, double aTol )
{
    std::optional<CONSTRAINT_MEMBER> hit = NearestAnchorAmong( resolveShapes(), aPos, aTol );

    if( !hit )
        return false;

    ToggleMember( *hit );
    return true;
}


void CONSTRAINT_ENDPOINT_OVERLAY::ToggleMember( const CONSTRAINT_MEMBER& aMember )
{
    if( auto it = std::find( m_pointSet.begin(), m_pointSet.end(), aMember ); it != m_pointSet.end() )
        m_pointSet.erase( it );
    else
        m_pointSet.push_back( aMember );
}


void CONSTRAINT_ENDPOINT_OVERLAY::Redraw()
{
    if( !m_overlay )
        return;

    m_overlay->Clear();

    // Markers track a constant on-screen size, matching the point editor's handles.
    const double radius = m_view->ToWorld( EDIT_POINT::POINT_SIZE );

    const COLOR4D markerColor( 0.20, 0.65, 0.95, 0.9 );    // cyan, bindable anchor
    const COLOR4D selectedColor( 0.95, 0.62, 0.10, 1.0 );  // amber, in the point-set
    const COLOR4D outlineColor( 0.05, 0.05, 0.05, 0.9 );

    for( const PCB_SHAPE* shape : resolveShapes() )
    {
        for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintShapeAnchors( shape ) )
        {
            bool selected = inPointSet( CONSTRAINT_MEMBER( shape->m_Uuid, a.anchor ) );

            m_overlay->SetIsFill( true );
            m_overlay->SetIsStroke( true );
            m_overlay->SetFillColor( selected ? selectedColor : markerColor );
            m_overlay->SetStrokeColor( outlineColor );
            m_overlay->SetLineWidth( radius * 0.15 );
            m_overlay->Circle( a.pos, selected ? radius * 1.5 : radius );
        }
    }

    m_view->Update( m_overlay.get() );
}
