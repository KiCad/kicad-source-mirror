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
#include <lset.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>
#include <view/view.h>
#include <board_commit.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_actions.h>
#include <connectivity/connectivity_data.h>
#include <teardrop/teardrop.h>

#include <functional>
using namespace std::placeholders;


BOARD_COMMIT::BOARD_COMMIT( TOOL_BASE* aTool ) :
        m_toolMgr( aTool->GetManager() ),
        m_isBoardEditor( false ),
        m_isFootprintEditor( false )
{
    if( PCB_TOOL_BASE* pcb_tool = dynamic_cast<PCB_TOOL_BASE*>( aTool ) )
    {
        m_isBoardEditor = pcb_tool->IsBoardEditor();
        m_isFootprintEditor = pcb_tool->IsFootprintEditor();
    }
}


BOARD_COMMIT::BOARD_COMMIT( EDA_DRAW_FRAME* aFrame ) :
        m_toolMgr( aFrame->GetToolManager() ),
        m_isBoardEditor( aFrame->IsType( FRAME_PCB_EDITOR ) ),
        m_isFootprintEditor( aFrame->IsType( FRAME_FOOTPRINT_EDITOR ) )
{
}


BOARD_COMMIT::BOARD_COMMIT( TOOL_MANAGER* aMgr ) :
        m_toolMgr( aMgr ),
        m_isBoardEditor( false ),
        m_isFootprintEditor( false )
{
    EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( aMgr->GetToolHolder() );

    if( frame && frame->IsType( FRAME_PCB_EDITOR ) )
        m_isBoardEditor = true;
    else if( frame && frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
        m_isFootprintEditor = true;
}


BOARD* BOARD_COMMIT::GetBoard() const
{
    return static_cast<BOARD*>( m_toolMgr->GetModel() );
}


COMMIT& BOARD_COMMIT::Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType, BASE_SCREEN* aScreen )
{
    wxCHECK( aItem, *this );

    // Many operations (move, rotate, etc.) are applied directly to a group's children, so they
    // must be staged as well.
    if( aChangeType == CHT_MODIFY )
    {
        if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( aItem ) )
        {
            group->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        Stage( child, aChangeType );
                    } );
        }
    }

    return COMMIT::Stage( aItem, aChangeType );
}


COMMIT& BOARD_COMMIT::Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType,
                             BASE_SCREEN* aScreen )
{
    return COMMIT::Stage( container, aChangeType, aScreen );
}


COMMIT& BOARD_COMMIT::Stage( const PICKED_ITEMS_LIST& aItems, UNDO_REDO aModFlag,
                             BASE_SCREEN* aScreen )
{
    return COMMIT::Stage( aItems, aModFlag, aScreen );
}


void BOARD_COMMIT::dirtyIntersectingZones( BOARD_ITEM* item, int aChangeType )
{
    wxCHECK( item, /* void */ );

    ZONE_FILLER_TOOL* zoneFillerTool = m_toolMgr->GetTool<ZONE_FILLER_TOOL>();

    if( item->Type() == PCB_ZONE_T )
        zoneFillerTool->DirtyZone( static_cast<ZONE*>( item ) );

    item->RunOnChildren( std::bind( &BOARD_COMMIT::dirtyIntersectingZones, this, _1, aChangeType ) );

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
}


void BOARD_COMMIT::Push( const wxString& aMessage, int aCommitFlags )
{
    KIGFX::VIEW*        view = m_toolMgr->GetView();
    BOARD*              board = static_cast<BOARD*>( m_toolMgr->GetModel() );
    PCB_BASE_FRAME*     frame = dynamic_cast<PCB_BASE_FRAME*>( m_toolMgr->GetToolHolder() );
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    // Notification info
    PICKED_ITEMS_LIST   undoList;
    bool                itemsDeselected = false;
    bool                selectedModified = false;

    // Dirty flags and lists
    bool                     solderMaskDirty = false;
    bool                     autofillZones = false;
    std::vector<BOARD_ITEM*> staleTeardropPadsAndVias;
    std::set<PCB_TRACK*>     staleTeardropTracks;
    PCB_GROUP*               addedGroup = nullptr;

    if( Empty() )
        return;

    undoList.SetDescription( aMessage );

    TEARDROP_MANAGER                   teardropMgr( board, m_toolMgr );
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();

    // Note: frame == nullptr happens in QA tests

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
        BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( ent.m_item );

        if( m_isBoardEditor && boardItem )
        {
            if( boardItem->Type() == PCB_VIA_T || boardItem->Type() == PCB_FOOTPRINT_T
                    || boardItem->IsOnLayer( F_Mask ) || boardItem->IsOnLayer( B_Mask ) )
            {
                solderMaskDirty = true;
            }

            if( !( aCommitFlags & SKIP_TEARDROPS ) )
            {
                if( boardItem->Type() == PCB_FOOTPRINT_T )
                {
                    for( PAD* pad : static_cast<FOOTPRINT*>( boardItem )->Pads() )
                        staleTeardropPadsAndVias.push_back( pad );
                }
                else if( boardItem->Type() == PCB_PAD_T || boardItem->Type() == PCB_VIA_T )
                {
                    staleTeardropPadsAndVias.push_back( boardItem );
                }
                else if( boardItem->Type() == PCB_TRACE_T || boardItem->Type() == PCB_ARC_T )
                {
                    PCB_TRACK* track = static_cast<PCB_TRACK*>( boardItem );

                    staleTeardropTracks.insert( track );

                    std::vector<PAD*>     connectedPads;
                    std::vector<PCB_VIA*> connectedVias;

                    connectivity->GetConnectedPadsAndVias( track, &connectedPads, &connectedVias );

                    for( PAD* pad : connectedPads )
                        staleTeardropPadsAndVias.push_back( pad );

                    for( PCB_VIA* via : connectedVias )
                        staleTeardropPadsAndVias.push_back( via );
                }
            }
        }

        if( boardItem && boardItem->IsSelected() )
            selectedModified = true;
    }

    // Old teardrops must be removed before connectivity is rebuilt
    if( !staleTeardropPadsAndVias.empty() || !staleTeardropTracks.empty() )
        teardropMgr.RemoveTeardrops( *this, &staleTeardropPadsAndVias, &staleTeardropTracks );

    for( COMMIT_LINE& ent : m_changes )
    {
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;
        BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( ent.m_item );

        wxASSERT( ent.m_item );
        wxCHECK2( boardItem, continue );

        switch( changeType )
        {
        case CHT_ADD:
            if( selTool && selTool->GetEnteredGroup() && !boardItem->GetParentGroup()
                    && PCB_GROUP::IsGroupableType( boardItem->Type() ) )
            {
                selTool->GetEnteredGroup()->AddItem( boardItem );
            }

            if( !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::NEWITEM ) );

            if( !( changeFlags & CHT_DONE ) )
            {
                if( m_isFootprintEditor )
                {
                    FOOTPRINT* parentFP = board->GetFirstFootprint();
                    wxCHECK2_MSG( parentFP, continue, "Commit thinks this is footprint editor, but "
                                                      "there is no first footprint!" );
                    parentFP->Add( boardItem );
                }
                else if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
                {
                    parentFP->Add( boardItem );
                }
                else
                {
                    board->Add( boardItem, ADD_MODE::BULK_INSERT ); // handles connectivity
                    bulkAddedItems.push_back( boardItem );
                }
            }

            if( boardItem->Type() == PCB_GROUP_T || boardItem->Type() == PCB_GENERATOR_T )
                addedGroup = static_cast<PCB_GROUP*>( boardItem );

            if( m_isBoardEditor && autofillZones && boardItem->Type() != PCB_MARKER_T )
                dirtyIntersectingZones( boardItem, changeType );

            if( view && boardItem->Type() != PCB_NETINFO_T )
                view->Add( boardItem );

            break;

        case CHT_REMOVE:
        {
            FOOTPRINT* parentFP = boardItem->GetParentFootprint();
            PCB_GROUP* parentGroup = boardItem->GetParentGroup();

            if( !( aCommitFlags & SKIP_UNDO ) )
                undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::DELETED ) );

            if( boardItem->IsSelected() )
            {
                if( selTool )
                    selTool->RemoveItemFromSel( boardItem, true /* quiet mode */ );

                itemsDeselected = true;
            }

            if( parentGroup && !( parentGroup->GetFlags() & STRUCT_DELETED ) )
                parentGroup->RemoveItem( boardItem );

            if( parentFP && !( parentFP->GetFlags() & STRUCT_DELETED ) )
                ent.m_parent = parentFP->m_Uuid;

            if( m_isBoardEditor && autofillZones && boardItem->Type() != PCB_MARKER_T )
                dirtyIntersectingZones( boardItem, changeType );

            switch( boardItem->Type() )
            {
            case PCB_FIELD_T:
                static_cast<PCB_FIELD*>( boardItem )->SetVisible( false );
                break;

            case PCB_TEXT_T:
            case PCB_PAD_T:
            case PCB_SHAPE_T:            // a shape (normally not on copper layers)
            case PCB_REFERENCE_IMAGE_T:  // a bitmap on an associated layer
            case PCB_GENERATOR_T:        // a generator on a layer
            case PCB_TEXTBOX_T:          // a line-wrapped (and optionally bordered) text item
            case PCB_TABLE_T:            // rows and columns of tablecells
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
            case PCB_FOOTPRINT_T:
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

            // The item has been removed from the board; it is now owned by undo/redo.
            boardItem->SetFlags( UR_TRANSIENT );

            break;
        }

        case CHT_UNGROUP:
            if( PCB_GROUP* group = boardItem->GetParentGroup() )
            {
                if( !( aCommitFlags & SKIP_UNDO ) )
                {
                    ITEM_PICKER itemWrapper( nullptr, boardItem, UNDO_REDO::UNGROUP );
                    itemWrapper.SetGroupId( group->m_Uuid );
                    undoList.PushItem( itemWrapper );
                }

                group->RemoveItem( boardItem );
            }

            break;

        case CHT_GROUP:
            if( addedGroup )
            {
                addedGroup->AddItem( boardItem );

                if( !( aCommitFlags & SKIP_UNDO ) )
                    undoList.PushItem( ITEM_PICKER( nullptr, boardItem, UNDO_REDO::REGROUP ) );
            }

            break;

        case CHT_MODIFY:
        {
            BOARD_ITEM* boardItemCopy = dynamic_cast<BOARD_ITEM*>( ent.m_copy );

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( nullptr, boardItem, UNDO_REDO::CHANGED );
                wxASSERT( boardItemCopy );
                itemWrapper.SetLink( boardItemCopy );
                undoList.PushItem( itemWrapper );
            }

            if( !( aCommitFlags & SKIP_CONNECTIVITY ) )
            {
                if( boardItemCopy )
                    connectivity->MarkItemNetAsDirty( boardItemCopy );

                connectivity->Update( boardItem );
            }

            if( m_isBoardEditor && autofillZones && boardItem->Type() != PCB_MARKER_T )
            {
                dirtyIntersectingZones( boardItemCopy, changeType );   // before
                dirtyIntersectingZones( boardItem, changeType );       // after
            }

            if( view )
                view->Update( boardItem );

            itemsChanged.push_back( boardItem );

            // if no undo entry is needed, the copy would create a memory leak
            if( aCommitFlags & SKIP_UNDO )
                delete ent.m_copy;

            break;
        }

        default:
            UNIMPLEMENTED_FOR( boardItem->GetClass() );
            break;
        }

        boardItem->ClearEditFlags();
        boardItem->RunOnDescendants(
                [&]( BOARD_ITEM* item )
                {
                    item->ClearEditFlags();
                } );
    }

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

            board->OnRatsnestChanged();
        }

        if( solderMaskDirty )
        {
            if( frame )
                frame->HideSolderMask();
        }

        if( !staleTeardropPadsAndVias.empty() || !staleTeardropTracks.empty() )
            teardropMgr.UpdateTeardrops( *this, &staleTeardropPadsAndVias, &staleTeardropTracks );

        // Log undo items for any connectivity or teardrop changes
        for( size_t i = num_changes; i < m_changes.size(); ++i )
        {
            COMMIT_LINE& ent = m_changes[i];
            BOARD_ITEM*  boardItem = dynamic_cast<BOARD_ITEM*>( ent.m_item );
            BOARD_ITEM*  boardItemCopy = dynamic_cast<BOARD_ITEM*>( ent.m_copy );

            wxCHECK2( boardItem, continue );

            if( !( aCommitFlags & SKIP_UNDO ) )
            {
                ITEM_PICKER itemWrapper( nullptr, boardItem, convert( ent.m_type & CHT_TYPE ) );
                itemWrapper.SetLink( boardItemCopy );
                undoList.PushItem( itemWrapper );
            }
            else
            {
                delete ent.m_copy;
            }

            if( view )
            {
                if( ( ent.m_type & CHT_TYPE ) == CHT_ADD )
                    view->Add( boardItem );
                else if( ( ent.m_type & CHT_TYPE ) == CHT_REMOVE )
                    view->Remove( boardItem );
                else
                    view->Update( boardItem );
            }
        }
    }

    if( bulkAddedItems.size() > 0 || bulkRemovedItems.size() > 0 || itemsChanged.size() > 0 )
        board->OnItemsCompositeUpdate( bulkAddedItems, bulkRemovedItems, itemsChanged );

    if( frame )
    {
        if( !( aCommitFlags & SKIP_UNDO ) )
        {
            if( aCommitFlags & APPEND_UNDO )
                frame->AppendCopyToUndoList( undoList, UNDO_REDO::UNSPECIFIED );
            else
                frame->SaveCopyInUndoList( undoList, UNDO_REDO::UNSPECIFIED );
        }
    }

    m_toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( itemsDeselected )
        m_toolMgr->PostEvent( EVENTS::UnselectedEvent );

    if( autofillZones )
        m_toolMgr->PostAction( PCB_ACTIONS::zoneFillDirty );

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
    return aItem;
}


EDA_ITEM* BOARD_COMMIT::makeImage( EDA_ITEM* aItem ) const
{
    return MakeImage( aItem );
}


EDA_ITEM* BOARD_COMMIT::MakeImage( EDA_ITEM* aItem )
{
    EDA_ITEM* clone = aItem->Clone();

    if( BOARD_ITEM* board_item = dynamic_cast<BOARD_ITEM*>( clone ) )
        board_item->SetParentGroup( nullptr );

    clone->SetFlags( UR_TRANSIENT );

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
        BOARD_ITEM*  boardItem = dynamic_cast<BOARD_ITEM*>( ent.m_item );
        int          changeType = ent.m_type & CHT_TYPE;
        int          changeFlags = ent.m_type & CHT_FLAGS;

        wxCHECK2( boardItem, continue );

        switch( changeType )
        {
        case CHT_ADD:
            // Items are auto-added to the parent group by BOARD_ITEM::Duplicate(), not when
            // the commit is pushed.
            if( PCB_GROUP* parentGroup = boardItem->GetParentGroup() )
            {
                if( GetStatus( parentGroup ) == 0 )
                    parentGroup->RemoveItem( boardItem );
            }

            if( !( changeFlags & CHT_DONE ) )
                break;

            view->Remove( boardItem );
            connectivity->Remove( boardItem );

            if( FOOTPRINT* parentFP = boardItem->GetParentFootprint() )
            {
                parentFP->Remove( boardItem );
            }
            else
            {
                board->Remove( boardItem, REMOVE_MODE::BULK );
                bulkRemovedItems.push_back( boardItem );
            }

            break;

        case CHT_REMOVE:
            if( !( changeFlags & CHT_DONE ) )
                break;

            view->Add( boardItem );
            connectivity->Add( boardItem );

            if( FOOTPRINT* parentFP = dynamic_cast<FOOTPRINT*>( board->GetItem( ent.m_parent ) ) )
            {
                parentFP->Add( boardItem, ADD_MODE::INSERT );
            }
            else
            {
                board->Add( boardItem, ADD_MODE::INSERT );
                bulkAddedItems.push_back( boardItem );
            }

            break;

        case CHT_MODIFY:
        {
            view->Remove( boardItem );
            connectivity->Remove( boardItem );

            BOARD_ITEM* boardItemCopy = dynamic_cast<BOARD_ITEM*>( ent.m_copy );
            wxASSERT( boardItemCopy );
            boardItem->SwapItemData( boardItemCopy );

            if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( boardItem ) )
            {
                group->RunOnChildren(
                        [&]( BOARD_ITEM* child )
                        {
                            child->SetParentGroup( group );
                        } );
            }

            view->Add( boardItem );
            connectivity->Add( boardItem );
            itemsChanged.push_back( boardItem );

            delete ent.m_copy;
            break;
        }

        default:
            UNIMPLEMENTED_FOR( boardItem->GetClass() );
            break;
        }

        boardItem->ClearEditFlags();
    }

    if( bulkAddedItems.size() > 0 || bulkRemovedItems.size() > 0 || itemsChanged.size() > 0 )
        board->OnItemsCompositeUpdate( bulkAddedItems, bulkRemovedItems, itemsChanged );

    if( m_isBoardEditor )
    {
        connectivity->RecalculateRatsnest();
        board->UpdateRatsnestExclusions();
        board->OnRatsnestChanged();
    }

    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    selTool->RebuildSelection();

    // Property panel needs to know about the reselect
    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    clear();
}

