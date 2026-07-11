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

#include <tools/constraint_overlay.h>

#include <algorithm>
#include <set>

#include <board.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <view/view.h>
#include <view/view_overlay.h>
#include <gal/color4d.h>
#include <base_units.h>
#include <geometry/eda_angle.h>

#include <constraints/pcb_constraint.h>
#include <constraints/constraint_builder.h>
#include <constraints/board_constraint_adapter.h>

using KIGFX::COLOR4D;


CONSTRAINT_OVERLAY::CONSTRAINT_OVERLAY( BOARD* aBoard, KIGFX::VIEW* aView ) :
        VIEW_OVERLAY_HOLDER( aView ),
        m_board( aBoard ),
        m_selected( niluuid )
{
}


void CONSTRAINT_OVERLAY::Clear()
{
    if( !m_overlay )
        return;

    m_overlay->Clear();
    m_view->Update( m_overlay.get() );
}


double CONSTRAINT_OVERLAY::BadgeHitRadius()
{
    return pcbIUScale.mmToIU( 1.2 );
}


bool CONSTRAINT_OVERLAY::SetSelected( const KIID& aConstraint )
{
    if( m_selected == aConstraint )
        return false;

    m_selected = aConstraint;
    return true;
}


void CONSTRAINT_OVERLAY::Update( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag )
{
    if( !m_overlay || !m_board )
        return;

    m_lastDiag = aDiag;
    render();
}


void CONSTRAINT_OVERLAY::RefreshSelection()
{
    // Only the selected-badge highlight changed; redraw from the cached diagnosis without the
    // expensive board-wide re-solve.
    if( m_overlay && m_board )
        render();
}


void CONSTRAINT_OVERLAY::render()
{
    if( !m_overlay || !m_board )
        return;

    m_overlay->Clear();
    m_badges.clear();

    const COLOR4D underColor( 0.95, 0.62, 0.10, 0.9 );   // amber, free DOF remain
    const COLOR4D wellColor( 0.18, 0.72, 0.30, 0.9 );    // green, fully constrained
    const COLOR4D overColor( 0.92, 0.18, 0.18, 0.9 );    // red, conflicting / errored
    const double  lineWidth = pcbIUScale.mmToIU( 0.15 );

    auto colorForState =
            [&]( CONSTRAINT_STATE aState ) -> COLOR4D
            {
                switch( aState )
                {
                case CONSTRAINT_STATE::WELL_CONSTRAINED:  return wellColor;
                case CONSTRAINT_STATE::OVER_CONSTRAINED:  return overColor;
                case CONSTRAINT_STATE::UNDER_CONSTRAINED: return underColor;
                default:                                  return underColor;
                }
            };

    auto outlineShape =
            [&]( const PCB_SHAPE* aShape, const COLOR4D& aColor )
            {
                m_overlay->SetIsFill( false );
                m_overlay->SetIsStroke( true );
                m_overlay->SetStrokeColor( aColor );
                m_overlay->SetLineWidth( lineWidth );

                if( aShape->GetShape() == SHAPE_T::CIRCLE )
                    m_overlay->Circle( aShape->GetCenter(), aShape->GetRadius() );
                else if( aShape->GetShape() == SHAPE_T::SEGMENT )
                    m_overlay->Segment( aShape->GetStart(), aShape->GetEnd(), lineWidth );
            };

    const BOARD_CONSTRAINT_DIAGNOSTICS& diag = m_lastDiag;

    for( const auto& [shapeId, state] : diag.shapeStates )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( m_board->ResolveItem( shapeId, true ) ) )
            outlineShape( shape, colorForState( state ) );
    }

    std::set<KIID> erroredIds( diag.errored.begin(), diag.errored.end() );
    std::set<KIID> conflictingIds( diag.conflicting.begin(), diag.conflicting.end() );

    const int glyphIU = pcbIUScale.mmToIU( 0.8 );
    const int bigGlyphIU = pcbIUScale.mmToIU( 1.2 );   // selected badge drawn 1.5x

    // Fan badges out by two hit-radii so adjacent click targets never overlap.
    const int fanStep = static_cast<int>( 2.0 * BadgeHitRadius() );

    auto badgePosition =
            [&]( const CONSTRAINT_MEMBER& aFirst ) -> std::optional<VECTOR2I>
            {
                if( std::optional<VECTOR2I> pos = ConstraintAnchorPosition( m_board, aFirst ) )
                    return pos;

                // A WHOLE member (segment/line/circle) has no single anchor; label its centre.
                if( PCB_SHAPE* shape =
                            dynamic_cast<PCB_SHAPE*>( m_board->ResolveItem( aFirst.m_item, true ) ) )
                {
                    return shape->GetBoundingBox().Centre();
                }

                return std::nullopt;
            };

    // Push a badge clear of any already-placed badge so glyphs sharing an anchor (coincident,
    // point-on-line, ...) fan out along a diagonal instead of stacking illegibly.
    auto fanOut =
            [&]( VECTOR2I aPos ) -> VECTOR2I
            {
                bool moved = true;

                while( moved )
                {
                    moved = false;

                    for( const CONSTRAINT_BADGE& placed : m_badges )
                    {
                        if( ( placed.pos - aPos ).SquaredEuclideanNorm()
                            < static_cast<int64_t>( fanStep ) * fanStep )
                        {
                            aPos += VECTOR2I( fanStep, fanStep );
                            moved = true;
                            break;
                        }
                    }
                }

                return aPos;
            };

    // Walk each constraint once, tinting an errored constraint's surviving members red (its cluster
    // can't be diagnosed), then placing a type-glyph badge coloured by the constraint's state.  The
    // selected constraint's badge is drawn enlarged and ringed.
    auto walkConstraints =
            [&]( const CONSTRAINTS& aConstraints )
            {
                for( PCB_CONSTRAINT* constraint : aConstraints )
                {
                    if( constraint->GetMembers().empty() )
                        continue;

                    bool errored = erroredIds.count( constraint->m_Uuid );
                    bool conflicting = conflictingIds.count( constraint->m_Uuid );

                    if( errored )
                    {
                        for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
                        {
                            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>(
                                        m_board->ResolveItem( member.m_item, true ) ) )
                            {
                                outlineShape( shape, overColor );
                            }
                        }
                    }

                    const CONSTRAINT_MEMBER& first = constraint->GetMembers().front();
                    std::optional<VECTOR2I>  pos = badgePosition( first );

                    if( !pos )
                        continue;

                    COLOR4D color;

                    if( errored || conflicting )
                    {
                        color = overColor;
                    }
                    else if( auto it = diag.shapeStates.find( first.m_item );
                             it != diag.shapeStates.end() )
                    {
                        color = colorForState( it->second );
                    }
                    else
                    {
                        color = underColor;   // undiagnosed (e.g. an unsupported cluster); not "well"
                    }

                    VECTOR2I placed = fanOut( *pos );
                    m_badges.push_back( { placed, constraint->m_Uuid } );

                    bool selected = constraint->m_Uuid == m_selected;

                    m_overlay->SetIsFill( false );
                    m_overlay->SetIsStroke( true );
                    m_overlay->SetStrokeColor( color );
                    m_overlay->SetLineWidth( pcbIUScale.mmToIU( 0.12 ) );

                    if( selected )
                        m_overlay->Circle( placed, BadgeHitRadius() );

                    m_overlay->SetGlyphSize( selected ? VECTOR2I( bigGlyphIU, bigGlyphIU )
                                                      : VECTOR2I( glyphIU, glyphIU ) );
                    m_overlay->BitmapText( ConstraintTypeGlyph( constraint->GetConstraintType() ),
                                           placed, ANGLE_0 );
                }
            };

    walkConstraints( m_board->Constraints() );

    for( FOOTPRINT* footprint : m_board->Footprints() )
        walkConstraints( footprint->Constraints() );

    // Drop a selection whose constraint no longer has a badge (deleted, or its members vanished).
    if( m_selected != niluuid
        && std::none_of( m_badges.begin(), m_badges.end(),
                         [&]( const CONSTRAINT_BADGE& aBadge )
                         { return aBadge.constraint == m_selected; } ) )
    {
        m_selected = niluuid;
    }

    m_view->Update( m_overlay.get() );
}
