/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/sch_tool_base.h>

#include <lib_symbol.h>

#include <sch_group.h>
#include <sch_screen.h>
#include <schematic.h>

#include <view/view.h>
#include <sch_commit.h>
#include <connection_graph.h>

#include <functional>
#include <wx/log.h>


SCH_COMMIT::SCH_COMMIT( TOOL_MANAGER* aToolMgr ) :
        COMMIT(),
        m_toolMgr( aToolMgr ),
        m_isLibEditor( false )
{
    SCH_BASE_FRAME* frame = static_cast<SCH_BASE_FRAME*>( m_toolMgr->GetToolHolder() );
    m_isLibEditor = frame && frame->IsType( FRAME_SCH_SYMBOL_EDITOR );
}


SCH_COMMIT::SCH_COMMIT( SCH_TOOL_BASE<SCH_BASE_FRAME>* aTool )
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


COMMIT& SCH_COMMIT::Stage( EDA_ITEM *aItem, CHANGE_TYPE aChangeType, BASE_SCREEN *aScreen,
                           RECURSE_MODE aRecurse )
{
    wxCHECK( aItem, *this );

    if( aRecurse == RECURSE_MODE::RECURSE )
    {
        if( SCH_GROUP* group = dynamic_cast<SCH_GROUP*>( aItem ) )
        {
            for( EDA_ITEM* member : group->GetItems() )
                Stage( member, aChangeType, aScreen, aRecurse );
        }
    }

    // IS_SELECTED flag should not be set on undo items which were added for a drag operation.
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


void SCH_COMMIT::pushLibEdit( const wxString& aMessage, int aCommitFlags )
{
    // Symbol editor just saves copies of the whole symbol, so grab the first and discard the rest
    LIB_SYMBOL* symbol = dynamic_cast<LIB_SYMBOL*>( m_entries.front().m_item );
    LIB_SYMBOL* copy = dynamic_cast<LIB_SYMBOL*>( m_entries.front().m_copy );

    if( symbol )
    {
        if( KIGFX::VIEW* view = m_toolMgr->GetView() )
        {
            view->Update( symbol );

            symbol->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        view->Update( aChild );
                    },
                    RECURSE_MODE::NO_RECURSE );
        }

        if( SYMBOL_EDIT_FRAME* frame = static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() ) )
        {
            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                if( copy )
                {
                    frame->PushSymbolToUndoList( aMessage, copy );
                    copy = nullptr;   // we've transferred ownership to the undo stack
                }
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
}


void SCH_COMMIT::pushSchEdit( const wxString& aMessage, int aCommitFlags )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST   undoList;
    KIGFX::VIEW*        view = m_toolMgr->GetView();

    SCH_EDIT_FRAME*     frame = static_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_GROUP*          enteredGroup = selTool ? selTool->GetEnteredGroup() : nullptr;
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

    auto updateConnectivityFlag =
            [&]( SCH_ITEM* schItem )
            {
                if( schItem->IsConnectable() || ( schItem->Type() == SCH_RULE_AREA_T ) )
                {
                    dirtyConnectivity = true;

                    // Do a local clean up if there are any connectable objects in the commit.
                    if( connectivityCleanUp == NO_CLEANUP )
                        connectivityCleanUp = LOCAL_CLEANUP;

                    // Do a full rebuild of the connectivity if there is a sheet in the commit.
                    if( schItem->Type() == SCH_SHEET_T )
                        connectivityCleanUp = GLOBAL_CLEANUP;
                }
            };

    // We don't know that anything will be added to the entered group, but it does no harm to
    // add it to the commit anyway.
    if( enteredGroup )
        Modify( enteredGroup, frame->GetScreen() );

    // Handle wires with Hop Over shapes:
    for( COMMIT_LINE& entry : m_entries )
    {
        SCH_ITEM* schCopyItem = dynamic_cast<SCH_ITEM*>( entry.m_copy );
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );

        if( schCopyItem && schCopyItem->Type() == SCH_LINE_T )
            frame->UpdateHopOveredWires( schCopyItem );

        if( schItem && schItem->Type() == SCH_LINE_T )
            frame->UpdateHopOveredWires( schItem );
    }


    for( COMMIT_LINE& entry : m_entries )
    {
        SCH_ITEM* schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );
        int       changeType = entry.m_type & CHT_TYPE;

        wxCHECK2( schItem, continue );

        if( changeType == CHT_REMOVE && schItem->GetParentGroup() )
            Modify( schItem->GetParentGroup()->AsEdaItem(), entry.m_screen );
    }

    for( COMMIT_LINE& entry : m_entries )
    {
        int         changeType = entry.m_type & CHT_TYPE;
        int         changeFlags = entry.m_type & CHT_FLAGS;
        SCH_ITEM*   schItem = dynamic_cast<SCH_ITEM*>( entry.m_item );
        SCH_SCREEN* screen = dynamic_cast<SCH_SCREEN*>( entry.m_screen );

        wxCHECK2( schItem, continue );
        wxCHECK2( screen, continue );

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
                    },
                    RECURSE_MODE::NO_RECURSE );
        }

        switch( changeType )
        {
        case CHT_ADD:
        {
            if( enteredGroup && schItem->IsGroupableType() && !schItem->GetParentGroup() )
                selTool->GetEnteredGroup()->AddItem( schItem );

            updateConnectivityFlag( schItem );

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
            updateConnectivityFlag( schItem );

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( screen, schItem, UNDO_REDO::DELETED );
                itemWrapper.SetLink( entry.m_copy );
                entry.m_copy = nullptr;   // We've transferred ownership to the undo list
                undoList.PushItem( itemWrapper );
            }

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

            if( EDA_GROUP* group = schItem->GetParentGroup() )
                group->RemoveItem( schItem );

            if( !( changeFlags & CHT_DONE ) )
            {
                screen->Remove( schItem );

                if( view )
                    view->Remove( schItem );
            }

            if( frame )
                frame->UpdateItem( schItem, true, true );

            if( schItem->Type() == SCH_SHEET_T )
                refreshHierarchy = true;

            bulkRemovedItems.push_back( schItem );
            break;
        }

        case CHT_MODIFY:
        {
            const SCH_ITEM* itemCopy = static_cast<const SCH_ITEM*>( entry.m_copy );
            SCH_SHEET_PATH  currentSheet;

            if( frame )
                currentSheet = frame->GetCurrentSheet();

            if( itemCopy->HasConnectivityChanges( schItem, &currentSheet )
                || ( itemCopy->Type() == SCH_RULE_AREA_T ) )
            {
                updateConnectivityFlag( schItem );
            }

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( screen, schItem, UNDO_REDO::CHANGED );
                itemWrapper.SetLink( entry.m_copy );
                entry.m_copy = nullptr;   // We've transferred ownership to the undo list
                undoList.PushItem( itemWrapper );
            }

            if( schItem->Type() == SCH_SHEET_T )
            {
                const SCH_SHEET* modifiedSheet = static_cast<const SCH_SHEET*>( schItem );
                const SCH_SHEET* originalSheet = static_cast<const SCH_SHEET*>( itemCopy );
                wxCHECK2( modifiedSheet && originalSheet, continue );

                if( originalSheet->HasPageNumberChanges( *modifiedSheet ) )
                    refreshHierarchy = true;
            }

            if( frame )
                frame->UpdateItem( schItem, false, true );

            itemsChanged.push_back( schItem );
            break;
        }

        default:
            wxASSERT( false );
            break;
        }

        // Delete any copies we still have ownership of
        delete entry.m_copy;
        entry.m_copy = nullptr;

        // Clear all flags but SELECTED and others used to move and rotate commands,
        // after edition (selected items must keep their selection flag).
        const int selected_mask = ( SELECTED | STARTPOINT | ENDPOINT );
        schItem->ClearFlags( EDA_ITEM_ALL_FLAGS - selected_mask );

        if( schItem->Type() == SCH_SHEET_T || schItem->Type() == SCH_SYMBOL_T )
        {
            schItem->RunOnChildren(
                    [&]( SCH_ITEM* child )
                    {
                        child->ClearFlags( EDA_ITEM_ALL_FLAGS - selected_mask );
                    },
                    RECURSE_MODE::NO_RECURSE );
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

        if( refreshHierarchy )
        {
            schematic->RefreshHierarchy();

            if( frame )
                frame->UpdateHierarchyNavigator();
        }
    }

    if( !( aCommitFlags & SKIP_UNDO ) )
    {
        if( frame )
        {
            frame->SaveCopyInUndoList( undoList, UNDO_REDO::UNSPECIFIED, false );

            if( dirtyConnectivity )
            {
                wxLogTrace( wxS( "CONN_PROFILE" ),
                            wxS( "SCH_COMMIT::pushSchEdit() %s clean up connectivity rebuild." ),
                            connectivityCleanUp == LOCAL_CLEANUP ? wxS( "local" ) : wxS( "global" ) );
                frame->RecalculateConnections( this, connectivityCleanUp );
            }
        }
    }

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( itemsDeselected )
        m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

    if( selectedModified )
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
}


void SCH_COMMIT::Push( const wxString& aMessage, int aCommitFlags )
{
    if( Empty() )
        return;

    if( m_isLibEditor )
        pushLibEdit( aMessage, aCommitFlags );
    else
        pushSchEdit( aMessage, aCommitFlags );

    if( SCH_BASE_FRAME* frame = static_cast<SCH_BASE_FRAME*>( m_toolMgr->GetToolHolder() ) )
    {
        if( !( aCommitFlags & SKIP_SET_DIRTY ) )
            frame->OnModify();

        if( frame && frame->GetCanvas() )
            frame->GetCanvas()->Refresh();
    }

    clear();
}


EDA_ITEM* SCH_COMMIT::undoLevelItem( EDA_ITEM* aItem ) const
{
    EDA_ITEM* parent = aItem->GetParent();

    if( m_isLibEditor )
        return static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() )->GetCurSymbol();

    if( parent && parent->IsType( { SCH_SYMBOL_T, SCH_TABLE_T, SCH_SHEET_T, SCH_LABEL_LOCATE_ANY_T } ) )
        return parent;

    return aItem;
}


EDA_ITEM* SCH_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    if( m_isLibEditor )
    {
        SYMBOL_EDIT_FRAME* frame = static_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
        LIB_SYMBOL*        symbol = frame->GetCurSymbol();
        std::vector<KIID>  selected;

        // Cloning will clear the selected flags, but we want to keep them.
        for( const SCH_ITEM& item : symbol->GetDrawItems() )
        {
            if( item.IsSelected() )
                selected.push_back( item.m_Uuid );
        }

        symbol = new LIB_SYMBOL( *symbol );

        // Restore selected flags.
        for( SCH_ITEM& item : symbol->GetDrawItems() )
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
    SYMBOL_EDIT_FRAME*  frame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    LIB_SYMBOL*         copy = dynamic_cast<LIB_SYMBOL*>( m_entries.front().m_copy );
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();

    if( frame && copy )
    {
        frame->SetCurSymbol( copy, false );
        m_toolMgr->ResetTools( TOOL_BASE::MODEL_RELOAD );
    }

    if( selTool )
        selTool->RebuildSelection();

    clear();
}


void SCH_COMMIT::Revert()
{
    KIGFX::VIEW*        view = m_toolMgr->GetView();
    SCH_EDIT_FRAME*     frame = dynamic_cast<SCH_EDIT_FRAME*>( m_toolMgr->GetToolHolder() );
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SHEET_LIST      sheets;

    if( m_entries.empty() )
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

    for( COMMIT_LINE& ent : m_entries )
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
            wxCHECK2( copy, break );

            if( view )
                view->Remove( item );

            bool unselect = !item->IsSelected();

            item->SwapItemData( copy );

            if( unselect )
            {
                item->ClearSelected();
                item->RunOnChildren( []( SCH_ITEM* aChild )
                                     {
                                         aChild->ClearSelected();
                                     },
                                     RECURSE_MODE::NO_RECURSE );
            }

            // Special cases for items which have instance data
            if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T && item->Type() == SCH_FIELD_T )
            {
                SCH_FIELD*  field = static_cast<SCH_FIELD*>( item );
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item->GetParent() );

                if( field->GetId() == FIELD_T::REFERENCE )
                {
                    // Lazy eval of sheet list; this is expensive even when unsorted
                    if( sheets.empty() )
                        sheets = schematic->Hierarchy();

                    SCH_SHEET_PATH sheet = sheets.FindSheetForScreen( screen );
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
        frame->RecalculateConnections( nullptr, NO_CLEANUP );

    clear();
}

