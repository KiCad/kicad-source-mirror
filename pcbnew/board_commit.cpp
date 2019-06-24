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

#include <class_board.h>
#include <class_module.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/selection_tool.h>
#include <view/view.h>
#include <board_commit.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_actions.h>
#include <connectivity/connectivity_data.h>

#include <functional>
using namespace std::placeholders;

#include "pcb_draw_panel_gal.h"

BOARD_COMMIT::BOARD_COMMIT( PCB_TOOL_BASE* aTool )
{
    m_toolMgr = aTool->GetManager();
    m_editModules = aTool->EditingModules();
}


BOARD_COMMIT::BOARD_COMMIT( EDA_DRAW_FRAME* aFrame )
{
    m_toolMgr = aFrame->GetToolManager();
    m_editModules = aFrame->IsType( FRAME_PCB_MODULE_EDITOR );
}


BOARD_COMMIT::~BOARD_COMMIT()
{
}


void BOARD_COMMIT::Push( const wxString& aMessage, bool aCreateUndoEntry, bool aSetDirtyBit )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST undoList;
    KIGFX::VIEW*      view = m_toolMgr->GetView();
    BOARD*            board = (BOARD*) m_toolMgr->GetModel();
    PCB_BASE_FRAME*   frame = (PCB_BASE_FRAME*) m_toolMgr->GetEditFrame();
    auto              connectivity = board->GetConnectivity();
    std::set<EDA_ITEM*>      savedModules;
    std::vector<BOARD_ITEM*> itemsToDeselect;

    if( Empty() )
        return;

    for( COMMIT_LINE& ent : m_changes )
    {
        int changeType = ent.m_type & CHT_TYPE;
        int changeFlags = ent.m_type & CHT_FLAGS;
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( ent.m_item );

        // Module items need to be saved in the undo buffer before modification
        if( m_editModules )
        {
            // Be sure that we are storing a module
            if( ent.m_item->Type() != PCB_MODULE_T )
                ent.m_item = ent.m_item->GetParent();

            // We have not saved the module yet, so let's create an entry
            if( savedModules.count( ent.m_item ) == 0 )
            {
                if( !ent.m_copy )
                {
                    wxASSERT( changeType != CHT_MODIFY );     // too late to make a copy..
                    ent.m_copy = ent.m_item->Clone();
                }

                wxASSERT( ent.m_item->Type() == PCB_MODULE_T );
                wxASSERT( ent.m_copy->Type() == PCB_MODULE_T );

                if( aCreateUndoEntry )
                {
                    ITEM_PICKER itemWrapper( ent.m_item, UR_CHANGED );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                    frame->SaveCopyInUndoList( undoList, UR_CHANGED );
                }

                savedModules.insert( ent.m_item );
                static_cast<MODULE*>( ent.m_item )->SetLastEditTime();
            }
        }

        switch( changeType )
        {
            case CHT_ADD:
            {
                if( !m_editModules )
                {
                    if( aCreateUndoEntry )
                        undoList.PushItem( ITEM_PICKER( boardItem, UR_NEW ) );

                    if( !( changeFlags & CHT_DONE ) )
                        board->Add( boardItem );        // handles connectivity
                }
                else
                {
                    // modules inside modules are not supported yet
                    wxASSERT( boardItem->Type() != PCB_MODULE_T );

                    boardItem->SetParent( board->Modules().front() );

                    if( !( changeFlags & CHT_DONE ) )
                        board->Modules().front()->Add( boardItem );
                }

                view->Add( boardItem );
                break;
            }

            case CHT_REMOVE:
            {
                if( !m_editModules && aCreateUndoEntry )
                    undoList.PushItem( ITEM_PICKER( boardItem, UR_DELETED ) );

                switch( boardItem->Type() )
                {
                // Module items
                case PCB_PAD_T:
                case PCB_MODULE_EDGE_T:
                case PCB_MODULE_TEXT_T:
                    // This level can only handle module items when editing modules
                    if( !m_editModules )
                        break;

                    if( boardItem->Type() == PCB_MODULE_TEXT_T )
                    {
                        TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( boardItem );

                        // don't allow deletion of Reference or Value
                        if( text->GetType() != TEXTE_MODULE::TEXT_is_DIVERS )
                            break;
                    }

                    view->Remove( boardItem );

                    if( !( changeFlags & CHT_DONE ) )
                    {
                        MODULE* module = static_cast<MODULE*>( boardItem->GetParent() );
                        wxASSERT( module && module->Type() == PCB_MODULE_T );
                        module->Delete( boardItem );
                    }

                    break;

                // Board items
                case PCB_LINE_T:                // a segment not on copper layers
                case PCB_TEXT_T:                // a text on a layer
                case PCB_TRACE_T:               // a track segment (segment on a copper layer)
                case PCB_VIA_T:                 // a via (like track segment on a copper layer)
                case PCB_DIMENSION_T:           // a dimension (graphic item)
                case PCB_TARGET_T:              // a target (graphic item)
                case PCB_MARKER_T:              // a marker used to show something
                case PCB_ZONE_AREA_T:
                    itemsToDeselect.push_back( boardItem );

                    view->Remove( boardItem );

                    if( !( changeFlags & CHT_DONE ) )
                        board->Remove( boardItem );

                    break;

                case PCB_MODULE_T:
                {
                    itemsToDeselect.push_back( boardItem );

                    // There are no modules inside a module yet
                    wxASSERT( !m_editModules );

                    MODULE* module = static_cast<MODULE*>( boardItem );
                    view->Remove( module );
                    module->ClearFlags();

                    if( !( changeFlags & CHT_DONE ) )
                        board->Remove( module );        // handles connectivity
                }
                break;

                default:                        // other types do not need to (or should not) be handled
                    wxASSERT( false );
                    break;
                }

                break;
            }

            case CHT_MODIFY:
            {
                if( !m_editModules && aCreateUndoEntry )
                {
                    ITEM_PICKER itemWrapper( boardItem, UR_CHANGED );
                    wxASSERT( ent.m_copy );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                }

                if( ent.m_copy )
                    connectivity->MarkItemNetAsDirty( static_cast<BOARD_ITEM*>( ent.m_copy ) );

                connectivity->Update( boardItem );
                view->Update( boardItem );

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

    // Removing an item should trigger the unselect action
    // but only after all items are removed otherwise we can get
    // flickering depending on the system
    if( itemsToDeselect.size() > 0 )
        m_toolMgr->RunAction( PCB_ACTIONS::unselectItems, true, &itemsToDeselect );

    if ( !m_editModules )
    {
        size_t num_changes = m_changes.size();

        connectivity->RecalculateRatsnest( this );
        connectivity->ClearDynamicRatsnest();
        frame->GetCanvas()->RedrawRatsnest();

        if( m_changes.size() > num_changes )
        {
            for( size_t i = num_changes; i < m_changes.size(); ++i )
            {
                COMMIT_LINE& ent = m_changes[i];

                // This should only be modifications from the connectivity algo
                wxASSERT( ( ent.m_type & CHT_TYPE ) == CHT_MODIFY );

                auto boardItem = static_cast<BOARD_ITEM*>( ent.m_item );

                if( aCreateUndoEntry )
                {
                    ITEM_PICKER itemWrapper( boardItem, UR_CHANGED );
                    wxASSERT( ent.m_copy );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                }
                else
                {
                    delete ent.m_copy;
                }

                view->Update( boardItem );
            }
        }
    }

    if( !m_editModules && aCreateUndoEntry )
        frame->SaveCopyInUndoList( undoList, UR_UNSPECIFIED );

    if( TOOL_MANAGER* toolMgr = frame->GetToolManager() )
        toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    if( aSetDirtyBit )
        frame->OnModify();

    frame->UpdateMsgPanel();

    clear();
}


EDA_ITEM* BOARD_COMMIT::parentObject( EDA_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
        case PCB_PAD_T:
        case PCB_MODULE_EDGE_T:
        case PCB_MODULE_TEXT_T:
            return aItem->GetParent();
        default:
            return aItem;
    }

    return aItem;
}


void BOARD_COMMIT::Revert()
{
    PICKED_ITEMS_LIST undoList;
    KIGFX::VIEW* view = m_toolMgr->GetView();
    BOARD* board = (BOARD*) m_toolMgr->GetModel();
    auto connectivity = board->GetConnectivity();

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
            board->Remove( item );
            break;

        case CHT_REMOVE:
            if( !( changeFlags & CHT_DONE ) )
                break;

            view->Add( item );
            connectivity->Add( item );
            board->Add( item );
            break;

        case CHT_MODIFY:
        {
            view->Remove( item );
            connectivity->Remove( item );

            item->SwapData( copy );

            view->Add( item );
            connectivity->Add( item );
            delete copy;
            break;
        }

        default:
            wxASSERT( false );
            break;
        }
    }

    if ( !m_editModules )
        connectivity->RecalculateRatsnest();

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    selTool->RebuildSelection();

    clear();
}
