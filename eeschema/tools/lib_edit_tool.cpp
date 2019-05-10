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
#include <tools/ee_selection_tool.h>
#include <tools/ee_picker_tool.h>
#include <tools/lib_pin_tool.h>
#include <tools/lib_drawing_tools.h>
#include <tools/lib_move_tool.h>
#include <ee_actions.h>
#include <ee_hotkeys.h>
#include <bitmaps.h>
#include <confirm.h>
#include <base_struct.h>
#include <sch_view.h>
#include <lib_edit_frame.h>
#include <eeschema_id.h>
#include <dialogs/dialog_lib_edit_draw_item.h>
#include <dialogs/dialog_lib_edit_text.h>
#include <dialogs/dialog_edit_one_field.h>
#include <sch_legacy_plugin.h>
#include "lib_edit_tool.h"

LIB_EDIT_TOOL::LIB_EDIT_TOOL() :
        TOOL_INTERACTIVE( "libedit.InteractiveEdit" ),
        m_selectionTool( nullptr ),
        m_frame( nullptr ),
        m_menu( *this )
{
}


LIB_EDIT_TOOL::~LIB_EDIT_TOOL()
{
}


bool LIB_EDIT_TOOL::Init()
{
    m_frame = getEditFrame<LIB_EDIT_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    LIB_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<LIB_DRAWING_TOOLS>();
    LIB_MOVE_TOOL* moveTool = m_toolMgr->GetTool<LIB_MOVE_TOOL>();

    wxASSERT_MSG( m_selectionTool, "eeshema.InteractiveSelection tool is not available" );
    wxASSERT_MSG( drawingTools, "libedit.InteractiveDrawing tool is not available" );

    //
    // Add edit actions to the move tool menu
    //
    if( moveTool )
    {
        CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

        moveMenu.AddSeparator( SELECTION_CONDITIONS::NotEmpty );
        moveMenu.AddItem( EE_ACTIONS::rotateCCW,       EE_CONDITIONS::NotEmpty );
        moveMenu.AddItem( EE_ACTIONS::rotateCW,        EE_CONDITIONS::NotEmpty );
        moveMenu.AddItem( EE_ACTIONS::mirrorX,         EE_CONDITIONS::NotEmpty );
        moveMenu.AddItem( EE_ACTIONS::mirrorY,         EE_CONDITIONS::NotEmpty );
        moveMenu.AddItem( EE_ACTIONS::duplicate,       EE_CONDITIONS::NotEmpty );
        moveMenu.AddItem( EE_ACTIONS::doDelete,        EE_CONDITIONS::NotEmpty );

        moveMenu.AddItem( EE_ACTIONS::properties,      EE_CONDITIONS::Count( 1 ) );

        moveMenu.AddSeparator( EE_CONDITIONS::IdleSelection );
        moveMenu.AddItem( EE_ACTIONS::cut,             EE_CONDITIONS::IdleSelection );
        moveMenu.AddItem( EE_ACTIONS::copy,            EE_CONDITIONS::IdleSelection );
    }

    //
    // Add editing actions to the drawing tool menu
    //
    CONDITIONAL_MENU& drawMenu = drawingTools->GetToolMenu().GetMenu();

    drawMenu.AddSeparator( EE_CONDITIONS::NotEmpty, 200 );
    drawMenu.AddItem( EE_ACTIONS::rotateCCW,       EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::rotateCW,        EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorX,         EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorY,         EE_CONDITIONS::IdleSelection, 200 );

    drawMenu.AddItem( EE_ACTIONS::properties,      EE_CONDITIONS::Count( 1 ), 200 );

    //
    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::rotateCCW,        EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::rotateCW,         EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorX,          EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorY,          EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::duplicate,        EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::doDelete,         EE_CONDITIONS::NotEmpty, 200 );

    selToolMenu.AddItem( EE_ACTIONS::properties,       EE_CONDITIONS::Count( 1 ), 200 );

    selToolMenu.AddSeparator( EE_CONDITIONS::Idle, 200 );
    selToolMenu.AddItem( EE_ACTIONS::cut,              EE_CONDITIONS::IdleSelection, 200 );
    selToolMenu.AddItem( EE_ACTIONS::copy,             EE_CONDITIONS::IdleSelection, 200 );
    selToolMenu.AddItem( EE_ACTIONS::paste,            EE_CONDITIONS::Idle, 200 );

    return true;
}


void LIB_EDIT_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
    {
        // Init variables used by every drawing tool
        m_frame = getEditFrame<LIB_EDIT_FRAME>();
    }
}


int LIB_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   rotPoint;
    bool      ccw = ( aEvent.Matches( EE_ACTIONS::rotateCCW.MakeEvent() ) );
    LIB_ITEM* item = static_cast<LIB_ITEM*>( selection.Front() );

    if( !item->IsMoving() )
        m_frame->SaveCopyInUndoList( m_frame->GetCurPart() );

    if( selection.GetSize() == 1 )
        rotPoint = item->GetPosition();
    else
        rotPoint = m_frame->GetNearestGridPosition( mapCoords( selection.GetCenter() ) );

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        item = static_cast<LIB_ITEM*>( selection.GetItem( ii ) );
        item->Rotate( rotPoint, ccw );
        m_frame->RefreshItem( item );
    }

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    if( !item->IsMoving() )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        m_frame->OnModify();
    }

    return 0;
}


int LIB_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   mirrorPoint;
    bool      xAxis = ( aEvent.Matches( EE_ACTIONS::mirrorX.MakeEvent() ) );
    LIB_ITEM* item = static_cast<LIB_ITEM*>( selection.Front() );

    if( !item->IsMoving() )
        m_frame->SaveCopyInUndoList( m_frame->GetCurPart() );

    if( selection.GetSize() == 1 )
        mirrorPoint = item->GetPosition();
    else
        mirrorPoint = m_frame->GetNearestGridPosition( mapCoords( selection.GetCenter() ) );

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        item = static_cast<LIB_ITEM*>( selection.GetItem( ii ) );

        if( xAxis )
            item->MirrorVertical( mirrorPoint );
        else
            item->MirrorHorizontal( mirrorPoint );

        m_frame->RefreshItem( item );
    }

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    if( !item->IsMoving() )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        m_frame->OnModify();
    }

    return 0;
}


static KICAD_T nonFields[] =
{
        LIB_PART_T,
        LIB_ALIAS_T,
        LIB_ARC_T,
        LIB_CIRCLE_T,
        LIB_TEXT_T,
        LIB_RECTANGLE_T,
        LIB_POLYLINE_T,
        LIB_BEZIER_T,
        LIB_PIN_T,
        EOT
};


int LIB_EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    LIB_PART*  part = m_frame->GetCurPart();
    SELECTION& selection = m_selectionTool->RequestSelection( nonFields );

    if( selection.GetSize() == 0 )
        return 0;

    // Doing a duplicate of a new object doesn't really make any sense; we'd just end
    // up dragging around a stack of objects...
    if( selection.Front()->IsNew() )
        return 0;

    if( !selection.Front()->IsMoving() )
        m_frame->SaveCopyInUndoList( m_frame->GetCurPart() );

    EDA_ITEMS newItems;

    for( unsigned ii = 0; ii < selection.GetSize(); ++ii )
    {
        LIB_ITEM* oldItem = static_cast<LIB_ITEM*>( selection.GetItem( ii ) );
        LIB_ITEM* newItem = (LIB_ITEM*) oldItem->Clone();
        newItem->SetFlags( IS_NEW );
        newItems.push_back( newItem );

        part->GetDrawItems().push_back( newItem );
        getView()->Add( newItem );
    }

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    m_toolMgr->RunAction( EE_ACTIONS::addItemsToSel, true, &newItems );
    m_toolMgr->RunAction( EE_ACTIONS::move, false );

    return 0;
}


int LIB_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    LIB_PART* part = m_frame->GetCurPart();
    auto      items = m_selectionTool->RequestSelection( nonFields ).GetItems();

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    m_frame->SaveCopyInUndoList( part );

    for( EDA_ITEM* item : items )
    {
        if( item->Type() == LIB_PIN_T )
        {
            LIB_PIN*  pin = static_cast<LIB_PIN*>( item );
            wxPoint   pos = pin->GetPosition();

            part->RemoveDrawItem( pin );

            // when pin editing is synchronized, all pins of the same body style are removed:
            if( m_frame->SynchronizePins() )
            {
                int curr_convert = pin->GetConvert();
                LIB_PIN* next_pin = part->GetNextPin();

                while( next_pin != NULL )
                {
                    pin = next_pin;
                    next_pin = part->GetNextPin( pin );

                    if( pin->GetPosition() != pos )
                        continue;

                    if( pin->GetConvert() != curr_convert )
                        continue;

                    part->RemoveDrawItem( pin );
                }
            }
        }
        else
        {
            part->RemoveDrawItem( (LIB_ITEM*) item );
        }
    }

    m_frame->RebuildView();
    m_frame->OnModify();

    return 0;
}


static bool deleteItem( SCH_BASE_FRAME* aFrame, const VECTOR2D& aPosition )
{
    EE_SELECTION_TOOL* selectionTool = aFrame->GetToolManager()->GetTool<EE_SELECTION_TOOL>();
    wxCHECK( selectionTool, false );

    aFrame->GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );

    EDA_ITEM* item = selectionTool->SelectPoint( aPosition );

    if( item )
        aFrame->GetToolManager()->RunAction( EE_ACTIONS::doDelete, true );

    return true;
}


int LIB_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    Activate();

    EE_PICKER_TOOL* picker = m_toolMgr->GetTool<EE_PICKER_TOOL>();
    wxCHECK( picker, 0 );

    m_frame->SetToolID( ID_LIBEDIT_DELETE_ITEM_BUTT, wxCURSOR_BULLSEYE, _( "Delete item" ) );
    picker->SetClickHandler( std::bind( deleteItem, m_frame, std::placeholders::_1 ) );
    picker->Activate();
    Wait();

    return 0;
}


int LIB_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() )
    {
        wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );

        cmd.SetId( ID_LIBEDIT_GET_FRAME_EDIT_PART );
        m_frame->GetEventHandler()->ProcessEvent( cmd );
    }
    else if( selection.Size() == 1 )
    {
        LIB_ITEM* item = (LIB_ITEM*) selection.Front();

        // Save copy for undo if not in edit (edit command already handle the save copy)
        if( !item->InEditMode() )
            m_frame->SaveCopyInUndoList( item->GetParent() );

        switch( item->Type() )
        {
        case LIB_PIN_T:
        {
            LIB_PIN_TOOL* pinTool = m_toolMgr->GetTool<LIB_PIN_TOOL>();

            if( pinTool )
                pinTool->EditPinProperties( (LIB_PIN*) item );

            break;
        }
        case LIB_ARC_T:
        case LIB_CIRCLE_T:
        case LIB_RECTANGLE_T:
        case LIB_POLYLINE_T:
            editGraphicProperties( item );
            break;

        case LIB_TEXT_T:
            editTextProperties( item );
            break;

        case LIB_FIELD_T:
            editFieldProperties( (LIB_FIELD*) item );
            break;

        default:
            wxFAIL_MSG( wxT( "Unhandled item <" ) + item->GetClass() + wxT( ">" ) );
            break;
        }
    }

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    return 0;
}


void LIB_EDIT_TOOL::editGraphicProperties( LIB_ITEM* aItem )
{
    if( aItem == NULL )
        return;

    DIALOG_LIB_EDIT_DRAW_ITEM dialog( m_frame, aItem );

    if( dialog.ShowModal() != wxID_OK )
        return;

    if( aItem->IsFillable() )
        aItem->SetFillMode( (FILL_T) dialog.GetFillStyle() );

    aItem->SetWidth( dialog.GetWidth() );

    m_frame->GetCanvas()->GetView()->Update( aItem );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify( );

    m_frame->g_LastLineWidth       = dialog.GetWidth();
    m_frame->m_DrawSpecificConvert = !dialog.GetApplyToAllConversions();
    m_frame->m_DrawSpecificUnit    = !dialog.GetApplyToAllUnits();

    MSG_PANEL_ITEMS items;
    aItem->GetMsgPanelInfo( m_frame->GetUserUnits(), items );
    m_frame->SetMsgPanel( items );
}


void LIB_EDIT_TOOL::editTextProperties( LIB_ITEM* aItem )
{
    if ( ( aItem == NULL ) || ( aItem->Type() != LIB_TEXT_T ) )
        return;

    DIALOG_LIB_EDIT_TEXT dlg( m_frame, (LIB_TEXT*) aItem );

    if( dlg.ShowModal() != wxID_OK )
        return;

    m_frame->GetCanvas()->GetView()->Update( aItem );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify( );
}


void LIB_EDIT_TOOL::editFieldProperties( LIB_FIELD* aField )
{
    if( aField == NULL )
        return;

    wxString  caption;
    LIB_PART* parent = aField->GetParent();
    wxCHECK( parent, /* void */ );

    // Editing the component value field is equivalent to creating a new component based
    // on the current component.  Set the dialog message to inform the user.
    if( aField->GetId() == VALUE )
        caption = _( "Edit Component Name" );
    else
        caption.Printf( _( "Edit %s Field" ), GetChars( aField->GetName() ) );

    DIALOG_LIB_EDIT_ONE_FIELD dlg( m_frame, caption, aField );

    // The dialog may invoke a kiway player for footprint fields
    // so we must use a quasimodal dialog.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    wxString newFieldValue = LIB_ID::FixIllegalChars( dlg.GetText(), LIB_ID::ID_SCH );
    wxString oldFieldValue = aField->GetFullText( m_frame->GetUnit() );
    bool     renamed = aField->GetId() == VALUE && newFieldValue != oldFieldValue;

    if( renamed )
        m_frame->UpdateAfterRename( parent, oldFieldValue, newFieldValue );

    dlg.UpdateField( aField );

    m_frame->GetCanvas()->GetView()->Update( aField );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify( );
}


int LIB_EDIT_TOOL::Cut( const TOOL_EVENT& aEvent )
{
    int retVal = Copy( aEvent );

    if( retVal == 0 )
        retVal = DoDelete( aEvent );

    return retVal;
}


int LIB_EDIT_TOOL::Copy( const TOOL_EVENT& aEvent )
{
    LIB_PART*  part = m_frame->GetCurPart();
    SELECTION& selection = m_selectionTool->RequestSelection( nonFields );

    if( !part || !selection.GetSize() )
        return 0;

    for( LIB_ITEM& item : part->GetDrawItems() )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        wxASSERT( ( item.GetFlags() & STRUCT_DELETED ) == 0 );

        if( !item.IsSelected() )
            item.SetFlags( STRUCT_DELETED );
    }

    LIB_PART* partCopy = new LIB_PART( *part );

    STRING_FORMATTER  formatter;
    SCH_LEGACY_PLUGIN::FormatPart( partCopy, formatter );

    delete partCopy;

    for( LIB_ITEM& item : part->GetDrawItems() )
        item.ClearFlags( STRUCT_DELETED );

    if( m_toolMgr->SaveClipboard( formatter.GetString() ) )
        return 0;
    else
        return -1;
}


int LIB_EDIT_TOOL::Paste( const TOOL_EVENT& aEvent )
{
    LIB_PART*           part = m_frame->GetCurPart();

    if( !part )
        return 0;

    std::string         text = m_toolMgr->GetClipboard();
    STRING_LINE_READER  reader( text, "Clipboard" );
    LIB_PART*           newPart;
    EDA_ITEMS           newItems;

    try
    {
        reader.ReadLine();
        newPart = SCH_LEGACY_PLUGIN::ParsePart( reader );
    }
    catch( IO_ERROR& e )
    {
        wxLogError( wxString::Format( "Malformed clipboard: %s" ), GetChars( e.What() ) );
        return -1;
    }

    for( LIB_ITEM& item : newPart->GetDrawItems() )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
        newItem->SetFlags( IS_NEW );
        newItems.push_back( newItem );

        part->GetDrawItems().push_back( newItem );
        getView()->Add( newItem );
    }

    delete newPart;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    m_toolMgr->RunAction( EE_ACTIONS::addItemsToSel, true, &newItems );
    m_toolMgr->RunAction( EE_ACTIONS::move, false );

    return 0;
}


void LIB_EDIT_TOOL::setTransitions()
{
    Go( &LIB_EDIT_TOOL::Duplicate,          EE_ACTIONS::duplicate.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCW.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCCW.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorX.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorY.MakeEvent() );
    Go( &LIB_EDIT_TOOL::DoDelete,           EE_ACTIONS::doDelete.MakeEvent() );
    Go( &LIB_EDIT_TOOL::DeleteItemCursor,   EE_ACTIONS::deleteItemCursor.MakeEvent() );

    Go( &LIB_EDIT_TOOL::Properties,         EE_ACTIONS::properties.MakeEvent() );

    Go( &LIB_EDIT_TOOL::Cut,                EE_ACTIONS::cut.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Copy,               EE_ACTIONS::copy.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Paste,              EE_ACTIONS::paste.MakeEvent() );
}
