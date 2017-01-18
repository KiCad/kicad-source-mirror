/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
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

#include <limits>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_zone.h>
#include <collectors.h>
#include <wxPcbStruct.h>
#include <kiway.h>
#include <class_draw_panel_gal.h>
#include <module_editor_frame.h>
#include <array_creator.h>

#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <ratsnest_data.h>
#include <confirm.h>

#include <cassert>
#include <functional>
using namespace std::placeholders;

#include "common_actions.h"
#include "selection_tool.h"
#include "edit_tool.h"
#include "grid_helper.h"

#include <router/router_tool.h>

#include <dialogs/dialog_move_exact.h>
#include <dialogs/dialog_track_via_properties.h>

#include <board_commit.h>

EDIT_TOOL::EDIT_TOOL() :
    PCB_TOOL( "pcbnew.InteractiveEdit" ), m_selectionTool( NULL ),
    m_dragging( false )
{
}


void EDIT_TOOL::Reset( RESET_REASON aReason )
{
    m_dragging = false;

    if( aReason != RUN )
        m_commit.reset( new BOARD_COMMIT( this ) );
}


bool EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveSelection" ) );

    if( !m_selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    // Add context menu entries that are displayed when selection tool is active
    CONDITIONAL_MENU& menu = m_selectionTool->GetToolMenu().GetMenu();
    menu.AddItem( COMMON_ACTIONS::editActivate, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( COMMON_ACTIONS::rotate, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( COMMON_ACTIONS::flip, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( COMMON_ACTIONS::remove, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( COMMON_ACTIONS::properties, SELECTION_CONDITIONS::Count( 1 )
                      || SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) );
    menu.AddItem( COMMON_ACTIONS::moveExact, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( COMMON_ACTIONS::duplicate, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( COMMON_ACTIONS::createArray, SELECTION_CONDITIONS::NotEmpty );

    // Footprint actions
    menu.AddItem( COMMON_ACTIONS::editFootprintInFpEditor,
                                        SELECTION_CONDITIONS::OnlyType( PCB_MODULE_T ) &&
                                        SELECTION_CONDITIONS::Count( 1 ) );

    m_offset.x = 0;
    m_offset.y = 0;

    return true;
}


bool EDIT_TOOL::invokeInlineRouter()
{
    TRACK* track = uniqueSelected<TRACK>();
    VIA* via = uniqueSelected<VIA>();

    if( track || via )
    {
        ROUTER_TOOL* theRouter = static_cast<ROUTER_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveRouter" ) );
        assert( theRouter );

        if( !theRouter->PNSSettings().InlineDragEnabled() )
            return false;

        m_toolMgr->RunAction( COMMON_ACTIONS::routerInlineDrag, true, track ? track : via );
        return true;
    }

    return false;
}


int EDIT_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    VECTOR2I originalCursorPos = controls->GetCursorPosition();
    SELECTION& selection = m_selectionTool->GetSelection();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection)
    if( !hoverSelection() )
        return 0;

    Activate();

    m_dragging = false;         // Are selected items being dragged?
    bool restore = false;       // Should items' state be restored when finishing the tool?
    bool lockOverride = false;

    controls->ShowCursor( true );

    // cumulative translation
    wxPoint totalMovement( 0, 0 );

    GRID_HELPER grid( editFrame );
    OPT_TOOL_EVENT evt = aEvent;

    // Main loop: keep receiving events
    do
    {
        if( evt->IsCancel() )
        {
            restore = true; // Cancelling the tool means that items have to be restored
            break;          // Finish
        }

        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }

        else if( evt->IsAction( &COMMON_ACTIONS::editActivate )
                || evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            BOARD_ITEM* curr_item = selection.Front();

            if( m_dragging && evt->Category() == TC_MOUSE )
            {
                m_cursor = grid.BestSnapAnchor( evt->Position(), curr_item );
                controls->ForceCursorPosition( true, m_cursor );

                wxPoint movement = wxPoint( m_cursor.x, m_cursor.y ) - curr_item->GetPosition();
                totalMovement += movement;

                // Drag items to the current cursor position
                for( auto item : selection )
                    item->Move( movement + m_offset );

                updateRatsnest( true );
            }
            else if( !m_dragging )    // Prepare to start dragging
            {
                if( !invokeInlineRouter() )
                {
                    m_selectionTool->SanitizeSelection();

                    if( selection.Empty() )
                        break;

                    // deal with locked items (override lock or abort the operation)
                    SELECTION_LOCK_FLAGS lockFlags = m_selectionTool->CheckLock();

                    if( lockFlags == SELECTION_LOCKED )
                        break;
                    else if( lockFlags == SELECTION_LOCK_OVERRIDE )
                        lockOverride = true;

                    // Save items, so changes can be undone
                    for( auto item : selection )
                        m_commit->Modify( item );

                    m_cursor = controls->GetCursorPosition();

                    if( selection.Size() == 1 )
                    {
                        // Set the current cursor position to the first dragged item origin, so the
                        // movement vector could be computed later
                        m_cursor = grid.BestDragOrigin( originalCursorPos, curr_item );
                        grid.SetAuxAxes( true, m_cursor );
                    }
                    else
                    {
                        m_cursor = grid.Align( m_cursor );
                    }

                    controls->ForceCursorPosition( true, m_cursor );
                    controls->WarpCursor( m_cursor, true );

                    VECTOR2I o = VECTOR2I( curr_item->GetPosition() );
                    m_offset.x = o.x - m_cursor.x;
                    m_offset.y = o.y - m_cursor.y;

                    controls->SetAutoPan( true );
                    m_dragging = true;
                }
            }

            getView()->Update( &selection );
            m_toolMgr->RunAction( COMMON_ACTIONS::editModifiedSelection, true );
        }

        // Dispatch TOOL_ACTIONs
        else if( evt->Category() == TC_COMMAND )
        {
            wxPoint modPoint = getModificationPoint( selection );

            if( evt->IsAction( &COMMON_ACTIONS::remove ) )
            {
                // exit the loop, as there is no further processing for removed items
                break;
            }
            else if( evt->IsAction( &COMMON_ACTIONS::duplicate ) )
            {
                // On duplicate, stop moving this item
                // The duplicate tool should then select the new item and start
                // a new move procedure
                break;
            }
            else if( evt->IsAction( &COMMON_ACTIONS::moveExact ) )
            {
                // Can't do this, because the selection will then contain
                // stale pointers and it will all go horribly wrong...
                //editFrame->RestoreCopyFromUndoList( dummy );
                //
                // So, instead, reset the position manually
                for( auto item : selection )
                {
                    item->SetPosition( item->GetPosition() - totalMovement );

                    // And what about flipping and rotation?
                    // for now, they won't be undone, but maybe that is how
                    // it should be, so you can flip and move exact in the
                    // same action?
                }

                // This causes a double event, so we will get the dialogue
                // correctly, somehow - why does Rotate not?
                //MoveExact( aEvent );
                break;      // exit the loop - we move exactly, so we have finished moving
            }

            if( m_dragging )
            {
                // Update dragging offset (distance between cursor and the first dragged item)
                m_offset = selection.Front()->GetPosition() - modPoint;
                getView()->Update( &selection );
                updateRatsnest( true );
            }
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            if( !lockOverride )
                break; // Finish

            lockOverride = false;
        }
    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    m_dragging = false;
    m_offset.x = 0;
    m_offset.y = 0;

    if( unselect || restore )
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    if( restore )
        m_commit->Revert();
    else
        m_commit->Push( _( "Drag" ) );

    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    return 0;
}


int EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !hoverSelection( false ) )
        return 0;

    // Tracks & vias are treated in a special way:
    if( ( SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) )( selection ) )
    {
        DIALOG_TRACK_VIA_PROPERTIES dlg( editFrame, selection );

        if( dlg.ShowModal() )
        {
            dlg.Apply( *m_commit );
            m_commit->Push( _( "Edit track/via properties" ) );
        }
    }
    else if( selection.Size() == 1 ) // Properties are displayed when there is only one item selected
    {
        // Display properties dialog
        BOARD_ITEM* item = selection.Front();

        // Some of properties dialogs alter pointers, so we should deselect them
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

        // Store flags, so they can be restored later
        STATUS_FLAGS flags = item->GetFlags();
        item->ClearFlags();

        // Do not handle undo buffer, it is done by the properties dialogs @todo LEGACY
        // Display properties dialog provided by the legacy canvas frame
        editFrame->OnEditItemRequest( NULL, item );

        m_toolMgr->RunAction( COMMON_ACTIONS::editModifiedSelection, true );
        item->SetFlags( flags );
    }

    if( unselect )
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    return 0;
}


int EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !hoverSelection() || m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    wxPoint rotatePoint = getModificationPoint( selection );

    for( auto item : selection )
    {
        m_commit->Modify( item );
        item->Rotate( rotatePoint, editFrame->GetRotationAngle() );
    }

    if( !m_dragging )
        m_commit->Push( _( "Rotate" ) );

    if( unselect )
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    // TODO selectionModified
    m_toolMgr->RunAction( COMMON_ACTIONS::editModifiedSelection, true );
    editFrame->Refresh();

    return 0;
}


int EDIT_TOOL::Flip( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !hoverSelection() || m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    wxPoint flipPoint = getModificationPoint( selection );

    for( auto item : selection )
    {
        m_commit->Modify( item );
        item->Flip( flipPoint );
    }

    if( !m_dragging )
        m_commit->Push( _( "Flip" ) );

    if( unselect )
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( COMMON_ACTIONS::editModifiedSelection, true );
    getEditFrame<PCB_BASE_EDIT_FRAME>()->Refresh();

    return 0;
}


int EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    if( !hoverSelection() || m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    // Get a copy instead of a reference, as we are going to clear current selection
    auto selection = m_selectionTool->GetSelection().GetItems();

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    for( auto item : selection )
    {
        m_commit->Remove( item );
    }

    m_commit->Push( _( "Delete" ) );

    return 0;
}


int EDIT_TOOL::MoveExact( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    // Shall the selection be cleared at the end?
    bool unselect = selection.Empty();

    if( !hoverSelection() || m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    wxPoint translation;
    double rotation = 0;

    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    DIALOG_MOVE_EXACT dialog( editFrame, translation, rotation );
    int ret = dialog.ShowModal();

    if( ret == wxID_OK )
    {
        VECTOR2I rp = selection.GetCenter();
        wxPoint rotPoint( rp.x, rp.y );

        for( auto item : selection )
        {

            m_commit->Modify( item );
            item->Move( translation );
            item->Rotate( rotPoint, rotation );

            if( !m_dragging )
                getView()->Update( item, KIGFX::GEOMETRY );
        }

        m_commit->Push( _( "Move exact" ) );

        if( unselect )
            m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

        m_toolMgr->RunAction( COMMON_ACTIONS::editModifiedSelection, true );
    }

    return 0;
}


int EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    // Note: original items are no more modified.

    bool increment = aEvent.IsAction( &COMMON_ACTIONS::duplicateIncrement );

    // first, check if we have a selection, or try to get one
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    SELECTION& selection = selTool->GetSelection();

    // Be sure that there is at least one item that we can modify
    if( !hoverSelection() )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    std::vector<BOARD_ITEM*> old_items;

    for( auto item : selection )
    {
        if( item )
            old_items.push_back( item );
    }

    for( unsigned i = 0; i < old_items.size(); ++i )
    {
        BOARD_ITEM* item = old_items[i];

        // Unselect the item, so we won't pick it up again
        // Do this first, so a single-item duplicate will correctly call
        // SetCurItem and show the item properties
        m_toolMgr->RunAction( COMMON_ACTIONS::unselectItem, true, item );

        BOARD_ITEM* new_item = NULL;

        if( m_editModules )
        {
            new_item = editFrame->GetBoard()->m_Modules->Duplicate( item, increment );
        }
        else
        {
#if 0
            // @TODO: see if we allow zone duplication here
            // Duplicate zones is especially tricky (overlaping zones must be merged)
            // so zones are not duplicated
            if( item->Type() != PCB_ZONE_AREA_T )
#endif
            new_item = editFrame->GetBoard()->Duplicate( item );
        }

        if( new_item )
        {
            m_commit->Add( new_item );

            // Select the new item, so we can pick it up
            m_toolMgr->RunAction( COMMON_ACTIONS::selectItem, true, new_item );
        }
    }

    // record the new items as added
    if( !selection.Empty() )
    {
        editFrame->DisplayToolMsg( wxString::Format( _( "Duplicated %d item(s)" ),
                (int) old_items.size() ) );

        // If items were duplicated, pick them up
        // this works well for "dropping" copies around and pushes the commit
        TOOL_EVENT evt = COMMON_ACTIONS::editActivate.MakeEvent();
        Main( evt );
    }

    return 0;
};


class GAL_ARRAY_CREATOR: public ARRAY_CREATOR
{
public:

    GAL_ARRAY_CREATOR( PCB_BASE_FRAME& editFrame, bool editModules,
                       const SELECTION& selection ):
        ARRAY_CREATOR( editFrame ),
        m_editModules( editModules ),
        m_selection( selection )
    {}

private:

    int getNumberOfItemsToArray() const override
    {
        // only handle single items
        return m_selection.Size();
    }

    BOARD_ITEM* getNthItemToArray( int n ) const override
    {
        return m_selection[n];
    }

    BOARD* getBoard() const override
    {
        return m_parent.GetBoard();
    }

    MODULE* getModule() const override
    {
        // Remember this is valid and used only in the module editor.
        // in board editor, the parent of items is usually the board.
        return m_editModules ? m_parent.GetBoard()->m_Modules.GetFirst() : NULL;
    }

    wxPoint getRotationCentre() const override
    {
        const VECTOR2I rp = m_selection.GetCenter();
        return wxPoint( rp.x, rp.y );
    }

    void prePushAction( BOARD_ITEM* new_item ) override
    {
        m_parent.GetToolManager()->RunAction( COMMON_ACTIONS::unselectItem,
                                              true, new_item );
    }

    void postPushAction( BOARD_ITEM* new_item ) override
    {
    }

    void finalise() override
    {
    }

    bool m_editModules;
    const SELECTION& m_selection;
};


int EDIT_TOOL::CreateArray( const TOOL_EVENT& aEvent )
{
    // first, check if we have a selection, or try to get one
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    SELECTION& selection = selTool->GetSelection();

    // pick up items under the cursor if needed
    if( !hoverSelection() )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    GAL_ARRAY_CREATOR array_creator( *editFrame, m_editModules, selection );
    array_creator.Invoke();

    return 0;
}


void EDIT_TOOL::SetTransitions()
{
    Go( &EDIT_TOOL::Main,       COMMON_ACTIONS::editActivate.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,     COMMON_ACTIONS::rotate.MakeEvent() );
    Go( &EDIT_TOOL::Flip,       COMMON_ACTIONS::flip.MakeEvent() );
    Go( &EDIT_TOOL::Remove,     COMMON_ACTIONS::remove.MakeEvent() );
    Go( &EDIT_TOOL::Properties, COMMON_ACTIONS::properties.MakeEvent() );
    Go( &EDIT_TOOL::MoveExact,  COMMON_ACTIONS::moveExact.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,  COMMON_ACTIONS::duplicate.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,  COMMON_ACTIONS::duplicateIncrement.MakeEvent() );
    Go( &EDIT_TOOL::CreateArray,COMMON_ACTIONS::createArray.MakeEvent() );
    Go( &EDIT_TOOL::editFootprintInFpEditor, COMMON_ACTIONS::editFootprintInFpEditor.MakeEvent() );
}


void EDIT_TOOL::updateRatsnest( bool aRedraw )
{
    SELECTION& selection = m_selectionTool->GetSelection();
    RN_DATA* ratsnest = getModel<BOARD>()->GetRatsnest();

    ratsnest->ClearSimple();

    for( auto item : selection )
    {
        ratsnest->Update( item );

        if( aRedraw )
            ratsnest->AddSimple( item );
    }
}


wxPoint EDIT_TOOL::getModificationPoint( const SELECTION& aSelection )
{
    if( aSelection.Size() == 1 )
    {
        return aSelection.Front()->GetPosition() - m_offset;
    }
    else
    {
        // If EDIT_TOOL is not currently active then it means that the cursor position is not
        // updated, so we have to fetch the latest value
        if( m_toolMgr->GetCurrentToolId() != m_toolId )
            m_cursor = getViewControls()->GetCursorPosition();

        return wxPoint( m_cursor.x, m_cursor.y );
    }
}


bool EDIT_TOOL::hoverSelection( bool aSanitize )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )                        // Try to find an item that could be modified
    {
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionCursor, true );

        if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        {
            m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );
            return false;
        }
    }

    if( aSanitize )
        m_selectionTool->SanitizeSelection();

    if( selection.Empty() )        // TODO is it necessary?
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    return !selection.Empty();
}


int EDIT_TOOL::editFootprintInFpEditor( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->GetSelection();
    bool unselect = selection.Empty();

    if( !hoverSelection() )
        return 0;

    MODULE* mod = uniqueSelected<MODULE>();

    if( !mod )
        return 0;

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    editFrame->SetCurItem( mod );

    if( editFrame->GetCurItem()->GetTimeStamp() == 0 )    // Module Editor needs a non null timestamp
    {
        editFrame->GetCurItem()->SetTimeStamp( GetNewTimeStamp() );
        editFrame->OnModify();
    }

    FOOTPRINT_EDIT_FRAME* editor = (FOOTPRINT_EDIT_FRAME*) editFrame->Kiway().Player( FRAME_PCB_MODULE_EDITOR, true );

    editor->Load_Module_From_BOARD( (MODULE*) editFrame->GetCurItem() );
    editFrame->SetCurItem( NULL );     // the current module could be deleted by

    editor->Show( true );
    editor->Raise();        // Iconize( false );

    if( unselect )
        m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    return 0;
}
