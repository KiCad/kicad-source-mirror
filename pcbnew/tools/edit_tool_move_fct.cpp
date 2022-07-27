/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <advanced_config.h>
#include <limits>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <fp_shape.h>
#include <fp_textbox.h>
#include <collectors.h>
#include <pcb_edit_frame.h>
#include <drawing_sheet/ds_proxy_view_item.h>
#include <kiway.h>
#include <array_creator.h>
#include <pcbnew_settings.h>
#include <status_popup.h>
#include <tool/selection_conditions.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/edit_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/tool_event_utils.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pad_tool.h>
#include <tools/drc_tool.h>
#include <tools/drawing_tool.h>
#include <view/view_controls.h>
#include <connectivity/connectivity_algo.h>
#include <connectivity/connectivity_items.h>
#include <bitmaps.h>
#include <cassert>
#include <functional>
using namespace std::placeholders;
#include "kicad_clipboard.h"
#include <wx/hyperlink.h>
#include <router/router_tool.h>
#include <dialogs/dialog_move_exact.h>
#include <dialogs/dialog_track_via_properties.h>
#include <dialogs/dialog_unit_entry.h>
#include <board_commit.h>
#include <pcb_group.h>
#include <pcb_target.h>
#include <zone_filler.h>

#include <geometry/shape_poly_set.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <pad.h>
#include <geometry/shape_segment.h>
#include <drc/drc_test_provider_clearance_base.h>


class DRC_TEST_PROVIDER_COURTYARD_CLEARANCE_ON_MOVE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_COURTYARD_CLEARANCE_ON_MOVE() :
        DRC_TEST_PROVIDER_CLEARANCE_BASE(),
        m_largestCourtyardClearance( 0 )
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_COURTYARD_CLEARANCE_ON_MOVE ()
    {
    }

    void Init( BOARD* aBoard );

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "courtyard_clearance" );
    }

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests footprints' courtyard clearance" );
    }

    // The list of footprints having issues
    std::set<FOOTPRINT*> m_FpInConflict;

    // The list of moved footprints
    std::vector<FOOTPRINT*> m_FpInMove;

private:
    void testCourtyardClearances();

private:
    int m_largestCourtyardClearance;
};



void DRC_TEST_PROVIDER_COURTYARD_CLEARANCE_ON_MOVE::testCourtyardClearances()
{
    for( FOOTPRINT* fpA: m_board->Footprints() )
    {
        if( fpA->IsSelected() )
            continue;

        const SHAPE_POLY_SET& frontA = fpA->GetPolyCourtyard( F_CrtYd );
        const SHAPE_POLY_SET& backA = fpA->GetPolyCourtyard( B_CrtYd );

        if( frontA.OutlineCount() == 0 && backA.OutlineCount() == 0 )
             // No courtyards defined and no hole testing against other footprint's courtyards
            continue;

        BOX2I frontBBox = frontA.BBoxFromCaches();
        BOX2I backBBox = backA.BBoxFromCaches();

        frontBBox.Inflate( m_largestCourtyardClearance );
        backBBox.Inflate( m_largestCourtyardClearance );

        EDA_RECT fpABBox = fpA->GetBoundingBox();

        for( FOOTPRINT* fpB : m_FpInMove )
        {
            fpB->BuildPolyCourtyards();
            EDA_RECT              fpBBBox = fpB->GetBoundingBox();
            const SHAPE_POLY_SET& frontB = fpB->GetPolyCourtyard( F_CrtYd );
            const SHAPE_POLY_SET& backB = fpB->GetPolyCourtyard( B_CrtYd );
            DRC_CONSTRAINT        constraint;
            int                   clearance;
            int                   actual;
            VECTOR2I              pos;

            if( frontA.OutlineCount() > 0 && frontB.OutlineCount() > 0
                    && frontBBox.Intersects( frontB.BBoxFromCaches() ) )
            {
                // Currently, do not use DRC engine for calculation time reasons
                // constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, B_Cu );
                // constraint.GetValue().Min();
                clearance = 0;

                if( frontA.Collide( &frontB, clearance, &actual, &pos ) )
                {
                    m_FpInConflict.insert( fpA );
                    m_FpInConflict.insert( fpB );
                 }
            }

            if( backA.OutlineCount() > 0 && backB.OutlineCount() > 0
                    && backBBox.Intersects( backB.BBoxFromCaches() ) )
            {
                // Currently, do not use DRC engine for calculation time reasons
                // constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, B_Cu );
                // constraint.GetValue().Min();
                clearance = 0;


                if( backA.Collide( &backB, clearance, &actual, &pos ) )
                {
                    m_FpInConflict.insert( fpA );
                    m_FpInConflict.insert( fpB );
                }
            }

            // Now test if a pad hole of some other footprint is inside the courtyard area
            // of the moved footprint
            auto testPadAgainstCourtyards =
                    [&]( const PAD* pad, FOOTPRINT* footprint ) -> bool
                    {
                        if( pad->HasHole() )
                        {
                            std::shared_ptr<SHAPE_SEGMENT> hole = pad->GetEffectiveHoleShape();
                            const SHAPE_POLY_SET& front = footprint->GetPolyCourtyard( F_CrtYd );
                            const SHAPE_POLY_SET& back = footprint->GetPolyCourtyard( B_CrtYd );

                            if( front.OutlineCount() > 0 && front.Collide( hole.get(), 0 ) )
                                return true;
                            else if( back.OutlineCount() > 0 && back.Collide( hole.get(), 0 ) )
                                return true;
                        }

                        return false;
                    };

            bool skipNextCmp = false;

            if( ( frontA.OutlineCount() > 0 && frontA.BBoxFromCaches().Intersects( fpBBBox ) )
                || ( backA.OutlineCount() > 0 && backA.BBoxFromCaches().Intersects( fpBBBox ) ) )
            {
                for( const PAD* padB : fpB->Pads() )
                {
                    if( testPadAgainstCourtyards( padB, fpA ) )
                    {
                        m_FpInConflict.insert( fpA );
                        m_FpInConflict.insert( fpB );
                        skipNextCmp = true;
                        break;
                    }
                }
            }

            if( skipNextCmp )
                continue;       // fpA and fpB are already in list

            if( ( frontB.OutlineCount() > 0 && frontB.BBoxFromCaches().Intersects( fpABBox ) )
                || ( backB.OutlineCount() > 0 && backB.BBoxFromCaches().Intersects( fpABBox ) ) )
            {
                for( const PAD* padA : fpA->Pads() )
                {
                    if( testPadAgainstCourtyards( padA, fpB ) )
                    {
                        m_FpInConflict.insert( fpA );
                        m_FpInConflict.insert( fpB );
                        break;
                    }
                }
            }
        }
    }
}


void DRC_TEST_PROVIDER_COURTYARD_CLEARANCE_ON_MOVE::Init( BOARD* aBoard )
{
    m_board = aBoard;

    // Update courtyard data and clear the COURTYARD_CONFLICT flag
    for( FOOTPRINT* fp: m_board->Footprints() )
    {
        fp->ClearFlags( COURTYARD_CONFLICT );
        fp->BuildPolyCourtyards();
    }
}


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE_ON_MOVE::Run()
{
    m_FpInConflict.clear();
    m_largestCourtyardClearance = 0;

    DRC_CONSTRAINT constraint;

    if( m_drcEngine->QueryWorstConstraint( COURTYARD_CLEARANCE_CONSTRAINT, constraint ) )
        m_largestCourtyardClearance = constraint.GetValue().Min();

    testCourtyardClearances();

    return true;
}


int EDIT_TOOL::Move( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    return doMoveSelection( aEvent );
}


int EDIT_TOOL::MoveWithReference( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    return doMoveSelection( aEvent, true );
}


// Note: aEvent MUST NOT be const&; the source will get de-allocated if we go into the picker's
// event loop.
int EDIT_TOOL::doMoveSelection( TOOL_EVENT aEvent, bool aPickReference )
{
    PCB_BASE_EDIT_FRAME*  editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD*                board = editFrame->GetBoard();
    KIGFX::VIEW_CONTROLS* controls  = getViewControls();
    VECTOR2I              originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. KiCad old-style hover selection)
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
            },
            // Prompt user regarding locked items if in board editor and in free-pad-mode (if
            // we're not in free-pad mode we delay this until the second RequestSelection()).
            editFrame->GetPcbNewSettings()->m_AllowFreePads && !m_isFootprintEditor );

    if( m_dragging || selection.Empty() )
        return 0;

    LSET     item_layers = selection.GetSelectionLayers();
    bool     is_hover    = selection.IsHover(); // N.B. This must be saved before the second call
                                                // to RequestSelection() below
    VECTOR2I pickedReferencePoint;

    // Now filter out pads if not in free pads mode.  We cannot do this in the first
    // RequestSelection() as we need the item_layers when a pad is the selection front.
    if( !m_isFootprintEditor && !editFrame->GetPcbNewSettings()->m_AllowFreePads )
    {
        selection = m_selectionTool->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                {
                    sTool->FilterCollectorForMarkers( aCollector );
                    sTool->FilterCollectorForHierarchy( aCollector, true );
                    sTool->FilterCollectorForFreePads( aCollector );
                },
                true /* prompt user regarding locked items */ );
    }

    if( selection.Empty() )
        return 0;

    std::string tool = aEvent.GetCommandStr().get();
    editFrame->PushTool( tool );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->SetAutoPan( true );
    controls->ForceCursorPosition( false );

    if( aPickReference && !pickReferencePoint( _( "Select reference point for move..." ), "", "",
                                               pickedReferencePoint ) )
    {
        if( is_hover )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        editFrame->PopTool( tool );
        return 0;
    }

    auto displayConstraintsMessage =
            [editFrame]( bool constrained )
            {
                editFrame->DisplayConstraintsMsg( constrained ? _( "Constrain to H, V, 45" )
                                                            : wxT( "" ) );
            };

    std::vector<BOARD_ITEM*> sel_items;         // All the items operated on by the move below
    std::vector<BOARD_ITEM*> orig_items;        // All the original items in the selection
    std::vector<FOOTPRINT*> lastFpInConflict;   // last footprints with courtyard overlapping

    for( EDA_ITEM* item : selection )
    {
        BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( item );
        FOOTPRINT*  footprint = dynamic_cast<FOOTPRINT*>( item );

        if( boardItem )
        {
            if( !is_hover )
                orig_items.push_back( boardItem );

            sel_items.push_back( boardItem );
        }

        if( footprint )
        {
            for( PAD* pad : footprint->Pads() )
                sel_items.push_back( pad );

            // Clear this flag here; it will be set by the netlist updater if the footprint is new
            // so that it was skipped in the initial connectivity update in OnNetlistChanged
            footprint->SetAttributes( footprint->GetAttributes() & ~FP_JUST_ADDED );
        }
    }

    bool            restore_state = false;
    VECTOR2I        originalPos;
    VECTOR2I        totalMovement;
    PCB_GRID_HELPER grid( m_toolMgr, editFrame->GetMagneticItemsSettings() );
    TOOL_EVENT*     evt = &aEvent;
    VECTOR2I        prevPos;

    bool hv45Mode        = false;
    bool eatFirstMouseUp = true;
    bool hasRedrawn3D    = false;
    bool allowRedraw3D   = editFrame->GetPcbNewSettings()->m_Display.m_Live3DRefresh;
    // Courtyard conflicts will be tested only if the LAYER_CONFLICTS_SHADOW gal layer is visible
    bool showCourtyardConflicts = !m_isFootprintEditor
                                    && board->IsElementVisible( LAYER_CONFLICTS_SHADOW );

    displayConstraintsMessage( hv45Mode );

    // Used to test courtyard overlaps
    DRC_TEST_PROVIDER_COURTYARD_CLEARANCE_ON_MOVE drc_on_move;

    if( showCourtyardConflicts )
    {
        drc_on_move.Init( board );
        drc_on_move.SetDRCEngine( m_toolMgr->GetTool<DRC_TOOL>()->GetDRCEngine().get() );
    }

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    // Main loop: keep receiving events
    do
    {
        VECTOR2I movement;
        editFrame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
            eatFirstMouseUp = false;

        if( evt->IsAction( &PCB_ACTIONS::move ) || evt->IsMotion() || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview )
                || evt->IsAction( &PCB_ACTIONS::moveWithReference ) )
        {
            if( m_dragging && evt->Category() == TC_MOUSE )
            {
                bool redraw3D = false;

                VECTOR2I mousePos( controls->GetMousePosition() );

                m_cursor = grid.BestSnapAnchor( mousePos, item_layers, sel_items );

                if( controls->GetSettings().m_lastKeyboardCursorPositionValid )
                {
                    long action = controls->GetSettings().m_lastKeyboardCursorCommand;

                    // The arrow keys are by definition SINGLE AXIS.  Do not allow the other
                    // axis to be snapped to the grid.
                    if( action == ACTIONS::CURSOR_LEFT || action == ACTIONS::CURSOR_RIGHT )
                        m_cursor.y = prevPos.y;
                    else if( action == ACTIONS::CURSOR_UP || action == ACTIONS::CURSOR_DOWN )
                        m_cursor.x = prevPos.x;
                }

                if( !selection.HasReferencePoint() )
                    originalPos = m_cursor;

                if( hv45Mode )
                {
                    VECTOR2I moveVector = m_cursor - originalPos;
                    m_cursor = originalPos + GetVectorSnapped45( moveVector );
                }

                controls->ForceCursorPosition( true, m_cursor );
                selection.SetReferencePoint( m_cursor );

                movement = m_cursor - prevPos;
                prevPos = m_cursor;
                totalMovement += movement;

                // Drag items to the current cursor position
                for( EDA_ITEM* item : sel_items )
                {
                    // Don't double move footprint pads, fields, etc.
                    //
                    // For PCB_GROUP_T, we make sure the selection includes only the top level
                    // group and not its descendants.
                    if( !item->GetParent() || !item->GetParent()->IsSelected() )
                        static_cast<BOARD_ITEM*>( item )->Move( movement );

                    if( !redraw3D && item->Type() == PCB_FOOTPRINT_T )
                        redraw3D = true;
                }

                if( redraw3D && allowRedraw3D )
                {
                    editFrame->Update3DView( false, true );
                    hasRedrawn3D = true;
                }

                if( showCourtyardConflicts && drc_on_move.m_FpInMove.size() )
                {
                    drc_on_move.Run();

                    bool need_redraw = false;   // will be set to true if a COURTYARD_CONFLICT
                                                // has changed

                    // Ensure the "old" conflicts are cleared
                    for( FOOTPRINT* fp: lastFpInConflict )
                    {
                        fp->ClearFlags( COURTYARD_CONFLICT );
                        m_toolMgr->GetView()->Update( fp );
                        need_redraw = true;
                    }

                    lastFpInConflict.clear();

                    for( FOOTPRINT* fp: drc_on_move.m_FpInConflict )
                    {
                        if( !fp->HasFlag( COURTYARD_CONFLICT ) )
                        {
                            fp->SetFlags( COURTYARD_CONFLICT );
                            m_toolMgr->GetView()->Update( fp );
                            need_redraw = true;
                        }

                        lastFpInConflict.push_back( fp );
                    }

                    if( need_redraw )
                        m_toolMgr->GetView()->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
                }

                m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
            }
            else if( !m_dragging && !evt->IsAction( &ACTIONS::refreshPreview ) )
            {
                // Prepare to start dragging
                if( !( evt->IsAction( &PCB_ACTIONS::move )
                       || evt->IsAction( &PCB_ACTIONS::moveWithReference ) )
                    && ( editFrame->GetPcbNewSettings()->m_TrackDragAction != TRACK_DRAG_ACTION::MOVE ) )
                {
                    if( invokeInlineRouter( PNS::DM_ANY ) )
                        break;
                }

                editFrame->HideSolderMask();

                m_dragging = true;

                // When editing footprints, all items have the same parent
                if( IsFootprintEditor() )
                {
                    m_commit->Modify( selection.Front() );
                }
                else
                {
                    // Save items, so changes can be undone
                    for( EDA_ITEM* item : selection )
                    {
                        // Don't double move footprint pads, fields, etc.
                        //
                        // For PCB_GROUP_T, the parent is the board.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        m_commit->Modify( item );

                        // If moving a group, record position of all the descendants for undo
                        if( item->Type() == PCB_GROUP_T )
                        {
                            PCB_GROUP* group = static_cast<PCB_GROUP*>( item );
                            group->RunOnDescendants( [&]( BOARD_ITEM* bItem )
                                                     {
                                                         m_commit->Modify( bItem );
                                                     });
                        }
                    }
                }

                editFrame->UndoRedoBlock( true );
                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    // start moving with the reference point attached to the cursor
                    grid.SetAuxAxes( false );

                    if( hv45Mode )
                    {
                        VECTOR2I moveVector = m_cursor - originalPos;
                        m_cursor = originalPos + GetVectorSnapped45( moveVector );
                    }

                    movement = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
                    {
                        // Don't double move footprint pads, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        static_cast<BOARD_ITEM*>( item )->Move( movement );
                    }

                    selection.SetReferencePoint( m_cursor );
                }
                else
                {
                    std::vector<BOARD_ITEM*> items;

                    for( EDA_ITEM* item : selection )
                    {
                        items.push_back( static_cast<BOARD_ITEM*>( item ) );

                        if( item->Type() == PCB_FOOTPRINT_T )
                            drc_on_move.m_FpInMove.push_back( static_cast<FOOTPRINT*>( item ) );
                    }

                    m_cursor = grid.BestDragOrigin( originalCursorPos, items );

                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
                    if( aPickReference )
                    {
                        selection.SetReferencePoint( pickedReferencePoint );
                        controls->ForceCursorPosition( true, pickedReferencePoint );
                        m_cursor = pickedReferencePoint;
                    }
                    else
                    {
                        // Check if user wants to warp the mouse to origin of moved object
                        if( !editFrame->GetMoveWarpsCursor() )
                            m_cursor = originalCursorPos; // No, so use original mouse pos instead

                        selection.SetReferencePoint( m_cursor );
                        grid.SetAuxAxes( true, m_cursor );
                    }

                    originalPos = m_cursor;
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            }

            m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false, new VECTOR2I( movement ) );
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_dragging && evt->IsCancelInteractive() )
                evt->SetPassEvent( false );

            restore_state = true; // Canceling the tool means that items have to be restored
            break;                // Finish
        }
        else if( evt->IsAction( &ACTIONS::undo ) )
        {
            restore_state = true; // Perform undo locally
            break;                // Finish
        }
        else if( evt->IsAction( &ACTIONS::doDelete ) || evt->IsAction( &ACTIONS::cut ) )
        {
            // Dispatch TOOL_ACTIONs
            evt->SetPassEvent();
            break; // finish -- there is no further processing for removed items
        }
        else if( evt->IsAction( &ACTIONS::duplicate ) )
        {
            evt->SetPassEvent();
            break; // finish -- Duplicate tool will start a new Move with the dup'ed items
        }
        else if( evt->IsAction( &PCB_ACTIONS::rotateCw )
                || evt->IsAction( &PCB_ACTIONS::rotateCcw )
                || evt->IsAction( &PCB_ACTIONS::flip )
                || evt->IsAction( &PCB_ACTIONS::mirror ) )
        {
            eatFirstMouseUp = false;
            evt->SetPassEvent();
        }
        else if( evt->IsAction( &PCB_ACTIONS::moveExact ) )
        {
            // Reset positions so the Move Exactly is from the start.
            for( EDA_ITEM* item : selection )
            {
                BOARD_ITEM* i = static_cast<BOARD_ITEM*>( item );
                i->Move( -totalMovement );
            }

            break; // finish -- we moved exactly, so we are finished
        }
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            // Eat mouse-up/-click events that leaked through from the lock dialog
            if( eatFirstMouseUp && evt->Parameter<intptr_t>() != ACTIONS::CURSOR_CLICK )
            {
                eatFirstMouseUp = false;
                continue;
            }

            break; // finish
        }
        else if( evt->IsAction( &PCB_ACTIONS::toggleHV45Mode ) )
        {
            hv45Mode = !hv45Mode;
            displayConstraintsMessage( hv45Mode );
            evt->SetPassEvent( false );
        }
        else
        {
            evt->SetPassEvent();
        }

    } while( ( evt = Wait() ) ); // Assignment (instead of equality test) is intentional

    // Clear temporary COURTYARD_CONFLICT flag and ensure the conflict shadow is cleared
    for( FOOTPRINT* fp: lastFpInConflict )
    {
        m_toolMgr->GetView()->Update( fp );
        fp->ClearFlags( COURTYARD_CONFLICT );
    }

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    m_dragging = false;
    editFrame->UndoRedoBlock( false );

    m_toolMgr->GetTool<DRAWING_TOOL>()->UpdateStatusBar();

    if( hasRedrawn3D && restore_state )
        editFrame->Update3DView( false, true );

    // Discard reference point when selection is "dropped" onto the board
    selection.ClearReferencePoint();

    // TODO: there's an encapsulation leak here: this commit often has more than just the move
    // in it; for instance it might have a paste, append board, etc. as well.
    if( restore_state )
        m_commit->Revert();
    else
        m_commit->Push( _( "Drag" ) );

    // Remove the dynamic ratsnest from the screen
    m_toolMgr->RunAction( PCB_ACTIONS::hideDynamicRatsnest, true );

    // Unselect all items to clear selection flags and then re-select the originally selected
    // items.
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_toolMgr->RunAction( PCB_ACTIONS::selectItems, true, &orig_items );

    editFrame->PopTool( tool );
    editFrame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    return restore_state ? -1 : 0;
}

