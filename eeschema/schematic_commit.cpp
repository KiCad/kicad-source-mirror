/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <macros.h>
#include <tool/tool_manager.h>
#include <tools/ee_tool_base.h>

#include <lib_item.h>
#include <lib_symbol.h>

#include <sch_screen.h>
#include <schematic.h>

#include <view/view.h>
#include <schematic_commit.h>

#include <functional>


SCHEMATIC_COMMIT::SCHEMATIC_COMMIT( TOOL_MANAGER* aToolMgr ) :
        m_toolMgr( aToolMgr ),
        m_isLibEditor( false )
{
}


SCHEMATIC_COMMIT::SCHEMATIC_COMMIT( EE_TOOL_BASE<SCH_BASE_FRAME>* aTool )
{
    m_toolMgr = aTool->GetManager();
    m_isLibEditor = aTool->IsSymbolEditor();
}


SCHEMATIC_COMMIT::SCHEMATIC_COMMIT( EDA_DRAW_FRAME* aFrame )
{
    m_toolMgr = aFrame->GetToolManager();
    m_isLibEditor = aFrame->IsType( FRAME_SCH_SYMBOL_EDITOR );
}


SCHEMATIC_COMMIT::~SCHEMATIC_COMMIT()
{
}


COMMIT& SCHEMATIC_COMMIT::Stage( EDA_ITEM *aItem, CHANGE_TYPE aChangeType, BASE_SCREEN *aScreen )
{
    wxCHECK( aItem, *this );

    aItem->ClearFlags( IS_MODIFIED_CHILD );

    // If aItem belongs a symbol, the full symbol will be saved because undo/redo does
    // not handle "sub items" modifications.
    if( aItem->GetParent() && aItem->GetParent()->IsType( { SCH_SYMBOL_T, LIB_SYMBOL_T,
                                                            SCH_SHEET_T } ) )
    {
        aItem->SetFlags( IS_MODIFIED_CHILD );
        aItem = aItem->GetParent();
        aChangeType = CHT_MODIFY;
    }

    // IS_SELECTED flag should not be set on undo items which were added for
    // a drag operation.
    if( aItem->IsSelected() && aItem->HasFlag( SELECTED_BY_DRAG ) )
    {
        aItem->ClearSelected();
        COMMIT::Stage( aItem, aChangeType, aScreen );
        aItem->SetSelected();
    }
    else
    {
        COMMIT::Stage( aItem, aChangeType, aScreen );
    }

    return *this;
}


COMMIT& SCHEMATIC_COMMIT::Stage( std::vector<EDA_ITEM*> &container, CHANGE_TYPE aChangeType,
                                 BASE_SCREEN *aScreen )
{
    for( EDA_ITEM* item : container )
        Stage( item, aChangeType, aScreen );

    return *this;
}


COMMIT& SCHEMATIC_COMMIT::Stage( const PICKED_ITEMS_LIST &aItems, UNDO_REDO aModFlag,
                                 BASE_SCREEN *aScreen )
{
    return COMMIT::Stage( aItems, aModFlag, aScreen );
}


void SCHEMATIC_COMMIT::pushLibEdit( const wxString& aMessage, int aCommitFlags )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST   undoList;
    KIGFX::VIEW*        view = m_toolMgr->GetView();

    SYMBOL_EDIT_FRAME*  sym_frame = static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    LIB_SYMBOL*         symbol = sym_frame->GetCurSymbol();
    std::set<EDA_ITEM*> savedModules;
    EE_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    bool                itemsDeselected = false;
    bool                selectedModified = false;

    if( Empty() )
        return;

    undoList.SetDescription( aMessage );

    for( COMMIT_LINE& ent : m_changes )
    {
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;
        LIB_ITEM* libItem = static_cast<LIB_ITEM*>( ent.m_item );

        wxASSERT( ent.m_item );

        // Module items need to be saved in the undo buffer before modification
        if( ent.m_item->Type() != LIB_SYMBOL_T )
        {
            ent.m_item = ent.m_item->GetParent();
            wxASSERT( ent.m_item );
        }

        // We have not saved the symbol yet, so let's create an entry
        if( savedModules.count( ent.m_item ) == 0 )
        {
            if( !( aCommitFlags & SKIP_UNDO ) && sym_frame )
                sym_frame->SaveCopyInUndoList( ent.m_item, UNDO_REDO::CHANGED, aCommitFlags & APPEND_UNDO );

            savedModules.insert( ent.m_item );
        }

        if( ent.m_item->IsSelected() )
            selectedModified = true;

        symbol->RunOnChildren(
            [&selectedModified]( LIB_ITEM* aItem )
            {
                if( aItem->HasFlag( IS_MODIFIED_CHILD ) )
                    selectedModified = true;
            } );

        switch( changeType )
        {
        case CHT_ADD:
        {
            wxASSERT( libItem->Type() != LIB_SYMBOL_T );

            libItem->SetParent( symbol );

            if( !( changeFlags & CHT_DONE ) )
                symbol->AddDrawItem( libItem );

            if( view )
                view->Add( libItem );

            break;
        }

        case CHT_REMOVE:
        {
            if( libItem->IsSelected() )
            {
                if( selTool )
                    selTool->RemoveItemFromSel( libItem, true /* quiet mode */ );

                itemsDeselected = true;
            }

            // Avoid removing mandatory fields
            if( libItem->Type() == LIB_FIELD_T && static_cast<LIB_FIELD*>( libItem )->IsMandatory() )
                break;

            if( view )
                view->Remove( libItem );

            if( !( changeFlags & CHT_DONE ) )
                symbol->RemoveDrawItem( libItem );

            break;

        }

        case CHT_MODIFY:
        {
            if( view )
            {
                view->Update( libItem );

                symbol->RunOnChildren(
                        [&]( LIB_ITEM* aChild )
                        {
                            view->Update( aChild );
                        });
            }

            // if no undo entry is needed, the copy would create a memory leak
            if( aCommitFlags & SKIP_UNDO )
                delete ent.m_copy;

            break;
        }

        default:
            wxASSERT( false );
            break;
        }
    }

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( itemsDeselected )
        m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

    if( selectedModified )
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( sym_frame && !( aCommitFlags & SKIP_SET_DIRTY ) )
        sym_frame->OnModify();

    clear();
}


void SCHEMATIC_COMMIT::pushSchEdit( const wxString& aMessage, int aCommitFlags )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST   undoList;
    KIGFX::VIEW*        view = m_toolMgr->GetView();

    SCH_EDIT_FRAME*     frame = static_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    EE_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    bool                itemsDeselected = false;
    bool                selectedModified = false;

    if( Empty() )
        return;

    undoList.SetDescription( aMessage );

    SCHEMATIC& schematic = static_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() )->Schematic();
    std::vector<SCH_ITEM*>   bulkAddedItems;
    std::vector<SCH_ITEM*>   bulkRemovedItems;
    std::vector<SCH_ITEM*>   itemsChanged;

    for( COMMIT_LINE& ent : m_changes )
    {
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;
        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( ent.m_item );
        SCH_SCREEN* screen = static_cast<SCH_SCREEN*>( ent.m_screen );

        wxASSERT( ent.m_item );

        if( ent.m_item->IsSelected() )
            selectedModified = true;

        switch( changeType )
        {
        case CHT_ADD:
        {
            if( !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( screen, schItem, UNDO_REDO::NEWITEM ) );

            if( !( changeFlags & CHT_DONE ) )
            {
                if( !schItem->GetParent() )
                    frame->GetScreen()->Append( schItem );

                if( view )
                    view->Add( schItem );
            }

            bulkAddedItems.push_back( schItem );

            break;
        }

        case CHT_REMOVE:
        {
            if( !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( screen, schItem, UNDO_REDO::DELETED ) );

            if( schItem->IsSelected() )
            {
                if( selTool )
                    selTool->RemoveItemFromSel( schItem, true /* quiet mode */ );

                itemsDeselected = true;
            }

            if( schItem->Type() == SCH_FIELD_T )
            {
                static_cast<SCH_FIELD*>( schItem )->SetVisible( false );
                break;
            }

            if( !( changeFlags & CHT_DONE ) )
            {
                frame->GetScreen()->Remove( schItem );

                if( view )
                    view->Remove( schItem );
            }

            bulkRemovedItems.push_back( schItem );

            break;
        }

        case CHT_MODIFY:
        {
            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( screen, schItem, UNDO_REDO::CHANGED );
                wxASSERT( ent.m_copy );
                itemWrapper.SetLink( ent.m_copy );
                undoList.PushItem( itemWrapper );
            }

            if( view )
                view->Update( schItem );

            itemsChanged.push_back( schItem );

            // if no undo entry is needed, the copy would create a memory leak
            if( aCommitFlags & SKIP_UNDO )
                delete ent.m_copy;

            break;
        }

        default:
            wxASSERT( false );
            break;
        }
    }

    if( bulkAddedItems.size() > 0 )
        schematic.OnItemsAdded( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        schematic.OnItemsRemoved( bulkRemovedItems );

    if( itemsChanged.size() > 0 )
        schematic.OnItemsChanged( itemsChanged );

    if( !( aCommitFlags & SKIP_UNDO ) && frame )
    {
        frame->SaveCopyInUndoList( undoList, UNDO_REDO::UNSPECIFIED, aCommitFlags & APPEND_UNDO,
                                   ( aCommitFlags & SKIP_CONNECTIVITY ) == 0 );
    }

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( itemsDeselected )
        m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

    if( selectedModified )
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( frame && !( aCommitFlags & SKIP_SET_DIRTY ) )
        frame->OnModify();

    clear();
}


void SCHEMATIC_COMMIT::Push( const wxString& aMessage, int aCommitFlags )
{
    if( m_isLibEditor )
        pushLibEdit( aMessage, aCommitFlags );
    else
        pushSchEdit( aMessage, aCommitFlags );
}


EDA_ITEM* SCHEMATIC_COMMIT::parentObject( EDA_ITEM* aItem ) const
{
    if( SCH_SYMBOL* parentSymbol = dyn_cast<SCH_SYMBOL*>( aItem->GetParent() ) )
        return parentSymbol;

    if( LIB_SYMBOL* parentSymbol = dyn_cast<LIB_SYMBOL*>( aItem->GetParent() ) )
        return parentSymbol;

    if( m_isLibEditor )
        return static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() )->GetCurSymbol();

    return aItem;
}


EDA_ITEM* SCHEMATIC_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    if( m_isLibEditor )
    {
        SYMBOL_EDIT_FRAME* frame = static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
        return new LIB_SYMBOL( *frame->GetCurSymbol() );
    }

    return aItem->Clone();
}


void SCHEMATIC_COMMIT::revertLibEdit()
{
    // The first element in the commit is the original, and libedit
    // just saves copies of the whole symbol, so grab the original and discard the rest
    SYMBOL_EDIT_FRAME*  sym_frame = static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    LIB_SYMBOL*         sym = static_cast<LIB_SYMBOL*>( m_changes.front().m_item );

    sym_frame->SetCurSymbol( sym,  false );

    for( size_t ii = 1; ii < m_changes.size(); ++ii )
        delete m_changes[ii].m_item;

    clear();
}


void SCHEMATIC_COMMIT::Revert()
{
    PICKED_ITEMS_LIST undoList;
    KIGFX::VIEW*      view = m_toolMgr->GetView();

    if( m_changes.empty() )
        return;

    if( m_isLibEditor )
    {
        revertLibEdit();
        return;
    }

    SCH_EDIT_FRAME* frame = static_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    SCHEMATIC&      schematic = frame->Schematic();

    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> itemsChanged;

    for( auto it = m_changes.rbegin(); it != m_changes.rend(); ++it )
    {
        COMMIT_LINE& ent = *it;
        SCH_ITEM* item = static_cast<SCH_ITEM*>( ent.m_item );
        SCH_ITEM* copy = static_cast<SCH_ITEM*>( ent.m_copy );
        SCH_SCREEN* screen = static_cast<SCH_SCREEN*>( ent.m_screen );
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;

        switch( changeType )
        {
        case CHT_ADD:
            if( !( changeFlags & CHT_DONE ) )
                break;

            view->Remove( item );
            screen->Remove( item );
            bulkRemovedItems.push_back( item );
            break;

        case CHT_REMOVE:
            item->SetConnectivityDirty();

            if( !( changeFlags & CHT_DONE ) )
                break;

            view->Add( item );
            screen->Append( item );
            bulkAddedItems.push_back( item );
            break;

        case CHT_MODIFY:
        {
            view->Remove( item );
            item->SwapData( copy );
            item->SetConnectivityDirty();

            // Special cases for items which have instance data
            if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T
                    && item->Type() == SCH_FIELD_T )
            {
                SCH_FIELD*  field = static_cast<SCH_FIELD*>( item );
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item->GetParent() );

                if( field->GetId() == REFERENCE_FIELD )
                {
                    symbol->SetRef( schematic.GetSheets().FindSheetForScreen( screen ),
                                    field->GetText() );
                }
            }

            view->Add( item );

            delete copy;
            break;
        }

        default:
            wxASSERT( false );
            break;
        }
    }

    if( bulkAddedItems.size() > 0 )
        schematic.OnItemsAdded( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        schematic.OnItemsRemoved( bulkRemovedItems );

    if( itemsChanged.size() > 0 )
        schematic.OnItemsChanged( itemsChanged );

    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    selTool->RebuildSelection();

    frame->RecalculateConnections( nullptr, NO_CLEANUP );

    clear();
}

