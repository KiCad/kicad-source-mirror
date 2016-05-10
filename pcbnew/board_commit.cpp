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
#include <wxPcbStruct.h>
#include <tool/tool_manager.h>
#include <ratsnest_data.h>
#include <view/view.h>
#include <board_commit.h>

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <tools/pcb_tool.h>

BOARD_COMMIT::BOARD_COMMIT( PCB_TOOL* aTool )
{
    m_tool = aTool;
}


BOARD_COMMIT::~BOARD_COMMIT()
{
}


void BOARD_COMMIT::Push( const wxString& aMessage )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST undoList;
    TOOL_MANAGER* toolMgr = m_tool->GetManager();
    KIGFX::VIEW* view = toolMgr->GetView();
    BOARD* board = (BOARD*) toolMgr->GetModel();
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) toolMgr->GetEditFrame();
    RN_DATA* ratsnest = board->GetRatsnest();

    bool editModules = m_tool->EditingModules();

    // Module items need to be saved in the undo buffer before modification
    if( editModules )
    {
        frame->SaveCopyInUndoList( board->m_Modules, UR_CHANGED );
    }

    BOOST_FOREACH( COMMIT_LINE& ent, m_changes )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( ent.m_item );

        switch( ent.m_type )
        {
            case CHT_ADD:
            {
                if( !editModules )
                {
                    ITEM_PICKER itemWrapper( boardItem, UR_NEW );
                    undoList.PushItem( itemWrapper );
                    board->Add( boardItem );
                }
                else
                {
                    board->m_Modules->Add( boardItem );
                }

                if( boardItem->Type() == PCB_MODULE_T )
                {
                    MODULE* mod = static_cast<MODULE*>( boardItem );
                    mod->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, view, _1 ) );
                }

                view->Add( boardItem );
                break;
            }

            case CHT_REMOVE:
            {
                if( !editModules )
                {
                    ITEM_PICKER itemWrapper( boardItem, UR_DELETED );
                    undoList.PushItem( itemWrapper );
                    board->Remove( boardItem );
                } else {
                    board->m_Modules->Remove( boardItem );
                }

                if(boardItem->Type() == PCB_MODULE_T )
                {
                    MODULE* mod = static_cast<MODULE*>( boardItem );
                    mod->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove, view, _1 ) );
                }

                view->Remove( boardItem );
                break;
            }

            case CHT_MODIFY:
            {
                if( !editModules )
                {
                    ITEM_PICKER itemWrapper( boardItem, UR_CHANGED );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                }

                if( boardItem->Type() == PCB_MODULE_T )
                {
                    MODULE* mod = static_cast<MODULE*>( boardItem );
                    //mod->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove, view, _1 ) );
                    mod->RunOnChildren( boost::bind( &RN_DATA::Update, ratsnest, _1 ) );
                }

                boardItem->ViewUpdate( KIGFX::VIEW_ITEM::ALL );
                ratsnest->Update( boardItem );
                break;
            }

            default:
                break;
        }
    }

    if( !editModules )
        frame->SaveCopyInUndoList( undoList, UR_UNSPECIFIED );

    frame->OnModify();
    ratsnest->Recalculate();

    m_committed = true;
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
    #if 0
    PICKED_ITEMS_LIST undoList;
    KIGFX::VIEW* view = m_toolMgr->GetView();
    BOARD *board = (BOARD*) m_toolMgr->GetModel();
    PCB_EDIT_FRAME *frame = (PCB_EDIT_FRAME*) m_toolMgr->GetEditFrame();


    BOOST_FOREACH( COMMIT_LINE& ent, m_changes )
    {
        BOARD_ITEM *item = static_cast<BOARD_ITEM *> (ent.m_item);
        BOARD_ITEM *copy = static_cast<BOARD_ITEM *> (ent.m_copy);

        if(ent.m_type == CHT_MODIFY)
        {
            printf("revert %p\n", item );
            RN_DATA *ratsnest = board->GetRatsnest();

            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* oldModule = static_cast<MODULE*>( item );
                oldModule->RunOnChildren( boost::bind( &KIGFX::VIEW::Remove, view, _1 ) );
            }
            view->Remove( item );
            ratsnest->Remove( static_cast<BOARD_ITEM*> ( item ) );

            item->SwapData( copy );

            // Update all pads/drawings/texts, as they become invalid
            // for the VIEW after SwapData() called for modules
            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* newModule = static_cast<MODULE*>( item );
                newModule->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, view, _1 ) );
            }
            view->Add( item );
            ratsnest->Add( item );

            item->ClearFlags( SELECTED );

            break;
        }
    }
    #endif
}

