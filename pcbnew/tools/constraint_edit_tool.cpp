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

#include <set>

#include <bitmaps.h>
#include <collectors.h>
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
#include <tools/constraint_endpoint_overlay.h>
#include <view/view.h>
#include <tool/actions.h>
#include <tool/edit_points.h>
#include <widgets/wx_infobar.h>
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
// The whole segment nearest aPos within aMaxDist (for point-on-line / axis picks).
std::optional<KIID> nearestSegment( BOARD* aBoard, const VECTOR2I& aPos, double aMaxDist )
{
    double              best = aMaxDist;
    std::optional<KIID> result;

    for( PCB_SHAPE* shape : CollectConstraintShapes( aBoard ) )
    {
        if( shape->GetShape() != SHAPE_T::SEGMENT )
            continue;

        double dist = SEG( shape->GetStart(), shape->GetEnd() ).Distance( aPos );

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
    // The board (and its view) is being torn down or reloaded; drop the overlays so they do not
    // dangle on a stale view.
    m_overlay.reset();
    m_endpoints.reset();

    // At shutdown the frame/info bar may already be tearing down, so don't touch UI then; on a
    // model reload it is still valid and the stale readout should be cleared (overlay is gone, so
    // the diagnosis is unused -- pass an empty one).
    if( aReason != SHUTDOWN )
        updateConstraintInfoBar( {} );
}


std::vector<PCB_SHAPE*> CONSTRAINT_EDIT_TOOL::selectedShapes() const
{
    std::vector<PCB_SHAPE*> shapes;

    for( EDA_ITEM* item : m_selectionTool->GetSelection() )
    {
        if( item->Type() == PCB_SHAPE_T )
            shapes.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    return shapes;
}


void CONSTRAINT_EDIT_TOOL::refreshEndpointMarkers()
{
    std::vector<PCB_SHAPE*> shapes = selectedShapes();

    // Endpoint binding joins several items; a lone selection is served by the point editor's own
    // handles, so markers only appear once two or more shapes are selected.
    if( shapes.size() < 2 )
    {
        m_endpoints.reset();
        return;
    }

    if( !m_endpoints )
        m_endpoints = std::make_unique<CONSTRAINT_ENDPOINT_OVERLAY>( board(), getView() );

    m_endpoints->SetShapes( shapes );
    m_endpoints->Redraw();
}


int CONSTRAINT_EDIT_TOOL::onSelectionChanged( const TOOL_EVENT& aEvent )
{
    refreshEndpointMarkers();

    // A real board selection supersedes a badge selection (and keeps Delete unambiguous).  This
    // does not fire for SelectConstraintAt's own ClearSelection, which leaves the board empty.
    if( m_selectionTool && !m_selectionTool->GetSelection().Empty() )
        setSelectedConstraint( nullptr );

    return 0;
}


PCB_CONSTRAINT* CONSTRAINT_EDIT_TOOL::hitTestBadge( const VECTOR2I& aPos ) const
{
    if( !m_overlay || !board() )
        return nullptr;

    double          best = CONSTRAINT_OVERLAY::BadgeHitRadius();
    PCB_CONSTRAINT* result = nullptr;

    for( const CONSTRAINT_BADGE& badge : m_overlay->Badges() )
    {
        double dist = ( badge.pos - aPos ).EuclideanNorm();

        if( dist <= best )
        {
            best = dist;
            result = dynamic_cast<PCB_CONSTRAINT*>( board()->ResolveItem( badge.constraint, true ) );
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


void CONSTRAINT_EDIT_TOOL::ClearConstraintSelection()
{
    setSelectedConstraint( nullptr );
}


BOARD_ITEM* CONSTRAINT_EDIT_TOOL::constraintParent() const
{
    if( IsFootprintEditor() && board() )
        return board()->GetFirstFootprint();

    return board();
}


void CONSTRAINT_EDIT_TOOL::refreshDiagnostics()
{
    // The overlay tint/badges, the info bar, and the docked list all read the same board-wide
    // diagnosis; solve it once here and hand the result to each so a model change costs one solve,
    // not one per view.  Refresh the panel only while it is shown so a hidden pane costs nothing.
    PANEL_CONSTRAINTS* panel = nullptr;

    if( PCB_EDIT_FRAME* pcbFrame = dynamic_cast<PCB_EDIT_FRAME*>( frame() ) )
        panel = pcbFrame->GetConstraintsPanel();

    bool panelShown = panel && panel->IsShownOnScreen();

    if( !m_overlay && !panelShown )
    {
        updateConstraintInfoBar( {} );   // nothing shown, so just dismiss our info-bar readout
        return;
    }

    BOARD_CONSTRAINT_DIAGNOSTICS diag = DiagnoseBoardConstraints( board() );

    if( m_overlay )
        m_overlay->Update( diag );

    updateConstraintInfoBar( diag );

    if( panelShown )
        panel->RefreshList( diag );
}


void CONSTRAINT_EDIT_TOOL::removeConstraint( PCB_CONSTRAINT* aConstraint )
{
    BOARD_COMMIT commit( this );
    commit.Remove( aConstraint );
    commit.Push( _( "Remove Geometric Constraint" ) );

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
    editConstraint( resolveConstraint( aId ) );
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

    for( int i = 0; i < static_cast<int>( members.size() ); ++i )
    {
        // A click on a blank item cell (index past the members) falls back to highlighting all.
        if( aMemberIndex >= 0 && aMemberIndex < static_cast<int>( members.size() )
            && i != aMemberIndex )
        {
            continue;
        }

        if( BOARD_ITEM* item = board()->ResolveItem( members[i].m_item, true ) )
            m_selectionTool->select( item );
    }
}


void CONSTRAINT_EDIT_TOOL::finishConstraintCommit( BOARD_COMMIT& aCommit,
                                                   const std::vector<PCB_CONSTRAINT*>& aAdded )
{
    aCommit.Push( _( "Add Geometric Constraint" ) );

    // Solving any one constraint pins its first member and pulls the rest of the cluster into place.
    if( !aAdded.empty() )
        solveAddedConstraint( aAdded.front() );

    refreshDiagnostics();
}


void CONSTRAINT_EDIT_TOOL::clearEndpointPointSet()
{
    if( m_endpoints )
    {
        m_endpoints->ClearPointSet();
        m_endpoints->Redraw();
    }
}


void CONSTRAINT_EDIT_TOOL::bindCoincidentPointSet()
{
    std::vector<CONSTRAINT_MEMBER> pts = m_endpoints->PointSet();
    std::vector<PCB_CONSTRAINT*>   added;
    BOARD_COMMIT                   commit( this );

    // Make every picked point coincident with the first, so they all coincide (a star pattern).
    for( size_t i = 1; i < pts.size(); ++i )
    {
        auto constraint = std::make_unique<PCB_CONSTRAINT>( constraintParent(),
                                                            PCB_CONSTRAINT_TYPE::COINCIDENT );
        constraint->AddMember( pts[0].m_item, pts[0].m_anchor );
        constraint->AddMember( pts[i].m_item, pts[i].m_anchor );
        added.push_back( constraint.get() );
        commit.Add( constraint.release() );
    }

    clearEndpointPointSet();
    finishConstraintCommit( commit, added );
}


bool CONSTRAINT_EDIT_TOOL::ToggleEndpointAt( const VECTOR2I& aPos )
{
    if( !m_endpoints )
        return false;

    // Use the selected (enlarged) marker radius so the whole visible marker is clickable.
    if( m_endpoints->ToggleNearest( aPos, getView()->ToWorld( EDIT_POINT::POINT_SIZE * 1.5 ) ) )
    {
        m_endpoints->Redraw();
        return true;
    }

    return false;
}


bool CONSTRAINT_EDIT_TOOL::TryDeleteSelectedConstraint()
{
    // No badge-selected constraint: let the normal item-delete path run.
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


void CONSTRAINT_EDIT_TOOL::solveAddedConstraint( PCB_CONSTRAINT* aConstraint )
{
    // The solve is a separate commit on purpose, run after the add was pushed: SolveCluster gathers
    // the cluster from board->Constraints(), and BOARD_COMMIT only adds the constraint to the board
    // at Push time, so the constraint must already be live here.  APPEND_UNDO folds the snap into
    // the add's undo step, so creating a constraint is a single undoable action.
    BOARD_COMMIT            commit( this );
    std::vector<PCB_SHAPE*> modified;

    ApplyConstraintImmediately( board(), aConstraint, &modified,
                                [&]( PCB_SHAPE* aShape ) { commit.Modify( aShape ); } );

    if( !modified.empty() )
        commit.Push( _( "Apply Geometric Constraint" ), APPEND_UNDO );
}


int CONSTRAINT_EDIT_TOOL::ShowConstraints( const TOOL_EVENT& aEvent )
{
    if( m_overlay )
    {
        m_overlay.reset();
    }
    else
    {
        m_overlay = std::make_unique<CONSTRAINT_OVERLAY>( board(), getView() );
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

    // No overlay -> no readout; clear ours but leave any other subsystem's message alone.
    if( !m_overlay )
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

    // Re-show when the text changes, or when we need to (re)claim the bar; gating on the text keeps
    // editing from re-animating the bar each frame.
    if( summary != m_infoBarSummary || !ours )
    {
        infoBar->ShowMessage( summary, overConstrained ? wxICON_WARNING : wxICON_INFORMATION,
                              WX_INFOBAR::MESSAGE_TYPE::CONSTRAINT_DIAGNOSTICS );
        m_infoBarSummary = summary;
    }
}


int CONSTRAINT_EDIT_TOOL::ManageConstraints( const TOOL_EVENT& aEvent )
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
    refreshDiagnostics();

    // A deleted shape may still sit in the marker set; rebuild it against the changed board.
    if( m_endpoints )
        refreshEndpointMarkers();

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
    static const std::vector<KICAD_T> circleType = { PCB_SHAPE_LOCATE_CIRCLE_T };

    auto twoSegments = S_C::Count( 2 ) && S_C::OnlyTypes( segmentType );
    auto oneSegment = S_C::Count( 1 ) && S_C::OnlyTypes( segmentType );
    auto twoCircles = S_C::Count( 2 ) && S_C::OnlyTypes( circleType );
    auto oneCircle = S_C::Count( 1 ) && S_C::OnlyTypes( circleType );
    auto anySelection = S_C::MoreThan( 0 );

    // One "Constraints" submenu holds every constraint command.  The add-type items are gated by
    // the current selection so only constraints valid for what is selected are offered; the
    // manage/show/remove items are always present.
    m_menu = new CONDITIONAL_MENU( this );
    m_menu->SetIcon( BITMAPS::measurement );
    m_menu->SetUntranslatedTitle( _HKI( "Constraints" ) );

    m_menu->AddItem( PCB_ACTIONS::addConstraintParallel,      twoSegments );
    m_menu->AddItem( PCB_ACTIONS::addConstraintPerpendicular, twoSegments );
    m_menu->AddItem( PCB_ACTIONS::addConstraintEqualLength,   twoSegments );
    m_menu->AddItem( PCB_ACTIONS::addConstraintCollinear,     twoSegments );
    m_menu->AddItem( PCB_ACTIONS::addConstraintAngular,       twoSegments );
    m_menu->AddItem( PCB_ACTIONS::addConstraintHorizontal,    oneSegment );
    m_menu->AddItem( PCB_ACTIONS::addConstraintVertical,      oneSegment );
    m_menu->AddItem( PCB_ACTIONS::addConstraintFixedLength,   oneSegment );
    m_menu->AddItem( PCB_ACTIONS::addConstraintConcentric,    twoCircles );
    m_menu->AddItem( PCB_ACTIONS::addConstraintEqualRadius,   twoCircles );
    m_menu->AddItem( PCB_ACTIONS::addConstraintFixedRadius,   oneCircle );

    // Point-anchored families are authored by clicking, so they need no prior selection.
    m_menu->AddSeparator();
    m_menu->AddItem( PCB_ACTIONS::addConstraintCoincident,    S_C::ShowAlways );
    m_menu->AddItem( PCB_ACTIONS::addConstraintPointOnLine,   S_C::ShowAlways );
    m_menu->AddItem( PCB_ACTIONS::addConstraintMidpoint,      S_C::ShowAlways );
    m_menu->AddItem( PCB_ACTIONS::addConstraintSymmetric,     S_C::ShowAlways );

    m_menu->AddSeparator();
    m_menu->AddItem( PCB_ACTIONS::removeConstraints,          anySelection );
    m_menu->AddItem( PCB_ACTIONS::showConstraints,            S_C::ShowAlways );
    m_menu->AddItem( PCB_ACTIONS::manageConstraints,          S_C::ShowAlways );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_menu, S_C::ShowAlways, 100 );

    return true;
}


int CONSTRAINT_EDIT_TOOL::AddConstraint( const TOOL_EVENT& aEvent )
{
    PCB_CONSTRAINT_TYPE type = aEvent.Parameter<PCB_CONSTRAINT_TYPE>();
    PCB_SELECTION&      selection = m_selectionTool->GetSelection();

    std::vector<BOARD_ITEM*> items;

    for( EDA_ITEM* item : selection )
    {
        if( item->IsBOARD_ITEM() )
            items.push_back( static_cast<BOARD_ITEM*>( item ) );
    }

    // This selection-based command supersedes any clicked endpoints.
    clearEndpointPointSet();

    std::unique_ptr<PCB_CONSTRAINT> constraint =
            BuildConstraintFromItems( constraintParent(), type, items );

    if( !constraint )
        return 0;

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

    // If endpoints were already clicked into the point-set on the canvas, bind them directly
    // instead of entering the click-to-pick flow (mode-less authoring, #2329).
    if( type == PCB_CONSTRAINT_TYPE::COINCIDENT && m_endpoints
        && m_endpoints->PointSet().size() >= 2 )
    {
        bindCoincidentPointSet();
        return 0;
    }

    // In the pick plan, true means click a point anchor and false means click a whole segment.
    std::vector<bool> plan;

    switch( type )
    {
    case PCB_CONSTRAINT_TYPE::COINCIDENT:    plan = { true, true }; break;
    case PCB_CONSTRAINT_TYPE::POINT_ON_LINE: plan = { true, false }; break;
    case PCB_CONSTRAINT_TYPE::MIDPOINT:      plan = { true, false }; break;
    case PCB_CONSTRAINT_TYPE::SYMMETRIC:     plan = { true, true, false }; break;
    default:                                 return 0;
    }

    PCB_PICKER_TOOL* picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    if( !picker )
        return 0;

    // Any endpoints clicked earlier belong to a coincident bind, not this picked family; start fresh
    // so stale markers don't linger or mislead.
    clearEndpointPointSet();

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

                    // Echo the pick on the endpoint markers (the picked anchor grows 50%).  Drive it
                    // from the resolved member so the highlight matches exactly what was bound; it
                    // simply shows nothing if that anchor's shape is not among the markers.
                    if( m_endpoints )
                    {
                        m_endpoints->ToggleMember( *anchor );
                        m_endpoints->Redraw();
                    }
                }
                else   // wants a whole segment
                {
                    std::optional<KIID> seg = nearestSegment( board(), pos, snapTol );

                    if( !seg )
                        return true;

                    members.emplace_back( *seg, CONSTRAINT_ANCHOR::WHOLE );
                }

                if( members.size() < plan.size() )
                    return true;   // need more picks

                std::unique_ptr<PCB_CONSTRAINT> constraint =
                        std::make_unique<PCB_CONSTRAINT>( constraintParent(), type );

                for( const CONSTRAINT_MEMBER& member : members )
                    constraint->AddMember( member.m_item, member.m_anchor );

                PCB_CONSTRAINT* added = constraint.get();

                BOARD_COMMIT commit( this );
                commit.Add( constraint.release() );
                clearEndpointPointSet();
                finishConstraintCommit( commit, { added } );

                return false;   // done
            } );

    picker->SetCancelHandler( [&]() { clearEndpointPointSet(); } );
    picker->SetFinalizeHandler( [&]( const int& aFinalState ) {} );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );
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
                    for( const CONSTRAINT_MEMBER& member : constraint->GetMembers() )
                    {
                        if( selectedIds.count( member.m_item ) )
                        {
                            commit.Remove( constraint );
                            any = true;
                            break;
                        }
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
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraint.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintParallel.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintPerpendicular.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintEqualLength.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintCollinear.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintAngular.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintHorizontal.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintVertical.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintFixedLength.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintConcentric.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintEqualRadius.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddConstraint, PCB_ACTIONS::addConstraintFixedRadius.MakeEvent() );

    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintCoincident.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintPointOnLine.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintMidpoint.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::AddPointConstraint, PCB_ACTIONS::addConstraintSymmetric.MakeEvent() );

    Go( &CONSTRAINT_EDIT_TOOL::RemoveConstraints, PCB_ACTIONS::removeConstraints.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::ShowConstraints,   PCB_ACTIONS::showConstraints.MakeEvent() );
    Go( &CONSTRAINT_EDIT_TOOL::ManageConstraints, PCB_ACTIONS::manageConstraints.MakeEvent() );

    // Keep the diagnostics overlay current as the board changes underneath it.
    Go( &CONSTRAINT_EDIT_TOOL::refreshOverlay,    TOOL_EVENT( TC_MESSAGE, TA_MODEL_CHANGE ) );

    // Show/refresh the endpoint markers as the selection changes.
    Go( &CONSTRAINT_EDIT_TOOL::onSelectionChanged, EVENTS::SelectedEvent );
    Go( &CONSTRAINT_EDIT_TOOL::onSelectionChanged, EVENTS::UnselectedEvent );
    Go( &CONSTRAINT_EDIT_TOOL::onSelectionChanged, EVENTS::ClearedEvent );

}
