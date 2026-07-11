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
#include <ranges>
#include <set>

#include <board.h>
#include <footprint.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <view/view.h>
#include <view/view_overlay.h>
#include <gal/color4d.h>
#include <gal/graphics_abstraction_layer.h>
#include <base_units.h>
#include <math/util.h>
#include <geometry/eda_angle.h>

#include <constraints/pcb_constraint.h>
#include <constraints/constraint_builder.h>
#include <constraints/board_constraint_adapter.h>

using KIGFX::COLOR4D;


CONSTRAINT_OVERLAY::CONSTRAINT_OVERLAY( BOARD* aBoard, KIGFX::VIEW* aView ) :
        VIEW_OVERLAY_HOLDER( aView ),
        m_board( aBoard ),
        m_selected( niluuid ),
        m_isolated( niluuid )
{
    m_view->Add( &m_badgeItem );
}


CONSTRAINT_OVERLAY::~CONSTRAINT_OVERLAY()
{
    // Remove the badge item before the base removes the tint overlay. Each is removed exactly once.
    m_view->Remove( &m_badgeItem );
}


void CONSTRAINT_OVERLAY::Clear()
{
    if( !m_overlay )
        return;

    m_overlay->Clear();
    m_view->Update( m_overlay.get() );

    // Drop the badges from both the draw item and Badges(), so a hidden overlay is not still
    // clickable through hitTestBadge.
    m_badges.clear();
    m_badgeItem.SetBadges( {}, niluuid );
    m_view->Update( &m_badgeItem );
}


double CONSTRAINT_OVERLAY::BadgeHitRadius()
{
    return 14.0; // In screen pixels. The caller converts to world units with the view scale.
}


VECTOR2D CONSTRAINT_OVERLAY::BadgeScreenOffset()
{
    // Up-right of the anchor, in screen pixels, so the glyph and its hit disc sit clear of the
    // geometry and a click on the shape at the anchor still selects the shape.
    return VECTOR2D( 16.0, -16.0 );
}


double CONSTRAINT_OVERLAY::BadgeWorldPerPixel( double aWorldScale )
{
    // Capped so a zoomed-out glyph shrinks instead of dominating the board. Shared by the drawing
    // and the hit-test so the clickable disc always matches the visible glyph.
    const double maxWorldPerPx = pcbIUScale.mmToIU( 3.0 ) / 16.0;

    return aWorldScale > 0.0 ? std::min( 1.0 / aWorldScale, maxWorldPerPx ) : maxWorldPerPx;
}


std::vector<VECTOR2D> CONSTRAINT_OVERLAY::LayoutBadges( const std::vector<CONSTRAINT_BADGE>& aBadges,
                                                       double aWorldPerPx )
{
    // Screen-constant amounts scaled into world units, so the fan tracks the glyph size at any zoom
    // instead of the old fixed 1.5 mm that collapsed zoomed-out and exploded zoomed-in.
    const double   fanStep = 18.0 * aWorldPerPx;   // a little more than the 16 px glyph
    const double   fanStepSq = fanStep * fanStep;
    const VECTOR2D offset = BadgeScreenOffset() * aWorldPerPx;

    // Cap the push-apart so a pathological board cannot hurt the pan/zoom framerate.
    const size_t deOverlapLimit = 200;

    std::vector<VECTOR2D> positions;
    positions.reserve( aBadges.size() );

    for( const CONSTRAINT_BADGE& badge : aBadges )
    {
        VECTOR2D pos = VECTOR2D( badge.pos ) + offset;

        // Push clear of any already-placed badge so glyphs sharing (or near) an anchor fan out along
        // a diagonal instead of stacking illegibly.  Capped so a pathological board cannot stall pan.
        auto overlaps = [&]( const VECTOR2D& aPlaced )
        {
            return ( aPlaced - pos ).SquaredEuclideanNorm() < fanStepSq;
        };

        while( positions.size() < deOverlapLimit && std::ranges::any_of( positions, overlaps ) )
            pos += VECTOR2D( fanStep, fanStep );

        positions.push_back( pos );
    }

    return positions;
}


bool CONSTRAINT_OVERLAY::SetSelected( const KIID& aConstraint )
{
    if( m_selected == aConstraint )
        return false;

    m_selected = aConstraint;
    return true;
}


bool CONSTRAINT_OVERLAY::SetIsolated( const KIID& aConstraint )
{
    if( m_isolated == aConstraint )
        return false;

    m_isolated = aConstraint;
    return true;
}


bool CONSTRAINT_OVERLAY::SetVisibilityMode( OVERLAY_MODE aMode )
{
    if( m_mode == aMode )
        return false;

    m_mode = aMode;
    return true;
}


bool CONSTRAINT_OVERLAY::SetHoverShape( const KIID& aShape )
{
    if( m_hoverShape == aShape )
        return false;

    m_hoverShape = aShape;
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
    // expensive board-wide re-solve.  render() no-ops when the overlay is not ready.
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
                case CONSTRAINT_STATE::UNDER_CONSTRAINED:
                default:                                  return underColor;
                }
            };

    auto setStroke =
            [&]( const COLOR4D& aColor )
            {
                m_overlay->SetIsFill( false );
                m_overlay->SetIsStroke( true );
                m_overlay->SetStrokeColor( aColor );
                m_overlay->SetLineWidth( lineWidth );
            };

    auto outlineShape =
            [&]( const PCB_SHAPE* aShape, const COLOR4D& aColor )
            {
                setStroke( aColor );

                if( aShape->GetShape() == SHAPE_T::CIRCLE )
                {
                    m_overlay->Circle( aShape->GetCenter(), aShape->GetRadius() );
                }
                else if( aShape->GetShape() == SHAPE_T::SEGMENT )
                {
                    m_overlay->Segment( aShape->GetStart(), aShape->GetEnd(), lineWidth );
                }
                else if( aShape->GetShape() == SHAPE_T::ARC )
                {
                    EDA_ANGLE startAngle( aShape->GetStart() - aShape->GetCenter() );
                    m_overlay->Arc( aShape->GetCenter(), aShape->GetRadius(), startAngle,
                                    startAngle + aShape->GetArcAngle() );
                }
            };

    const BOARD_CONSTRAINT_DIAGNOSTICS& diag = m_lastDiag;

    // Decide what is shown.  A panel-row isolation wins; otherwise ALWAYS shows everything and HOVER
    // shows only the hovered shape's constraints (or nothing when nothing is hovered).
    bool           showAll = false;
    std::set<KIID> shownShapes;
    KIID           focusConstraint = niluuid;   // isolation: show only this constraint
    KIID           focusShape = niluuid;         // hover: show only constraints referencing this shape

    auto referencesShape = []( const PCB_CONSTRAINT* aConstraint, const KIID& aShape )
    {
        return std::ranges::any_of( aConstraint->GetMembers(),
                                    [&]( const CONSTRAINT_MEMBER& m ) { return m.m_item == aShape; } );
    };

    if( m_isolated != niluuid )
    {
        if( PCB_CONSTRAINT* c = dynamic_cast<PCB_CONSTRAINT*>( m_board->ResolveItem( m_isolated, true ) ) )
        {
            focusConstraint = m_isolated;

            for( const CONSTRAINT_MEMBER& member : c->GetMembers() )
                shownShapes.insert( member.m_item );
        }
        else
        {
            m_isolated = niluuid;   // the isolated constraint is gone; fall through to the mode
        }
    }

    if( m_isolated == niluuid )
    {
        if( m_mode == OVERLAY_MODE::ALWAYS )
        {
            showAll = true;
        }
        else if( m_hoverShape != niluuid )
        {
            focusShape = m_hoverShape;

            // Every member of every constraint that references the hovered shape is shown, so the
            // hover reveals the whole relation, not just the one shape.
            auto collect =
                    [&]( const CONSTRAINTS& aConstraints )
                    {
                        for( PCB_CONSTRAINT* c : aConstraints )
                        {
                            if( referencesShape( c, focusShape ) )
                            {
                                for( const CONSTRAINT_MEMBER& m : c->GetMembers() )
                                    shownShapes.insert( m.m_item );
                            }
                        }
                    };

            collect( m_board->Constraints() );

            for( FOOTPRINT* footprint : m_board->Footprints() )
                collect( footprint->Constraints() );
        }
        // else HOVER with nothing hovered: showAll stays false and shownShapes stays empty -> nothing
    }

    auto shapeShown = [&]( const KIID& aId ) { return showAll || shownShapes.contains( aId ); };

    auto constraintShown =
            [&]( PCB_CONSTRAINT* aConstraint ) -> bool
            {
                if( focusConstraint != niluuid )
                    return aConstraint->m_Uuid == focusConstraint;

                if( showAll )
                    return true;

                if( focusShape == niluuid )
                    return false;

                return referencesShape( aConstraint, focusShape );
            };

    for( const auto& [shapeId, state] : diag.shapeStates )
    {
        if( !shapeShown( shapeId ) )
            continue;

        BOARD_ITEM* item = m_board->ResolveItem( shapeId, true );

        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
        {
            outlineShape( shape, colorForState( state ) );
        }
        else if( PCB_DIMENSION_BASE* dimension = dynamic_cast<PCB_DIMENSION_BASE*>( item ) )
        {
            // A solver-bound dimension has no outline to tint; mark its bindable feature points
            // so it reads as part of the cluster rather than a free annotation.
            setStroke( colorForState( state ) );

            for( const CONSTRAINT_ANCHOR_POINT& anchor : ConstraintItemAnchors( dimension ) )
                m_overlay->Circle( anchor.pos, 2 * lineWidth );
        }
    }

    std::set<KIID> erroredIds( diag.errored.begin(), diag.errored.end() );
    std::set<KIID> conflictingIds( diag.conflicting.begin(), diag.conflicting.end() );

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

    // An errored constraint's cluster can't be diagnosed, so tint its surviving members red.  The
    // selected constraint's badge is drawn enlarged and ringed.
    auto walkConstraints =
            [&]( const CONSTRAINTS& aConstraints )
            {
                for( PCB_CONSTRAINT* constraint : aConstraints )
                {
                    const std::vector<CONSTRAINT_MEMBER>& members = constraint->GetMembers();

                    if( members.empty() || !constraintShown( constraint ) )
                        continue;

                    bool errored = erroredIds.contains( constraint->m_Uuid );
                    bool conflicting = conflictingIds.contains( constraint->m_Uuid );

                    if( errored )
                    {
                        for( const CONSTRAINT_MEMBER& member : members )
                        {
                            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>(
                                        m_board->ResolveItem( member.m_item, true ) ) )
                            {
                                outlineShape( shape, overColor );
                            }
                        }
                    }

                    const CONSTRAINT_MEMBER& first = members.front();
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

                    // Store the anchor; LayoutBadges fans and offsets the badges in screen space at
                    // draw time so they stay legible and clickable at any zoom.
                    m_badges.push_back( { *pos, constraint->m_Uuid,
                                          ConstraintTypeGlyph( constraint->GetConstraintType() ), color } );
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

    m_badgeItem.SetBadges( m_badges, m_selected );
    m_view->Update( &m_badgeItem );
    m_view->Update( m_overlay.get() );
}


void CONSTRAINT_BADGE_ITEM::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL*  gal = aView->GetGAL();
    const double scale = gal->GetWorldScale();

    if( scale <= 0.0 )
        return;

    const double glyphPx = 16.0;
    const double bigGlyphPx = 24.0;
    const double ringPx = 20.0; // Clears the enlarged (24 px) selected glyph.
    const double linePx = 1.5;

    // Keep the glyph a constant on-screen size at any zoom.  A world-size floor would make it grow
    // without bound as you keep zooming in, so there is none.
    const double worldPerPx = CONSTRAINT_OVERLAY::BadgeWorldPerPixel( scale );

    gal->SetIsFill( false );
    gal->SetIsStroke( true );

    gal->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
    gal->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );

    // The same layout drives hit-testing (CONSTRAINT_EDIT_TOOL::hitTestBadge), so draw and click
    // positions can never diverge.  Recompute only when the zoom or badge set changed.
    if( m_layoutScale != worldPerPx )
    {
        m_layout = CONSTRAINT_OVERLAY::LayoutBadges( m_badges, worldPerPx );
        m_layoutScale = worldPerPx;
    }

    for( size_t i = 0; i < m_badges.size(); ++i )
    {
        const CONSTRAINT_BADGE& badge = m_badges[i];
        bool     selected = m_selected != niluuid && badge.constraint == m_selected;
        double   glyph = ( selected ? bigGlyphPx : glyphPx ) * worldPerPx;
        VECTOR2I pos( KiROUND( m_layout[i].x ), KiROUND( m_layout[i].y ) );

        gal->SetStrokeColor( badge.color );
        gal->SetLineWidth( static_cast<float>( linePx * worldPerPx ) );

        if( selected )
            gal->DrawCircle( pos, ringPx * worldPerPx );

        VECTOR2I glyphPos = pos + VECTOR2I( 0, KiROUND( 0.15 * glyph ) );

        // Keep at least 1 IU so an extreme zoom-in never rounds the glyph away to nothing.
        int glyphIU = std::max( 1, KiROUND( glyph ) );
        gal->SetGlyphSize( VECTOR2I( glyphIU, glyphIU ) );
        gal->BitmapText( badge.glyph, glyphPos, ANGLE_0 );
    }
}
