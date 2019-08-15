/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <kiway.h>
#include <footprint_edit_frame.h>
#include <array_creator.h>
#include <pcbnew_id.h>
#include <status_popup.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>
#include <tools/edit_tool.h>
#include <tools/pcbnew_picker_tool.h>
#include <tools/tool_event_utils.h>
#include <tools/grid_helper.h>
#include <view/view_controls.h>
#include <connectivity/connectivity_data.h>
#include <confirm.h>
#include <bitmaps.h>
#include <cassert>
#include <functional>
using namespace std::placeholders;
#include "kicad_clipboard.h"
#include <router/router_tool.h>
#include <dialogs/dialog_move_exact.h>
#include <dialogs/dialog_track_via_properties.h>
#include <preview_items/ruler_item.h>
#include <board_commit.h>


void EditToolSelectionFilter( GENERAL_COLLECTOR& aCollector, int aFlags )
{
    // Iterate from the back so we don't have to worry about removals.
    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
    {
        BOARD_ITEM* item = aCollector[ i ];

        if( ( aFlags & EXCLUDE_LOCKED ) && item->IsLocked() )
        {
            aCollector.Remove( item );
        }
        else if( item->Type() == PCB_PAD_T )
        {
            MODULE* mod = static_cast<MODULE*>( item->GetParent() );

            // case 1: handle locking
            if( ( aFlags & EXCLUDE_LOCKED ) && mod && mod->IsLocked() )
            {
                aCollector.Remove( item );
            }
            else if( ( aFlags & EXCLUDE_LOCKED_PADS ) && mod && mod->PadsLocked() )
            {
                // Pad locking is considerably "softer" than item locking
                aCollector.Remove( item );

                if( !mod->IsLocked() && !aCollector.HasItem( mod ) )
                    aCollector.Append( mod );
            }

            // case 2: selection contains both the module and its pads - remove the pads
            if( mod && aCollector.HasItem( mod ) )
                aCollector.Remove( item );
        }
        else if( ( aFlags & EXCLUDE_TRANSIENTS ) && item->Type() == PCB_MARKER_T )
        {
            aCollector.Remove( item );
        }
    }
}


EDIT_TOOL::EDIT_TOOL() :
    PCB_TOOL_BASE( "pcbnew.InteractiveEdit" ),
    m_selectionTool( NULL ),
    m_dragging( false ),
    m_lockedSelected( false )
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
    wxASSERT_MSG( m_selectionTool, "pcbnew.InteractiveSelection tool is not available" );

    auto editingModuleCondition = [ this ] ( const SELECTION& aSelection ) {
        return m_editModules;
    };

    auto singleModuleCondition = SELECTION_CONDITIONS::OnlyType( PCB_MODULE_T )
                                    && SELECTION_CONDITIONS::Count( 1 );

    auto noActiveToolCondition = [ this ] ( const SELECTION& aSelection ) {
        return frame()->ToolStackIsEmpty();
    };

    // Add context menu entries that are displayed when selection tool is active
    CONDITIONAL_MENU& menu = m_selectionTool->GetToolMenu().GetMenu();

    menu.AddItem( PCB_ACTIONS::move, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::inlineBreakTrack, SELECTION_CONDITIONS::Count( 1 )
                      && SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) );
    menu.AddItem( PCB_ACTIONS::drag45Degree, SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) );
    menu.AddItem( PCB_ACTIONS::dragFreeAngle, SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) );
    menu.AddItem( PCB_ACTIONS::rotateCcw, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::rotateCw, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::flip, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( ACTIONS::doDelete, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::properties, SELECTION_CONDITIONS::Count( 1 )
                      || SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) );

    menu.AddItem( PCB_ACTIONS::moveExact, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::positionRelative, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( ACTIONS::duplicate, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::createArray, SELECTION_CONDITIONS::NotEmpty );

    menu.AddSeparator();
    menu.AddItem( ACTIONS::cut, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( ACTIONS::copy, SELECTION_CONDITIONS::NotEmpty );
    // Selection tool handles the context menu for some other tools, such as the Picker.
    // Don't add things like Paste when another tool is active.
    menu.AddItem( ACTIONS::paste, noActiveToolCondition );

    // Mirror only available in modedit
    menu.AddSeparator();
    menu.AddItem( PCB_ACTIONS::mirror, editingModuleCondition && SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::createPadFromShapes, editingModuleCondition && SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::explodePadToShapes, editingModuleCondition && SELECTION_CONDITIONS::NotEmpty );

    // Footprint actions
    menu.AddSeparator();
    menu.AddItem( PCB_ACTIONS::editFootprintInFpEditor, singleModuleCondition );
    menu.AddItem( PCB_ACTIONS::updateFootprint, singleModuleCondition );
    menu.AddItem( PCB_ACTIONS::changeFootprint, singleModuleCondition );

    // Populate the context menu displayed during the edit tool (primarily the measure tool)
    auto activeToolCondition = [ this ] ( const SELECTION& aSel ) {
        return frame()->ToolStackIsEmpty();
    };

    auto frame = getEditFrame<PCB_BASE_FRAME>();
    auto& ctxMenu = m_menu.GetMenu();

    // "Cancel" goes at the top of the context menu when a tool is active
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1 );
    ctxMenu.AddSeparator( 1 );

    if( frame )
        frame->AddStandardSubMenus( m_menu );

    return true;
}


int EDIT_TOOL::GetAndPlace( const TOOL_EVENT& aEvent )
{
    SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    MODULE*         module = getEditFrame<PCB_BASE_FRAME>()->GetFootprintFromBoardByReference();

    if( module )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, (void*) module );

        selectionTool->GetSelection().SetReferencePoint( module->GetPosition() );
        m_toolMgr->RunAction( PCB_ACTIONS::move, false );
    }

    return 0;
}


bool EDIT_TOOL::invokeInlineRouter( int aDragMode )
{
    ROUTER_TOOL* theRouter = m_toolMgr->GetTool<ROUTER_TOOL>();

    if( !theRouter )
        return false;

	// make sure we don't accidentally invoke inline routing mode while the router is already active!
    if( theRouter->IsToolActive() )
        return false;

    if( theRouter->CanInlineDrag() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::routerInlineDrag, true, aDragMode );
        return true;
    }

    return false;
}


bool EDIT_TOOL::isInteractiveDragEnabled() const
{
    auto theRouter = static_cast<ROUTER_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveRouter" ) );

    return theRouter ? theRouter->Router()->Settings().InlineDragEnabled() : false;
}


int EDIT_TOOL::Drag( const TOOL_EVENT& aEvent )
{
    int mode = PNS::DM_ANY;

    if( aEvent.IsAction( &PCB_ACTIONS::dragFreeAngle ) )
        mode |= PNS::DM_FREE_ANGLE;

    invokeInlineRouter( mode );

    return 0;
}

int EDIT_TOOL::Move( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    VECTOR2I originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection)
    PCBNEW_SELECTION& selection = m_selectionTool->RequestSelection(
        []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
        {
            EditToolSelectionFilter( aCollector, EXCLUDE_TRANSIENTS );
        } );

    if( m_dragging || selection.Empty() )
        return 0;

    LSET item_layers = static_cast<BOARD_ITEM*>( selection.Front() )->GetLayerSet();
    bool unselect = selection.IsHover(); //N.B. This must be saved before the re-selection below

    // Now filter out locked pads.  We cannot do this in the first RequestSelection() as we need
    // the item_layers when a pad is the selection front (ie: will become curr_tiem).
    selection = m_selectionTool->RequestSelection(
        []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
        {
            EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS );
        } );

    if( selection.Empty() )
        return 0;

    std::string tool = aEvent.GetCommandStr().get();
    editFrame->PushTool( tool );
    Activate();
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    auto curr_item = static_cast<BOARD_ITEM*>( selection.Front() );
    std::vector<BOARD_ITEM*> sel_items;

    for( EDA_ITEM* item : selection )
    {
        BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( item );
        MODULE*     module = dynamic_cast<MODULE*>( item );

        if( boardItem )
            sel_items.push_back( boardItem );

        if( module )
        {
            for( D_PAD* pad : module->Pads() )
                sel_items.push_back( pad );
        }
    }

    bool        restore_state = false;
    VECTOR2I    totalMovement;
    GRID_HELPER grid( editFrame );
    TOOL_EVENT* evt = const_cast<TOOL_EVENT*>( &aEvent );
    VECTOR2I    prevPos;

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    // Main loop: keep receiving events
    do
    {
        editFrame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

        if( evt->IsAction( &PCB_ACTIONS::move ) || evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ||
            evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            if( m_dragging && evt->Category() == TC_MOUSE )
            {
                m_cursor = grid.BestSnapAnchor( controls->GetMousePosition(), item_layers,
                                                sel_items );
                controls->ForceCursorPosition(true, m_cursor );
                VECTOR2I movement( m_cursor - prevPos );
                selection.SetReferencePoint( m_cursor );

                totalMovement += movement;
                prevPos = m_cursor;

                // Drag items to the current cursor position
                for( EDA_ITEM* item : selection )
                {
                    // Don't double move footprint pads, fields, etc.
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    static_cast<BOARD_ITEM*>( item )->Move( movement );
                    getView()->Update( item );
                }

                frame()->UpdateMsgPanel();
            }
            else if( !m_dragging )    // Prepare to start dragging
            {
                if ( !evt->IsAction( &PCB_ACTIONS::move ) && isInteractiveDragEnabled() )
                {
                    if( invokeInlineRouter( PNS::DM_ANY ) )
                        break;
                }

                // deal with locked items (override lock or abort the operation)
                SELECTION_LOCK_FLAGS lockFlags = m_selectionTool->CheckLock();

                if( lockFlags == SELECTION_LOCKED )
                    break;

                m_dragging = true;

                // When editing modules, all items have the same parent
                if( EditingModules() )
                {
                    m_commit->Modify( selection.Front() );
                }
                else
                {
                    // Save items, so changes can be undone
                    for( auto item : selection )
                    {
                        // Don't double move footprint pads, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        m_commit->Modify( item );
                    }
                }

                editFrame->UndoRedoBlock( true );
                m_cursor = controls->GetCursorPosition();

                if ( selection.HasReferencePoint() )
                {
                    // start moving with the reference point attached to the cursor
                    grid.SetAuxAxes( false );

                    auto delta = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( auto item : selection )
                    {
                        // Don't double move footprint pads, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        static_cast<BOARD_ITEM*>( item )->Move( delta );
                        getView()->Update( item );
                    }

                    selection.SetReferencePoint( m_cursor );
                }
                else if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
                    updateModificationPoint( selection );
                    m_cursor = grid.BestDragOrigin( originalCursorPos, curr_item );
                    grid.SetAuxAxes( true, m_cursor );
                }
                else
                {
                    updateModificationPoint( selection );
                    m_cursor = grid.Align( m_cursor );
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
            }

            m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );
        }

        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            restore_state = true; // Canceling the tool means that items have to be restored
            break;                // Finish
        }

        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }

        // Dispatch TOOL_ACTIONs
        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &ACTIONS::doDelete ) )
            {
                // exit the loop, as there is no further processing for removed items
                break;
            }
            else if( evt->IsAction( &ACTIONS::duplicate ) )
            {
                // On duplicate, stop moving this item
                // The duplicate tool should then select the new item and start
                // a new move procedure
                break;
            }
            else if( evt->IsAction( &PCB_ACTIONS::moveExact ) )
            {
                // Can't do this, because the selection will then contain
                // stale pointers and it will all go horribly wrong...
                //editFrame->RestoreCopyFromUndoList( dummy );
                //
                // So, instead, reset the position manually
                for( auto item : selection )
                {
                    BOARD_ITEM* i = static_cast<BOARD_ITEM*>( item );
                    auto delta = VECTOR2I( i->GetPosition() ) - totalMovement;
                    i->SetPosition( wxPoint( delta.x, delta.y ) );

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
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            break; // Finish
        }

        else
            evt->SetPassEvent();

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    m_lockedSelected = false;
    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    m_dragging = false;
    editFrame->UndoRedoBlock( false );

    // Discard reference point when selection is "dropped" onto the board (ie: not dragging anymore)
    selection.ClearReferencePoint();

    if( unselect )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // If canceled, we need to remove the dynamic ratsnest from the screen
    if( restore_state )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::hideDynamicRatsnest, true );
        m_commit->Revert();
    }
    else
    {
        m_commit->Push( _( "Drag" ) );
    }

    editFrame->PopTool( tool );
    return 0;
}

bool EDIT_TOOL::changeTrackWidthOnClick( const PCBNEW_SELECTION& selection )
{
    if ( selection.Size() == 1 && frame()->Settings().m_EditHotkeyChangesTrackWidth )
    {
        auto item = static_cast<BOARD_ITEM *>( selection[0] );

        m_commit->Modify( item );

        if( auto via = dyn_cast<VIA*>( item ) )
        {
            int new_width;
            int new_drill;

            if( via->GetViaType() == VIA_MICROVIA )
            {
                auto net = via->GetNet();

                new_width = net->GetMicroViaSize();
                new_drill = net->GetMicroViaDrillSize();
            }
            else
            {
                new_width = board()->GetDesignSettings().GetCurrentViaSize();
                new_drill = board()->GetDesignSettings().GetCurrentViaDrill();
            }

            via->SetDrill( new_drill );
            via->SetWidth( new_width );
        }
        else if ( auto track = dyn_cast<TRACK*>( item ) )
        {
            int new_width = board()->GetDesignSettings().GetCurrentTrackWidth();
            track->SetWidth( new_width );
        }

        m_commit->Push( _("Edit track width/via size") );
        return true;
    }

    return false;
}

int EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_TRANSIENTS ); } );

    // Tracks & vias are treated in a special way:
    if( ( SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) )( selection ) )
    {
        if ( !changeTrackWidthOnClick( selection ) )
        {
            DIALOG_TRACK_VIA_PROPERTIES dlg( editFrame, selection, *m_commit );
            dlg.ShowQuasiModal();       // QuasiModal required for NET_SELECTOR
        }
    }
    else if( selection.Size() == 1 ) // Properties are displayed when there is only one item selected
    {
        // Display properties dialog
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.Front() );

        // Do not handle undo buffer, it is done by the properties dialogs
        editFrame->OnEditItemRequest( item );

        // Notify other tools of the changes
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }

    if( selection.IsHover() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        // Notify other tools of the changes -- This updates the visual ratsnest
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
    }

    return 0;
}


int EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); }, nullptr, ! m_dragging );

    if( selection.Empty() )
        return 0;

    updateModificationPoint( selection );
    auto refPt = selection.GetReferencePoint();
    const int rotateAngle = TOOL_EVT_UTILS::GetEventRotationAngle( *editFrame, aEvent );

    // When editing modules, all items have the same parent
    if( EditingModules() )
        m_commit->Modify( selection.Front() );

    for( auto item : selection )
    {
        if( !item->IsNew() && !EditingModules() )
            m_commit->Modify( item );

        static_cast<BOARD_ITEM*>( item )->Rotate( refPt, rotateAngle );
    }

    if( !m_dragging )
        m_commit->Push( _( "Rotate" ) );

    if( selection.IsHover() && !m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );

    return 0;
}


/*!
 * Mirror a point about the vertical axis passing through another point
 */
static wxPoint mirrorPointX( const wxPoint& aPoint, const wxPoint& aMirrorPoint )
{
    wxPoint mirrored = aPoint;

    mirrored.x -= aMirrorPoint.x;
    mirrored.x = -mirrored.x;
    mirrored.x += aMirrorPoint.x;

    return mirrored;
}


/**
 * Mirror a pad in the vertical axis passing through a point
 */
static void mirrorPadX( D_PAD& aPad, const wxPoint& aMirrorPoint )
{
    wxPoint tmpPt = mirrorPointX( aPad.GetPosition(), aMirrorPoint );

    if( aPad.GetShape() == PAD_SHAPE_CUSTOM )
        aPad.MirrorXPrimitives( tmpPt.x );

    aPad.SetPosition( tmpPt );

    aPad.SetX0( aPad.GetPosition().x );

    tmpPt = aPad.GetOffset();
    tmpPt.x = -tmpPt.x;
    aPad.SetOffset( tmpPt );

    auto tmpz = aPad.GetDelta();
    tmpz.x = -tmpz.x;
    aPad.SetDelta( tmpz );

    aPad.SetOrientation( -aPad.GetOrientation() );
}


int EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); }, nullptr, ! m_dragging );

    if( selection.Empty() )
        return 0;

    updateModificationPoint( selection );
    auto refPoint = selection.GetReferencePoint();
    wxPoint mirrorPoint( refPoint.x, refPoint.y );

    // When editing modules, all items have the same parent
    if( EditingModules() )
        m_commit->Modify( selection.Front() );

    for( EDA_ITEM* item : selection )
    {
        // only modify items we can mirror
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        case PCB_MODULE_TEXT_T:
        case PCB_PAD_T:
            // Only create undo entry for items on the board
            if( !item->IsNew() && !EditingModules() )
                m_commit->Modify( item );

            break;
        default:
            continue;
        }

        // modify each object as necessary
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        {
            auto& edge = static_cast<EDGE_MODULE&>( *item );
            edge.Mirror( mirrorPoint, false );
            break;
        }

        case PCB_MODULE_TEXT_T:
        {
            auto& modText = static_cast<TEXTE_MODULE&>( *item );
            modText.Mirror( mirrorPoint, false );
            break;
        }

        case PCB_PAD_T:
        {
            auto& pad = static_cast<D_PAD&>( *item );
            mirrorPadX( pad, mirrorPoint );
            break;
        }

        default:
            // it's likely the commit object is wrong if you get here
            assert( false );
            break;
        }
    }

    if( !m_dragging )
        m_commit->Push( _( "Mirror" ) );

    if( selection.IsHover() && !m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );

    return 0;
}


int EDIT_TOOL::Flip( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); },
            nullptr, ! m_dragging );

    if( selection.Empty() )
        return 0;

    updateModificationPoint( selection );
    VECTOR2I modPoint = selection.GetCenter();
    bool leftRight = frame()->Settings().m_FlipLeftRight;

    // When editing modules, all items have the same parent
    if( EditingModules() )
        m_commit->Modify( selection.Front() );

    for( EDA_ITEM* item : selection )
    {
        if( !item->IsNew() && !EditingModules() )
            m_commit->Modify( item );

        static_cast<BOARD_ITEM*>( item )->Flip( modPoint, leftRight );
    }

    if( !m_dragging )
        m_commit->Push( _( "Flip" ) );

    if( selection.IsHover() && !m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( m_dragging )
        m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );

    return 0;
}


int EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    ROUTER_TOOL* routerTool = static_cast<ROUTER_TOOL*>
            ( m_toolMgr->FindTool( "pcbnew.InteractiveRouter" ) );

    // Do not delete items while actively routing.
    if( routerTool && routerTool->Router() && routerTool->Router()->RoutingInProgress() )
        return 1;

    std::vector<BOARD_ITEM*> lockedItems;
    Activate();

    // get a copy instead of reference (as we're going to clear the selection before removing items)
    PCBNEW_SELECTION selectionCopy;
    bool isCut = aEvent.Parameter<PCB_ACTIONS::REMOVE_FLAGS>() == PCB_ACTIONS::REMOVE_FLAGS::CUT;
    bool isAlt = aEvent.Parameter<PCB_ACTIONS::REMOVE_FLAGS>() == PCB_ACTIONS::REMOVE_FLAGS::ALT;

    // If we are in a "Cut" operation, then the copied selection exists already
    if( isCut )
    {
        selectionCopy = m_selectionTool->GetSelection();
    }
    else
    {
        selectionCopy = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); } );
    }

    bool isHover = selectionCopy.IsHover();

    // in "alternative" mode, deletion is not just a simple list of selected items,
    // it removes whole tracks, not just segments
    if( isAlt && isHover
            && ( selectionCopy.HasType( PCB_TRACE_T ) || selectionCopy.HasType( PCB_VIA_T ) ) )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::expandSelectedConnection, true );
    }

    if( selectionCopy.Empty() )
        return 0;

    // N.B. Setting the CUT flag prevents lock filtering as we only want to delete the items that
    // were copied to the clipboard, no more, no fewer.  Filtering for locked item, if any will be done
    // in the copyToClipboard() routine
    if( !m_lockedSelected && !isCut )
    {
        // Second RequestSelection removes locked items but keeps a copy of their pointers
        selectionCopy = m_selectionTool->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
                { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED ); }, &lockedItems );
    }


    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    for( EDA_ITEM* item : selectionCopy )
    {
        if( m_editModules )
        {
            m_commit->Remove( item );
            continue;
        }

        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            {
                auto text = static_cast<TEXTE_MODULE*>( item );
                auto parent = static_cast<MODULE*>( item->GetParent() );

                if( text->GetType() == TEXTE_MODULE::TEXT_is_DIVERS )
                {
                    m_commit->Modify( text );
                    getView()->Remove( text );
                    parent->Remove( text );
                }
            }
            break;

        default:
            m_commit->Remove( item );
            break;
        }
    }

    if( isCut )
        m_commit->Push( _( "Cut" ) );
    else
        m_commit->Push( _( "Delete" ) );

    if( !m_lockedSelected && !lockedItems.empty() )
    {
        ///> Popup nag for deleting locked items
        STATUS_TEXT_POPUP statusPopup( frame() );

        m_lockedSelected = true;
        m_toolMgr->RunAction( PCB_ACTIONS::selectItems, true, &lockedItems );
        statusPopup.SetText( _( "Delete again to remove locked items" ) );
        statusPopup.PopupFor( 2000 );
        statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );

        Activate();

        while( m_lockedSelected && statusPopup.IsShown() )
        {
            statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
            Wait();
        }
    }

    m_lockedSelected = false;

    return 0;
}


int EDIT_TOOL::MoveExact( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED | EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); } );

    if( selection.Empty() )
        return 0;

    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    wxPoint         translation;
    double          rotation;
    ROTATION_ANCHOR rotationAnchor = selection.Size() > 1 ? ROTATE_AROUND_SEL_CENTER
                                                          : ROTATE_AROUND_ITEM_ANCHOR;

    // TODO: Implement a visible bounding border at the edge
    auto sel_box = selection.GetBoundingBox();

    DIALOG_MOVE_EXACT dialog( editFrame, translation, rotation, rotationAnchor, sel_box );
    int ret = dialog.ShowModal();

    if( ret == wxID_OK )
    {
        VECTOR2I rp = selection.GetCenter();
        wxPoint selCenter( rp.x, rp.y );

        // Make sure the rotation is from the right reference point
        selCenter += translation;

        // When editing modules, all items have the same parent
        if( EditingModules() )
            m_commit->Modify( selection.Front() );

        for( EDA_ITEM* selItem : selection )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selItem );

            if( !item->IsNew() && !EditingModules() )
                m_commit->Modify( item );

            item->Move( translation );

            switch( rotationAnchor )
            {
            case ROTATE_AROUND_ITEM_ANCHOR:
                item->Rotate( item->GetPosition(), rotation );
                break;
            case ROTATE_AROUND_SEL_CENTER:
                item->Rotate( selCenter, rotation );
                break;
            case ROTATE_AROUND_USER_ORIGIN:
                item->Rotate( (wxPoint) editFrame->GetScreen()->m_LocalOrigin, rotation );
                break;
            case ROTATE_AROUND_AUX_ORIGIN:
                item->Rotate( editFrame->GetAuxOrigin(), rotation );
                break;
            }

            if( !m_dragging )
                getView()->Update( item );
        }

        m_commit->Push( _( "Move exact" ) );

        if( selection.IsHover() )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

        if( m_dragging )
            m_toolMgr->RunAction( PCB_ACTIONS::updateLocalRatsnest, false );
    }

    return 0;
}


int EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    bool increment = aEvent.IsAction( &PCB_ACTIONS::duplicateIncrement );

    // Be sure that there is at least one item that we can modify
    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); } );

    if( selection.Empty() )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    std::vector<BOARD_ITEM*> new_items;
    new_items.reserve( selection.Size() );

    BOARD_ITEM* orig_item = nullptr;
    BOARD_ITEM* dupe_item = nullptr;

    // Each selected item is duplicated and pushed to new_items list
    // Old selection is cleared, and new items are then selected.
    for( EDA_ITEM* item : selection )
    {
        if( !item )
            continue;

        orig_item = static_cast<BOARD_ITEM*>( item );

        if( m_editModules )
        {
            dupe_item = editFrame->GetBoard()->GetFirstModule()->Duplicate( orig_item, increment );
        }
        else
        {
#if 0
            // @TODO: see if we allow zone duplication here
            // Duplicate zones is especially tricky (overlaping zones must be merged)
            // so zones are not duplicated
            if( item->Type() != PCB_ZONE_AREA_T )
#endif
            dupe_item = editFrame->GetBoard()->Duplicate( orig_item );
        }

        if( dupe_item )
        {
            // Clear the selection flag here, otherwise the SELECTION_TOOL
            // will not properly select it later on
            dupe_item->ClearSelected();

            new_items.push_back( dupe_item );
            m_commit->Add( dupe_item );
        }
    }

    // Clear the old selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // Select the new items
    m_toolMgr->RunAction( PCB_ACTIONS::selectItems, true, &new_items );

    // record the new items as added
    if( !selection.Empty() )
    {
        editFrame->DisplayToolMsg( wxString::Format( _( "Duplicated %d item(s)" ),
                                                     (int) new_items.size() ) );

        // If items were duplicated, pick them up
        // this works well for "dropping" copies around and pushes the commit
        TOOL_EVENT evt = PCB_ACTIONS::move.MakeEvent();
        Move( evt );
    }

    return 0;
}


int EDIT_TOOL::CreateArray( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); } );

    if( selection.Empty() )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    ARRAY_CREATOR   array_creator( *editFrame, m_editModules, selection );
    array_creator.Invoke();

    return 0;
}


void EDIT_TOOL::PadFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector )
{
    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aCollector[i] );

        if( item->Type() != PCB_PAD_T )
            aCollector.Remove( i );
    }
}


void EDIT_TOOL::FootprintFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector )
{
    for( int i = aCollector.GetCount() - 1; i >= 0; i-- )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( aCollector[i] );

        if( item->Type() != PCB_MODULE_T )
            aCollector.Remove( i );
    }
}


int EDIT_TOOL::MeasureTool( const TOOL_EVENT& aEvent )
{
    if( EditingModules() && !frame()->GetModel())
        return 0;

    auto& view = *getView();
    auto& controls = *getViewControls();

    std::string tool = aEvent.GetCommandStr().get();
    frame()->PushTool( tool );
    Activate();

    EDA_UNITS_T units = frame()->GetUserUnits();
    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr, units );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    GRID_HELPER grid( frame() );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetAutoPan( false );
    controls.CaptureCursor( false );

    while( auto evt = Wait() )
    {
        frame()->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        controls.SetSnapping( !evt->Modifier( MD_ALT ) );
        const VECTOR2I cursorPos = grid.BestSnapAnchor( controls.GetMousePosition(), nullptr );
        controls.ForceCursorPosition(true, cursorPos );

        auto clearRuler = [&] () {
            view.SetVisible( &ruler, false );
            controls.SetAutoPan( false );
            controls.CaptureCursor( false );
            originSet = false;
        };

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
                clearRuler();
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }

        else if( evt->IsActivate() )
        {
            if( originSet )
                clearRuler();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }

        // click or drag starts
        else if( !originSet && ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }

        // second click or mouse up after drag ends
        else if( originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );
        }

        // move or drag when origin set updates rules
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            twoPtMgr.SetAngleSnap( evt->Modifier( MD_CTRL ) );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }

        else if( evt->IsAction( &ACTIONS::toggleUnits )
                    || evt->IsAction( &PCB_ACTIONS::updateUnits ) )
        {
            if( frame()->GetUserUnits() != units )
            {
                units = frame()->GetUserUnits();
                ruler.SwitchUnits();
                view.Update( &ruler, KIGFX::GEOMETRY );
            }
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }

        else
            evt->SetPassEvent();
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );
    return 0;
}


bool EDIT_TOOL::updateModificationPoint( PCBNEW_SELECTION& aSelection )
{
    if( m_dragging && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        auto item =  static_cast<BOARD_ITEM*>( aSelection.Front() );
        auto pos = item->GetPosition();
        aSelection.SetReferencePoint( VECTOR2I( pos.x, pos.y ) );
    }
    // ...otherwise modify items with regard to the grid-snapped cursor position
    else
    {
        m_cursor = getViewControls()->GetCursorPosition( true );
        aSelection.SetReferencePoint( m_cursor );
    }

    return true;
}


int EDIT_TOOL::EditFpInFpEditor( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection( FootprintFilter );

    if( selection.Empty() )
        return 0;

    MODULE* mod = selection.FirstOfKind<MODULE>();

    if( !mod )
        return 0;

    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    if( mod->GetTimeStamp() == 0 )    // Module Editor needs a non null timestamp
    {
        mod->SetTimeStamp( GetNewTimeStamp() );
        editFrame->OnModify();
    }

    auto editor = (FOOTPRINT_EDIT_FRAME*) editFrame->Kiway().Player( FRAME_PCB_MODULE_EDITOR, true );

    editor->Load_Module_From_BOARD( mod );

    editor->Show( true );
    editor->Raise();        // Iconize( false );

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    return 0;
}


bool EDIT_TOOL::pickCopyReferencePoint( VECTOR2I& aReferencePoint )
{
    std::string         tool = "pcbnew.InteractiveEdit.selectReferencePoint";
    STATUS_TEXT_POPUP   statusPopup( frame() );
    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    OPT<VECTOR2I>       pickedPoint;
    bool                done = false;

    statusPopup.SetText( _( "Select reference point for the copy..." ) );

    picker->SetClickHandler(
        [&]( const VECTOR2D& aPoint ) -> bool
        {
            pickedPoint = aPoint;
            statusPopup.SetText( _( "Selection copied." ) );
            statusPopup.Expire( 800 );
            return false;  // we don't need any more points
        } );

    picker->SetMotionHandler(
        [&] ( const VECTOR2D& aPos )
        {
            statusPopup.Move( wxGetMousePosition() + wxPoint( 20, -50 ) );
        } );

    picker->SetCancelHandler(
        [&]()
        {
            statusPopup.SetText( _( "Copy cancelled." ) );
            statusPopup.Expire( 800 );
        } );

    picker->SetFinalizeHandler(
        [&]( const int& aFinalState )
        {
            done = true;
        } );

    statusPopup.Move( wxGetMousePosition() + wxPoint( 20, -50 ) );
    statusPopup.Popup();

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    while( !done )
        Wait();

    if( pickedPoint.is_initialized() )
        aReferencePoint = pickedPoint.get();

    return pickedPoint.is_initialized();
}


int EDIT_TOOL::copyToClipboard( const TOOL_EVENT& aEvent )
{
    CLIPBOARD_IO io;

    Activate();

    PCBNEW_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED_PADS | EXCLUDE_TRANSIENTS ); } );

    if( selection.Empty() )
        return 1;

    VECTOR2I refPoint;
    bool rv = pickCopyReferencePoint( refPoint );
    frame()->SetMsgPanel( board() );

    if( !rv )
        return 1;

    selection.SetReferencePoint( refPoint );

    io.SetBoard( board() );
    io.SaveSelection( selection );

    return 0;
}


int EDIT_TOOL::cutToClipboard( const TOOL_EVENT& aEvent )
{
    if( !copyToClipboard( aEvent ) )
    {
        // N.B. Setting the CUT flag prevents lock filtering as we only want to delete the items that
        // were copied to the clipboard, no more, no fewer.  Filtering for locked item, if any will be done
        // in the copyToClipboard() routine
        TOOL_EVENT evt( aEvent.Category(), aEvent.Action(), TOOL_ACTION_SCOPE::AS_GLOBAL );
        evt.SetParameter( PCB_ACTIONS::REMOVE_FLAGS::CUT );
        Remove( evt );
    }

    return 0;
}


void EDIT_TOOL::setTransitions()
{
    Go( &EDIT_TOOL::GetAndPlace,         PCB_ACTIONS::getAndPlace.MakeEvent() );
    Go( &EDIT_TOOL::Move,                PCB_ACTIONS::move.MakeEvent() );
    Go( &EDIT_TOOL::Drag,                PCB_ACTIONS::drag45Degree.MakeEvent() );
    Go( &EDIT_TOOL::Drag,                PCB_ACTIONS::dragFreeAngle.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,              PCB_ACTIONS::rotateCw.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,              PCB_ACTIONS::rotateCcw.MakeEvent() );
    Go( &EDIT_TOOL::Flip,                PCB_ACTIONS::flip.MakeEvent() );
    Go( &EDIT_TOOL::Remove,              ACTIONS::doDelete.MakeEvent() );
    Go( &EDIT_TOOL::Remove,              PCB_ACTIONS::deleteFull.MakeEvent() );
    Go( &EDIT_TOOL::Properties,          PCB_ACTIONS::properties.MakeEvent() );
    Go( &EDIT_TOOL::MoveExact,           PCB_ACTIONS::moveExact.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,           ACTIONS::duplicate.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,           PCB_ACTIONS::duplicateIncrement.MakeEvent() );
    Go( &EDIT_TOOL::CreateArray,         PCB_ACTIONS::createArray.MakeEvent() );
    Go( &EDIT_TOOL::Mirror,              PCB_ACTIONS::mirror.MakeEvent() );

    Go( &EDIT_TOOL::EditFpInFpEditor,    PCB_ACTIONS::editFootprintInFpEditor.MakeEvent() );
    Go( &EDIT_TOOL::MeasureTool,         ACTIONS::measureTool.MakeEvent() );

    Go( &EDIT_TOOL::copyToClipboard,     ACTIONS::copy.MakeEvent() );
    Go( &EDIT_TOOL::cutToClipboard,      ACTIONS::cut.MakeEvent() );
}


