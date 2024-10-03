/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_commit.h>
#include <connection_graph.h>

#include <functional>


SCH_COMMIT::SCH_COMMIT( TOOL_MANAGER* aToolMgr ) :
        m_toolMgr( aToolMgr ),
        m_isLibEditor( false )
{
    SCH_BASE_FRAME* frame = static_cast<SCH_BASE_FRAME*>( m_toolMgr->GetToolHolder() );
    m_isLibEditor = frame && frame->IsType( FRAME_SCH_SYMBOL_EDITOR );
}


SCH_COMMIT::SCH_COMMIT( EE_TOOL_BASE<SCH_BASE_FRAME>* aTool )
{
    m_toolMgr = aTool->GetManager();
    m_isLibEditor = aTool->IsSymbolEditor();
}


SCH_COMMIT::SCH_COMMIT( EDA_DRAW_FRAME* aFrame )
{
    m_toolMgr = aFrame->GetToolManager();
    m_isLibEditor = aFrame->IsType( FRAME_SCH_SYMBOL_EDITOR );
}


SCH_COMMIT::~SCH_COMMIT()
{
}


COMMIT& SCH_COMMIT::Stage( EDA_ITEM *aItem, CHANGE_TYPE aChangeType, BASE_SCREEN *aScreen )
{
    wxCHECK( aItem, *this );

    // If aItem belongs a symbol, sheet or label, the full parent will be saved because undo/redo
    // does not handle "sub items" modifications.
    if( aItem->Type() != SCH_SHEET_T
            && aItem->GetParent() && aItem->GetParent()->IsType( { SCH_SYMBOL_T, LIB_SYMBOL_T,
                                                                   SCH_SHEET_T,
                                                                   SCH_LABEL_LOCATE_ANY_T } ) )
    {
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


COMMIT& SCH_COMMIT::Stage( std::vector<EDA_ITEM*> &container, CHANGE_TYPE aChangeType,
                           BASE_SCREEN *aScreen )
{
    for( EDA_ITEM* item : container )
        Stage( item, aChangeType, aScreen );

    return *this;
}


COMMIT& SCH_COMMIT::Stage( const PICKED_ITEMS_LIST &aItems, UNDO_REDO aModFlag,
                           BASE_SCREEN *aScreen )
{
    return COMMIT::Stage( aItems, aModFlag, aScreen );
}


void SCH_COMMIT::pushLibEdit( const wxString& aMessage, int aCommitFlags )
{
    KIGFX::VIEW*       view = m_toolMgr->GetView();
    SYMBOL_EDIT_FRAME* frame = static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );

    if( Empty() )
        return;

    // Symbol editor just saves copies of the whole symbol, so grab the first and discard the rest
    LIB_SYMBOL* symbol = dynamic_cast<LIB_SYMBOL*>( m_changes.front().m_item );
    LIB_SYMBOL* copy = dynamic_cast<LIB_SYMBOL*>( m_changes.front().m_copy );

    if( symbol )
    {
        if( view )
        {
            view->Update( symbol );

            symbol->RunOnChildren(
                    [&]( LIB_ITEM* aChild )
                    {
                        view->Update( aChild );
                    });
        }

        if( !( aCommitFlags & SKIP_UNDO ) )
        {
            if( frame && copy )
            {
                frame->PushSymbolToUndoList( aMessage, copy );
                copy = nullptr;   // we've transferred ownership to the undo stack
            }
        }

        if( copy )
        {
            // if no undo entry was needed, the copy would create a memory leak
            delete copy;
            copy = nullptr;
        }
    }

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( !( aCommitFlags & SKIP_SET_DIRTY ) )
    {
        if( frame )
            frame->OnModify();
    }

    for( size_t ii = 1; ii < m_changes.size(); ++ii )
        delete m_changes[ii].m_copy;

    clear();
}


void SCH_COMMIT::pushSchEdit( const wxString& aMessage, int aCommitFlags )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST   undoList;
    KIGFX::VIEW*        view = m_toolMgr->GetView();

    SCH_EDIT_FRAME*     frame = static_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    EE_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    bool                itemsDeselected = false;
    bool                selectedModified = false;
    bool                dirtyConnectivity = false;
    bool                refreshHierarchy = false;
    SCH_CLEANUP_FLAGS   connectivityCleanUp = NO_CLEANUP;

    if( Empty() )
        return;

    undoList.SetDescription( aMessage );

    SCHEMATIC*             schematic = nullptr;
    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> itemsChanged;

    for( COMMIT_LINE& ent : m_changes )
    {
        int         changeType = ent.m_type & CHT_TYPE;
        int         changeFlags = ent.m_type & CHT_FLAGS;
        SCH_ITEM*   schItem = dynamic_cast<SCH_ITEM*>( ent.m_item );
        SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( ent.m_screen );

        wxCHECK2( schItem && screen, continue );

        if( !schematic )
            schematic = schItem->Schematic();

        if( schItem->IsSelected() )
        {
            selectedModified = true;
        }
        else
        {
            schItem->RunOnChildren(
                    [&selectedModified]( SCH_ITEM* aChild )
                    {
                        if( aChild->IsSelected() )
                            selectedModified = true;
                    } );
        }

        auto updateConnectivityFlag = [&]()
        {
            if( schItem->IsConnectable() )
            {
                dirtyConnectivity = true;

                // Do a local clean up if there are any connectable objects in the commit.
                if( connectivityCleanUp == NO_CLEANUP )
                    connectivityCleanUp = LOCAL_CLEANUP;

                // Do a full rebauild of the connectivity if there is a sheet in the commit.
                if( schItem->Type() == SCH_SHEET_T )
                    connectivityCleanUp = GLOBAL_CLEANUP;
            }
        };

        switch( changeType )
        {
        case CHT_ADD:
        {
            updateConnectivityFlag();

            if( !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( screen, schItem, UNDO_REDO::NEWITEM ) );

            if( !( changeFlags & CHT_DONE ) )
            {
                if( !screen->CheckIfOnDrawList( schItem ) )  // don't want a loop!
                    screen->Append( schItem );

                if( view )
                    view->Add( schItem );
            }

            if( frame )
                frame->UpdateItem( schItem, true, true );

            bulkAddedItems.push_back( schItem );

            if( schItem->Type() == SCH_SHEET_T )
                refreshHierarchy = true;

            break;
        }

        case CHT_REMOVE:
        {
            updateConnectivityFlag();

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
                screen->Remove( schItem );

                if( view )
                    view->Remove( schItem );
            }

            if( frame )
                frame->UpdateItem( schItem, true, true );

            bulkRemovedItems.push_back( schItem );

            if( schItem->Type() == SCH_SHEET_T )
                refreshHierarchy = true;

            break;
        }

        case CHT_MODIFY:
        {
            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( screen, schItem, UNDO_REDO::CHANGED );
                wxASSERT( ent.m_copy );
                itemWrapper.SetLink( ent.m_copy );

                const SCH_ITEM* itemCopy = static_cast<const SCH_ITEM*>( ent.m_copy );

                wxCHECK2( itemCopy, continue );

                SCH_SHEET_PATH currentSheet;

                if( frame )
                    currentSheet = frame->GetCurrentSheet();

                if( itemCopy->HasConnectivityChanges( schItem, &currentSheet ) )
                    updateConnectivityFlag();


                if( schItem->Type() == SCH_SHEET_T )
                {
                    const SCH_SHEET* modifiedSheet = static_cast<const SCH_SHEET*>( schItem );
                    const SCH_SHEET* originalSheet = static_cast<const SCH_SHEET*>( itemCopy );
                    wxCHECK2( modifiedSheet && originalSheet, continue );

                    if( originalSheet->HasPageNumberChanges( *modifiedSheet ) )
                        refreshHierarchy = true;
                }

                undoList.PushItem( itemWrapper );
                ent.m_copy = nullptr;   // We've transferred ownership to the undo list
            }

            if( frame )
                frame->UpdateItem( schItem, false, true );

            itemsChanged.push_back( schItem );

            if( ent.m_copy )
            {
                // if no undo entry is needed, the copy would create a memory leak
                delete ent.m_copy;
                ent.m_copy = nullptr;
            }

            break;
        }

        default:
            wxASSERT( false );
            break;
        }

        // Clear all flags but SELECTED and others used to move and rotate commands,
        // after edition (selected items must keep their selection flag).
        const int selected_mask = ( SELECTED | STARTPOINT | ENDPOINT );
        schItem->ClearFlags( EDA_ITEM_ALL_FLAGS - selected_mask );
    }

    if( schematic )
    {
        if( bulkAddedItems.size() > 0 )
            schematic->OnItemsAdded( bulkAddedItems );

        if( bulkRemovedItems.size() > 0 )
            schematic->OnItemsRemoved( bulkRemovedItems );

        if( itemsChanged.size() > 0 )
            schematic->OnItemsChanged( itemsChanged );

        if( frame && refreshHierarchy )
            frame->UpdateHierarchyNavigator();
    }

    if( !( aCommitFlags & SKIP_UNDO ) )
    {
        if( frame )
        {
            frame->SaveCopyInUndoList( undoList, UNDO_REDO::UNSPECIFIED, false, dirtyConnectivity );

            if( dirtyConnectivity )
            {
                wxLogTrace( wxS( "CONN_PROFILE" ),
                            wxS( "SCH_COMMIT::pushSchEdit() %s clean up connectivity rebuild." ),
                            ( connectivityCleanUp == LOCAL_CLEANUP ) ? wxS( "local" ) : wxS( "global" ) );
                frame->RecalculateConnections( this, connectivityCleanUp );
            }
        }
    }

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( itemsDeselected )
        m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

    if( selectedModified )
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( frame && frame->GetCanvas() )
        frame->GetCanvas()->Refresh();

    if( !( aCommitFlags & SKIP_SET_DIRTY ) )
    {
        if( frame )
            frame->OnModify();
    }

    clear();
}


void SCH_COMMIT::Push( const wxString& aMessage, int aCommitFlags )
{
    if( m_isLibEditor )
        pushLibEdit( aMessage, aCommitFlags );
    else
        pushSchEdit( aMessage, aCommitFlags );
}


EDA_ITEM* SCH_COMMIT::parentObject( EDA_ITEM* aItem ) const
{
    EDA_ITEM* parent = aItem->GetParent();

    if( parent && parent->Type() == SCH_SYMBOL_T )
        return parent;

    if( parent && parent->Type() == LIB_SYMBOL_T )
        return parent;

    if( m_isLibEditor )
        return static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() )->GetCurSymbol();

    return aItem;
}


EDA_ITEM* SCH_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    if( m_isLibEditor )
    {
        SYMBOL_EDIT_FRAME* frame = static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
        LIB_SYMBOL*        symbol = frame->GetCurSymbol();
        std::vector<KIID>  selected;

        for( const LIB_ITEM& item : symbol->GetDrawItems() )
        {
            if( item.IsSelected() )
                selected.push_back( item.m_Uuid );
        }

        symbol = new LIB_SYMBOL( *symbol );

        for( LIB_ITEM& item : symbol->GetDrawItems() )
        {
            if( alg::contains( selected, item.m_Uuid ) )
                item.SetSelected();
        }

        return symbol;
    }

    return aItem->Clone();
}


void SCH_COMMIT::revertLibEdit()
{
    if( Empty() )
        return;

    // Symbol editor just saves copies of the whole symbol, so grab the first and discard the rest
    SYMBOL_EDIT_FRAME* frame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    LIB_SYMBOL*        copy = dynamic_cast<LIB_SYMBOL*>( m_changes.front().m_copy );
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();

    if( frame && copy )
    {
        frame->SetCurSymbol( copy, false );
        m_toolMgr->ResetTools( TOOL_BASE::MODEL_RELOAD );
    }

    for( size_t ii = 1; ii < m_changes.size(); ++ii )
        delete m_changes[ii].m_copy;

    if( selTool )
        selTool->RebuildSelection();

    clear();
}


void SCH_COMMIT::Revert()
{
    KIGFX::VIEW*       view = m_toolMgr->GetView();
    SCH_EDIT_FRAME*    frame = dynamic_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();

    if( m_changes.empty() )
        return;

    if( m_isLibEditor )
    {
        revertLibEdit();
        return;
    }

    SCHEMATIC*             schematic = nullptr;
    std::vector<SCH_ITEM*> bulkAddedItems;
    std::vector<SCH_ITEM*> bulkRemovedItems;
    std::vector<SCH_ITEM*> itemsChanged;

    for( COMMIT_LINE& ent : m_changes )
    {
        int         changeType = ent.m_type & CHT_TYPE;
        int         changeFlags = ent.m_type & CHT_FLAGS;
        SCH_ITEM*   item = dynamic_cast<SCH_ITEM*>( ent.m_item );
        SCH_ITEM*   copy = dynamic_cast<SCH_ITEM*>( ent.m_copy );
        SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( ent.m_screen );

        wxCHECK2( item && screen, continue );

        if( !schematic )
            schematic = item->Schematic();

        switch( changeType )
        {
        case CHT_ADD:
            if( !( changeFlags & CHT_DONE ) )
                break;

            if( view )
                view->Remove( item );

            screen->Remove( item );
            bulkRemovedItems.push_back( item );
            break;

        case CHT_REMOVE:
            item->SetConnectivityDirty();

            if( !( changeFlags & CHT_DONE ) )
                break;

            if( view )
                view->Add( item );

            screen->Append( item );
            bulkAddedItems.push_back( item );
            break;

        case CHT_MODIFY:
        {
            if( view )
                view->Remove( item );

            bool unselect = !item->IsSelected();

            item->SwapData( copy );

            if( unselect )
            {
                item->ClearSelected();
                item->RunOnChildren( []( SCH_ITEM* aChild ) { aChild->ClearSelected(); } );
            }

            // Special cases for items which have instance data
            if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T
                    && item->Type() == SCH_FIELD_T )
            {
                SCH_FIELD*  field = static_cast<SCH_FIELD*>( item );
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item->GetParent() );

                if( field->GetId() == REFERENCE_FIELD )
                {
                    SCH_SHEET_PATH sheet = schematic->GetSheets().FindSheetForScreen( screen );
                    symbol->SetRef( &sheet, field->GetText() );
                }
            }

            // This must be called before any calls that require stable object pointers.
            screen->Update( item );

            // This hack is to prevent incorrectly parented symbol pins from breaking the
            // connectivity algorithm.
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                symbol->UpdatePins();

                CONNECTION_GRAPH* graph = schematic->ConnectionGraph();

                SCH_SYMBOL* symbolCopy = static_cast<SCH_SYMBOL*>( copy );
                graph->RemoveItem( symbolCopy );

                for( SCH_PIN* pin : symbolCopy->GetPins() )
                    graph->RemoveItem( pin );
            }

            item->SetConnectivityDirty();

            if( view )
                view->Add( item );

            delete copy;
            break;
        }

        default:
            wxASSERT( false );
            break;
        }
    }

    if( schematic )
    {
        if( bulkAddedItems.size() > 0 )
            schematic->OnItemsAdded( bulkAddedItems );

        if( bulkRemovedItems.size() > 0 )
            schematic->OnItemsRemoved( bulkRemovedItems );

        if( itemsChanged.size() > 0 )
            schematic->OnItemsChanged( itemsChanged );
    }

    if( selTool )
        selTool->RebuildSelection();

    if( frame )
    {
        frame->RecalculateConnections( nullptr, NO_CLEANUP );
    }

    clear();
}

