/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
#include <limits>
#include <kiplatform/ui.h>
#include <board.h>
#include <board_commit.h>
#include <gal/graphics_abstraction_layer.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <pcb_edit_frame.h>
#include <spread_footprints.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/edit_tool.h>
#include <tools/pcb_grid_helper.h>
#include <tools/drc_tool.h>
#include <tools/zone_filler_tool.h>
#include <router/router_tool.h>
#include <dialogs/dialog_move_exact.h>
#include <zone_filler.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_interactive_courtyard_clearance.h>
#include <view/view_controls.h>


int EDIT_TOOL::Swap( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector );

                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( item->Type() == PCB_TRACE_T )
                        aCollector.Remove( item );
                }
            },
            true /* prompt user regarding locked items */ );

    if( selection.Size() < 2 )
        return 0;

    BOARD_COMMIT  localCommit( this );
    BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    std::vector<EDA_ITEM*> sorted = selection.GetItemsSortedBySelectionOrder();

    // Save items, so changes can be undone
    for( EDA_ITEM* item : selection )
    {
        if( !item->IsNew() && !item->IsMoving() )
            commit->Modify( item );
    }

    for( size_t i = 0; i < sorted.size() - 1; i++ )
    {
        EDA_ITEM* edaItemA = sorted[i];
        EDA_ITEM* edaItemB = sorted[( i + 1 ) % sorted.size()];

        if( !edaItemA->IsBOARD_ITEM() || !edaItemB->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* a = static_cast<BOARD_ITEM*>( edaItemA );
        BOARD_ITEM* b = static_cast<BOARD_ITEM*>( edaItemB );

        // Swap X,Y position
        VECTOR2I aPos = a->GetPosition(), bPos = b->GetPosition();
        std::swap( aPos, bPos );
        a->SetPosition( aPos );
        b->SetPosition( bPos );

        // Handle footprints specially. They can be flipped to the back of the board which
        // requires a special transformation.
        if( a->Type() == PCB_FOOTPRINT_T && b->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* aFP = static_cast<FOOTPRINT*>( a );
            FOOTPRINT* bFP = static_cast<FOOTPRINT*>( b );

            // Store initial orientation of footprints, before flipping them.
            EDA_ANGLE aAngle = aFP->GetOrientation();
            EDA_ANGLE bAngle = bFP->GetOrientation();

            // Flip both if needed
            if( aFP->IsFlipped() != bFP->IsFlipped() )
            {
                aFP->Flip( aPos, false );
                bFP->Flip( bPos, false );
            }

            // Set orientation
            std::swap( aAngle, bAngle );
            aFP->SetOrientation( aAngle );
            bFP->SetOrientation( bAngle );
        }
        // We can also do a layer swap safely for two objects of the same type,
        // except groups which don't support layer swaps.
        else if( a->Type() == b->Type() && a->Type() != PCB_GROUP_T )
        {
            // Swap layers
            PCB_LAYER_ID aLayer = a->GetLayer(), bLayer = b->GetLayer();
            std::swap( aLayer, bLayer );
            a->SetLayer( aLayer );
            b->SetLayer( bLayer );
        }
    }

    if( !localCommit.Empty() )
        localCommit.Push( _( "Swap" ) );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    return 0;
}


int EDIT_TOOL::PackAndMoveFootprints( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() || m_dragging )
    {
        wxBell();
        return 0;
    }

    BOARD_COMMIT   commit( this );
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector, true );

                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( !dynamic_cast<FOOTPRINT*>( item ) )
                        aCollector.Remove( item );
                }
            },
            true /* prompt user regarding locked items */ );

    std::vector<FOOTPRINT*> footprintsToPack;

    for( EDA_ITEM* item : selection )
        footprintsToPack.push_back( static_cast<FOOTPRINT*>( item ) );

    if( footprintsToPack.empty() )
        return 0;

    BOX2I footprintsBbox;

    for( FOOTPRINT* item : footprintsToPack )
    {
        commit.Modify( item );
        item->SetFlags( IS_MOVING );
        footprintsBbox.Merge( item->GetBoundingBox( false, false ) );
    }

    SpreadFootprints( &footprintsToPack, footprintsBbox.Normalize().GetOrigin(), false );

    if( doMoveSelection( aEvent, &commit, true ) )
        commit.Push( _( "Pack Footprints" ) );
    else
        commit.Revert();

    return 0;
}


int EDIT_TOOL::Move( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() || m_dragging )
    {
        wxBell();
        return 0;
    }

    if( BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() ) )
    {
        wxCHECK( aEvent.SynchronousState(), 0 );
        aEvent.SynchronousState()->store( STS_RUNNING );

        if( doMoveSelection( aEvent, commit, true ) )
            aEvent.SynchronousState()->store( STS_FINISHED );
        else
            aEvent.SynchronousState()->store( STS_CANCELLED );
    }
    else
    {
        BOARD_COMMIT localCommit( this );

        if( doMoveSelection( aEvent, &localCommit, false ) )
            localCommit.Push( _( "Move" ) );
        else
            localCommit.Revert();
    }

    // Notify point editor.  (While doMoveSelection() will re-select the items and post this
    // event, it's done before the edit flags are cleared in BOARD_COMMIT::Push() so the point
    // editor doesn't fire up.)
    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


VECTOR2I EDIT_TOOL::getSafeMovement( const VECTOR2I& aMovement, const BOX2I& aSourceBBox,
                                     const VECTOR2D& aBBoxOffset )
{
    typedef std::numeric_limits<int> coord_limits;

    static const double max = coord_limits::max() - (int) COORDS_PADDING;
    static const double min = -max;

    BOX2D testBox( aSourceBBox.GetPosition(), aSourceBBox.GetSize() );
    testBox.Offset( aBBoxOffset );

    // Do not restrict movement if bounding box is already out of bounds
    if( testBox.GetLeft() < min || testBox.GetTop() < min || testBox.GetRight() > max
        || testBox.GetBottom() > max )
    {
        return aMovement;
    }

    testBox.Offset( aMovement );

    if( testBox.GetLeft() < min )
        testBox.Offset( min - testBox.GetLeft(), 0 );

    if( max < testBox.GetRight() )
        testBox.Offset( -( testBox.GetRight() - max ), 0 );

    if( testBox.GetTop() < min )
        testBox.Offset( 0, min - testBox.GetTop() );

    if( max < testBox.GetBottom() )
        testBox.Offset( 0, -( testBox.GetBottom() - max ) );

    return KiROUND( testBox.GetPosition() - aBBoxOffset - aSourceBBox.GetPosition() );
}


bool EDIT_TOOL::doMoveSelection( const TOOL_EVENT& aEvent, BOARD_COMMIT* aCommit, bool aAutoStart )
{
    bool moveWithReference = aEvent.IsAction( &PCB_ACTIONS::moveWithReference );
    bool moveIndividually = aEvent.IsAction( &PCB_ACTIONS::moveIndividually );

    PCB_BASE_EDIT_FRAME*               editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    PCBNEW_SETTINGS*                   cfg = editFrame->GetPcbNewSettings();
    BOARD*                             board = editFrame->GetBoard();
    KIGFX::VIEW_CONTROLS*              controls = getViewControls();
    VECTOR2I                           originalCursorPos = controls->GetCursorPosition();
    std::unique_ptr<STATUS_TEXT_POPUP> statusPopup;
    wxString                           status;
    size_t                             itemIdx = 0;

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. KiCad old-style hover selection)
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector );
                sTool->FilterCollectorForTableCells( aCollector );
            },
            true /* prompt user regarding locked items */ );

    if( m_dragging || selection.Empty() )
        return false;

    editFrame->PushTool( aEvent );
    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->SetAutoPan( true );
    controls->ForceCursorPosition( false );

    auto displayConstraintsMessage =
            [editFrame]( bool constrained )
            {
                editFrame->DisplayConstraintsMsg( constrained ? _( "Constrain to H, V, 45" )
                                                              : wxString( wxT( "" ) ) );
            };

    auto updateStatusPopup =
            [&]( EDA_ITEM* item, size_t ii, size_t count )
            {
                wxString popuptext = _( "Click to place %s (item %zu of %zu)\n"
                                     "Press <esc> to cancel all; double-click to finish" );
                wxString msg;

                if( item->Type() == PCB_FOOTPRINT_T )
                {
                    FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );
                    msg = fp->GetReference();
                }
                else if( item->Type() == PCB_PAD_T )
                {
                    PAD*       pad = static_cast<PAD*>( item );
                    FOOTPRINT* fp = pad->GetParentFootprint();
                    msg = wxString::Format( _( "%s pad %s" ), fp->GetReference(), pad->GetNumber() );
                }
                else
                {
                    msg = item->GetTypeDesc().Lower();
                }

                if( !statusPopup )
                    statusPopup = std::make_unique<STATUS_TEXT_POPUP>( frame() );

                statusPopup->SetText( wxString::Format( popuptext, msg, ii, count ) );
            };

    std::vector<BOARD_ITEM*> sel_items;         // All the items operated on by the move below
    std::vector<BOARD_ITEM*> orig_items;        // All the original items in the selection

    for( EDA_ITEM* item : selection )
    {
        if( item->IsBOARD_ITEM() )
        {
            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

            if( !selection.IsHover() )
                orig_items.push_back( boardItem );

            sel_items.push_back( boardItem );
        }

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

            for( PAD* pad : footprint->Pads() )
                sel_items.push_back( pad );

            // Clear this flag here; it will be set by the netlist updater if the footprint is new
            // so that it was skipped in the initial connectivity update in OnNetlistChanged
            footprint->SetAttributes( footprint->GetAttributes() & ~FP_JUST_ADDED );
        }
    }

    VECTOR2I pickedReferencePoint;

    if( moveWithReference && !pickReferencePoint( _( "Select reference point for move..." ), "", "",
                                                  pickedReferencePoint ) )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

        editFrame->PopTool( aEvent );
        return false;
    }

    if( moveIndividually )
    {
        orig_items.clear();

        for( EDA_ITEM* item : selection.GetItemsSortedBySelectionOrder() )
        {
            if( item->IsBOARD_ITEM() )
                orig_items.push_back( static_cast<BOARD_ITEM*>( item ) );
        }

        updateStatusPopup( orig_items[ itemIdx ], itemIdx + 1, orig_items.size() );
        statusPopup->Popup();
        statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
        canvas()->SetStatusPopup( statusPopup->GetPanel() );

        m_selectionTool->ClearSelection();
        m_selectionTool->AddItemToSel( orig_items[ itemIdx ] );

        sel_items.clear();
        sel_items.push_back( orig_items[ itemIdx ] );
    }

    bool            restore_state = false;
    VECTOR2I        originalPos;
    VECTOR2D        bboxMovement;
    BOX2I           originalBBox;
    bool            updateBBox = true;
    LSET            layers( editFrame->GetActiveLayer() );
    PCB_GRID_HELPER grid( m_toolMgr, editFrame->GetMagneticItemsSettings() );
    TOOL_EVENT      copy = aEvent;
    TOOL_EVENT*     evt = &copy;
    VECTOR2I        prevPos;
    bool            enableLocalRatsnest = true;

    bool hv45Mode        = false;
    bool eatFirstMouseUp = true;
    bool allowRedraw3D   = cfg->m_Display.m_Live3DRefresh;
    bool showCourtyardConflicts = !m_isFootprintEditor && cfg->m_ShowCourtyardCollisions;

    // Used to test courtyard overlaps
    std::unique_ptr<DRC_INTERACTIVE_COURTYARD_CLEARANCE> drc_on_move = nullptr;

    if( showCourtyardConflicts )
    {
        std::shared_ptr<DRC_ENGINE> drcEngine = m_toolMgr->GetTool<DRC_TOOL>()->GetDRCEngine();
        drc_on_move.reset( new DRC_INTERACTIVE_COURTYARD_CLEARANCE( drcEngine ) );
        drc_on_move->Init( board );
    }

    displayConstraintsMessage( hv45Mode );

    // Prime the pump
    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    // Main loop: keep receiving events
    do
    {
        VECTOR2I movement;
        editFrame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        bool isSkip = evt->IsAction( &PCB_ACTIONS::skip ) && moveIndividually;

        if( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
            eatFirstMouseUp = false;

        if( evt->IsAction( &PCB_ACTIONS::move ) || evt->IsMotion() || evt->IsDrag( BUT_LEFT )
                || evt->IsAction( &ACTIONS::refreshPreview )
                || evt->IsAction( &PCB_ACTIONS::moveWithReference )
                || evt->IsAction( &PCB_ACTIONS::moveIndividually ) )
        {
            if( m_dragging && evt->Category() == TC_MOUSE )
            {
                bool redraw3D = false;

                VECTOR2I mousePos( controls->GetMousePosition() );

                m_cursor = grid.BestSnapAnchor( mousePos, layers,
                                                grid.GetSelectionGrid( selection ), sel_items );

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

                if( updateBBox )
                {
                    originalBBox = BOX2I();
                    bboxMovement = VECTOR2D();

                    for( EDA_ITEM* item : sel_items )
                    {
                        originalBBox.Merge( item->ViewBBox() );
                    }

                    updateBBox = false;
                }

                // Constrain selection bounding box to coordinates limits
                movement = getSafeMovement( m_cursor - prevPos, originalBBox, bboxMovement );

                // Apply constrained movement
                m_cursor = prevPos + movement;

                controls->ForceCursorPosition( true, m_cursor );
                selection.SetReferencePoint( m_cursor );

                prevPos = m_cursor;
                bboxMovement += movement;

                // Drag items to the current cursor position
                for( EDA_ITEM* item : sel_items )
                {
                    // Don't double move child items.
                    if( !item->GetParent() || !item->GetParent()->IsSelected() )
                        static_cast<BOARD_ITEM*>( item )->Move( movement );

                    if( item->Type() == PCB_GENERATOR_T && sel_items.size() == 1 )
                    {
                        m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genUpdateEdit, aCommit,
                                                         static_cast<PCB_GENERATOR*>( item ) );
                    }

                    if( item->Type() == PCB_FOOTPRINT_T )
                        redraw3D = true;
                }

                if( redraw3D && allowRedraw3D )
                    editFrame->Update3DView( false, true );

                if( showCourtyardConflicts && drc_on_move->m_FpInMove.size() )
                {
                    drc_on_move->Run();
                    drc_on_move->UpdateConflicts( m_toolMgr->GetView(), true );
                }

                m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
            }
            else if( !m_dragging && ( aAutoStart || !evt->IsAction( &ACTIONS::refreshPreview ) ) )
            {
                // Prepare to start dragging
                editFrame->HideSolderMask();

                m_dragging = true;

                for( EDA_ITEM* item : selection )
                {
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    if( !item->IsNew() && !item->IsMoving() )
                    {
                        if( item->Type() == PCB_GENERATOR_T && sel_items.size() == 1 )
                        {
                            enableLocalRatsnest = false;

                            m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genStartEdit, aCommit,
                                                             static_cast<PCB_GENERATOR*>( item ) );
                        }
                        else
                        {
                            aCommit->Modify( item );
                        }

                        item->SetFlags( IS_MOVING );

                        static_cast<BOARD_ITEM*>( item )->RunOnDescendants(
                                [&]( BOARD_ITEM* bItem )
                                {
                                    item->SetFlags( IS_MOVING );
                                } );
                    }
                }

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
                    if( showCourtyardConflicts )
                    {
                        std::vector<FOOTPRINT*>& FPs = drc_on_move->m_FpInMove;

                        for( BOARD_ITEM* item : sel_items )
                        {
                            if( item->Type() == PCB_FOOTPRINT_T )
                                FPs.push_back( static_cast<FOOTPRINT*>( item ) );

                            item->RunOnDescendants(
                                    [&]( BOARD_ITEM* descendent )
                                    {
                                        if( descendent->Type() == PCB_FOOTPRINT_T )
                                            FPs.push_back( static_cast<FOOTPRINT*>( descendent ) );
                                    } );
                        }
                    }

                    m_cursor = grid.BestDragOrigin( originalCursorPos, sel_items,
                                                    grid.GetSelectionGrid( selection ),
                                                    &m_selectionTool->GetFilter() );

                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
                    if( moveWithReference )
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

                // Update variables for bounding box collision calculations
                updateBBox = true;

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            }

            if( statusPopup )
                statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );

            if( enableLocalRatsnest )
                m_toolMgr->PostAction( PCB_ACTIONS::updateLocalRatsnest, movement );
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_dragging && evt->IsCancelInteractive() )
                evt->SetPassEvent( false );

            restore_state = true; // Canceling the tool means that items have to be restored
            break;                // Finish
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_selectionTool->GetToolMenu().ShowContextMenu( selection );
        }
        else if( evt->IsAction( &ACTIONS::undo ) || evt->IsAction( &ACTIONS::doDelete ) )
        {
            restore_state = true; // Perform undo locally
            break;                // Finish
        }
        else if( evt->IsAction( &ACTIONS::duplicate ) || evt->IsAction( &ACTIONS::cut ) )
        {
        }
        else if( evt->IsAction( &PCB_ACTIONS::rotateCw )
                || evt->IsAction( &PCB_ACTIONS::rotateCcw )
                || evt->IsAction( &PCB_ACTIONS::flip )
                || evt->IsAction( &PCB_ACTIONS::mirrorH )
                || evt->IsAction( &PCB_ACTIONS::mirrorV ) )
        {
            updateBBox = true;
            eatFirstMouseUp = false;
            evt->SetPassEvent();
        }
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) || isSkip )
        {
            // Eat mouse-up/-click events that leaked through from the lock dialog
            if( eatFirstMouseUp && !evt->IsAction( &ACTIONS::cursorClick ) )
            {
                eatFirstMouseUp = false;
                continue;
            }
            else if( moveIndividually && m_dragging )
            {
                // Put skipped items back where they started
                if( isSkip )
                    orig_items[itemIdx]->SetPosition( originalPos );

                view()->Update( orig_items[itemIdx] );
                rebuildConnectivity();

                if( ++itemIdx < orig_items.size() )
                {
                    BOARD_ITEM* nextItem = orig_items[itemIdx];

                    m_selectionTool->ClearSelection();

                    originalPos = nextItem->GetPosition();
                    m_selectionTool->AddItemToSel( nextItem );
                    selection.SetReferencePoint( originalPos );

                    sel_items.clear();
                    sel_items.push_back( nextItem );
                    updateStatusPopup( nextItem, itemIdx + 1, orig_items.size() );

                    // Pick up new item
                    aCommit->Modify( nextItem );
                    nextItem->Move( controls->GetCursorPosition( true ) - nextItem->GetPosition() );

                    continue;
                }
            }

            break; // finish
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            // The first click will move the new item, so put it back
            if( moveIndividually )
                orig_items[itemIdx]->SetPosition( originalPos );

            break; // finish
        }
        else if( evt->IsAction( &PCB_ACTIONS::toggleHV45Mode ) )
        {
            hv45Mode = !hv45Mode;
            displayConstraintsMessage( hv45Mode );
            evt->SetPassEvent( false );
        }
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                 || evt->IsAction( &PCB_ACTIONS::moveExact )
                 || evt->IsAction( &PCB_ACTIONS::moveWithReference )
                 || evt->IsAction( &PCB_ACTIONS::copyWithReference )
                 || evt->IsAction( &PCB_ACTIONS::positionRelative )
                 || evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

    } while( ( evt = Wait() ) ); // Assignment (instead of equality test) is intentional

    // Clear temporary COURTYARD_CONFLICT flag and ensure the conflict shadow is cleared
    if( showCourtyardConflicts )
        drc_on_move->ClearConflicts( m_toolMgr->GetView() );

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    m_dragging = false;

    // Discard reference point when selection is "dropped" onto the board
    selection.ClearReferencePoint();

    // Unselect all items to clear selection flags and then re-select the originally selected
    // items.
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    if( restore_state )
    {
        if( sel_items.size() == 1 && sel_items.back()->Type() == PCB_GENERATOR_T )
        {
            m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genRevertEdit, aCommit,
                                             static_cast<PCB_GENERATOR*>( sel_items.back() ) );
        }
    }
    else
    {
        if( sel_items.size() == 1 && sel_items.back()->Type() == PCB_GENERATOR_T )
        {
            m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genPushEdit, aCommit,
                                             static_cast<PCB_GENERATOR*>( sel_items.back() ) );
        }

        EDA_ITEMS oItems( orig_items.begin(), orig_items.end() );
        m_toolMgr->RunAction<EDA_ITEMS*>( PCB_ACTIONS::selectItems, &oItems );
    }

    // Remove the dynamic ratsnest from the screen
    m_toolMgr->RunAction( PCB_ACTIONS::hideLocalRatsnest );

    editFrame->PopTool( aEvent );
    editFrame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    return !restore_state;
}

