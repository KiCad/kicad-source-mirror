/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/picker_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/symbol_editor_pin_tool.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <tools/symbol_editor_move_tool.h>
#include <ee_actions.h>
#include <string_utils.h>
#include <symbol_edit_frame.h>
#include <sch_commit.h>
#include <dialogs/dialog_lib_shape_properties.h>
#include <dialogs/dialog_lib_text_properties.h>
#include <dialogs/dialog_lib_textbox_properties.h>
#include <dialogs/dialog_field_properties.h>
#include <dialogs/dialog_lib_symbol_properties.h>
#include <dialogs/dialog_lib_edit_pin_table.h>
#include <dialogs/dialog_update_symbol_fields.h>
#include <sch_plugins/kicad/sch_sexpr_plugin.h>
#include <lib_text.h>
#include <lib_textbox.h>
#include "symbol_editor_edit_tool.h"
#include <wx/textdlg.h>     // for wxTextEntryDialog
#include <math/util.h>      // for KiROUND

SYMBOL_EDITOR_EDIT_TOOL::SYMBOL_EDITOR_EDIT_TOOL() :
        EE_TOOL_BASE( "eeschema.SymbolEditTool" ),
        m_pickerItem( nullptr )
{
}


bool SYMBOL_EDITOR_EDIT_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    SYMBOL_EDITOR_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    SYMBOL_EDITOR_MOVE_TOOL*     moveTool = m_toolMgr->GetTool<SYMBOL_EDITOR_MOVE_TOOL>();

    wxASSERT_MSG( drawingTools, "eeschema.SymbolDrawing tool is not available" );

    auto haveSymbolCondition =
            [&]( const SELECTION& sel )
            {
                return m_isSymbolEditor &&
                       static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();
            };

    auto canEdit =
            [&]( const SELECTION& sel )
            {
                SYMBOL_EDIT_FRAME* editor = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
                wxCHECK( editor, false );

                if( !editor->IsSymbolEditable() )
                    return false;

                if( editor->IsSymbolAlias() )
                {
                    for( EDA_ITEM* item : sel )
                    {
                        if( item->Type() != LIB_FIELD_T )
                            return false;
                    }
                }

                return true;
            };

    // Add edit actions to the move tool menu
    if( moveTool )
    {
        CONDITIONAL_MENU& moveMenu = moveTool->GetToolMenu().GetMenu();

        moveMenu.AddSeparator( 200 );
        moveMenu.AddItem( EE_ACTIONS::rotateCCW,    canEdit && EE_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( EE_ACTIONS::rotateCW,     canEdit && EE_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( EE_ACTIONS::mirrorV,      canEdit && EE_CONDITIONS::NotEmpty, 200 );
        moveMenu.AddItem( EE_ACTIONS::mirrorH,      canEdit && EE_CONDITIONS::NotEmpty, 200 );

        moveMenu.AddItem( EE_ACTIONS::properties,   canEdit && EE_CONDITIONS::Count( 1 ), 200 );

        moveMenu.AddSeparator( 300 );
        moveMenu.AddItem( ACTIONS::cut,             EE_CONDITIONS::IdleSelection, 300 );
        moveMenu.AddItem( ACTIONS::copy,            EE_CONDITIONS::IdleSelection, 300 );
        moveMenu.AddItem( ACTIONS::duplicate,       canEdit && EE_CONDITIONS::NotEmpty, 300 );
        moveMenu.AddItem( ACTIONS::doDelete,        canEdit && EE_CONDITIONS::NotEmpty, 200 );

        moveMenu.AddSeparator( 400 );
        moveMenu.AddItem( ACTIONS::selectAll,       haveSymbolCondition, 400 );
    }

    // Add editing actions to the drawing tool menu
    CONDITIONAL_MENU& drawMenu = drawingTools->GetToolMenu().GetMenu();

    drawMenu.AddSeparator( 200 );
    drawMenu.AddItem( EE_ACTIONS::rotateCCW,        canEdit && EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::rotateCW,         canEdit && EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorV,          canEdit && EE_CONDITIONS::IdleSelection, 200 );
    drawMenu.AddItem( EE_ACTIONS::mirrorH,          canEdit && EE_CONDITIONS::IdleSelection, 200 );

    drawMenu.AddItem( EE_ACTIONS::properties,       canEdit && EE_CONDITIONS::Count( 1 ), 200 );

    // Add editing actions to the selection tool menu
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::rotateCCW,     canEdit && EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::rotateCW,      canEdit && EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorV,       canEdit && EE_CONDITIONS::NotEmpty, 200 );
    selToolMenu.AddItem( EE_ACTIONS::mirrorH,       canEdit && EE_CONDITIONS::NotEmpty, 200 );

    selToolMenu.AddItem( EE_ACTIONS::properties,    canEdit && EE_CONDITIONS::Count( 1 ), 200 );

    selToolMenu.AddSeparator( 300 );
    selToolMenu.AddItem( ACTIONS::cut,              EE_CONDITIONS::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::copy,             EE_CONDITIONS::IdleSelection, 300 );
    selToolMenu.AddItem( ACTIONS::paste,            canEdit && EE_CONDITIONS::Idle, 300 );
    selToolMenu.AddItem( ACTIONS::duplicate,        canEdit && EE_CONDITIONS::NotEmpty, 300 );
    selToolMenu.AddItem( ACTIONS::doDelete,         canEdit && EE_CONDITIONS::NotEmpty, 300 );

    selToolMenu.AddSeparator( 400 );
    selToolMenu.AddItem( ACTIONS::selectAll,        haveSymbolCondition, 400 );

    return true;
}


int SYMBOL_EDITOR_EDIT_TOOL::Rotate( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    VECTOR2I    rotPoint;
    bool        ccw = ( aEvent.Matches( EE_ACTIONS::rotateCCW.MakeEvent() ) );
    LIB_ITEM*   item = static_cast<LIB_ITEM*>( selection.Front() );
    SCH_COMMIT  localCommit( m_toolMgr );
    SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    if( !item->IsMoving() )
        commit->Modify( m_frame->GetCurSymbol(), m_frame->GetScreen() );

    if( selection.GetSize() == 1 )
        rotPoint = item->GetPosition();
    else
        rotPoint = m_frame->GetNearestHalfGridPosition( mapCoords( selection.GetCenter() ) );

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        item = static_cast<LIB_ITEM*>( selection.GetItem( ii ) );
        item->Rotate( rotPoint, ccw );
        m_frame->UpdateItem( item, false, true );
    }

    if( item->IsMoving() )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else
    {

        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

        if( !localCommit.Empty() )
            localCommit.Push( _( "Rotate" ) );
    }

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Mirror( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.GetSize() == 0 )
        return 0;

    VECTOR2I  mirrorPoint;
    bool      xAxis = ( aEvent.Matches( EE_ACTIONS::mirrorV.MakeEvent() ) );
    LIB_ITEM* item = static_cast<LIB_ITEM*>( selection.Front() );

    if( !item->IsMoving() )
        saveCopyInUndoList( m_frame->GetCurSymbol(), UNDO_REDO::LIBEDIT );

    if( selection.GetSize() == 1 )
        mirrorPoint = item->GetPosition();
    else
        mirrorPoint = m_frame->GetNearestHalfGridPosition( mapCoords( selection.GetCenter() ) );

    for( unsigned ii = 0; ii < selection.GetSize(); ii++ )
    {
        item = static_cast<LIB_ITEM*>( selection.GetItem( ii ) );

        if( xAxis )
            item->MirrorVertical( mirrorPoint );
        else
            item->MirrorHorizontal( mirrorPoint );

        m_frame->UpdateItem( item, false, true );
    }

    if( item->IsMoving() )
    {
        m_toolMgr->RunAction( ACTIONS::refreshPreview );
    }
    else
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

        m_frame->OnModify();
    }

    return 0;
}


static std::vector<KICAD_T> nonFields =
{
    LIB_SYMBOL_T,
    LIB_SHAPE_T,
    LIB_TEXT_T,
    LIB_TEXTBOX_T,
    LIB_PIN_T
};


int SYMBOL_EDITOR_EDIT_TOOL::DoDelete( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*           symbol = m_frame->GetCurSymbol();
    std::deque<EDA_ITEM*> items = m_selectionTool->RequestSelection( nonFields ).GetItems();
    SCH_COMMIT            commit( m_frame );

    if( items.empty() )
        return 0;

    // Don't leave a freed pointer in the selection
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    commit.Modify( symbol, m_frame->GetScreen() );

    std::set<LIB_ITEM*> toDelete;

    for( EDA_ITEM* item : items )
    {
        if( item->Type() == LIB_PIN_T )
        {
            LIB_PIN*  curr_pin = static_cast<LIB_PIN*>( item );
            VECTOR2I pos = curr_pin->GetPosition();

            toDelete.insert( curr_pin );

            // when pin editing is synchronized, pins in the same position, with the same name
            // in different units are also removed.  But only one pin per unit (matching)
            if( m_frame->SynchronizePins() )
            {
                std::vector<bool> got_unit( symbol->GetUnitCount() + 1 );

                got_unit[curr_pin->GetUnit()] = true;

                int curr_convert = curr_pin->GetConvert();
                ELECTRICAL_PINTYPE etype = curr_pin->GetType();
                wxString name = curr_pin->GetName();
                std::vector<LIB_PIN*> pins = symbol->GetAllLibPins();

                for( LIB_PIN* pin : pins )
                {
                    if( got_unit[pin->GetUnit()] )
                        continue;

                    if( pin->GetPosition() != pos )
                        continue;

                    if( pin->GetConvert() != curr_convert )
                        continue;

                    if( pin->GetType() != etype )
                        continue;

                    if( pin->GetName() != name )
                        continue;

                    toDelete.insert( pin );
                    got_unit[pin->GetUnit()] = true;
                }
            }
        }
        else
        {
            toDelete.insert( (LIB_ITEM*) item );
        }
    }

    for( LIB_ITEM* item : toDelete )
        symbol->RemoveDrawItem( item );

    commit.Push( _( "Delete" ) );
    m_frame->RebuildView();
    return 0;
}


#define HITTEST_THRESHOLD_PIXELS 5


int SYMBOL_EDITOR_EDIT_TOOL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );
    m_pickerItem = nullptr;

    // Deactivate other tools; particularly important if another PICKER is currently running
    Activate();

    picker->SetCursor( KICURSOR::REMOVE );

    picker->SetClickHandler(
            [this]( const VECTOR2D& aPosition ) -> bool
            {
                if( m_pickerItem )
                {
                    EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
                    selectionTool->UnbrightenItem( m_pickerItem );
                    selectionTool->AddItemToSel( m_pickerItem, true /*quiet mode*/ );
                    m_toolMgr->RunAction( ACTIONS::doDelete );
                    m_pickerItem = nullptr;
                }

                return true;
            } );

    picker->SetMotionHandler(
            [this]( const VECTOR2D& aPos )
            {
                EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
                EE_COLLECTOR       collector;

                selectionTool->CollectHits( collector, aPos, nonFields );

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
            [this]( const int& aFinalState )
            {
                if( m_pickerItem )
                    m_toolMgr->GetTool<EE_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

                // Wake the selection tool after exiting to ensure the cursor gets updated
                m_toolMgr->PostAction( EE_ACTIONS::selectionActivate );
            } );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Properties( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection();

    if( selection.Empty() || aEvent.IsAction( &EE_ACTIONS::symbolProperties ) )
    {
        if( m_frame->GetCurSymbol() )
            editSymbolProperties();
    }
    else if( selection.Size() == 1 )
    {
        LIB_ITEM* item = (LIB_ITEM*) selection.Front();

        // Save copy for undo if not in edit (edit command already handle the save copy)
        if( item->GetEditFlags() == 0 )
            saveCopyInUndoList( item->GetParent(), UNDO_REDO::LIBEDIT );

        switch( item->Type() )
        {
        case LIB_PIN_T:
        {
            SYMBOL_EDITOR_PIN_TOOL* pinTool = m_toolMgr->GetTool<SYMBOL_EDITOR_PIN_TOOL>();

            if( pinTool )
                pinTool->EditPinProperties( (LIB_PIN*) item );
        }
            break;

        case LIB_SHAPE_T:
            editShapeProperties( static_cast<LIB_SHAPE*>( item ) );
            break;

        case LIB_TEXT_T:
            editTextProperties( item );
            break;

        case LIB_TEXTBOX_T:
            editTextBoxProperties( item );
            break;

        case LIB_FIELD_T:
            editFieldProperties( (LIB_FIELD*) item );
            break;

        default:
            wxFAIL_MSG( wxT( "Unhandled item <" ) + item->GetClass() + wxT( ">" ) );
            break;
        }
    }

    if( selection.IsHover() )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    return 0;
}


void SYMBOL_EDITOR_EDIT_TOOL::editShapeProperties( LIB_SHAPE* aShape )
{
    DIALOG_LIB_SHAPE_PROPERTIES dlg( m_frame, aShape );

    if( dlg.ShowModal() != wxID_OK )
        return;

    updateItem( aShape, true );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    SYMBOL_EDITOR_DRAWING_TOOLS* drawingTools = m_toolMgr->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    drawingTools->SetDrawSpecificConvert( !dlg.GetApplyToAllConversions() );
    drawingTools->SetDrawSpecificUnit( !dlg.GetApplyToAllUnits() );

    std::vector<MSG_PANEL_ITEM> items;
    aShape->GetMsgPanelInfo( m_frame, items );
    m_frame->SetMsgPanel( items );
}


void SYMBOL_EDITOR_EDIT_TOOL::editTextProperties( LIB_ITEM* aItem )
{
    if ( aItem->Type() != LIB_TEXT_T )
        return;

    DIALOG_LIB_TEXT_PROPERTIES dlg( m_frame, static_cast<LIB_TEXT*>( aItem ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    updateItem( aItem, true );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify( );
}


void SYMBOL_EDITOR_EDIT_TOOL::editTextBoxProperties( LIB_ITEM* aItem )
{
    if ( aItem->Type() != LIB_TEXTBOX_T )
        return;

    DIALOG_LIB_TEXTBOX_PROPERTIES dlg( m_frame, static_cast<LIB_TEXTBOX*>( aItem ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    updateItem( aItem, true );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify( );
}


void SYMBOL_EDITOR_EDIT_TOOL::editFieldProperties( LIB_FIELD* aField )
{
    if( aField == nullptr )
        return;

    wxString    caption;
    LIB_SYMBOL* parent = aField->GetParent();
    wxCHECK( parent, /* void */ );

    if( aField->GetId() >= 0 && aField->GetId() < MANDATORY_FIELDS )
        caption.Printf( _( "Edit %s Field" ), TitleCaps( aField->GetName() ) );
    else
        caption.Printf( _( "Edit '%s' Field" ), aField->GetName() );

    DIALOG_LIB_FIELD_PROPERTIES dlg( m_frame, caption, aField );

    // The dialog may invoke a kiway player for footprint fields
    // so we must use a quasimodal dialog.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    wxString newFieldValue = EscapeString( dlg.GetText(), CTX_LIBID );
    wxString oldFieldValue = aField->GetFullText( m_frame->GetUnit() );

    saveCopyInUndoList( parent, UNDO_REDO::LIBEDIT );

    dlg.UpdateField( aField );

    updateItem( aField, true );
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();
    m_frame->UpdateSymbolMsgPanelInfo();
}


void SYMBOL_EDITOR_EDIT_TOOL::editSymbolProperties()
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();
    bool        partLocked = symbol->UnitsLocked();

    m_toolMgr->RunAction( ACTIONS::cancelInteractive );
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    DIALOG_LIB_SYMBOL_PROPERTIES dlg( m_frame, symbol );

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    m_frame->OnModify();

    // if m_UnitSelectionLocked has changed, set some edit options or defaults
    // to the best value
    if( partLocked != symbol->UnitsLocked() )
    {
        SYMBOL_EDITOR_DRAWING_TOOLS* tools = m_toolMgr->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();

        // Enable synchronized pin edit mode for symbols with interchangeable units
        m_frame->m_SyncPinEdit = !symbol->UnitsLocked();

        // also set default edit options to the better value
        // Usually if units are locked, graphic items are specific to each unit
        // and if units are interchangeable, graphic items are common to units
        tools->SetDrawSpecificUnit( symbol->UnitsLocked() );
    }
}

void SYMBOL_EDITOR_EDIT_TOOL::handlePinDuplication( LIB_PIN* aOldPin, LIB_PIN* aNewPin,
                                                    int& aSymbolLastPinNumber )
{
    if( !aNewPin->GetNumber().IsEmpty() )
    {
        // when duplicating a pin in symbol editor, assigning identical pin number
        // to the old one does not makes any sense, so assign the next unassigned number to it
        aSymbolLastPinNumber++;
        aNewPin->SetNumber( wxString::Format( wxT( "%i" ), aSymbolLastPinNumber ) );
    }
}

int SYMBOL_EDITOR_EDIT_TOOL::PinTable( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    saveCopyInUndoList( symbol, UNDO_REDO::LIBEDIT );

    DIALOG_LIB_EDIT_PIN_TABLE dlg( m_frame, symbol );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    m_frame->RebuildView();
    m_frame->OnModify();

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::UpdateSymbolFields( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    if( !symbol->IsAlias() )
    {
        m_frame->ShowInfoBarError( _( "Symbol is not derived from another symbol." ) );
    }
    else
    {
        DIALOG_UPDATE_SYMBOL_FIELDS dlg( m_frame, symbol );

        if( dlg.ShowModal() == wxID_CANCEL )
            return -1;
    }

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::SetUnitDisplayName( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = m_frame->GetCurSymbol();

    if( !symbol )
        return 0;

    int unitid = m_frame->GetUnit();

    if( unitid == 0 )
    {
        return -1;
    }

    wxString promptText = wxString::Format( _( "Enter display name for unit %s" ),
                                            symbol->GetUnitReference( unitid ) );
    wxString currentvalue;

    if( symbol->HasUnitDisplayName( unitid ) )
    {
        currentvalue = symbol->GetUnitDisplayName( unitid );
    }

    wxTextEntryDialog dlg( m_frame, promptText, _( "Set Unit Display Name" ), currentvalue );

    if( dlg.ShowModal() == wxID_OK )
    {
        saveCopyInUndoList( symbol, UNDO_REDO::LIBEDIT );
        symbol->SetUnitDisplayName( unitid, dlg.GetValue() );
        m_frame->RebuildSymbolUnitsList();
        m_frame->OnModify();
    }
    else
    {
        return -1;
    }

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Undo( const TOOL_EVENT& aEvent )
{
    m_frame->GetSymbolFromUndoList();

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Redo( const TOOL_EVENT& aEvent )
{
    m_frame->GetSymbolFromRedoList();

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Cut( const TOOL_EVENT& aEvent )
{
    int retVal = Copy( aEvent );

    if( retVal == 0 )
        retVal = DoDelete( aEvent );

    return retVal;
}


int SYMBOL_EDITOR_EDIT_TOOL::Copy( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*   symbol = m_frame->GetCurSymbol();
    EE_SELECTION& selection = m_selectionTool->RequestSelection( nonFields );

    if( !symbol || !selection.GetSize() )
        return 0;

    for( LIB_ITEM& item : symbol->GetDrawItems() )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        wxASSERT( !item.HasFlag( STRUCT_DELETED ) );

        if( !item.IsSelected() )
            item.SetFlags( STRUCT_DELETED );
    }

    LIB_SYMBOL* partCopy = new LIB_SYMBOL( *symbol );

    STRING_FORMATTER  formatter;
    SCH_SEXPR_PLUGIN::FormatLibSymbol( partCopy, formatter );

    delete partCopy;

    for( LIB_ITEM& item : symbol->GetDrawItems() )
        item.ClearFlags( STRUCT_DELETED );

    if( m_toolMgr->SaveClipboard( formatter.GetString() ) )
        return 0;
    else
        return -1;
}


int SYMBOL_EDITOR_EDIT_TOOL::Paste( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol  = m_frame->GetCurSymbol();
    LIB_SYMBOL* newPart = nullptr;

    if( !symbol || symbol->IsAlias() )
        return 0;

    std::string clipboardData = m_toolMgr->GetClipboardUTF8();

    try
    {
        std::vector<LIB_SYMBOL*> newParts = SCH_SEXPR_PLUGIN::ParseLibSymbols( clipboardData, "Clipboard" );

        if( newParts.empty() || !newParts[0] )
            return -1;

        newPart = newParts[0];
    }
    catch( IO_ERROR& )
    {
        // If it's not a symbol then paste as text
        newPart = new LIB_SYMBOL( "dummy_part" );
        LIB_TEXT* newText = new LIB_TEXT( newPart );
        newText->SetText( clipboardData );
        newPart->AddDrawItem( newText );
    }

    SCH_COMMIT commit( m_toolMgr );

    commit.Modify( symbol );
    m_selectionTool->ClearSelection();

    for( LIB_ITEM& item : symbol->GetDrawItems() )
        item.ClearFlags( IS_NEW | IS_PASTED | SELECTED );

    for( LIB_ITEM& item : newPart->GetDrawItems() )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
        newItem->SetParent( symbol );
        newItem->SetFlags( IS_NEW | IS_PASTED | SELECTED );

        newItem->SetUnit( newItem->GetUnit() ? m_frame->GetUnit() : 0 );
        newItem->SetConvert( newItem->GetConvert() ? m_frame->GetConvert() : 0 );

        symbol->AddDrawItem( newItem );
        getView()->Add( newItem );
    }

    delete newPart;

    m_selectionTool->RebuildSelection();

    EE_SELECTION& selection = m_selectionTool->GetSelection();

    if( !selection.Empty() )
    {
        selection.SetReferencePoint( getViewControls()->GetCursorPosition( true ) );

        if( m_toolMgr->RunSynchronousAction( EE_ACTIONS::move, &commit ) )
            commit.Push( _( "Paste" ) );
        else
            commit.Revert();
    }

    return 0;
}


int SYMBOL_EDITOR_EDIT_TOOL::Duplicate( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL*   symbol = m_frame->GetCurSymbol();
    EE_SELECTION& selection = m_selectionTool->RequestSelection( nonFields );
    SCH_COMMIT    commit( m_toolMgr );

    if( selection.GetSize() == 0 )
        return 0;

    // Doing a duplicate of a new object doesn't really make any sense; we'd just end
    // up dragging around a stack of objects...
    if( selection.Front()->IsNew() )
        return 0;

    if( !selection.Front()->IsMoving() )
        commit.Modify( symbol, m_frame->GetScreen() );

    EDA_ITEMS newItems;
    int       symbolLastPinNumber = -1;

    for( unsigned ii = 0; ii < selection.GetSize(); ++ii )
    {
        LIB_ITEM* oldItem = static_cast<LIB_ITEM*>( selection.GetItem( ii ) );
        LIB_ITEM* newItem = (LIB_ITEM*) oldItem->Clone();

        if( oldItem->Type() == LIB_PIN_T )
        {
            if( symbolLastPinNumber == -1 )
            {
                symbolLastPinNumber = symbol->GetMaxPinNumber();
            }

            handlePinDuplication( static_cast<LIB_PIN*>( oldItem ),
                                  static_cast<LIB_PIN*>( newItem ), symbolLastPinNumber );
        }

        oldItem->ClearFlags( IS_NEW | IS_PASTED | SELECTED );
        newItem->SetFlags( IS_NEW | IS_PASTED | SELECTED );
        newItem->SetParent( symbol );
        newItems.push_back( newItem );

        symbol->AddDrawItem( newItem );
        getView()->Add( newItem );
    }

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection );
    m_toolMgr->RunAction<EDA_ITEMS*>( EE_ACTIONS::addItemsToSel, &newItems );

    selection.SetReferencePoint( mapCoords( getViewControls()->GetCursorPosition( true ) ) );

    if( m_toolMgr->RunSynchronousAction( EE_ACTIONS::move, &commit ) )
        commit.Push( _( "Duplicate" ) );
    else
        commit.Revert();

    return 0;
}


void SYMBOL_EDITOR_EDIT_TOOL::setTransitions()
{
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Undo,               ACTIONS::undo.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Redo,               ACTIONS::redo.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Cut,                ACTIONS::cut.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Copy,               ACTIONS::copy.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Paste,              ACTIONS::paste.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Duplicate,          ACTIONS::duplicate.MakeEvent() );

    Go( &SYMBOL_EDITOR_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCW.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Rotate,             EE_ACTIONS::rotateCCW.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorV.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Mirror,             EE_ACTIONS::mirrorH.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::DoDelete,           ACTIONS::doDelete.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::DeleteItemCursor,   ACTIONS::deleteTool.MakeEvent() );

    Go( &SYMBOL_EDITOR_EDIT_TOOL::Properties,         EE_ACTIONS::properties.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::Properties,         EE_ACTIONS::symbolProperties.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::PinTable,           EE_ACTIONS::pinTable.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::UpdateSymbolFields, EE_ACTIONS::updateSymbolFields.MakeEvent() );
    Go( &SYMBOL_EDITOR_EDIT_TOOL::SetUnitDisplayName, EE_ACTIONS::setUnitDisplayName.MakeEvent() );
}
