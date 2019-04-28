/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <tools/sch_edit_tool.h>
#include <tools/sch_selection_tool.h>
#include <tools/sch_picker_tool.h>
#include <sch_actions.h>
#include <hotkeys.h>
#include <bitmaps.h>
#include <confirm.h>
#include <sch_item_struct.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_text.h>
#include <sch_bitmap.h>
#include <sch_view.h>
#include <sch_line.h>
#include <sch_item_struct.h>
#include <sch_edit_frame.h>
#include <list_operations.h>
#include <eeschema_id.h>
#include <status_popup.h>

TOOL_ACTION SCH_ACTIONS::move( "eeschema.InteractiveEdit.move",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE_COMPONENT_OR_ITEM ),
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::drag( "eeschema.InteractiveEdit.drag",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DRAG ),
        _( "Drag" ), _( "Drags the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::duplicate( "eeschema.InteractiveEdit.duplicate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ), duplicate_xpm );

TOOL_ACTION SCH_ACTIONS::repeatDrawItem( "eeschema.InteractiveEdit.repeatDrawItem",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_REPEAT_LAST ),
        _( "Repeat Last Item" ), _( "Duplicates the last drawn item" ) );

TOOL_ACTION SCH_ACTIONS::rotateCW( "eeschema.InteractiveEdit.rotateCW",
        AS_GLOBAL, 0,
        _( "Rotate Clockwise" ), _( "Rotates selected item(s) clockwise" ), rotate_cw_xpm );

TOOL_ACTION SCH_ACTIONS::rotateCCW( "eeschema.InteractiveEdit.rotateCCW",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ROTATE ),
        _( "Rotate" ), _( "Rotates selected item(s) counter-clockwise" ), rotate_ccw_xpm );

TOOL_ACTION SCH_ACTIONS::mirrorX( "eeschema.InteractiveEdit.mirrorX",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MIRROR_X ),
        _( "Mirror X" ), _( "Mirrors selected item(s) across the X axis" ), mirror_h_xpm );

TOOL_ACTION SCH_ACTIONS::mirrorY( "eeschema.InteractiveEdit.mirrorY",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MIRROR_Y ),
        _( "Mirror Y" ), _( "Mirrors selected item(s) across the Y axis" ), mirror_v_xpm );

TOOL_ACTION SCH_ACTIONS::properties( "eeschema.InteractiveEdit.properties",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT ),
        _( "Properties..." ), _( "Displays item properties dialog" ), config_xpm );

TOOL_ACTION SCH_ACTIONS::editReference( "eeschema.InteractiveEdit.editReference",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COMPONENT_REFERENCE ),
        _( "Edit Reference..." ), _( "Displays reference field dialog" ), config_xpm );

TOOL_ACTION SCH_ACTIONS::editValue( "eeschema.InteractiveEdit.editValue",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COMPONENT_VALUE ),
        _( "Edit Value..." ), _( "Displays value field dialog" ), config_xpm );

TOOL_ACTION SCH_ACTIONS::editFootprint( "eeschema.InteractiveEdit.editFootprint",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT_COMPONENT_FOOTPRINT ),
        _( "Edit Footprint..." ), _( "Displays footprint field dialog" ), config_xpm );

TOOL_ACTION SCH_ACTIONS::doDelete( "eeschema.InteractiveEdit.doDelete",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DELETE ),
        _( "Delete" ), _( "Deletes selected item(s)" ), delete_xpm );

TOOL_ACTION SCH_ACTIONS::deleteItemCursor( "eeschema.InteractiveEdit.deleteItemCursor",
        AS_GLOBAL, 0,
        _( "DoDelete Items" ), _( "DoDelete clicked items" ), NULL, AF_ACTIVATE );



SCH_EDIT_TOOL::SCH_EDIT_TOOL() :
        TOOL_INTERACTIVE( "eeschema.InteractiveEdit" ),
        m_selectionTool( nullptr ),
        m_view( nullptr ),
        m_controls( nullptr ),
        m_frame( nullptr ),
        m_menu( *this ),
        m_moveInProgress( false )
{
}


SCH_EDIT_TOOL::~SCH_EDIT_TOOL()
{
}


bool SCH_EDIT_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    if( !m_selectionTool )
    {
        DisplayError( NULL, _( "eeshema.InteractiveSelection tool is not available" ) );
        return false;
    }

    auto activeToolFunctor = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() != ID_NO_TOOL_SELECTED );
    };

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolFunctor, 1 );
    ctxMenu.AddSeparator( activeToolFunctor, 1 );

    return true;
}


void SCH_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    m_moveInProgress = false;

    // Init variables used by every drawing tool
    m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    m_controls = getViewControls();
    m_frame = getEditFrame<SCH_EDIT_FRAME>();
}


int SCH_EDIT_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->SetSnapping( true );
    VECTOR2I originalCursorPos = controls->GetCursorPosition();

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. Kicad old-style hover selection)
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::MovableItems );
    bool unselect = selection.IsHover();

    if( selection.Empty() )
        return 0;

    Activate();
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool restore_state = false;
    bool isDragOperation = aEvent.IsAction( &SCH_ACTIONS::drag );
    VECTOR2I totalMovement;
    OPT_TOOL_EVENT evt = aEvent;
    VECTOR2I prevPos;

    // Main loop: keep receiving events
    do
    {
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

        if( evt->IsAction( &SCH_ACTIONS::move ) || evt->IsAction( &SCH_ACTIONS::drag )
                || evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            if( !m_moveInProgress )    // Prepare to start moving/dragging
            {
                //
                // Save items, so changes can be undone
                //
                for( int i = 0; i < selection.GetSize(); ++i )
                {
                    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                    item->ClearFlags( STARTPOINT | ENDPOINT );

                    // No need to save children of selected items
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    if( !item->IsNew() )
                        saveCopyInUndoList( item, UR_CHANGED, i > 0 );
                }

                //
                // Add connections to the selection for a drag; mark the edges of the block
                // with dangling pins for a move
                //
                if( isDragOperation )
                {
                    for( unsigned i = 0; i < selection.GetSize(); ++i )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                        if( item->IsConnectable() )
                        {
                            std::vector<wxPoint> connections;
                            item->GetConnectionPoints( connections );

                            for( wxPoint point : connections )
                                selectConnectedDragItems( item, point );
                        }
                    }
                }
                else
                {
                    std::vector<DANGLING_END_ITEM> internalPoints;

                    for( int i = 0; i < selection.GetSize(); ++i )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );
                        item->GetEndPoints( internalPoints );
                    }

                    for( int i = 0; i < selection.GetSize(); ++i )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );
                        item->UpdateDanglingState( internalPoints );
                    }
                }

                //
                // Set up the starting position and move/drag offset
                //
                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    VECTOR2I delta = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( int i = 0; i < selection.GetSize(); ++i )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                        // Don't double move pins, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        moveItem( item, delta, isDragOperation );
                        updateView( item );
                    }

                    selection.SetReferencePoint( m_cursor );
                }
                else if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin,
                    // so the movement vector can be computed later
                    updateModificationPoint( selection );
                    m_cursor = originalCursorPos;
                }
                else
                {
                    updateModificationPoint( selection );
                }

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_moveInProgress = true;
            }

            else /* m_moveInProgress */
            {
                //
                // Follow the mouse
                //
                m_cursor = controls->GetCursorPosition();
                VECTOR2I movement( m_cursor - prevPos );
                selection.SetReferencePoint( m_cursor );

                totalMovement += movement;
                prevPos = m_cursor;

                for( int i = 0; i < selection.GetSize(); ++i )
                {
                    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                    // Don't double move pins, fields, etc.
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    moveItem( item, movement, isDragOperation );
                    updateView( item );
                }

                m_frame->UpdateMsgPanel();
            }
        }

        else if( evt->IsAction( &ACTIONS::cancelInteractive ) || evt->IsCancel() || evt->IsActivate() )
        {
            if( m_moveInProgress )
                restore_state = true;

            break;
        }

        else if( evt->Action() == TA_UNDO_REDO_PRE )
        {
            unselect = true;
            break;
        }

        // Dispatch TOOL_ACTIONs
        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &SCH_ACTIONS::doDelete ) )
            {
                // Exit on a remove operation; there is no further processing for removed items.
                break;
            }
            else if( evt->IsAction( &SCH_ACTIONS::duplicate ) )
            {
                // Exit on a duplicate action; it will start its own move operation.
                break;
            }
        }

        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) )
        {
            break; // Finish
        }

    } while( ( evt = Wait() ) ); //Should be assignment not equality test

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );

    m_moveInProgress = false;

    selection.ClearReferencePoint();

    for( auto item : selection )
        item->ClearFlags( IS_MOVED );

    if( restore_state )
    {
        m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
        m_frame->RollbackSchematicFromUndo();
        return 0;
    }

    m_frame->CheckConnections( selection, true );
    m_frame->SchematicCleanUp( true );
    m_frame->TestDanglingEnds();
    m_frame->OnModify();

    if( unselect )
        m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    return 0;
}


void SCH_EDIT_TOOL::selectConnectedDragItems( SCH_ITEM* aSourceItem, wxPoint aPoint )
{
    for( SCH_ITEM* item = m_frame->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->IsSelected() || !item->IsConnectable() || !item->CanConnect( aSourceItem ) )
            continue;

        bool doSelect = false;

        switch( item->Type() )
        {
        default:
        case SCH_LINE_T:
        {
            // Select wires/busses that are connected at one end and/or the other.  Any
            // unconnected ends must be flagged (STARTPOINT or ENDPOINT).
            SCH_LINE* line = (SCH_LINE*) item;

            if( !line->IsSelected() )
                line->SetFlags( STARTPOINT | ENDPOINT );

            if( line->GetStartPoint() == aPoint )
            {
                line->ClearFlags( STARTPOINT );

                if( !line->IsSelected() )
                    doSelect = true;
            }
            else if( line->GetEndPoint() == aPoint )
            {
                line->ClearFlags( ENDPOINT );

                if( !line->IsSelected() )
                    doSelect = true;
            }
            break;
        }

        case SCH_SHEET_T:
            // Dragging a sheet just because it's connected to something else feels a bit like
            // the tail wagging the dog, but this could be moved down to the next case.
            break;

        case SCH_COMPONENT_T:
        case SCH_NO_CONNECT_T:
        case SCH_JUNCTION_T:
            // Select connected items that have no wire between them.
            if( aSourceItem->Type() != SCH_LINE_T && item->IsConnected( aPoint ) )
                doSelect = true;

            break;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_BUS_BUS_ENTRY_T:
            // Select labels and bus entries that are connected to a wire being moved.
            if( aSourceItem->Type() == SCH_LINE_T )
            {
                std::vector<wxPoint> connections;
                item->GetConnectionPoints( connections );

                for( wxPoint& point : connections )
                {
                    if( aSourceItem->HitTest( point ) )
                        doSelect = true;
                }
            }
            break;
        }

        if( doSelect )
        {
            m_toolMgr->RunAction( SCH_ACTIONS::selectItem, true, item );
            saveCopyInUndoList( item, UR_CHANGED, true );
        }
    }
}


void SCH_EDIT_TOOL::moveItem( SCH_ITEM* aItem, VECTOR2I aDelta, bool isDrag )
{
    switch( aItem->Type() )
    {
    case SCH_LINE_T:
    {
        SCH_LINE* line = (SCH_LINE*) aItem;

        if( isDrag && ( line->GetFlags() & STARTPOINT ) )
            line->MoveEnd( (wxPoint)aDelta );
        else if( isDrag && ( line->GetFlags() & ENDPOINT ) )
            line->MoveStart( (wxPoint)aDelta );
        else
            line->Move( (wxPoint)aDelta );
        break;
    }

    case SCH_PIN_T:
    case SCH_FIELD_T:
        aItem->Move( wxPoint( aDelta.x, -aDelta.y ) );
        break;

    default:
        aItem->Move( (wxPoint)aDelta );
        break;
    }

    aItem->SetFlags( IS_MOVED );
}


bool SCH_EDIT_TOOL::updateModificationPoint( SELECTION& aSelection )
{
    if( m_moveInProgress && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        SCH_ITEM* item =  static_cast<SCH_ITEM*>( aSelection.Front() );

        // For some items, moving the cursor to anchor is not good (for instance large
        // hierarchical sheets or components can have the anchor outside the view)
        if( item->IsMovableFromAnchorPoint() )
        {
            wxPoint pos = item->GetPosition();
            aSelection.SetReferencePoint( pos );

            return true;
        }
    }

    // ...otherwise modify items with regard to the grid-snapped cursor position
    m_cursor = getViewControls()->GetCursorPosition( true );
    aSelection.SetReferencePoint( m_cursor );

    return true;
}


int SCH_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SELECTION           selection = selTool->RequestSelection( SCH_COLLECTOR::RotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   rotPoint;
    bool      clockwise = ( aEvent.Matches( SCH_ACTIONS::rotateCW.MakeEvent() ) );
    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( 0 ) );
    bool      connections = false;
    bool      moving = item->IsMoving();

    if( selection.GetSize() == 1 )
    {
        if( !moving )
            saveCopyInUndoList( item, UR_CHANGED );

        switch( item->Type() )
        {
        case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

            if( clockwise )
                component->SetOrientation( CMP_ROTATE_CLOCKWISE );
            else
                component->SetOrientation( CMP_ROTATE_COUNTERCLOCKWISE );

            if( m_frame->GetAutoplaceFields() )
                component->AutoAutoplaceFields( m_frame->GetScreen() );

            break;
        }

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_TEXT* textItem = static_cast<SCH_TEXT*>( item );
            textItem->SetLabelSpinStyle( ( textItem->GetLabelSpinStyle() + 1 ) & 3 );
            break;
        }

        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T:
            item->Rotate( item->GetPosition() );
            break;

        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

            if( field->GetTextAngle() == TEXT_ANGLE_HORIZ )
                field->SetTextAngle( TEXT_ANGLE_VERT );
            else
                field->SetTextAngle( TEXT_ANGLE_HORIZ );

            // Now that we're moving a field, they're no longer autoplaced.
            if( item->GetParent()->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT *parent = static_cast<SCH_COMPONENT*>( item->GetParent() );
                parent->ClearFieldsAutoplaced();
            }

            break;
        }

        case SCH_BITMAP_T:
            item->Rotate( item->GetPosition() );

            // The bitmap is cached in Opengl: clear the cache to redraw
            getView()->RecacheAllItems();
            break;

        case SCH_SHEET_T:
            // Rotate the sheet on itself. Sheets do not have a anchor point.
            rotPoint = m_frame->GetNearestGridPosition( item->GetBoundingBox().Centre() );

            if( clockwise )
            {
                item->Rotate( rotPoint );
            }
            else
            {
                item->Rotate( rotPoint );
                item->Rotate( rotPoint );
                item->Rotate( rotPoint );
            }

            break;

        default:
            break;
        }

        connections = item->IsConnectable();
        m_frame->RefreshItem( item );
    }
    else if( selection.GetSize() > 1 )
    {
        rotPoint = m_frame->GetNearestGridPosition( (wxPoint)selection.GetCenter() );

        for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
        {
            item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

            if( !moving )
                saveCopyInUndoList( item, UR_CHANGED, ii > 0 );

            item->Rotate( rotPoint );

            connections |= item->IsConnectable();
            m_frame->RefreshItem( item );
        }
    }

    if( !item->IsMoving() )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

        if( connections )
            m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    return 0;
}


int SCH_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SELECTION           selection = selTool->RequestSelection( SCH_COLLECTOR::RotatableItems );

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   mirrorPoint;
    bool      xAxis = ( aEvent.Matches( SCH_ACTIONS::mirrorX.MakeEvent() ) );
    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( 0 ) );
    bool      connections = false;
    bool      moving = item->IsMoving();

    if( selection.GetSize() == 1 )
    {
        if( !moving )
            saveCopyInUndoList( item, UR_CHANGED );

        switch( item->Type() )
        {
        case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

            if( xAxis )
                component->SetOrientation( CMP_MIRROR_X );
            else
                component->SetOrientation( CMP_MIRROR_Y );

            if( m_frame->GetAutoplaceFields() )
                component->AutoAutoplaceFields( m_frame->GetScreen() );

            break;
        }

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        {
            SCH_TEXT* textItem = static_cast<SCH_TEXT*>( item );
            int       spin = textItem->GetLabelSpinStyle();

            if( xAxis && spin % 2 )
                textItem->SetLabelSpinStyle( ( spin + 2 ) % 4 );
            else if ( !xAxis && !( spin % 2 ) )
                textItem->SetLabelSpinStyle( ( spin + 2 ) % 4 );
            break;
        }

        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T:
            if( xAxis )
                item->MirrorX( item->GetPosition().y );
            else
                item->MirrorY( item->GetPosition().x );
            break;

        case SCH_FIELD_T:
        {
            SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

            if( xAxis )
                field->SetVertJustify( (EDA_TEXT_VJUSTIFY_T)-field->GetVertJustify() );
            else
                field->SetHorizJustify( (EDA_TEXT_HJUSTIFY_T)-field->GetHorizJustify() );

            // Now that we're re-justifying a field, they're no longer autoplaced.
            if( item->GetParent()->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT *parent = static_cast<SCH_COMPONENT*>( item->GetParent() );
                parent->ClearFieldsAutoplaced();
            }

            break;
        }

        case SCH_BITMAP_T:
            if( xAxis )
                item->MirrorX( item->GetPosition().y );
            else
                item->MirrorY( item->GetPosition().x );

            // The bitmap is cached in Opengl: clear the cache to redraw
            getView()->RecacheAllItems();
            break;

        case SCH_SHEET_T:
            // Mirror the sheet on itself. Sheets do not have a anchor point.
            mirrorPoint = m_frame->GetNearestGridPosition( item->GetBoundingBox().Centre() );

            if( xAxis )
                item->MirrorX( mirrorPoint.y );
            else
                item->MirrorY( mirrorPoint.x );

            break;

        default:
            break;
        }

        connections = item->IsConnectable();
        m_frame->RefreshItem( item );
    }
    else if( selection.GetSize() > 1 )
    {
        mirrorPoint = m_frame->GetNearestGridPosition( (wxPoint)selection.GetCenter() );

        for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
        {
            item = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );

            if( !moving )
                saveCopyInUndoList( item, UR_CHANGED, ii > 0 );

            if( xAxis )
                item->MirrorX( mirrorPoint.y );
            else
                item->MirrorY( mirrorPoint.x );

            connections |= item->IsConnectable();
            m_frame->RefreshItem( item );
        }
    }

    if( !item->IsMoving() )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

        if( connections )
            m_frame->TestDanglingEnds();

        m_frame->OnModify();
    }

    return 0;
}


int SCH_EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SELECTION           selection = selTool->RequestSelection( SCH_COLLECTOR::DraggableItems );

    if( selection.GetSize() == 0 )
        return 0;

    std::vector<SCH_ITEM*> newItems;

    // Keep track of existing sheet paths. Duplicating a selection can modify this list
    bool hasSheetCopied = false;
    SCH_SHEET_LIST initial_sheetpathList( g_RootSheet );

    for( unsigned ii = 0; ii < selection.GetSize(); ++ii )
    {
        SCH_ITEM* oldItem = static_cast<SCH_ITEM*>( selection.GetItem( ii ) );
        SCH_ITEM* newItem = DuplicateStruct( oldItem );
        newItems.push_back( newItem );

        saveCopyInUndoList( newItem, UR_NEW, ii > 0 );

        switch( newItem->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
        case SCH_BUS_BUS_ENTRY_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_MARKER_T:
        case SCH_NO_CONNECT_T:
            newItem->SetParent( m_frame->GetScreen() );
            m_frame->AddToScreen( newItem );
            break;

        case SCH_SHEET_T:
        {
            SCH_SHEET* sheet = (SCH_SHEET*) newItem;
            // Duplicate sheet names and sheet time stamps are not valid.  Use a time stamp
            // based sheet name and update the time stamp for each sheet in the block.
            timestamp_t timeStamp = GetNewTimeStamp();

            sheet->SetName( wxString::Format( wxT( "sheet%8.8lX" ), (unsigned long)timeStamp ) );
            sheet->SetTimeStamp( timeStamp );

            sheet->SetParent( m_frame->GetScreen() );
            m_frame->AddToScreen( sheet );

            hasSheetCopied = true;
            break;
        }

        case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) newItem;

            component->SetTimeStamp( GetNewTimeStamp() );
            component->ClearAnnotation( NULL );

            component->SetParent( m_frame->GetScreen() );
            m_frame->AddToScreen( component );
            break;
        }

        default:
            break;
        }
    }

    if( hasSheetCopied )
    {
        // We clear annotation of new sheet paths.
        // Annotation of new components added in current sheet is already cleared.
        SCH_SCREENS screensList( g_RootSheet );
        screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
    }

    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );
    m_toolMgr->RunAction( SCH_ACTIONS::selectItems, true, &newItems );

    TOOL_EVENT evt = SCH_ACTIONS::move.MakeEvent();
    Main( evt );

    return 0;
}


int SCH_EDIT_TOOL::RepeatDrawItem( const TOOL_EVENT& aEvent )
{
    SCH_ITEM* sourceItem = m_frame->GetRepeatItem();

    if( !sourceItem )
        return 0;

    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    SCH_ITEM* newItem = (SCH_ITEM*) sourceItem->Clone();
    bool      performDrag = false;

    // If cloning a component then put into 'move' mode.
    if( newItem->Type() == SCH_COMPONENT_T )
    {
        ( (SCH_COMPONENT*) newItem )->SetTimeStamp( GetNewTimeStamp() );

        newItem->Move( (wxPoint)m_controls->GetCursorPosition( true ) - newItem->GetPosition() );
        performDrag = true;
    }
    else
    {
        if( newItem->CanIncrementLabel() )
            ( (SCH_TEXT*) newItem )->IncrementLabel( m_frame->GetRepeatDeltaLabel() );

        newItem->Move( m_frame->GetRepeatStep() );
    }

    newItem->SetFlags( IS_NEW );
    m_frame->AddToScreen( newItem );
    m_frame->SaveCopyInUndoList( newItem, UR_NEW );

    m_toolMgr->RunAction( SCH_ACTIONS::selectItem, true, newItem );

    if( performDrag )
    {
        TOOL_EVENT evt = SCH_ACTIONS::move.MakeEvent();
        Main( evt );
    }

    newItem->ClearFlags();

    if( newItem->IsConnectable() )
        m_frame->TestDanglingEnds();

    // newItem newItem, now that it has been moved, thus saving new position.
    m_frame->SetRepeatItem( newItem );

    return 0;
}


int SCH_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL*    selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    std::vector<SCH_ITEM*> items;

    // get a copy instead of reference (we're going to clear the selection before removing items)
    SELECTION selectionCopy = selTool->RequestSelection();

    if( selectionCopy.Empty() )
        return 0;

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    for( unsigned ii = 0; ii < selectionCopy.GetSize(); ii++ )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selectionCopy.GetItem( ii ) );

        // Junctions, in particular, may have already been deleted if deleting wires made
        // them redundant
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        m_frame->DeleteItem( item, ii > 0 );
    }

    m_frame->GetScreen()->SetCurItem( nullptr );
    m_frame->SetRepeatItem( nullptr );
    m_frame->TestDanglingEnds();

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


static bool deleteItem( SCH_EDIT_FRAME* aFrame, const VECTOR2D& aPosition )
{
    SCH_SELECTION_TOOL* selectionTool = aFrame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    wxCHECK( selectionTool, false );

    aFrame->GetToolManager()->RunAction( SCH_ACTIONS::selectionClear, true );

    SCH_ITEM* item = selectionTool->SelectPoint( aPosition );

    if( item )
    {
        if( item->IsLocked() )
        {
            STATUS_TEXT_POPUP statusPopup( aFrame );
            statusPopup.SetText( _( "Item locked." ) );
            statusPopup.Expire( 2000 );
            statusPopup.Popup();
            statusPopup.Move( wxGetMousePosition() + wxPoint( 20, 20 ) );
        }
        else
        {
            aFrame->GetToolManager()->RunAction( SCH_ACTIONS::doDelete, true );
        }
    }

    return true;
}


int SCH_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    Activate();

    SCH_PICKER_TOOL* picker = m_toolMgr->GetTool<SCH_PICKER_TOOL>();
    wxCHECK( picker, 0 );

    m_frame->SetToolID( ID_SCHEMATIC_DELETE_ITEM_BUTT, wxCURSOR_BULLSEYE, _( "DoDelete item" ) );
    picker->SetClickHandler( std::bind( deleteItem, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


int SCH_EDIT_TOOL::EditField( const TOOL_EVENT& aEvent )
{
    static KICAD_T Nothing[]        = { EOT };
    static KICAD_T CmpOrReference[] = { SCH_FIELD_LOCATE_REFERENCE_T, SCH_COMPONENT_T, EOT };
    static KICAD_T CmpOrValue[]     = { SCH_FIELD_LOCATE_VALUE_T,     SCH_COMPONENT_T, EOT };
    static KICAD_T CmpOrFootprint[] = { SCH_FIELD_LOCATE_FOOTPRINT_T, SCH_COMPONENT_T, EOT };

    KICAD_T* filter = Nothing;

    if( aEvent.IsAction( &SCH_ACTIONS::editReference ) )
        filter = CmpOrReference;
    else if( aEvent.IsAction( &SCH_ACTIONS::editValue ) )
        filter = CmpOrValue;
    else if( aEvent.IsAction( &SCH_ACTIONS::editFootprint ) )
        filter = CmpOrFootprint;

    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SELECTION&          selection = selTool->RequestSelection( filter );
    SCH_ITEM*           item = nullptr;

    if( selection.GetSize() >= 1 )
        item = (SCH_ITEM*)selection.GetItem( 0 );

    if( !item )
        return 0;

    if( item->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) item;

        if( aEvent.IsAction( &SCH_ACTIONS::editReference ) )
            m_frame->EditComponentFieldText( component->GetField( REFERENCE ) );
        else if( aEvent.IsAction( &SCH_ACTIONS::editValue ) )
            m_frame->EditComponentFieldText( component->GetField( VALUE ) );
        else if( aEvent.IsAction( &SCH_ACTIONS::editFootprint ) )
            m_frame->EditComponentFieldText( component->GetField( FOOTPRINT ) );
    }
    else if( item->Type() == SCH_FIELD_T )
    {
        m_frame->EditComponentFieldText( (SCH_FIELD*) item );
    }

    return 0;
}


int SCH_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SELECTION&          selection = selTool->RequestSelection( SCH_COLLECTOR::EditableItems );
    SCH_ITEM*           item = nullptr;

    if( selection.GetSize() >= 1 )
        item = (SCH_ITEM*)selection.GetItem( 0 );

    if( !item )
        return 0;

    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
        m_frame->EditComponent( (SCH_COMPONENT*) item );
        break;

    case SCH_SHEET_T:
    {
        bool doClearAnnotation;
        bool doRefresh = false;
        // Keep track of existing sheet paths. EditSheet() can modify this list
        SCH_SHEET_LIST initial_sheetpathList( g_RootSheet );

        doRefresh = m_frame->EditSheet( (SCH_SHEET*) item, g_CurrentSheet, &doClearAnnotation );

        if( doClearAnnotation )     // happens when the current sheet load a existing file
        {                           // we must clear "new" components annotation
            SCH_SCREENS screensList( g_RootSheet );
            // We clear annotation of new sheet paths here:
            screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
            // Clear annotation of g_CurrentSheet itself, because its sheetpath
            // is not a new path, but components managed by its sheet path must have
            // their annotation cleared, becuase they are new:
            ((SCH_SHEET*) item)->GetScreen()->ClearAnnotation( g_CurrentSheet );
        }

        if( doRefresh )
            m_frame->GetCanvas()->Refresh();

        break;
    }

    case SCH_SHEET_PIN_T:
        m_frame->EditSheetPin( (SCH_SHEET_PIN*) item, true );
        break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
        m_frame->EditSchematicText( (SCH_TEXT*) item );
        break;

    case SCH_FIELD_T:
        m_frame->EditComponentFieldText( (SCH_FIELD*) item );
        break;

    case SCH_BITMAP_T:
        if( m_frame->EditImage( (SCH_BITMAP*) item ) )
        {
            // The bitmap is cached in Opengl: clear the cache in case it has become invalid
            getView()->RecacheAllItems();
        }

        break;

    case SCH_LINE_T:
        m_frame->EditLine( (SCH_LINE*) item, true );
        break;

    case SCH_MARKER_T:        // These items have no properties to edit
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
        break;

    default:                // Unexpected item
        wxFAIL_MSG( wxString( "Cannot edit schematic item type " ) + item->GetClass() );
    }

    updateView( item );

    return 0;
}


void SCH_EDIT_TOOL::updateView( EDA_ITEM* aItem )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent() );

    getView()->Update( aItem );
}


void SCH_EDIT_TOOL::saveCopyInUndoList( SCH_ITEM* aItem, UNDO_REDO_T aType, bool aAppend )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        m_frame->SaveCopyInUndoList( (SCH_ITEM*)aItem->GetParent(), aType, aAppend );
    else
        m_frame->SaveCopyInUndoList( aItem, aType, aAppend );
}


void SCH_EDIT_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TOOL::Main,               SCH_ACTIONS::move.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Main,               SCH_ACTIONS::drag.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Duplicate,          SCH_ACTIONS::duplicate.MakeEvent() );
    Go( &SCH_EDIT_TOOL::RepeatDrawItem,     SCH_ACTIONS::repeatDrawItem.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorX.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorY.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DoDelete,             SCH_ACTIONS::doDelete.MakeEvent() );
    Go( &SCH_EDIT_TOOL::DeleteItemCursor,   SCH_ACTIONS::deleteItemCursor.MakeEvent() );

    Go( &SCH_EDIT_TOOL::Properties,         SCH_ACTIONS::properties.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editReference.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editValue.MakeEvent() );
    Go( &SCH_EDIT_TOOL::EditField,          SCH_ACTIONS::editFootprint.MakeEvent() );

}
