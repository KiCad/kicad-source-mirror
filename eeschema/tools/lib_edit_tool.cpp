/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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
#include <tool/picker_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/lib_pin_tool.h>
#include <tools/lib_drawing_tools.h>
#include <tools/lib_move_tool.h>
#include <ee_actions.h>
#include <bitmaps.h>
#include <confirm.h>
#include <sch_view.h>
#include <lib_edit_frame.h>
#include <eeschema_id.h>
#include <dialogs/dialog_lib_edit_draw_item.h>
#include <dialogs/dialog_lib_edit_text.h>
#include <dialogs/dialog_edit_one_field.h>
#include <dialogs/dialog_edit_component_in_lib.h>
#include <dialogs/dialog_lib_edit_pin_table.h>
#include <sch_legacy_plugin.h>
#include <lib_text.h>
#include "lib_edit_tool.h"


LIB_EDIT_TOOL::LIB_EDIT_TOOL() :
        EE_TOOL_BASE( "eeschema.SymbolEditTool" )
{
}


bool LIB_EDIT_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    LIB_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<LIB_DRAWING_TOOLS>();
    LIB_MOVE_TOOL*     moveTool = m_toolMgr->GetTool<LIB_MOVE_TOOL>();

    wxASSERT_MSG( drawingTools, "eeschema.SymbolDrawing tool is not available" );

    // Add edit actions to the move tool menu
    //
    if( moveTool )
    {
        CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

        moveMenu.AddSeparator( 200 );
        moveMenu.AddItem( EE_ACTIONS::rotateCCW,       EE_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( EE_ACTIONS::rotateCW,        EE_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( EE_ACTIONS::mirrorX,         EE_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( EE_ACTIONS::mirrorY,         EE_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( ACTIONS::doDelete,           EE_CONDITIONS::NotEmpty, 200 );

        moveMenu.AddItem( EE_ACTIONS::properties,      EE_CONDITIONS::Count( 1 ), 200 );

        moveMenu.AddSeparator( 300 );
        moveMenu.AddItem( ACTIONS::cut,                EE_CONDITIONS::IdleSelection, 300 );
        moveMenu.AddItem( ACTIONS::copy,               EE_CONDITIONS::IdleSelection, 300 );
        moveMenu.AddItem( ACTIONS::duplicate,          EE_CONDITIONS::NotEmpty, 300 );
    }

    // Add editing actions to the drawing tool menu
    //
    CONDITIONAL_MENU& drawMenu = drawingTools->GetToolMenu().GetMenu();

    drawMenu.AddSeparator( 200 );
    drawMenu.AddItem( EE_ACTIONS::rotateCCW,           EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::rotateCW,            EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorX,             EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorY,             EE_CONDITIONS::IdleSelection, 200 );

    drawMenu.AddItem( EE_ACTIONS::properties,          EE_CONDITIONS::Count( 1 ), 200 );

    // Add editing actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::rotateCCW,        EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::rotateCW,         EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorX,          EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorY,          EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( ACTIONS::doDelete,            EE_CONDITIONS::NotEmpty, 200 );

    selToolMenu.AddItem( EE_ACTIONS::properties,       EE_CONDITIONS::Count( 1 ), 200 );

    selToolMenu.AddSeparator( 300 );
    selToolMenu.AddItem( ACTIONS::cut,                 EE_CONDITIONS::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copy,                EE_CONDITIONS::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::paste,               EE_CONDITIONS::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::duplicate,           EE_CONDITIONS::NotEmpty, 300 );

    return true;
}


int LIB_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   rotPoint;
    bool      ccw = ( aEvent.Matches( EE_ACTIONS::rotateCCW.MakeEvent() ) );
    LIB_ITEM* item = static_cast<LIB_ITEM*>( selection.Front() );

    if( !item->IsMoving() )
        saveCopyInUndoList( m_frame->GetCurPart(), UR_LIBEDIT );

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

    if( item->IsMoving() )
        m_toolMgr->RunAction( ACTIONS::refreshPreview, true );
    else
    {
        m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        m_frame->OnModify();
    }

    return 0;
}


int LIB_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    wxPoint   mirrorPoint;
    bool      xAxis = ( aEvent.Matches( EE_ACTIONS::mirrorX.MakeEvent() ) );
    LIB_ITEM* item = static_cast<LIB_ITEM*>( selection.Front() );

    if( !item->IsMoving() )
        saveCopyInUndoList( m_frame->GetCurPart(), UR_LIBEDIT );

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

    if( item->IsMoving() )
        m_toolMgr->RunAction( ACTIONS::refreshPreview, true );
    else
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


int LIB_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    LIB_PART* part = m_frame->GetCurPart();
    auto      items = m_selectionTool->RequestSelection( nonFields ).GetItems();

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    saveCopyInUndoList( part, UR_LIBEDIT );

    std::set<LIB_ITEM *> toDelete;

    for( EDA_ITEM* item : items )
    {
        if( item->Type() == LIB_PIN_T )
        {
            LIB_PIN*  pin = static_cast<LIB_PIN*>( item );
            wxPoint   pos = pin->GetPosition();

            toDelete.insert( pin );

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

                    toDelete.insert( pin );
                }
            }
        }
        else
        {
            toDelete.insert( (LIB_ITEM*) item );
        }
    }

    for( auto item : toDelete )
    {
        part->RemoveDrawItem( item );
    }

    m_frame->RebuildView();
    m_frame->OnModify();

    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int LIB_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    std::string  tool = aEvent.GetCommandStr().get();
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );
    m_pickerItem = nullptr;

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( wxStockCursor( wxCURSOR_BULLSEYE ) );

    picker->SetClickHandler(
        [this] ( const VECTOR2D& aPosition ) -> bool
        {
            if( m_pickerItem )
            {
                EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
                selectionTool->UnbrightenItem( m_pickerItem );
                selectionTool->AddItemToSel( m_pickerItem, true );
                m_toolMgr->RunAction( ACTIONS::doDelete, true );
                m_pickerItem = nullptr;
            }

            return true;
        } );

    picker->SetMotionHandler(
        [this] ( const VECTOR2D& aPos )
        {
            EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
            EE_COLLECTOR collector;
            collector.m_Threshold = KiROUND( getView()->ToWorld( HITTEST_THRESHOLD_PIXELS ) );
            collector.Collect( m_frame->GetCurPart(), nonFields, (wxPoint) aPos,
                               m_frame->GetUnit(), m_frame->GetConvert() );

            // Remove unselectable items
            for( int i = collector.GetCount() - 1; i >= 0; --i )
            {
                if( !selectionTool->Selectable( collector[ i ] ) )
                    collector.Remove( i );
            }

            if( collector.GetCount() > 1 )
                selectionTool->GuessSelectionCandidates( collector, aPos );

            EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

            if( m_pickerItem != item )
            {
                if( m_pickerItem )
                    selectionTool->UnbrightenItem( m_pickerItem );

                m_pickerItem = item;

                if( m_pickerItem )
                    selectionTool->BrightenItem( m_pickerItem );
            }
        } );

    picker->SetFinalizeHandler(
        [this] ( const int& aFinalState )
        {
            if( m_pickerItem )
                m_toolMgr->GetTool<EE_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );
        } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    return 0;
}


int LIB_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() || aEvent.IsAction( &EE_ACTIONS::symbolProperties ) )
    {
        if( m_frame->GetCurPart() )
            editSymbolProperties();
    }
    else if( selection.Size() == 1 )
    {
        LIB_ITEM* item = (LIB_ITEM*) selection.Front();

        // Save copy for undo if not in edit (edit command already handle the save copy)
        if( item->GetEditFlags() == 0 )
            saveCopyInUndoList( item->GetParent(), UR_LIBEDIT );

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

    if( dialog.GetApplyToAllConversions() )
        aItem->SetConvert( 0 );
    else
        aItem->SetConvert( m_frame->GetConvert() );

    if( dialog.GetApplyToAllUnits() )
        aItem->SetUnit( 0 );
    else
        aItem->SetUnit( m_frame->GetUnit() );

    updateView( aItem );
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

    updateView( aItem );
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
        saveCopyInUndoList( parent, UR_LIB_RENAME );
    else
        saveCopyInUndoList( parent, UR_LIBEDIT );

    dlg.UpdateField( aField );

    if( renamed )
    {
        parent->SetName( newFieldValue );
        m_frame->UpdateAfterSymbolProperties( &oldFieldValue, nullptr );
    }
    else
    {
        updateView( aField );
        m_frame->GetCanvas()->Refresh();
        m_frame->OnModify( );
    }
}


void LIB_EDIT_TOOL::editSymbolProperties()
{
    LIB_PART*     part = m_frame->GetCurPart();
    bool          partLocked = part->UnitsLocked();
    wxString      oldName = part->GetName();
    wxArrayString oldAliases = part->GetAliasNames( false );

    m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    DIALOG_EDIT_COMPONENT_IN_LIBRARY dlg( m_frame, part );

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    // if m_UnitSelectionLocked has changed, set some edit options or defaults
    // to the best value
    if( partLocked != part->UnitsLocked() )
    {
        // Enable synchronized pin edit mode for symbols with interchangeable units
        m_frame->m_SyncPinEdit = !part->UnitsLocked();
        // also set default edit options to the better value
        // Usually if units are locked, graphic items are specific to each unit
        // and if units are interchangeable, graphic items are common to units
        m_frame->m_DrawSpecificUnit = part->UnitsLocked();
    }

    m_frame->UpdateAfterSymbolProperties( &oldName, &oldAliases );
}


int LIB_EDIT_TOOL::PinTable( const TOOL_EVENT& aEvent )
{
    LIB_PART* part = m_frame->GetCurPart();

    if( !part )
        return 0;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    saveCopyInUndoList( part, UR_LIBEDIT );

    DIALOG_LIB_EDIT_PIN_TABLE dlg( m_frame, part );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    m_frame->RebuildView();
    m_frame->OnModify();

    return 0;
}


int LIB_EDIT_TOOL::Undo( const TOOL_EVENT& aEvent )
{
    m_frame->GetComponentFromUndoList();

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    return 0;
}


int LIB_EDIT_TOOL::Redo( const TOOL_EVENT& aEvent )
{
    m_frame->GetComponentFromRedoList();

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    return 0;
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
    LIB_PART*     part = m_frame->GetCurPart();
    EE_SELECTION& selection = m_selectionTool->RequestSelection( nonFields );

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

    EE_SELECTION&       selection = m_selectionTool->GetSelection();
    std::string         text = m_toolMgr->GetClipboard();
    STRING_LINE_READER  reader( text, "Clipboard" );
    LIB_PART*           newPart;

    try
    {
        reader.ReadLine();
        newPart = SCH_LEGACY_PLUGIN::ParsePart( reader );
    }
    catch( IO_ERROR& e )
    {
        // If it's not a part then paste as text
        newPart = new LIB_PART( "dummy_part" );
        LIB_TEXT* newText = new LIB_TEXT( newPart );
        newText->SetText( text );
        newPart->AddDrawItem( newText );
    }

    if( !newPart )
        return -1;

    m_frame->SaveCopyInUndoList( part );
    m_selectionTool->ClearSelection();

    for( LIB_ITEM& item : newPart->GetDrawItems() )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
        newItem->SetParent( part );
        newItem->SetFlags( IS_NEW | IS_PASTED | SELECTED );

        newItem->SetUnit( m_frame->m_DrawSpecificUnit ? m_frame->GetUnit() : 0 );
        newItem->SetConvert( m_frame->m_DrawSpecificConvert ? m_frame->GetConvert() : 0 );

        part->GetDrawItems().push_back( newItem );
        getView()->Add( newItem );
    }

    delete newPart;

    m_selectionTool->RebuildSelection();

    if( !selection.Empty() )
    {
        selection.SetReferencePoint( getViewControls()->GetCursorPosition( true ) );
        m_toolMgr->RunAction( EE_ACTIONS::move, false );
    }

    return 0;
}


int LIB_EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    LIB_PART*     part = m_frame->GetCurPart();
    EE_SELECTION& selection = m_selectionTool->RequestSelection( nonFields );

    if( selection.GetSize() == 0 )
        return 0;

    // Doing a duplicate of a new object doesn't really make any sense; we'd just end
    // up dragging around a stack of objects...
    if( selection.Front()->IsNew() )
        return 0;

    if( !selection.Front()->IsMoving() )
        saveCopyInUndoList( m_frame->GetCurPart(), UR_LIBEDIT );

    for( unsigned ii = 0; ii < selection.GetSize(); ++ii )
    {
        LIB_ITEM* oldItem = static_cast<LIB_ITEM*>( selection.GetItem( ii ) );
        LIB_ITEM* newItem = (LIB_ITEM*) oldItem->Clone();
        oldItem->ClearFlags( SELECTED );
        newItem->SetFlags( IS_NEW | IS_PASTED | SELECTED );
        newItem->SetParent( part );

        part->GetDrawItems().push_back( newItem );
        getView()->Add( newItem );
    }

    m_selectionTool->RebuildSelection();

    if( !selection.Empty() )
    {
        selection.SetReferencePoint( mapCoords( getViewControls()->GetCursorPosition( true ) ) );
        m_toolMgr->RunAction( EE_ACTIONS::move, false );
    }

    return 0;
}


void LIB_EDIT_TOOL::setTransitions()
{
    Go( &LIB_EDIT_TOOL::Undo,               ACTIONS::undo.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Redo,               ACTIONS::redo.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Cut,                ACTIONS::cut.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Copy,               ACTIONS::copy.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Paste,              ACTIONS::paste.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Duplicate,          ACTIONS::duplicate.MakeEvent() );

    Go( &LIB_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCW.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCCW.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorX.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorY.MakeEvent() );
    Go( &LIB_EDIT_TOOL::DoDelete,           ACTIONS::doDelete.MakeEvent() );
    Go( &LIB_EDIT_TOOL::DeleteItemCursor,   ACTIONS::deleteTool.MakeEvent() );

    Go( &LIB_EDIT_TOOL::Properties,         EE_ACTIONS::properties.MakeEvent() );
    Go( &LIB_EDIT_TOOL::Properties,         EE_ACTIONS::symbolProperties.MakeEvent() );
    Go( &LIB_EDIT_TOOL::PinTable,           EE_ACTIONS::pinTable.MakeEvent() );
}
