/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <board.h>
#include <footprint.h>
#include <pcb_edit_frame.h>
#include <pcb_group.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <view/view.h>
#include <board_commit.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_actions.h>
#include <connectivity/connectivity_data.h>

#include <functional>
using namespace std::placeholders;


BOARD_COMMIT::BOARD_COMMIT( TOOL_MANAGER* aToolMgr ) :
        m_toolMgr( aToolMgr ),
        m_isFootprintEditor( false ),
        m_resolveNetConflicts( false )
{
}


BOARD_COMMIT::BOARD_COMMIT( PCB_TOOL_BASE* aTool ) :
        m_resolveNetConflicts( false )
{
    m_toolMgr = aTool->GetManager();
    m_isFootprintEditor = aTool->IsFootprintEditor();
}


BOARD_COMMIT::BOARD_COMMIT( EDA_DRAW_FRAME* aFrame ) :
        m_resolveNetConflicts( false )
{
    m_toolMgr = aFrame->GetToolManager();
    m_isFootprintEditor = aFrame->IsType( FRAME_FOOTPRINT_EDITOR );
}


BOARD_COMMIT::~BOARD_COMMIT()
{
}


COMMIT& BOARD_COMMIT::Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType )
{
    // if aItem belongs a footprint, the full footprint will be saved
    // because undo/redo does not handle "sub items" modifications
    if( aItem && aItem->Type() != PCB_FOOTPRINT_T && aChangeType == CHT_MODIFY )
    {
        EDA_ITEM* item = aItem->GetParent();

        if( item && item->Type() == PCB_FOOTPRINT_T )  // means aItem belongs a footprint
            aItem = item;
    }

    return COMMIT::Stage( aItem, aChangeType );
}


COMMIT& BOARD_COMMIT::Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType )
{
    return COMMIT::Stage( container, aChangeType );
}


COMMIT& BOARD_COMMIT::Stage( const PICKED_ITEMS_LIST& aItems, UNDO_REDO aModFlag )
{
    return COMMIT::Stage( aItems, aModFlag );
}


void BOARD_COMMIT::Push( const wxString& aMessage, bool aCreateUndoEntry, bool aSetDirtyBit )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST   undoList;
    KIGFX::VIEW*        view = m_toolMgr->GetView();
    BOARD*              board = (BOARD*) m_toolMgr->GetModel();
    PCB_BASE_FRAME*     frame = (PCB_BASE_FRAME*) m_toolMgr->GetToolHolder();
    auto                connectivity = board->GetConnectivity();
    std::set<EDA_ITEM*> savedModules;
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    bool                itemsDeselected = false;

    std::vector<BOARD_ITEM*> bulkAddedItems;
    std::vector<BOARD_ITEM*> bulkRemovedItems;
    std::vector<BOARD_ITEM*> itemsChanged;

    if( Empty() )
        return;

    for( COMMIT_LINE& ent : m_changes )
    {
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( ent.m_item );

        // Module items need to be saved in the undo buffer before modification
        if( m_isFootprintEditor )
        {
            // Be sure that we are storing a footprint
            if( ent.m_item->Type() != PCB_FOOTPRINT_T )
                ent.m_item = ent.m_item->GetParent();

            // We have not saved the footprint yet, so let's create an entry
            if( savedModules.count( ent.m_item ) == 0 )
            {
                if( !ent.m_copy )
                {
                    wxASSERT( changeType != CHT_MODIFY );     // too late to make a copy..
                    ent.m_copy = ent.m_item->Clone();
                }

                wxASSERT( ent.m_item->Type() == PCB_FOOTPRINT_T );
                wxASSERT( ent.m_copy->Type() == PCB_FOOTPRINT_T );

                if( aCreateUndoEntry )
                {
                    ITEM_PICKER itemWrapper( nullptr, ent.m_item, UNDO_REDO::CHANGED );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                    frame->SaveCopyInUndoList( undoList, UNDO_REDO::CHANGED );
                }

                savedModules.insert( ent.m_item );
                static_cast<FOOTPRINT*>( ent.m_item )->SetLastEditTime();
            }
        }

        switch( changeType )
        {
            case CHT_ADD:
            {
                if( m_isFootprintEditor )
                {
                    // footprints inside footprints are not supported yet
                    wxASSERT( boardItem->Type() != PCB_FOOTPRINT_T );

                    boardItem->SetParent( board->Footprints().front() );

                    if( !( changeFlags & CHT_DONE ) )
                        board->Footprints().front()->Add( boardItem );
                }
                else if( boardItem->Type() == PCB_PAD_T ||
                         boardItem->Type() == PCB_FP_TEXT_T ||
                         boardItem->Type() == PCB_FP_SHAPE_T ||
                         boardItem->Type() == PCB_FP_ZONE_T )
                {
                    wxASSERT( boardItem->GetParent() &&
                              boardItem->GetParent()->Type() == PCB_FOOTPRINT_T );
                }
                else
                {
                    if( aCreateUndoEntry )
                        undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::NEWITEM ) );

                    if( !( changeFlags & CHT_DONE ) )
                    {
                        board->Add( boardItem, ADD_MODE::BULK_INSERT ); // handles connectivity
                        bulkAddedItems.push_back( boardItem );
                    }
                }

                if( view && boardItem->Type() != PCB_NETINFO_T )
                    view->Add( boardItem );

                break;
            }

            case CHT_REMOVE:
            {
                PCB_GROUP* parentGroup = boardItem->GetParentGroup();

                if( !m_isFootprintEditor && aCreateUndoEntry )
                    undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::DELETED ) );

                if( boardItem->IsSelected() )
                {
                    selTool->RemoveItemFromSel( boardItem, true /* quiet mode */ );
                    itemsDeselected = true;
                }

                switch( boardItem->Type() )
                {
                // Footprint items
                case PCB_PAD_T:
                case PCB_FP_SHAPE_T:
                case PCB_FP_TEXT_T:
                case PCB_FP_ZONE_T:
                    // This level can only handle footprint children in the footprint editor as
                    // only in that case has the entire footprint (and all its children) already
                    // been saved for undo.
                    wxASSERT( m_isFootprintEditor );

                    if( boardItem->Type() == PCB_FP_TEXT_T )
                    {
                        FP_TEXT* text = static_cast<FP_TEXT*>( boardItem );

                        // don't allow deletion of Reference or Value
                        if( text->GetType() != FP_TEXT::TEXT_is_DIVERS )
                            break;
                    }

                    if( parentGroup && !( parentGroup->GetFlags() & STRUCT_DELETED ) )
                        parentGroup->RemoveItem( boardItem );

                    if( view )
                        view->Remove( boardItem );

                    if( !( changeFlags & CHT_DONE ) )
                    {
                        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( boardItem->GetParent() );
                        wxASSERT( footprint && footprint->Type() == PCB_FOOTPRINT_T );
                        footprint->Delete( boardItem );
                    }

                    break;

                // Board items
                case PCB_SHAPE_T:            // a shape (normally not on copper layers)
                case PCB_TEXT_T:             // a text on a layer
                case PCB_TRACE_T:            // a track segment (segment on a copper layer)
                case PCB_ARC_T:              // an arced track segment (segment on a copper layer)
                case PCB_VIA_T:              // a via (like track segment on a copper layer)
                case PCB_DIM_ALIGNED_T:      // a dimension (graphic item)
                case PCB_DIM_CENTER_T:
                case PCB_DIM_ORTHOGONAL_T:
                case PCB_DIM_LEADER_T:       // a leader dimension
                case PCB_TARGET_T:           // a target (graphic item)
                case PCB_MARKER_T:           // a marker used to show something
                case PCB_ZONE_T:
                    if( view )
                        view->Remove( boardItem );

                    if( !( changeFlags & CHT_DONE ) )
                    {
                        board->Remove( boardItem, REMOVE_MODE::BULK );
                        bulkRemovedItems.push_back( boardItem );
                    }

                    break;

                case PCB_FOOTPRINT_T:
                {
                    // No support for nested footprints (yet)
                    wxASSERT( !m_isFootprintEditor );

                    FOOTPRINT* footprint = static_cast<FOOTPRINT*>( boardItem );

                    if( view )
                        view->Remove( footprint );

                    footprint->ClearFlags();

                    if( !( changeFlags & CHT_DONE ) )
                    {
                        board->Remove( footprint, REMOVE_MODE::BULK ); // handles connectivity
                        bulkRemovedItems.push_back( footprint );
                    }
                }
                break;

                case PCB_GROUP_T:
                    if( view )
                        view->Remove( boardItem );

                    if( !( changeFlags & CHT_DONE ) )
                    {
                        if( m_isFootprintEditor )
                            board->GetFirstFootprint()->Remove( boardItem );
                        else
                        {
                            board->Remove( boardItem, REMOVE_MODE::BULK );
                            bulkRemovedItems.push_back( boardItem );
                        }
                    }
                    break;

                // Metadata items
                case PCB_NETINFO_T:
                    board->Remove( boardItem, REMOVE_MODE::BULK );
                    bulkRemovedItems.push_back( boardItem );
                    break;

                default:                        // other types do not need to (or should not) be handled
                    wxASSERT( false );
                    break;
                }

                break;
            }

            case CHT_MODIFY:
            {
                if( !m_isFootprintEditor && aCreateUndoEntry )
                {
                    ITEM_PICKER itemWrapper( nullptr, boardItem, UNDO_REDO::CHANGED );
                    wxASSERT( ent.m_copy );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                }

                if( ent.m_copy )
                    connectivity->MarkItemNetAsDirty( static_cast<BOARD_ITEM*>( ent.m_copy ) );

                connectivity->Update( boardItem );

                if( view )
                {
                    view->Update( boardItem );

                    if( m_isFootprintEditor )
                    {
                        static_cast<FOOTPRINT*>( boardItem )->RunOnChildren(
                                [&]( BOARD_ITEM* aChild )
                                {
                                    view->Update( aChild );
                                });
                    }
                }

                itemsChanged.push_back( boardItem );

                // if no undo entry is needed, the copy would create a memory leak
                if( !aCreateUndoEntry )
                    delete ent.m_copy;

                break;
            }

            default:
                wxASSERT( false );
                break;
        }
    }

    if( bulkAddedItems.size() > 0 )
        board->FinalizeBulkAdd( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        board->FinalizeBulkRemove( bulkRemovedItems );

    if( itemsChanged.size() > 0 )
        board->OnItemsChanged( itemsChanged );

    if( !m_isFootprintEditor )
    {
        size_t num_changes = m_changes.size();

        if( m_resolveNetConflicts )
            connectivity->PropagateNets( this, PROPAGATE_MODE::RESOLVE_CONFLICTS );

        connectivity->RecalculateRatsnest( this );
        connectivity->ClearDynamicRatsnest();

        if( frame )
            frame->GetCanvas()->RedrawRatsnest();

        if( m_changes.size() > num_changes )
        {
            for( size_t i = num_changes; i < m_changes.size(); ++i )
            {
                COMMIT_LINE& ent = m_changes[i];

                // This should only be modifications from the connectivity algo
                wxASSERT( ( ent.m_type & CHT_TYPE ) == CHT_MODIFY );

                BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( ent.m_item );

                if( aCreateUndoEntry )
                {
                    ITEM_PICKER itemWrapper( nullptr, boardItem, UNDO_REDO::CHANGED );
                    wxASSERT( ent.m_copy );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                }
                else
                {
                    delete ent.m_copy;
                }

                if( view )
                    view->Update( boardItem );
            }
        }
    }

    if( !m_isFootprintEditor && aCreateUndoEntry )
        frame->SaveCopyInUndoList( undoList, UNDO_REDO::UNSPECIFIED );

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( itemsDeselected )
        m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

    if( aSetDirtyBit )
        frame->OnModify();
    else if( frame )
        frame->Update3DView( true, frame->GetDisplayOptions().m_Live3DRefresh );

    clear();
}


EDA_ITEM* BOARD_COMMIT::parentObject( EDA_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
        case PCB_PAD_T:
        case PCB_FP_SHAPE_T:
        case PCB_FP_TEXT_T:
        case PCB_FP_ZONE_T:
            return aItem->GetParent();

        case PCB_ZONE_T:
            wxASSERT( !dynamic_cast<FOOTPRINT*>( aItem->GetParent() ) );
            return aItem;

        default:
            break;
    }

    return aItem;
}


void BOARD_COMMIT::Revert()
{
    PICKED_ITEMS_LIST undoList;
    KIGFX::VIEW* view = m_toolMgr->GetView();
    BOARD* board = (BOARD*) m_toolMgr->GetModel();
    auto connectivity = board->GetConnectivity();

    std::vector<BOARD_ITEM*> bulkAddedItems;
    std::vector<BOARD_ITEM*> bulkRemovedItems;
    std::vector<BOARD_ITEM*> itemsChanged;

    for( auto it = m_changes.rbegin(); it != m_changes.rend(); ++it )
    {
        COMMIT_LINE& ent = *it;
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( ent.m_item );
        BOARD_ITEM* copy = static_cast<BOARD_ITEM*>( ent.m_copy );
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;

        switch( changeType )
        {
        case CHT_ADD:
            if( !( changeFlags & CHT_DONE ) )
                break;

            view->Remove( item );
            connectivity->Remove( item );
            board->Remove( item, REMOVE_MODE::BULK );
            bulkRemovedItems.push_back( item );
            break;

        case CHT_REMOVE:
            if( !( changeFlags & CHT_DONE ) )
                break;

            view->Add( item );
            connectivity->Add( item );
            board->Add( item, ADD_MODE::INSERT );
            bulkAddedItems.push_back( item );
            break;

        case CHT_MODIFY:
        {
            view->Remove( item );
            connectivity->Remove( item );

            item->SwapData( copy );

            view->Add( item );
            connectivity->Add( item );
            board->OnItemChanged( item );
            itemsChanged.push_back( item );

            delete copy;
            break;
        }

        default:
            wxASSERT( false );
            break;
        }
    }

    if( bulkAddedItems.size() > 0 )
        board->FinalizeBulkAdd( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        board->FinalizeBulkRemove( bulkRemovedItems );

    if( itemsChanged.size() > 0 )
        board->OnItemsChanged( itemsChanged );

    if ( !m_isFootprintEditor )
        connectivity->RecalculateRatsnest();

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    selTool->RebuildSelection();

    clear();
}

