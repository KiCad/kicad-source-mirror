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
#include <sch_item_struct.h>
#include <sch_edit_frame.h>
#include <list_operations.h>

TOOL_ACTION SCH_ACTIONS::move( "eeschema.InteractiveEdit.move",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE_COMPONENT_OR_ITEM ),
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::duplicate( "eeschema.InteractiveEdit.duplicate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ), duplicate_xpm );

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

TOOL_ACTION SCH_ACTIONS::remove( "eeschema.InteractiveEdit.remove",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DELETE ),
        _( "Delete" ), _( "Deletes selected item(s)" ), delete_xpm );


SCH_EDIT_TOOL::SCH_EDIT_TOOL() :
        TOOL_INTERACTIVE( "eeschema.InteractiveEdit" ),
        m_selectionTool( nullptr ),
        m_view( nullptr ),
        m_controls( nullptr ),
        m_frame( nullptr ),
        m_menu( *this ),
        m_dragging( false )
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
    m_dragging = false;

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
    SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::DraggableItems );
    bool unselect = selection.IsHover();

    if( m_dragging || selection.Empty() )
        return 0;

    Activate();
    controls->ShowCursor( true );
    controls->SetAutoPan( true );

    bool restore_state = false;
    VECTOR2I totalMovement;
    OPT_TOOL_EVENT evt = aEvent;
    VECTOR2I prevPos;

    // Main loop: keep receiving events
    do
    {
        controls->SetSnapping( !evt->Modifier( MD_ALT ) );

        if( evt->IsAction( &SCH_ACTIONS::move ) || evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
        {
            if( m_dragging && evt->Category() == TC_MOUSE )
            {
                m_cursor = controls->GetCursorPosition();
                VECTOR2I movement( m_cursor - prevPos );
                selection.SetReferencePoint( m_cursor );

                totalMovement += movement;
                prevPos = m_cursor;

                // Drag items to the current cursor position
                for( int i = 0; i < selection.GetSize(); ++i )
                {
                    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                    // Don't double move footprint pads, fields, etc.
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    item->Move( (wxPoint)movement );
                    item->SetFlags( IS_MOVED );
                    updateView( item );
                }

                m_frame->UpdateMsgPanel();
            }
            else if( !m_dragging )    // Prepare to start dragging
            {
                // Save items, so changes can be undone
                for( int i = 0; i < selection.GetSize(); ++i )
                {
                    SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                    // Don't double move footprint pads, fields, etc.
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    m_frame->SaveCopyInUndoList( item, UR_CHANGED, i > 0 );
                }

                // Mark dangling pins at the edges of the block:
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

                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    // start moving with the reference point attached to the cursor
                    VECTOR2I delta = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( int i = 0; i < selection.GetSize(); ++i )
                    {
                        SCH_ITEM* item = static_cast<SCH_ITEM*>( selection.GetItem( i ) );

                        // Don't double move footprint pads, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        item->Move( (wxPoint)delta );
                    }

                    selection.SetReferencePoint( m_cursor );
                }
                else if( selection.Size() == 1 )
                {
                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
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
                m_dragging = true;
            }
        }

        else if( evt->IsAction( &ACTIONS::cancelInteractive ) || evt->IsCancel() || evt->IsActivate() )
        {
            if( m_dragging )
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
            if( evt->IsAction( &SCH_ACTIONS::remove ) )
            {
                // exit the loop, as there is no further processing for removed items
                break;
            }
            else if( evt->IsAction( &SCH_ACTIONS::duplicate ) )
            {
                // On duplicate, stop moving this item
                // The duplicate tool should then select the new item and start
                // a new move procedure
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

    m_dragging = false;

    // Discard reference point when selection is "dropped" onto the board (ie: not dragging anymore)
    selection.ClearReferencePoint();

    for( auto item : selection )
        item->ClearFlags( IS_MOVED );

    if( unselect || restore_state )
        m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    if( restore_state )
        m_frame->RollbackSchematicFromUndo();
    else
    {
        m_frame->TestDanglingEnds();
        m_frame->OnModify();
    }

    return 0;
}


bool SCH_EDIT_TOOL::updateModificationPoint( SELECTION& aSelection )
{
    if( m_dragging && aSelection.HasReferencePoint() )
        return false;

    // When there is only one item selected, the reference point is its position...
    if( aSelection.Size() == 1 )
    {
        SCH_ITEM* item =  static_cast<SCH_ITEM*>( aSelection.Front() );
        wxPoint pos = item->GetPosition();
        aSelection.SetReferencePoint( pos );
    }
        // ...otherwise modify items with regard to the grid-snapped cursor position
    else
    {
        m_cursor = getViewControls()->GetCursorPosition( true );
        aSelection.SetReferencePoint( m_cursor );
    }

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
            m_frame->SaveCopyInUndoList( item, UR_CHANGED );

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
                m_frame->SaveCopyInUndoList( item, UR_CHANGED, ii > 0 );

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
            m_frame->SaveCopyInUndoList( item, UR_CHANGED );

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
                m_frame->SaveCopyInUndoList( item, UR_CHANGED, ii > 0 );

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

        m_frame->SaveCopyInUndoList( newItem, UR_NEW, ii > 0 );

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


int SCH_EDIT_TOOL::Remove( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION_TOOL*    selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    std::vector<SCH_ITEM*> lockedItems;

    // get a copy instead of reference (we're going to clear the selection before removing items)
    SELECTION selectionCopy = selTool->RequestSelection();

    if( selectionCopy.Empty() )
        return 0;

    // As we are about to remove items, they have to be removed from the selection first
    m_toolMgr->RunAction( SCH_ACTIONS::selectionClear, true );

    for( unsigned ii = 0; ii < selectionCopy.GetSize(); ii++ )
    {
        SCH_ITEM* item = static_cast<SCH_ITEM*>( selectionCopy.GetItem( ii ) );
        bool      itemHasConnections = item->IsConnectable();

        m_frame->GetScreen()->SetCurItem( nullptr );
        m_frame->SetRepeatItem( nullptr );
        m_frame->DeleteItem( item, ii > 0 );

        if( itemHasConnections )
            m_frame->TestDanglingEnds();
    }

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return 0;
}


void SCH_EDIT_TOOL::updateView( EDA_ITEM* aItem )
{
    KICAD_T itemType = aItem->Type();

    if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
        getView()->Update( aItem->GetParent() );
    else
        getView()->Update( aItem );
}


void SCH_EDIT_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TOOL::Main,               SCH_ACTIONS::move.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Duplicate,          SCH_ACTIONS::duplicate.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Rotate,             SCH_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorX.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Mirror,             SCH_ACTIONS::mirrorY.MakeEvent() );
    Go( &SCH_EDIT_TOOL::Remove,             SCH_ACTIONS::remove.MakeEvent() );
}
