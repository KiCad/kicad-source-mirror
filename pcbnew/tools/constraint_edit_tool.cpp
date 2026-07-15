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

#include <tools/constraint_edit_tool.h>

#include <algorithm>
#include <cmath>
#include <ranges>
#include <set>

#include <bitmaps.h>
#include <collectors.h>
#include <core/kicad_algo.h>
#include <tool/tool_manager.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tools/pcb_picker_tool.h>
#include <pcb_shape.h>
#include <board.h>
#include <board_commit.h>
#include <footprint.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_selection_conditions.h>
#include <tools/constraint_overlay.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/actions.h>
#include <tool/edit_points.h>
#include <widgets/wx_infobar.h>
#include <widgets/msgpanel.h>
#include <pcb_base_frame.h>
#include <dialogs/dialog_constraint_value.h>
#include <dialogs/dialog_constraint_list.h>
#include <widgets/panel_constraints.h>
#include <pcb_edit_frame.h>
#include <constraints/pcb_constraint.h>
#include <constraints/constraint_builder.h>
#include <constraints/board_constraint_adapter.h>

#include <base_units.h>
#include <geometry/seg.h>


namespace
{
// The whole shape whose outline is nearest aPos within aMaxDist. Circles and arcs count only when
// aAllowCircle is set (point-on-line targets), never for midpoint or symmetry-axis picks.
std::optional<KIID> nearestOutline( BOARD* aBoard, const VECTOR2I& aPos, double aMaxDist, bool aAllowCircle )
{
    double              best = aMaxDist;
    std::optional<KIID> result;

    for( PCB_SHAPE* shape : CollectConstraintShapes( aBoard ) )
    {
        const SHAPE_T shapeType = shape->GetShape();
        double        dist = 0;

        if( shapeType == SHAPE_T::SEGMENT )
        {
            dist = SEG( shape->GetStart(), shape->GetEnd() ).Distance( aPos );
        }
        else if( aAllowCircle && ( shapeType == SHAPE_T::CIRCLE || shapeType == SHAPE_T::ARC ) )
        {
            dist = std::abs( ( aPos - shape->GetCenter() ).EuclideanNorm() - shape->GetRadius() );
        }
        else if( aAllowCircle && ( shapeType == SHAPE_T::ELLIPSE || shapeType == SHAPE_T::ELLIPSE_ARC ) )
        {
            // Radial distance to the outline at the click's polar angle in the ellipse frame.
            // Not the exact outline distance, but exact on the outline, which is all a snap needs.
            double   a = shape->GetEllipseMajorRadius();
            double   b = shape->GetEllipseMinorRadius();
            double   phi = shape->GetEllipseRotation().AsRadians();
            VECTOR2D d = VECTOR2D( aPos - shape->GetEllipseCenter() );
            double   lx = d.x * std::cos( phi ) + d.y * std::sin( phi );
            double   ly = -d.x * std::sin( phi ) + d.y * std::cos( phi );
            double   r = std::hypot( lx, ly );

            if( a <= 0 || b <= 0 )
                continue;

            double theta = std::atan2( ly, lx );
            double re = a * b / std::hypot( b * std::cos( theta ), a * std::sin( theta ) );

            dist = std::abs( r - re );
        }
        else
        {
            continue;
        }

        if( dist <= best )
        {
            best = dist;
            result = shape->m_Uuid;
        }
    }

    return result;
}
}


CONSTRAINT_EDIT_TOOL::CONSTRAINT_EDIT_TOOL() :
        PCB_TOOL_BASE( "pcbnew.ConstraintEditor" ),
        m_selectionTool( nullptr ),
        m_menu( nullptr )
{
}


void CONSTRAINT_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    // The board (and its view) is being torn down or reloaded; drop the overlay so it does not
    // dangle on a stale view.
    m_overlay.reset();

    // At shutdown the frame/info bar may already be tearing down, so don't touch UI then.
    if( aReason == SHUTDOWN )
        return;

    // The overlay is long-lived so hover can reveal constraints at any time; the sticky setting only
    // chooses whether it always shows everything (ALWAYS) or reveals on hover (HOVER).
    if( frame() && board() && getView() )
    {
        m_overlay = std::make_unique<CONSTRAINT_OVERLAY>( board(), getView() );
        m_overlay->SetVisibilityMode( frame()->GetPcbNewSettings()->m_Display.m_ShowConstraints
                                              ? OVERLAY_MODE::ALWAYS
                                              : OVERLAY_MODE::HOVER );
    }

    refreshDiagnostics();   // marks the diagnosis dirty and re-renders
}


int CONSTRAINT_EDIT_TOOL::onSelectionChanged( const TOOL_EVENT& aEvent )
{
    // A real board selection supersedes a badge selection (and keeps Delete unambiguous).  This
    // does not fire for SelectConstraintAt's own ClearSelection, which leaves the board empty.
    if( m_selectionTool && !m_selectionTool->GetSelection().Empty() )
        setSelectedConstraint( nullptr );

    // A canvas selection shows all constraints again.
    if( m_overlay && m_overlay->SetIsolated( niluuid ) )
        m_overlay->RefreshSelection();

    return 0;
}


PCB_CONSTRAINT* CONSTRAINT_EDIT_TOOL::hitTestBadge( const VECTOR2I& aPos ) const
{
    if( !m_overlay || !board() )
        return nullptr;

    // Hit-test against the exact positions the badges draw at (same LayoutBadges call), so a click
    // and a glyph can never drift apart at any zoom.
    double worldPerPx = CONSTRAINT_OVERLAY::BadgeWorldPerPixel( getView()->GetGAL()->GetWorldScale() );
    double best = CONSTRAINT_OVERLAY::BadgeHitRadius() * worldPerPx;

    const std::vector<CONSTRAINT_BADGE>& badges = m_overlay->Badges();
    std::vector<VECTOR2D>                layout = CONSTRAINT_OVERLAY::LayoutBadges( badges, worldPerPx );
    PCB_CONSTRAINT*                      result = nullptr;

    for( size_t i = 0; i < badges.size(); ++i )
    {
        double dist = ( layout[i] - VECTOR2D( aPos ) ).EuclideanNorm();

        if( dist <= best )
        {
            best = dist;
            result = dynamic_cast<PCB_CONSTRAINT*>( board()->ResolveItem( badges[i].constraint, true ) );
        }
    }

    return result;
}


void CONSTRAINT_EDIT_TOOL::setSelectedConstraint( PCB_CONSTRAINT* aConstraint )
{
    if( !m_overlay )
        return;

    if( m_overlay->SetSelected( aConstraint ? aConstraint->m_Uuid : niluuid ) )
        m_overlay->RefreshSelection();

    if( aConstraint )
    {
        if( PCB_EDIT_FRAME* pcbFrame = dynamic_cast<PCB_EDIT_FRAME*>( frame() ) )
        {
            if( PANEL_CONSTRAINTS* panel = pcbFrame->GetConstraintsPanel() )
                panel->SelectConstraint( aConstraint->m_Uuid );
        }
    }

    updateConstraintMsgPanel( aConstraint );
}


void CONSTRAINT_EDIT_TOOL::updateConstraintMsgPanel( PCB_CONSTRAINT* aConstraint )
{
    if( !frame() )
        return;

    if( !aConstraint || !board() )
    {
        // Restore the default board readout (Pads/Vias/Tracks), unless a board selection owns the
        // panel now.
        if( board() && m_selectionTool && m_selectionTool->GetSelection().Empty() )
            frame()->SetMsgPanel( board() );

        return;
    }

    std::vector<MSG_PANEL_ITEM> items;

    items.emplace_back( _( "Constraint" ), ConstraintDisplayLabel( *aConstraint, frame()->GetUserUnits() ) );

    wxString members;

    for( const CONSTRAINT_MEMBER& member : aConstraint->GetMembers() )
    {
        if( !members.IsEmpty() )
            members += wxT( ", " );

        members += ConstraintMemberLabel( board()->ResolveItem( member.m_item, true ), member.m_anchor, frame() );
    }

    items.emplace_back( _( "Items" ), members );

    const BOARD_CONSTRAINT_DIAGNOSTICS& diag = ensureDiagnosis();
    wxString                            state = _( "OK" );

    if( alg::contains( diag.errored, aConstraint->m_Uuid ) )
        state = _( "Error (missing item)" );
    else if( alg::contains( diag.conflicting, aConstraint->m_Uuid ) )
        state = _( "Over-constrained" );
    else if( alg::contains( diag.redundant, aConstraint->m_Uuid ) )
        state = _( "Redundant" );

    items.emplace_back( _( "State" ), state );

    frame()->SetMsgPanel( items );
}


bool CONSTRAINT_EDIT_TOOL::SelectConstraintAt( const VECTOR2I& aPos )
{
    PCB_CONSTRAINT* constraint = hitTestBadge( aPos );

    if( !constraint )
        return false;

    // A constraint is not a board selection; clear it so a following Delete targets the relation.
    m_selectionTool->ClearSelection();
    setSelectedConstraint( constraint );
    return true;
}


bool CONSTRAINT_EDIT_TOOL::EditConstraintAt( const VECTOR2I& aPos )
{
    PCB_CONSTRAINT* constraint = hitTestBadge( aPos );

    if( !constraint )
        return false;

    setSelectedConstraint( constraint );
    editConstraint( constraint );
    return true;
}


bool CONSTRAINT_EDIT_TOOL::ClearConstraintSelection()
{
    bool wasSelected = m_overlay && m_overlay->GetSelected() != niluuid;
    setSelectedConstraint( nullptr );
    return wasSelected;
}


BOARD_ITEM* CONSTRAINT_EDIT_TOOL::constraintParent() const
{
    if( IsFootprintEditor() && board() )
        return board()->GetFirstFootprint();

    return board();
}


void CONSTRAINT_EDIT_TOOL::refreshDiagnostics()
{
    // Called when the model changed, so the cached diagnosis is stale.  A bare re-render (hover) uses
    // renderConstraintViews() directly and keeps the cache.
    m_diagDirty = true;
    m_hoverCandidates.reset();   // the constrained-shape set may have changed too
    renderConstraintViews();
}


void CONSTRAINT_EDIT_TOOL::renderConstraintViews()
{
    // The overlay tint/badges, the info bar, and the docked list all read the same board-wide
    // diagnosis; solve it once here and hand the result to each so a model change costs one solve,
    // not one per view.  Refresh the panel only while it is shown so a hidden pane costs nothing.
    PANEL_CONSTRAINTS* panel = nullptr;

    if( PCB_EDIT_FRAME* pcbFrame = dynamic_cast<PCB_EDIT_FRAME*>( frame() ) )
        panel = pcbFrame->GetConstraintsPanel();

    bool panelShown = panel && panel->IsShownOnScreen();

    // With a long-lived overlay, only actually solve when something reads the result: ALWAYS mode, an
    // active hover, or the docked panel.  A HOVER-idle canvas costs nothing.
    bool overlayActive = m_overlay
                         && ( m_overlay->GetVisibilityMode() == OVERLAY_MODE::ALWAYS
                              || m_overlay->GetHoverShape() != niluuid );

    if( !overlayActive && !panelShown )
    {
        if( m_overlay )
        {
            m_overlay->SetIsolated( niluuid );   // a panel isolation must not linger while hidden
            m_overlay->Update( {} );             // draw nothing while hidden
        }

        updateConstraintInfoBar( {} );   // and dismiss our info-bar readout
        return;
    }

    applyDiagnostics( ensureDiagnosis() );
}


const BOARD_CONSTRAINT_DIAGNOSTICS& CONSTRAINT_EDIT_TOOL::ensureDiagnosis()
{
    if( m_diagDirty )
    {
        m_cachedDiag = DiagnoseBoardConstraints( board() );
        m_diagDirty = false;
    }

    return m_cachedDiag;
}


const std::vector<PCB_SHAPE*>& CONSTRAINT_EDIT_TOOL::hoverCandidates()
{
    if( m_hoverCandidates )
        return *m_hoverCandidates;

    std::set<KIID>          ids;
    std::vector<PCB_SHAPE*> shapes;

    auto collect =
            [&]( const CONSTRAINTS& aConstraints )
            {
                for( PCB_CONSTRAINT* c : aConstraints )
                {
                    for( const CONSTRAINT_MEMBER& m : c->GetMembers() )
                    {
                        if( ids.insert( m.m_item ).second )
                        {
                            if( PCB_SHAPE* shape =
                                        dynamic_cast<PCB_SHAPE*>( board()->ResolveItem( m.m_item, true ) ) )
                            {
                                shapes.push_back( shape );
                            }
                        }
                    }
                }
            };

    if( board() )
    {
        collect( board()->Constraints() );

        for( FOOTPRINT* footprint : board()->Footprints() )
            collect( footprint->Constraints() );
    }

    m_hoverCandidates = std::move( shapes );
    return *m_hoverCandidates;
}


int CONSTRAINT_EDIT_TOOL::onHoverMotion( const TOOL_EVENT& aEvent )
{
    // Cheapest guards first: only HOVER mode with constraints on the board does any work.  Waiting
    // tools (selection, router, move) already saw this motion before the transition loop reached us,
    // so there is nothing to forward.
    if( !m_overlay || m_overlay->GetVisibilityMode() != OVERLAY_MODE::HOVER || !board()
        || !BoardHasConstraints( board() ) )
    {
        return 0;
    }

    VECTOR2I cursor = getViewControls()->GetMousePosition();
    double   tol = getView()->ToWorld( CONSTRAINT_OVERLAY::BadgeHitRadius() );

    // Stay sticky while the cursor is still over the current shape or one of its badges, so a badge
    // does not blink out as the pointer moves off the thin outline toward it.
    if( m_overlay->GetHoverShape() != niluuid )
    {
        if( PCB_SHAPE* shape =
                    dynamic_cast<PCB_SHAPE*>( board()->ResolveItem( m_overlay->GetHoverShape(), true ) ) )
        {
            double worldPerPx = CONSTRAINT_OVERLAY::BadgeWorldPerPixel( getView()->GetGAL()->GetWorldScale() );
            std::vector<VECTOR2D> layout = CONSTRAINT_OVERLAY::LayoutBadges( m_overlay->Badges(), worldPerPx );
            double                badgeTol = CONSTRAINT_OVERLAY::BadgeHitRadius() * worldPerPx;

            bool overBadge = std::ranges::any_of( layout,
                                          [&]( const VECTOR2D& aPos )
                                          { return ( aPos - VECTOR2D( cursor ) ).EuclideanNorm() <= badgeTol; } );

            if( overBadge || shape->HitTest( cursor, KiROUND( tol ) ) )
                return 0;   // keep the current hover
        }
    }

    std::optional<KIID> hit = NearestConstrainedShape( hoverCandidates(), cursor, KiROUND( tol ) );

    // Only the hover filter changed, not the model, so redraw just the overlay from the cached
    // diagnosis -- the panel (its row selection) and the info bar are left untouched.
    if( m_overlay->SetHoverShape( hit.value_or( niluuid ) ) )
        m_overlay->Update( ensureDiagnosis() );

    return 0;
}


void CONSTRAINT_EDIT_TOOL::applyDiagnostics( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag )
{
    if( m_overlay )
        m_overlay->Update( aDiag );

    updateConstraintInfoBar( aDiag );

    if( PCB_EDIT_FRAME* pcbFrame = dynamic_cast<PCB_EDIT_FRAME*>( frame() ) )
    {
        if( PANEL_CONSTRAINTS* panel = pcbFrame->GetConstraintsPanel(); panel && panel->IsShownOnScreen() )
            panel->RefreshList( aDiag );
    }
}


void CONSTRAINT_EDIT_TOOL::removeConstraint( PCB_CONSTRAINT* aConstraint )
{
    // Remember the member shapes so their clusters can re-settle once the constraint is gone.
    std::vector<PCB_SHAPE*> members;

    for( const CONSTRAINT_MEMBER& member : aConstraint->GetMembers() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( board()->ResolveItem( member.m_item, true ) ) )
            members.push_back( shape );
    }

    BOARD_COMMIT commit( this );
    commit.Remove( aConstraint );
    commit.Push( _( "Remove Geometric Constraint" ) );

    SolveAfterMove( members );

    refreshDiagnostics();
}


PCB_CONSTRAINT* CONSTRAINT_EDIT_TOOL::resolveConstraint( const KIID& aId ) const
{
    if( !board() || aId == niluuid )
        return nullptr;

    return dynamic_cast<PCB_CONSTRAINT*>( board()->ResolveItem( aId, true ) );
}


void CONSTRAINT_EDIT_TOOL::RemoveConstraintById( const KIID& aId )
{
    if( PCB_CONSTRAINT* constraint = resolveConstraint( aId ) )
        removeConstraint( constraint );
}


void CONSTRAINT_EDIT_TOOL::editConstraint( PCB_CONSTRAINT* aConstraint )
{
    if( !aConstraint )
        return;

    BOARD_COMMIT commit( this );

    if( EditConstraintValue( frame(), aConstraint, commit ) )
        refreshDiagnostics();
}


void CONSTRAINT_EDIT_TOOL::EditConstraintById( const KIID& aId )
{
    PCB_CONSTRAINT* constraint = resolveConstraint( aId );

    if( !constraint )
        return;

    // A valueless relation has nothing to edit, so locate its members on the canvas instead of
    // ignoring the gesture, matching the modal list's double-click behavior.
    if( !constraint->HasValue() )
    {
        HighlightConstraintMembers( aId, -1 );
        return;
    }

    editConstraint( constraint );
}


void CONSTRAINT_EDIT_TOOL::HighlightConstraintMembers( const KIID& aId, int aMemberIndex )
{
    PCB_CONSTRAINT* constraint = resolveConstraint( aId );

    if( !constraint )
        return;

    // Selecting the members is a board selection, so drop any badge selection to keep the two
    // selection models mutually exclusive (Delete then targets the highlighted members).
    setSelectedConstraint( nullptr );
    m_selectionTool->ClearSelection();

    const std::vector<CONSTRAINT_MEMBER>& members = constraint->GetMembers();

    // A click on a blank item cell (index past the members) falls back to highlighting all.
    bool highlightOne = aMemberIndex >= 0 && aMemberIndex < static_cast<int>( members.size() );

    auto selectMember = [&]( const CONSTRAINT_MEMBER& aMember )
    {
        if( BOARD_ITEM* item = board()->ResolveItem( aMember.m_item, true ) )
            m_selectionTool->select( item );
    };

    if( highlightOne )
        selectMember( members[aMemberIndex] );
    else
        std::ranges::for_each( members, selectMember );

    // Zoom to what we just selected so the affected items fill the view.
    if( !m_selectionTool->GetSelection().Empty() )
        m_toolMgr->RunAction( ACTIONS::zoomFitSelection );

    // Set this after selecting, which clears the isolate.
    if( m_overlay && m_overlay->SetIsolated( aId ) )
        m_overlay->RefreshSelection();
}


bool CONSTRAINT_EDIT_TOOL::allMembersLocked( const PCB_CONSTRAINT* aConstraint ) const
{
    if( !board() || !aConstraint || aConstraint->GetMembers().empty() )
        return false;

    return std::ranges::all_of( aConstraint->GetMembers(),
                                [&]( const CONSTRAINT_MEMBER& aMember )
                                {
                                    BOARD_ITEM* item = board()->ResolveItem( aMember.m_item, true );
                                    return item && ConstraintItemIsLocked( item );
                                } );
}


bool CONSTRAINT_EDIT_TOOL::isDuplicateConstraint( const PCB_CONSTRAINT* aConstraint ) const
{
    if( !aConstraint || !board() )
        return false;

    auto scan = [&]( const CONSTRAINTS& aList )
    {
        return std::ranges::any_of( aList,
                            [&]( const PCB_CONSTRAINT* aExisting )
                            {
                                return aExisting != aConstraint && ConstraintsAreDuplicate( *aExisting, *aConstraint );
                            } );
    };

    if( scan( board()->Constraints() ) )
        return true;

    return board()->GetFirstFootprint() && scan( board()->GetFirstFootprint()->Constraints() );
}


void CONSTRAINT_EDIT_TOOL::finishConstraintCommit( BOARD_COMMIT& aCommit,
                                                   const std::vector<PCB_CONSTRAINT*>& aAdded )
{
    aCommit.Push( _( "Add Geometric Constraint" ) );

    // Solving any one constraint pins its first member and pulls the rest of the cluster into place.
    if( !aAdded.empty() )
        solveAddedConstraint( aAdded.front() );

    refreshDiagnostics();

    if( aAdded.empty() || !frame() )
        return;

    // A locked shape is a fixed reference the solver may not move.  If every referenced item is
    // locked there is nothing it can adjust, so the relation is recorded but the geometry cannot
    // snap; tell the user rather than appearing to silently do nothing.
    if( allMembersLocked( aAdded.front() ) )
    {
        frame()->ShowInfoBarWarning( _( "All items referenced by this constraint are locked, so the constraint cannot "
                                        "move any geometry." ),
                                     true );
        return;
    }

    // In the default configuration (hover overlay idle, panel hidden) nothing else surfaces the
    // diagnosis, so an unsatisfiable new constraint must be called out here or the add appears to
    // succeed silently.  Mirrors the SolveAfterMove() warning for the same condition.
    const BOARD_CONSTRAINT_DIAGNOSTICS& diag = ensureDiagnosis();

    for( const PCB_CONSTRAINT* added : aAdded )
    {
        if( alg::contains( diag.conflicting, added->m_Uuid ) )
        {
            frame()->ShowInfoBarWarning( _( "The new geometric constraint conflicts with existing constraints and "
                                            "could not be satisfied." ),
                                         true );
            return;
        }

        if( alg::contains( diag.errored, added->m_Uuid ) )
        {
            frame()->ShowInfoBarWarning( _( "The new geometric constraint could not be applied to the referenced "
                                            "items." ),
                                         true );
            return;
        }
    }
}


bool CONSTRAINT_EDIT_TOOL::TryDeleteSelectedConstraint()
{
    // With no badge selected, fall through to the normal item-delete path.
    if( !m_overlay || m_overlay->GetSelected() == niluuid )
        return false;

    PCB_CONSTRAINT* constraint = resolveConstraint( m_overlay->GetSelected() );

    setSelectedConstraint( nullptr );

    // The selected constraint vanished (undo / panel delete) but the badge selection lingered;
    // consume the Delete so it does not fall through and remove a hovered board item.
    if( !constraint )
        return true;

    removeConstraint( constraint );
    return true;
}


bool CONSTRAINT_EDIT_TOOL::TryEditSelectedConstraint()
{
    // With no badge selected, fall through to the normal properties path.
    if( !m_overlay || m_overlay->GetSelected() == niluuid )
        return false;

    PCB_CONSTRAINT* constraint = resolveConstraint( m_overlay->GetSelected() );

    // The selected constraint vanished (undo / panel delete) but the badge selection lingered;
    // clear it and fall through instead of consuming the key with no effect.
    if( !constraint )
    {
        setSelectedConstraint( nullptr );
        return false;
    }

    // editConstraint only opens a dialog for a valued constraint; others just consume the key.
    editConstraint( constraint );
    return true;
}


void CONSTRAINT_EDIT_TOOL::solveAddedConstraint( PCB_CONSTRAINT* aConstraint )
{
    // This solve runs after the add was pushed because SolveCluster gathers the cluster from
    // board->Constraints(), and BOARD_COMMIT only makes the constraint live at Push time.
    // APPEND_UNDO folds the snap into the add so creating a constraint is a single undoable action.
    BOARD_COMMIT            commit( this );
    std::vector<PCB_SHAPE*> modified;

    ApplyConstraintImmediately( board(), aConstraint, &modified,
                                [&]( BOARD_ITEM* aItem ) { commit.Modify( aItem ); } );

    // Push if the snap moved a shape or re-measured a reference value in this cluster.
    if( !commit.Empty() )
        commit.Push( _( "Apply Geometric Constraint" ), APPEND_UNDO );
}


void CONSTRAINT_EDIT_TOOL::SolveAfterMove( const std::vector<PCB_SHAPE*>& aShapes )
{
    if( aShapes.empty() || !board() || !BoardHasConstraints( board() ) )
        return;

    BOARD_COMMIT            commit( this );
    std::vector<PCB_SHAPE*> modified;

    ReSolveShapeClusters( board(), aShapes, &modified,
                          [&]( BOARD_ITEM* aItem )
                          {
                              commit.Modify( aItem );
                          } );

    if( !commit.Empty() )
        commit.Push( _( "Apply Geometric Constraint" ), APPEND_UNDO );

    // Diagnose once and use it for both the views and the warning below, so a transform does not
    // pay for two board-wide solves.  Cache it so a following hover reuses this fresh result.
    m_cachedDiag = DiagnoseBoardConstraints( board() );
    m_diagDirty = false;
    const BOARD_CONSTRAINT_DIAGNOSTICS& diag = m_cachedDiag;
    applyDiagnostics( diag );

    // If the edit left a moved shape's constraint unsatisfiable, say so even when the overlay is off.
    bool overConstrained = std::ranges::any_of( aShapes,
            [&]( const PCB_SHAPE* aShape )
            {
                auto it = diag.shapeStates.find( aShape->m_Uuid );
                return it != diag.shapeStates.end() && it->second == CONSTRAINT_STATE::OVER_CONSTRAINED;
            } );

    if( overConstrained && frame() )
        frame()->ShowInfoBarWarning( _( "A geometric constraint could not be satisfied by this edit." ), true );
}


int CONSTRAINT_EDIT_TOOL::ShowConstraints( const TOOL_EVENT& aEvent )
{
    if( !m_overlay && board() && getView() )
        m_overlay = std::make_unique<CONSTRAINT_OVERLAY>( board(), getView() );

    if( m_overlay )
    {
        // The action, not a toggle, chooses the mode, so a freshly created overlay can never invert
        // the clicked label.  The overlay object itself stays alive either way.
        bool always = aEvent.IsAction( &PCB_ACTIONS::showConstraints );
        m_overlay->SetVisibilityMode( always ? OVERLAY_MODE::ALWAYS : OVERLAY_MODE::HOVER );

        if( !always )
            m_overlay->SetHoverShape( niluuid );   // hover mode starts hidden until a hover

        // Remember the choice so it persists across board reloads and sessions.
        if( frame() )
            frame()->GetPcbNewSettings()->m_Display.m_ShowConstraints = always;
    }

    refreshDiagnostics();
    return 0;
}


void CONSTRAINT_EDIT_TOOL::updateConstraintInfoBar( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag )
{
    WX_INFOBAR* infoBar = frame() ? frame()->GetInfoBar() : nullptr;

    if( !infoBar )
        return;

    // The info bar is shared, so only ever touch it while it is showing our own readout.
    bool ours = infoBar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::CONSTRAINT_DIAGNOSTICS;

    // The readout follows the always-show overlay; a transient hover reveal stays quiet.  Clear ours
    // but leave any other subsystem's message alone.
    if( !m_overlay || m_overlay->GetVisibilityMode() != OVERLAY_MODE::ALWAYS )
    {
        if( ours )
            infoBar->Dismiss();

        m_infoBarSummary.clear();
        return;
    }

    // Don't clobber another subsystem's visible message (e.g. a DRC or load warning).
    if( !ours && infoBar->IsShown() )
        return;

    bool     overConstrained = false;
    wxString summary = ConstraintStateSummary( aDiag, &overConstrained );

    // Re-show when the text changes, when we need to (re)claim the bar, or once it has auto-hidden.
    // Gating on the text keeps editing from re-animating the bar each frame.
    if( summary != m_infoBarSummary || !ours || !infoBar->IsShown() )
    {
        infoBar->ShowMessageFor( summary, 8000, overConstrained ? wxICON_WARNING : wxICON_INFORMATION,
                                 WX_INFOBAR::MESSAGE_TYPE::CONSTRAINT_DIAGNOSTICS );
        m_infoBarSummary = summary;
    }
}


int CONSTRAINT_EDIT_TOOL::ManageConstraints( const TOOL_EVENT& )
{
    // In the board editor the constraint list is a dockable pane; the footprint editor (which has
    // no such pane) falls back to the modal list dialog.
    if( PCB_EDIT_FRAME* pcbFrame = dynamic_cast<PCB_EDIT_FRAME*>( frame() ) )
    {
        pcbFrame->ToggleConstraintsPanel();
        return 0;
    }

    auto highlight =
            [&]( PCB_CONSTRAINT* aConstraint )
            {
                m_selectionTool->ClearSelection();

                for( const CONSTRAINT_MEMBER& member : aConstraint->GetMembers() )
                {
                    if( BOARD_ITEM* item = board()->ResolveItem( member.m_item, true ) )
                        m_selectionTool->select( item );
                }
            };

    auto remove = [&]( PCB_CONSTRAINT* aConstraint ) { removeConstraint( aConstraint ); };

    DIALOG_CONSTRAINT_LIST dlg( frame(), board(), highlight, remove );
    dlg.ShowModal();

    return 0;
}


int CONSTRAINT_EDIT_TOOL::refreshOverlay( const TOOL_EVENT& aEvent )
{
    m_diagDirty = true;   // the model changed, so the cached diagnosis is stale
    refreshDiagnostics();

    aEvent.PassEvent();
    return 0;
}


bool CONSTRAINT_EDIT_TOOL::Init()
{
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    if( !m_selectionTool )
        return false;

    using S_C = SELECTION_CONDITIONS;

    static const std::vector<KICAD_T> segmentType = { PCB_SHAPE_LOCATE_SEGMENT_T };

    auto twoSegments = S_C::Count( 2 ) && S_C::OnlyTypes( segmentType );
    auto oneSegment = S_C::Count( 1 ) && S_C::OnlyTypes( segmentType );

    auto kindOf = []( const EDA_ITEM* aItem ) -> SHAPE_T
    {
        if( !aItem || aItem->Type() != PCB_SHAPE_T )
            return SHAPE_T::UNDEFINED;

        return static_cast<const PCB_SHAPE*>( aItem )->GetShape();
    };

    // A circle or arc has a radius; a closed or arc ellipse adds to that a centre only.
    auto isRadial = []( SHAPE_T aShape )
    {
        return aShape == SHAPE_T::CIRCLE || aShape == SHAPE_T::ARC;
    };

    auto isCentered = [isRadial]( SHAPE_T aShape )
    {
        return isRadial( aShape ) || aShape == SHAPE_T::ELLIPSE || aShape == SHAPE_T::ELLIPSE_ARC;
    };

    auto oneRadial = [kindOf, isRadial]( const SELECTION& aSel )
    {
        return aSel.Size() == 1 && isRadial( kindOf( aSel[0] ) );
    };

    auto oneArc = [kindOf]( const SELECTION& aSel )
    {
        return aSel.Size() == 1 && kindOf( aSel[0] ) == SHAPE_T::ARC;
    };

    auto twoRadial = [kindOf, isRadial]( const SELECTION& aSel )
    {
        return aSel.Size() == 2 && isRadial( kindOf( aSel[0] ) ) && isRadial( kindOf( aSel[1] ) );
    };

    auto twoCentered = [kindOf, isCentered]( const SELECTION& aSel )
    {
        return aSel.Size() == 2 && isCentered( kindOf( aSel[0] ) ) && isCentered( kindOf( aSel[1] ) );
    };

    // Tangent joins a line with a curve, or two circles/arcs.
    auto tangentPair = [kindOf, isCentered, isRadial]( const SELECTION& aSel )
    {
        if( aSel.Size() != 2 )
            return false;

        SHAPE_T a = kindOf( aSel[0] );
        SHAPE_T b = kindOf( aSel[1] );

        return ( a == SHAPE_T::SEGMENT && isCentered( b ) ) || ( b == SHAPE_T::SEGMENT && isCentered( a ) )
               || ( isRadial( a ) && isRadial( b ) );
    };

    // Only offer Remove when something selected actually carries a constraint.
    auto selectionConstrained = [this]( const SELECTION& aSel ) -> bool
    {
        if( aSel.Empty() || !board() )
            return false;

        std::set<KIID> ids;

        for( EDA_ITEM* item : aSel )
            ids.insert( item->m_Uuid );

        auto anyMember = [&]( const CONSTRAINTS& aList )
        {
            return std::ranges::any_of( aList,
                    [&]( const PCB_CONSTRAINT* c )
                    {
                        return std::ranges::any_of( c->GetMembers(),
                                [&]( const CONSTRAINT_MEMBER& m ) { return ids.contains( m.m_item ); } );
                    } );
        };

        if( anyMember( board()->Constraints() ) )
            return true;

        return IsFootprintEditor() && board()->GetFirstFootprint()
               && anyMember( board()->GetFirstFootprint()->Constraints() );
    };

    // One "Constraints" submenu holds every constraint command.  The add-type items are gated by
    // the current selection so only constraints valid for what is selected are offered; the
    // manage/show/remove items are always present.
    m_menu = new CONDITIONAL_MENU( this );
    m_menu->SetIcon( BITMAPS::measurement );
    m_menu->SetUntranslatedTitle( _HKI( "Constraints" ) );

    // Gate each selection-based add-type by what it needs; the point-anchored families are authored
    // by clicking and need no prior selection, so they are absent here and default to ShowAlways.
    // The action list itself lives in PCB_ACTIONS::ConstraintAddActions() so the menubar's copy of
    // this submenu cannot drift from it.
    const std::map<const TOOL_ACTION*, SELECTION_CONDITION> gate = {
        { &PCB_ACTIONS::addConstraintParallel,      twoSegments },
        { &PCB_ACTIONS::addConstraintPerpendicular, twoSegments },
        { &PCB_ACTIONS::addConstraintEqualLength,   twoSegments },
        { &PCB_ACTIONS::addConstraintCollinear,     twoSegments },
        { &PCB_ACTIONS::addConstraintAngular,       twoSegments },
        { &PCB_ACTIONS::addConstraintTangent,       tangentPair },
        { &PCB_ACTIONS::addConstraintHorizontal,    oneSegment },
        { &PCB_ACTIONS::addConstraintVertical,      oneSegment },
        { &PCB_ACTIONS::addConstraintFixedLength,   oneSegment },
        { &PCB_ACTIONS::addConstraintConcentric,    twoCentered },
        { &PCB_ACTIONS::addConstraintEqualRadius,   twoRadial },
        { &PCB_ACTIONS::addConstraintFixedRadius,   oneRadial },
        { &PCB_ACTIONS::addConstraintArcAngle,      oneArc },
    };

    bool pointGroupSeparated = false;

    for( const TOOL_ACTION* action : PCB_ACTIONS::ConstraintAddActions() )
    {
        if( auto it = gate.find( action ); it != gate.end() )
        {
            m_menu->AddItem( *action, it->second );
        }
        else
        {
            if( !pointGroupSeparated )
            {
                m_menu->AddSeparator();
                pointGroupSeparated = true;
            }

            m_menu->AddItem( *action, S_C::ShowAlways );
        }
    }

    // Show while hidden, Hide while shown, so the label always names what the click will do.
    // "Shown" is the always-on mode; the hover mode reads as hidden and offers the Show action.
    // The ALWAYS/HOVER choice is a tool mode rather than an object visibility, which is why it
    // lives here and not in the Appearance panel's Objects tab.
    auto overlayShown = [this]( const SELECTION& )
    {
        return m_overlay && m_overlay->GetVisibilityMode() == OVERLAY_MODE::ALWAYS;
    };
    auto overlayHidden = [this]( const SELECTION& )
    {
        return !m_overlay || m_overlay->GetVisibilityMode() == OVERLAY_MODE::HOVER;
    };

    m_menu->AddSeparator();
    m_menu->AddItem( PCB_ACTIONS::removeConstraints, selectionConstrained );
    m_menu->AddItem( PCB_ACTIONS::showConstraints, overlayHidden );
    m_menu->AddItem( PCB_ACTIONS::hideConstraints, overlayShown );
    m_menu->AddItem( PCB_ACTIONS::manageConstraints,          S_C::ShowAlways );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_menu, S_C::ShowAlways, 100 );

    return true;
}


int CONSTRAINT_EDIT_TOOL::AddConstraint( const TOOL_EVENT& aEvent )
{
    PCB_CONSTRAINT_TYPE  type = aEvent.Parameter<PCB_CONSTRAINT_TYPE>();
    const PCB_SELECTION& selection = m_selectionTool->GetSelection();

    std::vector<BOARD_ITEM*> items;

    for( EDA_ITEM* item : selection )
    {
        if( item->IsBOARD_ITEM() )
            items.push_back( static_cast<BOARD_ITEM*>( item ) );
    }

    std::unique_ptr<PCB_CONSTRAINT> constraint =
            BuildConstraintFromItems( constraintParent(), type, items );

    // The context menu gates each entry by selection, but the same actions are reachable from the
    // command palette and user hotkeys, so an unbuildable selection needs an explanation here.
    if( !constraint )
    {
        if( frame() )
        {
            frame()->ShowInfoBarWarning(
                    wxString::Format( _( "Cannot form constraint type %s from the current selection." ),
                                      ConstraintTypeLabel( type ) ) );
        }

        return 0;
    }

    if( isDuplicateConstraint( constraint.get() ) )
    {
        if( frame() )
            frame()->ShowInfoBarWarning( _( "An identical geometric constraint already exists." ) );

        return 0;
    }

    // For a dimensional constraint, let the user confirm/override the measured value and choose
    // driving vs reference (issue #2329 step 7).
    if( constraint->HasValue() )
    {
        DIALOG_CONSTRAINT_VALUE dlg( frame(), type, *constraint->GetValue(), constraint->IsDriving() );

        if( dlg.ShowModal() != wxID_OK )
            return 0;

        constraint->SetValue( dlg.GetConstraintValue() );
        constraint->SetDriving( dlg.GetDriving() );
    }

    PCB_CONSTRAINT* added = constraint.get();

    BOARD_COMMIT commit( this );
    commit.Add( constraint.release() );
    finishConstraintCommit( commit, { added } );

    return 0;
}


int CONSTRAINT_EDIT_TOOL::AddPointConstraint( const TOOL_EVENT& aEvent )
{
    PCB_CONSTRAINT_TYPE type = aEvent.Parameter<PCB_CONSTRAINT_TYPE>();

    // In the pick plan, true means click a point anchor and false means click a whole segment.
    std::vector<bool> plan;

    switch( type )
    {
    case PCB_CONSTRAINT_TYPE::COINCIDENT:    plan = { true, true }; break;
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE:
    case PCB_CONSTRAINT_TYPE::MIDPOINT:      plan = { true, false }; break;
    case PCB_CONSTRAINT_TYPE::SYMMETRIC:     plan = { true, true, false }; break;
    default:                                 return 0;
    }

    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    if( !picker )
        return 0;

    std::vector<CONSTRAINT_MEMBER> members;
    const double                   snapTol = pcbIUScale.mmToIU( 1.0 );

    Activate();
    picker->SetCursor( KICURSOR::BULLSEYE );
    picker->SetSnapping( true );
    picker->ClearHandlers();

    picker->SetClickHandler(
            [&]( const VECTOR2D& aPoint ) -> bool
            {
                VECTOR2I pos( KiROUND( aPoint.x ), KiROUND( aPoint.y ) );

                if( plan[members.size()] )   // wants a point anchor
                {
                    std::optional<CONSTRAINT_MEMBER> anchor =
                            NearestConstraintAnchor( board(), pos, snapTol );

                    if( !anchor )
                        return true;   // nothing snapped; keep picking

                    members.push_back( *anchor );
                }
                else // wants a whole shape
                {
                    std::optional<KIID> target =
                            nearestOutline( board(), pos, snapTol, type == PCB_CONSTRAINT_TYPE::POINT_ON_LINE );

                    if( !target )
                        return true;

                    members.emplace_back( *target, CONSTRAINT_ANCHOR::WHOLE );
                }

                if( members.size() < plan.size() )
                    return true;   // need more picks

                std::unique_ptr<PCB_CONSTRAINT> constraint =
                        std::make_unique<PCB_CONSTRAINT>( constraintParent(), type );

                for( const CONSTRAINT_MEMBER& member : members )
                    constraint->AddMember( member.m_item, member.m_anchor );

                if( isDuplicateConstraint( constraint.get() ) )
                {
                    if( frame() )
                        frame()->ShowInfoBarWarning( _( "An identical geometric constraint already exists." ) );

                    return false; // done
                }

                PCB_CONSTRAINT* added = constraint.get();

                BOARD_COMMIT commit( this );
                commit.Add( constraint.release() );
                finishConstraintCommit( commit, { added } );

                return false;   // done
            } );

    bool done = false;

    picker->SetFinalizeHandler(
            [&]( const int& aFinalState )
            {
                done = true;
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    // RunAction returns before picking ends. Wait so the handlers' captures stay alive.
    while( !done )
    {
        if( TOOL_EVENT* evt = Wait() )
            evt->SetPassEvent();
        else
            break;
    }

    picker->ClearHandlers();

    return 0;
}


int CONSTRAINT_EDIT_TOOL::RemoveConstraints( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION& selection = m_selectionTool->GetSelection();
    std::set<KIID> selectedIds;

    for( EDA_ITEM* item : selection )
        selectedIds.insert( item->m_Uuid );

    if( selectedIds.empty() )
        return 0;

    BOARD_COMMIT commit( this );
    bool         any = false;

    auto removeReferencing =
            [&]( const CONSTRAINTS& aConstraints )
            {
                for( PCB_CONSTRAINT* constraint : aConstraints )
                {
                    bool referenced = std::ranges::any_of( constraint->GetMembers(),
                            [&]( const CONSTRAINT_MEMBER& aMember )
                            { return selectedIds.contains( aMember.m_item ); } );

                    if( referenced )
                    {
                        commit.Remove( constraint );
                        any = true;
                    }
                }
            };

    if( board() )
    {
        removeReferencing( board()->Constraints() );

        if( IsFootprintEditor() && board()->GetFirstFootprint() )
            removeReferencing( board()->GetFirstFootprint()->Constraints() );
    }

    if( any )
        commit.Push( _( "Remove Geometric Constraints" ) );

    refreshDiagnostics();

    return 0;
}


void CONSTRAINT_EDIT_TOOL::setTransitions()
{
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintParallel.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintPerpendicular.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintEqualLength.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintCollinear.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintAngular.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintTangent.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintHorizontal.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintVertical.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintFixedLength.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintConcentric.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintEqualRadius.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintFixedRadius.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintArcAngle.MakeEvent() );

    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintCoincident.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintPointOnLine.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintMidpoint.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintSymmetric.MakeEvent() );

    Go( &CONSTRAINT_EDIT_TOOL::RemoveConstraints, PCB_ACTIONS::removeConstraints.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::ShowConstraints,   PCB_ACTIONS::showConstraints.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::ShowConstraints, PCB_ACTIONS::hideConstraints.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::ManageConstraints, PCB_ACTIONS::manageConstraints.MakeEvent() );

    // Keep the diagnostics overlay current as the board changes underneath it.  Undo/redo posts its
    // own event (not TA_MODEL_CHANGE), so listen for it too or a restored/removed constraint's badge
    // would not reappear/disappear.
    Go( &CONSTRAINT_EDIT_TOOL::refreshOverlay,    TOOL_EVENT( TC_MESSAGE, TA_MODEL_CHANGE ) );
    Go( &CONSTRAINT_EDIT_TOOL::refreshOverlay, EVENTS::UndoRedoPostEvent );

    // Show/refresh the endpoint markers as the selection changes.
    Go( &CONSTRAINT_EDIT_TOOL::onSelectionChanged, EVENTS::SelectedEvent );
    Go( &CONSTRAINT_EDIT_TOOL::onSelectionChanged, EVENTS::UnselectedEvent );
    Go( &CONSTRAINT_EDIT_TOOL::onSelectionChanged, EVENTS::ClearedEvent );

    // No other pcbnew tool registers a plain-motion transition (only one transition runs per mouse
    // event), and waiting tools receive motion before this loop, so claiming it here is safe.
    Go( &CONSTRAINT_EDIT_TOOL::onHoverMotion, TOOL_EVENT( TC_MOUSE, TA_MOUSE_MOTION ) );
}
