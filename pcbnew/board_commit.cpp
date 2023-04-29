/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <macros.h>
#include <board.h>
#include <footprint.h>
#include <pcb_group.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>
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
        m_isBoardEditor( false )
{
}


BOARD_COMMIT::BOARD_COMMIT( PCB_TOOL_BASE* aTool )
{
    m_toolMgr = aTool->GetManager();
    m_isFootprintEditor = aTool->IsFootprintEditor();
    m_isBoardEditor = aTool->IsBoardEditor();
}


BOARD_COMMIT::BOARD_COMMIT( EDA_DRAW_FRAME* aFrame )
{
    m_toolMgr = aFrame->GetToolManager();
    m_isFootprintEditor = aFrame->IsType( FRAME_FOOTPRINT_EDITOR );
    m_isBoardEditor = aFrame->IsType( FRAME_PCB_EDITOR );
}


BOARD_COMMIT::~BOARD_COMMIT()
{
}


BOARD* BOARD_COMMIT::GetBoard() const
{
    return static_cast<BOARD*>( m_toolMgr->GetModel() );
}


COMMIT& BOARD_COMMIT::Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType, BASE_SCREEN* aScreen )
{
    wxCHECK( aItem, *this );

    aItem->ClearFlags( IS_MODIFIED_CHILD );

    // If aItem belongs a footprint, the full footprint will be saved because undo/redo does
    // not handle "sub items" modifications.  This has implications for auto-zone-refill, so
    // we need to store a bit more information.
    if( aChangeType == CHT_MODIFY )
    {
        if( aItem->Type() == PCB_FOOTPRINT_T )
        {
            static_cast<FOOTPRINT*>( aItem )->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        child->SetFlags( IS_MODIFIED_CHILD );
                    } );
        }
        else if( aItem->GetParent() && aItem->GetParent()->Type() == PCB_FOOTPRINT_T )
        {
            if( aItem->Type() == PCB_GROUP_T )
            {
                static_cast<PCB_GROUP*>( aItem )->RunOnChildren(
                        [&]( BOARD_ITEM* child )
                        {
                            child->SetFlags( IS_MODIFIED_CHILD );
                        } );
            }
            else
            {
                aItem->SetFlags( IS_MODIFIED_CHILD );
            }

            aItem = aItem->GetParent();
        }
        else if( aItem->Type() == PCB_GROUP_T )
        {
            // Many operations on group (move, rotate, etc.) are applied directly to their
            // children, so it's the children that must be staged.
            static_cast<PCB_GROUP*>( aItem )->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        COMMIT::Stage( child, aChangeType );
                    } );
        }
    }

    return COMMIT::Stage( aItem, aChangeType );
}


COMMIT& BOARD_COMMIT::Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType, BASE_SCREEN* aScreen )
{
    return COMMIT::Stage( container, aChangeType, aScreen );
}


COMMIT& BOARD_COMMIT::Stage( const PICKED_ITEMS_LIST& aItems, UNDO_REDO aModFlag, BASE_SCREEN* aScreen )
{
    return COMMIT::Stage( aItems, aModFlag, aScreen );
}


void BOARD_COMMIT::dirtyIntersectingZones( BOARD_ITEM* item, int aChangeType )
{
    wxCHECK( item, /* void */ );

    ZONE_FILLER_TOOL* zoneFillerTool = m_toolMgr->GetTool<ZONE_FILLER_TOOL>();

    if( item->Type() == PCB_ZONE_T )
        zoneFillerTool->DirtyZone( static_cast<ZONE*>( item ) );

    if( item->Type() == PCB_FOOTPRINT_T )
    {
        static_cast<FOOTPRINT*>( item )->RunOnChildren(
                [&]( BOARD_ITEM* child )
                {
                    if( aChangeType != CHT_MODIFY || ( child->GetFlags() & IS_MODIFIED_CHILD ) )
                        dirtyIntersectingZones( child, aChangeType );

                    child->ClearFlags( IS_MODIFIED_CHILD );
                } );
    }
    else if( item->Type() == PCB_GROUP_T )
    {
        static_cast<PCB_GROUP*>( item )->RunOnChildren(
                [&]( BOARD_ITEM* child )
                {
                    dirtyIntersectingZones( child, aChangeType );
                    child->ClearFlags( IS_MODIFIED_CHILD );
                } );
    }
    else
    {
        BOARD* board = static_cast<BOARD*>( m_toolMgr->GetModel() );
        BOX2I  bbox = item->GetBoundingBox();
        LSET   layers = item->GetLayerSet();

        if( layers.test( Edge_Cuts ) || layers.test( Margin ) )
            layers = LSET::PhysicalLayersMask();
        else
            layers &= LSET::AllCuMask();

        if( layers.any() )
        {
            for( ZONE* zone : board->Zones() )
            {
                if( zone->GetIsRuleArea() )
                    continue;

                if( ( zone->GetLayerSet() & layers ).any()
                        && zone->GetBoundingBox().Intersects( bbox ) )
                {
                    zoneFillerTool->DirtyZone( zone );
                }
            }
        }

        item->ClearFlags( IS_MODIFIED_CHILD );
    }
}


void BOARD_COMMIT::Push( const wxString& aMessage, int aCommitFlags )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST   undoList;
    KIGFX::VIEW*        view = m_toolMgr->GetView();
    BOARD*              board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    PCB_BASE_FRAME*     frame = dynamic_cast<PCB_BASE_FRAME*>( m_toolMgr->GetToolHolder() );
    std::set<EDA_ITEM*> savedModules;
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    bool                itemsDeselected = false;
    bool                solderMaskDirty = false;
    bool                autofillZones = false;
    bool                selectedModified = false;

    if( Empty() )
        return;

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();

    // Note:
    // frame == nullptr happens in QA tests
    // in this case m_isBoardEditor and m_isFootprintEditor are set to false
    // But we also test frame == nullptr mainly to make Coverity happy

    std::vector<BOARD_ITEM*> bulkAddedItems;
    std::vector<BOARD_ITEM*> bulkRemovedItems;
    std::vector<BOARD_ITEM*> itemsChanged;

    if( m_isBoardEditor
            && !( aCommitFlags & ZONE_FILL_OP )
            && ( frame && frame->GetPcbNewSettings()->m_AutoRefillZones ) )
    {
        autofillZones = true;

        for( ZONE* zone : board->Zones() )
            zone->CacheBoundingBox();
    }

    for( COMMIT_LINE& ent : m_changes )
    {
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( ent.m_item );

        wxASSERT( ent.m_item );

        // Module items need to be saved in the undo buffer before modification
        if( m_isFootprintEditor )
        {
            // Be sure that we are storing a footprint
            if( ent.m_item->Type() != PCB_FOOTPRINT_T )
            {
                ent.m_item = ent.m_item->GetParent();
                wxASSERT( ent.m_item );
            }

            // We have not saved the footprint yet, so let's create an entry
            if( savedModules.count( ent.m_item ) == 0 )
            {
                if( !ent.m_copy )
                {
                    wxASSERT( changeType != CHT_MODIFY );     // too late to make a copy..
                    ent.m_copy = makeImage( ent.m_item );
                }

                wxASSERT( ent.m_item->Type() == PCB_FOOTPRINT_T );
                wxASSERT( ent.m_copy->Type() == PCB_FOOTPRINT_T );

                if( !( aCommitFlags & SKIP_UNDO ) && frame )
                {
                    ITEM_PICKER itemWrapper( nullptr, ent.m_item, UNDO_REDO::CHANGED );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                    frame->SaveCopyInUndoList( undoList, UNDO_REDO::CHANGED );
                }

                savedModules.insert( ent.m_item );
            }
        }

        if( boardItem->Type() == PCB_VIA_T || boardItem->Type() == PCB_FOOTPRINT_T
                || boardItem->IsOnLayer( F_Mask ) || boardItem->IsOnLayer( B_Mask ) )
        {
            solderMaskDirty = true;
        }

        if( boardItem->IsSelected() )
            selectedModified = true;

        // If we're the footprint editor, the boardItem will always be the containing footprint
        if( m_isFootprintEditor && boardItem->Type() == PCB_FOOTPRINT_T )
        {
            static_cast<FOOTPRINT*>( boardItem )->RunOnChildren(
                    [&selectedModified]( BOARD_ITEM* aItem )
                    {
                        if( aItem->HasFlag( IS_MODIFIED_CHILD ) )
                            selectedModified = true;
                    } );
        }

        switch( changeType )
        {
        case CHT_ADD:
        {
            if( selTool && selTool->GetEnteredGroup() && !boardItem->GetParentGroup()
                    && PCB_GROUP::IsGroupableType( boardItem->Type() ) )
            {
                selTool->GetEnteredGroup()->AddItem( boardItem );
            }

            if( m_isFootprintEditor )
            {
                // footprints inside footprints are not supported yet
                wxASSERT( boardItem->Type() != PCB_FOOTPRINT_T );

                boardItem->SetParent( board->Footprints().front() );

                if( !( changeFlags & CHT_DONE ) )
                    board->Footprints().front()->Add( boardItem );
            }
            else if( !boardItem->GetParentFootprint() )
            {
                if( !( aCommitFlags & SKIP_UNDO ) )
                    undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::NEWITEM ) );

                if( !( changeFlags & CHT_DONE ) )
                {
                    board->Add( boardItem, ADD_MODE::BULK_INSERT ); // handles connectivity
                    bulkAddedItems.push_back( boardItem );
                }
            }

            if( autofillZones && boardItem->Type() != PCB_MARKER_T )
                dirtyIntersectingZones( boardItem, changeType );

            if( view && boardItem->Type() != PCB_NETINFO_T )
                view->Add( boardItem );

            break;
        }

        case CHT_REMOVE:
        {
            FOOTPRINT* parentFP = boardItem->GetParentFootprint();
            PCB_GROUP* parentGroup = boardItem->GetParentGroup();

            if( !m_isFootprintEditor && !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::DELETED ) );

            if( boardItem->IsSelected() )
            {
                if( selTool )
                    selTool->RemoveItemFromSel( boardItem, true /* quiet mode */ );

                itemsDeselected = true;
            }

            if( parentGroup && !( parentGroup->GetFlags() & STRUCT_DELETED ) )
                parentGroup->RemoveItem( boardItem );

            if( autofillZones )
                dirtyIntersectingZones( boardItem, changeType );

            switch( boardItem->Type() )
            {
            case PCB_TEXT_T:
                // don't allow deletion of Reference or Value
                if( static_cast<PCB_TEXT*>( boardItem )->GetType() != PCB_TEXT::TEXT_is_DIVERS )
                    break;

                KI_FALLTHROUGH;

            case PCB_PAD_T:
            case PCB_SHAPE_T:            // a shape (normally not on copper layers)
            case PCB_BITMAP_T:           // a bitmap on a user layer
            case PCB_TEXTBOX_T:          // a wrapped text on a layer
            case PCB_TRACE_T:            // a track segment (segment on a copper layer)
            case PCB_ARC_T:              // an arced track segment (segment on a copper layer)
            case PCB_VIA_T:              // a via (like track segment on a copper layer)
            case PCB_DIM_ALIGNED_T:      // a dimension (graphic item)
            case PCB_DIM_CENTER_T:
            case PCB_DIM_RADIAL_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_LEADER_T:       // a leader dimension
            case PCB_TARGET_T:           // a target (graphic item)
            case PCB_MARKER_T:           // a marker used to show something
            case PCB_ZONE_T:
                if( view )
                    view->Remove( boardItem );

                if( !( changeFlags & CHT_DONE ) )
                {
                    if( parentFP )
                    {
                        parentFP->Delete( boardItem );
                    }
                    else
                    {
                        board->Remove( boardItem, REMOVE_MODE::BULK );
                        bulkRemovedItems.push_back( boardItem );
                    }
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
                    if( parentFP )
                    {
                        parentFP->Remove( boardItem );
                    }
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

            default:                // other types do not need to (or should not) be handled
                wxASSERT( false );
                break;
            }

            break;
        }

        case CHT_MODIFY:
        {
            if( !m_isFootprintEditor && !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( nullptr, boardItem, UNDO_REDO::CHANGED );
                wxASSERT( ent.m_copy );
                itemWrapper.SetLink( ent.m_copy );
                undoList.PushItem( itemWrapper );
            }

            if( !( aCommitFlags & SKIP_CONNECTIVITY ) )
            {
                if( ent.m_copy )
                    connectivity->MarkItemNetAsDirty( static_cast<BOARD_ITEM*>( ent.m_copy ) );

                connectivity->Update( boardItem );
            }

            if( autofillZones )
            {
                dirtyIntersectingZones( static_cast<BOARD_ITEM*>( ent.m_copy ), changeType );   // before
                dirtyIntersectingZones( boardItem, changeType );                                // after
            }

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
        board->FinalizeBulkAdd( bulkAddedItems );

    if( bulkRemovedItems.size() > 0 )
        board->FinalizeBulkRemove( bulkRemovedItems );

    if( itemsChanged.size() > 0 )
        board->OnItemsChanged( itemsChanged );

    if( m_isBoardEditor )
    {
        size_t num_changes = m_changes.size();

        if( aCommitFlags & SKIP_CONNECTIVITY )
        {
            connectivity->ClearRatsnest();
            connectivity->ClearLocalRatsnest();
        }
        else
        {
            connectivity->RecalculateRatsnest( this );
            board->UpdateRatsnestExclusions();
            connectivity->ClearLocalRatsnest();

            if( frame )
                frame->GetCanvas()->RedrawRatsnest();
        }

        if( solderMaskDirty )
        {
            if( frame )
                frame->HideSolderMask();
        }

        // Log undo items for any connectivity changes
        for( size_t i = num_changes; i < m_changes.size(); ++i )
        {
            COMMIT_LINE& ent = m_changes[i];

            wxASSERT( ( ent.m_type & CHT_TYPE ) == CHT_MODIFY );

            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( ent.m_item );

            if( !( aCommitFlags & SKIP_UNDO ) )
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

    if( m_isBoardEditor && !( aCommitFlags & SKIP_UNDO ) && frame )
    {
        if( aCommitFlags & APPEND_UNDO )
            frame->AppendCopyToUndoList( undoList, UNDO_REDO::UNSPECIFIED );
        else
            frame->SaveCopyInUndoList( undoList, UNDO_REDO::UNSPECIFIED );
    }

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( itemsDeselected )
        m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

    if( autofillZones )
        m_toolMgr->RunAction( PCB_ACTIONS::zoneFillDirty );

    if( selectedModified )
        m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    if( frame )
    {
        if( !( aCommitFlags & SKIP_SET_DIRTY ) )
            frame->OnModify();
        else
            frame->Update3DView( true, frame->GetPcbNewSettings()->m_Display.m_Live3DRefresh );
    }

    clear();
}


EDA_ITEM* BOARD_COMMIT::parentObject( EDA_ITEM* aItem ) const
{
    if( BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( aItem ) )
    {
        if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
            return parentFP;
    }

    return aItem;
}


EDA_ITEM* BOARD_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( aItem->Clone() );

    clone->SetParentGroup( nullptr );
    return clone;
}


void BOARD_COMMIT::Revert()
{
    PICKED_ITEMS_LIST                  undoList;
    KIGFX::VIEW*                       view = m_toolMgr->GetView();
    BOARD*                             board = (BOARD*) m_toolMgr->GetModel();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();

    board->IncrementTimeStamp();   // clear caches

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

            item->SwapItemData( copy );

            if( item->Type() == PCB_GROUP_T )
            {
                PCB_GROUP* group = static_cast<PCB_GROUP*>( item );

                group->RunOnChildren( [&]( BOARD_ITEM* child )
                                      {
                                          child->SetParentGroup( group );
                                      } );
            }

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
    {
        connectivity->RecalculateRatsnest();
        board->UpdateRatsnestExclusions();
    }

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    selTool->RebuildSelection();

    clear();
}

