/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <pcbnew_id.h>

#include <tool/tool_manager.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <ratsnest_data.h>
#include <confirm.h>
#include <bitmaps.h>
#include <hotkeys.h>

#include <cassert>
#include <functional>
using namespace std::placeholders;

#include "pcb_actions.h"
#include "selection_tool.h"
#include "edit_tool.h"
#include "grid_helper.h"

#include <router/router_tool.h>

#include <dialogs/dialog_move_exact.h>
#include <dialogs/dialog_track_via_properties.h>
#include <dialogs/dialog_exchange_modules.h>

#include <tools/tool_event_utils.h>

#include <preview_items/ruler_item.h>

#include <board_commit.h>

// Edit tool actions
TOOL_ACTION PCB_ACTIONS::editFootprintInFpEditor( "pcbnew.InteractiveEdit.editFootprintInFpEditor",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_MODULE_WITH_MODEDIT ),
        _( "Open in Footprint Editor" ),
        _( "Opens the selected footprint in the Footprint Editor" ),
        module_editor_xpm );

TOOL_ACTION PCB_ACTIONS::copyPadToSettings( "pcbnew.InteractiveEdit.copyPadToSettings",
        AS_GLOBAL, 0,
        _( "Copy Pad Settings to Current Settings" ),
        _( "Copies the properties of selected pad to the current template pad settings." ) );

TOOL_ACTION PCB_ACTIONS::copySettingsToPads( "pcbnew.InteractiveEdit.copySettingsToPads",
        AS_GLOBAL, 0,
        _( "Copy Current Settings to Pads" ),
        _( "Copies the current template pad settings to the selected pad(s)." ) );

TOOL_ACTION PCB_ACTIONS::globalEditPads( "pcbnew.InteractiveEdit.globalPadEdit",
        AS_GLOBAL, 0,
        _( "Global Pad Edition" ),
        _( "Changes pad properties globally." ), push_pad_settings_xpm );

TOOL_ACTION PCB_ACTIONS::editActivate( "pcbnew.InteractiveEdit",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE_ITEM ),
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::duplicate( "pcbnew.InteractiveEdit.duplicate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE_ITEM ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ), duplicate_module_xpm );

TOOL_ACTION PCB_ACTIONS::duplicateIncrement( "pcbnew.InteractiveEdit.duplicateIncrementPads",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE_ITEM_AND_INCREMENT ),
        _( "Duplicate" ), _( "Duplicates the selected item(s), incrementing pad numbers" ) );

TOOL_ACTION PCB_ACTIONS::moveExact( "pcbnew.InteractiveEdit.moveExact",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE_ITEM_EXACT ),
        _( "Move Exactly" ), _( "Moves the selected item(s) by an exact amount" ),
        move_module_xpm );

TOOL_ACTION PCB_ACTIONS::createArray( "pcbnew.InteractiveEdit.createArray",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_CREATE_ARRAY ),
        _( "Create Array" ), _( "Create array" ), array_module_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::rotateCw( "pcbnew.InteractiveEdit.rotateCw",
        AS_GLOBAL, MD_SHIFT + 'R',
        _( "Rotate 90 deg CW" ), _( "Rotates selected item(s) clockwise" ),
        rotate_cw_xpm, AF_NONE, (void*) -1 );

TOOL_ACTION PCB_ACTIONS::rotateCcw( "pcbnew.InteractiveEdit.rotateCcw",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ROTATE_ITEM ),
        _( "Rotate 90 deg CCW" ), _( "Rotates selected item(s) counter-clockwise" ),
        rotate_ccw_xpm, AF_NONE, (void*) 1 );

TOOL_ACTION PCB_ACTIONS::flip( "pcbnew.InteractiveEdit.flip",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_FLIP_ITEM ),
        _( "Flip" ), _( "Flips selected item(s)" ), swap_layer_xpm );

TOOL_ACTION PCB_ACTIONS::mirror( "pcbnew.InteractiveEdit.mirror",
        AS_GLOBAL, 0,
        _( "Mirror" ), _( "Mirrors selected item" ), mirror_h_xpm );

TOOL_ACTION PCB_ACTIONS::remove( "pcbnew.InteractiveEdit.remove",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_BACK_SPACE ),
        _( "Delete" ), _( "Deletes selected item(s)" ), delete_xpm,
        AF_NONE, (void*) REMOVE_FLAGS::NORMAL );

TOOL_ACTION PCB_ACTIONS::removeAlt( "pcbnew.InteractiveEdit.removeAlt",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DELETE ),
        _( "Delete (Alternative)" ), _( "Deletes selected item(s)" ), delete_xpm,
        AF_NONE, (void*) REMOVE_FLAGS::ALT );

TOOL_ACTION PCB_ACTIONS::exchangeFootprints( "pcbnew.InteractiveEdit.ExchangeFootprints",
        AS_GLOBAL, 0,
        _( "Exchange Footprint" ), _( "Change the footprint used for modules" ),
        exchange_xpm );

TOOL_ACTION PCB_ACTIONS::properties( "pcbnew.InteractiveEdit.properties",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_ITEM ),
        _( "Properties" ), _( "Displays item properties dialog" ), config_xpm );

TOOL_ACTION PCB_ACTIONS::selectionModified( "pcbnew.InteractiveEdit.ModifiedSelection",
        AS_GLOBAL, 0,
        "", "", nullptr, AF_NOTIFY );

TOOL_ACTION PCB_ACTIONS::measureTool( "pcbnew.InteractiveEdit.measureTool",
        AS_GLOBAL, MD_CTRL + MD_SHIFT + 'M',
        _( "Measuring tool" ), _( "Interactively measure distance between points" ),
        nullptr, AF_ACTIVATE );


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

    auto editingModuleCondition = [ this ] ( const SELECTION& aSelection ) {
        return m_editModules;
    };

    auto singleModuleCondition = SELECTION_CONDITIONS::OnlyType( PCB_MODULE_T )
                                    && SELECTION_CONDITIONS::Count( 1 );

    // Add context menu entries that are displayed when selection tool is active
    CONDITIONAL_MENU& menu = m_selectionTool->GetToolMenu().GetMenu();
    menu.AddItem( PCB_ACTIONS::editActivate, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::rotateCcw, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::rotateCw, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::flip, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::remove, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::properties, SELECTION_CONDITIONS::Count( 1 )
                      || SELECTION_CONDITIONS::OnlyTypes( GENERAL_COLLECTOR::Tracks ) );
    menu.AddItem( PCB_ACTIONS::moveExact, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::positionRelative, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::duplicate, SELECTION_CONDITIONS::NotEmpty );
    menu.AddItem( PCB_ACTIONS::createArray, SELECTION_CONDITIONS::NotEmpty );

    // Mirror only available in modedit
    menu.AddItem( PCB_ACTIONS::mirror, editingModuleCondition && SELECTION_CONDITIONS::NotEmpty );

    // Footprint actions
    menu.AddItem( PCB_ACTIONS::editFootprintInFpEditor,
                  singleModuleCondition );
    menu.AddItem( PCB_ACTIONS::exchangeFootprints,
                  singleModuleCondition );

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

        m_toolMgr->RunAction( PCB_ACTIONS::routerInlineDrag, true );
        return true;
    }

    return false;
}


int EDIT_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    VECTOR2I originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection)
    auto& selection = m_selectionTool->RequestSelection( SELECTION_DELETABLE );

    if( selection.Empty() )
        return 0;

    bool unselect = selection.IsHover();

    Activate();

    m_dragging = false;         // Are selected items being dragged?
    bool restore = false;       // Should items' state be restored when finishing the tool?
    bool lockOverride = false;

    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    // cumulative translation
    wxPoint totalMovement( 0, 0 );

    GRID_HELPER grid( editFrame );
    OPT_TOOL_EVENT evt = aEvent;

    // Main loop: keep receiving events
    do
    {
        if( evt->IsAction( &PCB_ACTIONS::editActivate )
                || evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            if( selection.Empty() )
                break;

            BOARD_ITEM* curr_item = static_cast<BOARD_ITEM*>( selection.Front() );

            if( m_dragging && evt->Category() == TC_MOUSE )
            {
                m_cursor = grid.BestSnapAnchor( evt->Position(), curr_item );
                controls->ForceCursorPosition( true, m_cursor );

                wxPoint movement = wxPoint( m_cursor.x, m_cursor.y ) - curr_item->GetPosition();
                totalMovement += movement;

                // Drag items to the current cursor position
                for( auto item : selection )
                {
                    static_cast<BOARD_ITEM*>( item )->Move( movement + m_offset );
                }
            }
            else if( !m_dragging )    // Prepare to start dragging
            {
                if( !invokeInlineRouter() )
                {
                    selection = m_selectionTool->RequestSelection( SELECTION_DEFAULT );

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

            m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, false );
        }

        else if( evt->IsCancel() || evt->IsActivate() )
        {
            restore = true; // Cancelling the tool means that items have to be restored
            break;          // Finish
        }

        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }

        // Dispatch TOOL_ACTIONs
        else if( evt->Category() == TC_COMMAND )
        {
            wxPoint modPoint = getModificationPoint( selection );

            if( evt->IsAction( &PCB_ACTIONS::remove ) )
            {
                // exit the loop, as there is no further processing for removed items
                break;
            }
            else if( evt->IsAction( &PCB_ACTIONS::duplicate ) )
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
                    i->SetPosition( i->GetPosition() - totalMovement );

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
                m_offset = static_cast<BOARD_ITEM*>( selection.Front() )->GetPosition() - modPoint;
                getView()->Update( &selection );
            }
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            if( !lockOverride )
                break; // Finish

            lockOverride = false;
        }
    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    m_dragging = false;
    m_offset.x = 0;
    m_offset.y = 0;

    if( unselect || restore )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    if( restore )
        m_commit->Revert();
    else
        m_commit->Push( _( "Drag" ) );

    return 0;
}


int EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    const auto& selection = m_selectionTool->RequestSelection ( SELECTION_EDITABLE | SELECTION_DELETABLE );

    if( selection.Empty() )
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
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( selection.Front() );

        // Some of properties dialogs alter pointers, so we should deselect them
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        // Store flags, so they can be restored later
        STATUS_FLAGS flags = item->GetFlags();
        item->ClearFlags();

        // Do not handle undo buffer, it is done by the properties dialogs @todo LEGACY
        // Display properties dialog provided by the legacy canvas frame
        editFrame->OnEditItemRequest( NULL, item );

        m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );
        item->SetFlags( flags );
    }

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    return 0;
}


int EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    const auto& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() )
        return 0;

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    wxPoint modPoint = getModificationPoint( selection );
    const int rotateAngle = TOOL_EVT_UTILS::GetEventRotationAngle( *editFrame, aEvent );

    for( auto item : selection )
    {
        m_commit->Modify( item );
        static_cast<BOARD_ITEM*>( item )->Rotate( modPoint, rotateAngle );
    }

    // Update the dragging point offset
    m_offset = static_cast<BOARD_ITEM*>( selection.Front() )->GetPosition() - modPoint;

    if( !m_dragging )
        m_commit->Push( _( "Rotate" ) );

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );

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
    const auto& selection = m_selectionTool->RequestSelection();

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    if( selection.Empty() )
        return 0;

    wxPoint mirrorPoint = getModificationPoint( selection );

    for( auto item : selection )
    {
        // only modify items we can mirror
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        case PCB_MODULE_TEXT_T:
        case PCB_PAD_T:
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

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );

    return 0;
}


int EDIT_TOOL::Flip( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection();

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    if( selection.Empty() )
        return 0;

    wxPoint modPoint = getModificationPoint( selection );

    for( auto item : selection )
    {
        m_commit->Modify( item );
        static_cast<BOARD_ITEM*>( item )->Flip( modPoint );
    }

    // Update the dragging point offset
    m_offset = static_cast<BOARD_ITEM*>( selection.Front() )->GetPosition() - modPoint;

    if( !m_dragging )
        m_commit->Push( _( "Flip" ) );

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );

    return 0;
}


int EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    // get a copy instead of reference (as we're going to clear the selectio before removing items)
    auto selection = m_selectionTool->RequestSelection( SELECTION_DELETABLE | SELECTION_SANITIZE_PADS );

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    if( selection.Empty() )
        return 0;

    // is this "alternative" remove?
    const bool isAlt = aEvent.Parameter<intptr_t>() ==
            (int) PCB_ACTIONS::REMOVE_FLAGS::ALT;

    // in "alternative" mode, deletion is not just a simple list
    // of selected items, it is:
    //   - whole tracks, not just segments
    if( isAlt && selection.IsHover() )
    {
        m_toolMgr->RunAction( PCB_ACTIONS::selectConnection, true );
        selection = m_selectionTool->RequestSelection( SELECTION_DELETABLE | SELECTION_SANITIZE_PADS );
    }

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    for( auto item : selection )
    {
        m_commit->Remove( item );
    }

    m_commit->Push( _( "Delete" ) );

    return 0;
}


int EDIT_TOOL::MoveExact( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection();

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    if( selection.Empty() )
        return 0;

    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    MOVE_PARAMETERS params;
    params.editingFootprint = m_editModules;

    DIALOG_MOVE_EXACT dialog( editFrame, params );
    int ret = dialog.ShowModal();

    if( ret == wxID_OK )
    {
        VECTOR2I rp = selection.GetCenter();
        wxPoint rotPoint( rp.x, rp.y );

        // Begin at the center of the selection determined above
        wxPoint anchorPoint = rotPoint;

        // If the anchor is not ANCHOR_FROM_LIBRARY then the user applied an override.
        // Also run through this block if only one item is slected because it may be a module,
        // in which case we want something different than the center of the selection
        if( ( params.anchor != ANCHOR_FROM_LIBRARY ) || ( selection.GetSize() == 1 ) )
        {
            BOARD_ITEM* topLeftItem = static_cast<BOARD_ITEM*>( selection.GetTopLeftModule() );

            // no module found if the GetTopLeftModule() returns null, retry for
            if( topLeftItem == nullptr )
            {
                topLeftItem = static_cast<BOARD_ITEM*>( selection.GetTopLeftItem() );
                anchorPoint = topLeftItem->GetPosition();
            }

            if( topLeftItem->Type() == PCB_MODULE_T )
            {
                // Cast to module to allow access to the pads
                MODULE* mod = static_cast<MODULE*>( topLeftItem );

                switch( params.anchor )
                {
                case ANCHOR_FROM_LIBRARY:
                    anchorPoint = mod->GetPosition();
                    break;
                case ANCHOR_TOP_LEFT_PAD:
                    topLeftItem = mod->GetTopLeftPad();
                    break;
                case ANCHOR_CENTER_FOOTPRINT:
                    anchorPoint = mod->GetFootprintRect().GetCenter();
                    break;
                }
            }

            if( topLeftItem->Type() == PCB_PAD_T )
            {
                if( static_cast<D_PAD*>( topLeftItem )->GetAttribute() == PAD_ATTRIB_SMD )
                {
                    // Use the top left corner of SMD pads as an anchor instead of the center
                    anchorPoint = topLeftItem->GetBoundingBox().GetPosition();
                }
                else
                {
                    anchorPoint = topLeftItem->GetPosition();
                }
            }
        }

        wxPoint origin;

        switch( params.origin )
        {
        case RELATIVE_TO_USER_ORIGIN:
            origin = editFrame->GetScreen()->m_O_Curseur;
            break;

        case RELATIVE_TO_GRID_ORIGIN:
            origin = editFrame->GetGridOrigin();
            break;

        case RELATIVE_TO_DRILL_PLACE_ORIGIN:
            origin = editFrame->GetAuxOrigin();
            break;

        case RELATIVE_TO_SHEET_ORIGIN:
            origin = wxPoint( 0, 0 );
            break;

        case RELATIVE_TO_CURRENT_POSITION:
            // relative movement means that only the translation values should be used:
            // -> set origin and anchor to zero
            origin = wxPoint( 0, 0 );
            anchorPoint = wxPoint( 0, 0 );
            break;
        }

        wxPoint finalMoveVector = params.translation + origin - anchorPoint;

        // Make sure the rotation is from the right reference point
        rotPoint += finalMoveVector;

        for( auto item : selection )
        {
            m_commit->Modify( item );
            static_cast<BOARD_ITEM*>( item )->Move( finalMoveVector );
            static_cast<BOARD_ITEM*>( item )->Rotate( rotPoint, params.rotation );

            if( !m_dragging )
                getView()->Update( item );
        }

        m_commit->Push( _( "Move exact" ) );

        if( selection.IsHover() )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );
    }

    return 0;
}


int EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    bool increment = aEvent.IsAction( &PCB_ACTIONS::duplicateIncrement );

    // Be sure that there is at least one item that we can modify
    const auto& selection = m_selectionTool->RequestSelection( SELECTION_DELETABLE | SELECTION_SANITIZE_PADS );

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
    for( auto item : selection )
    {
        if( !item )
            continue;

        orig_item = static_cast<BOARD_ITEM*>( item );

        if( m_editModules )
        {
            dupe_item = editFrame->GetBoard()->m_Modules->Duplicate( orig_item, increment );
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
        TOOL_EVENT evt = PCB_ACTIONS::editActivate.MakeEvent();
        Main( evt );
    }

    return 0;
}


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
        return static_cast<BOARD_ITEM*>( m_selection[n] );
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

    void prePushAction( BOARD_ITEM* aItem ) override
    {
        // Because aItem is/can be created from a selected item, and inherits from
        // it this state, reset the selected stated of aItem:
        aItem->ClearSelected();

        if( aItem->Type() == PCB_MODULE_T )
        {
            static_cast<MODULE*>( aItem )->RunOnChildren( [&] ( BOARD_ITEM* item )
                                    {
                                        item->ClearSelected();
                                    }
                                );
        }
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
    const auto& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() )
        return 0;

    // we have a selection to work on now, so start the tool process
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    GAL_ARRAY_CREATOR array_creator( *editFrame, m_editModules, selection );
    array_creator.Invoke();

    return 0;
}


int EDIT_TOOL::ExchangeFootprints( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() )
        return 0;

    MODULE* mod = selection.FirstOfKind<MODULE> ();

    if( !mod )
        return 0;

    frame()->SetCurItem( mod );

    // Footprint exchange could remove modules, so they have to be
    // removed from the selection first
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // invoke the exchange dialog process
    {
        DIALOG_EXCHANGE_MODULE dialog( frame(), mod );
        dialog.ShowQuasiModal();
    }

    // The current item can be deleted by exchange module, and the
    // selection is emptied, so remove current item from frame info area
    frame()->SetCurItem( nullptr );

    return 0;
}


int EDIT_TOOL::MeasureTool( const TOOL_EVENT& aEvent )
{
    auto& view = *getView();
    auto& controls = *getViewControls();

    Activate();
    frame()->SetToolID( EditingModules() ? ID_MODEDIT_MEASUREMENT_TOOL
                                         : ID_PCB_MEASUREMENT_TOOL,
                        wxCURSOR_PENCIL, _( "Measure distance" ) );

    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;

    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetSnapping( true );

    while( auto evt = Wait() )
    {
        const VECTOR2I cursorPos = controls.GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            break;
        }

        // click or drag starts
        else if( !originSet &&
                ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            if( !evt->IsDrag( BUT_LEFT ) )
            {
                twoPtMgr.SetOrigin( cursorPos );
                twoPtMgr.SetEnd( cursorPos );
            }

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }

        else if( !originSet && evt->IsMotion() )
        {
            // make sure the origin is set before a drag starts
            // otherwise you can miss a step
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );
        }

        // second click or mouse up after drag ends
        else if( originSet &&
                ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );

            view.SetVisible( &ruler, false );
        }

        // move or drag when origin set updates rules
        else if( originSet &&
                ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            twoPtMgr.SetAngleSnap( evt->Modifier( MD_CTRL ) );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            GetManager()->PassEvent();
        }
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );

    frame()->SetNoToolSelected();

    return 0;
}


void EDIT_TOOL::SetTransitions()
{
    Go( &EDIT_TOOL::Main,       PCB_ACTIONS::editActivate.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,     PCB_ACTIONS::rotateCw.MakeEvent() );
    Go( &EDIT_TOOL::Rotate,     PCB_ACTIONS::rotateCcw.MakeEvent() );
    Go( &EDIT_TOOL::Flip,       PCB_ACTIONS::flip.MakeEvent() );
    Go( &EDIT_TOOL::Remove,     PCB_ACTIONS::remove.MakeEvent() );
    Go( &EDIT_TOOL::Remove,     PCB_ACTIONS::removeAlt.MakeEvent() );
    Go( &EDIT_TOOL::Properties, PCB_ACTIONS::properties.MakeEvent() );
    Go( &EDIT_TOOL::MoveExact,  PCB_ACTIONS::moveExact.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,  PCB_ACTIONS::duplicate.MakeEvent() );
    Go( &EDIT_TOOL::Duplicate,  PCB_ACTIONS::duplicateIncrement.MakeEvent() );
    Go( &EDIT_TOOL::CreateArray,PCB_ACTIONS::createArray.MakeEvent() );
    Go( &EDIT_TOOL::Mirror,     PCB_ACTIONS::mirror.MakeEvent() );
    Go( &EDIT_TOOL::editFootprintInFpEditor, PCB_ACTIONS::editFootprintInFpEditor.MakeEvent() );
    Go( &EDIT_TOOL::ExchangeFootprints,      PCB_ACTIONS::exchangeFootprints.MakeEvent() );
    Go( &EDIT_TOOL::MeasureTool,             PCB_ACTIONS::measureTool.MakeEvent() );
}


wxPoint EDIT_TOOL::getModificationPoint( const SELECTION& aSelection )
{
    if( aSelection.Size() == 1 )
    {
        return static_cast<BOARD_ITEM*>( aSelection.Front() )->GetPosition() - m_offset;
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

int EDIT_TOOL::editFootprintInFpEditor( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() )
        return 0;

    MODULE* mod = selection.FirstOfKind<MODULE>();

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

    if( selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    return 0;
}

template<class T>
T* EDIT_TOOL::uniqueSelected()
{
    const auto selection = m_selectionTool->GetSelection();

    if( selection.Size() != 1 )
        return nullptr;

    auto item = selection[0];
    return dyn_cast<T*>( item );
}
