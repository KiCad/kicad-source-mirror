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
#include <tools/pcb_tool.h>

#include <functional>
using namespace std::placeholders;

BOARD_COMMIT::BOARD_COMMIT( PCB_TOOL* aTool )
{
    m_toolMgr = aTool->GetManager();
    m_editModules = aTool->EditingModules();
}


BOARD_COMMIT::BOARD_COMMIT( PCB_BASE_FRAME* aFrame )
{
    m_toolMgr = aFrame->GetToolManager();
    m_editModules = aFrame->IsType( FRAME_PCB_MODULE_EDITOR );
}


BOARD_COMMIT::~BOARD_COMMIT()
{
}


void BOARD_COMMIT::Push( const wxString& aMessage )
{
    // Objects potentially interested in changes:
    PICKED_ITEMS_LIST undoList;
    KIGFX::VIEW* view = m_toolMgr->GetView();
    BOARD* board = (BOARD*) m_toolMgr->GetModel();
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) m_toolMgr->GetEditFrame();
    RN_DATA* ratsnest = board->GetRatsnest();
    std::set<EDA_ITEM*> savedModules;

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
                    assert( changeType != CHT_MODIFY );     // too late to make a copy..
                    ent.m_copy = ent.m_item->Clone();
                }

                assert( ent.m_item->Type() == PCB_MODULE_T );
                assert( ent.m_copy->Type() == PCB_MODULE_T );

                ITEM_PICKER itemWrapper( ent.m_item, UR_CHANGED );
                itemWrapper.SetLink( ent.m_copy );
                undoList.PushItem( itemWrapper );
                frame->SaveCopyInUndoList( undoList, UR_CHANGED );

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
                    undoList.PushItem( ITEM_PICKER( boardItem, UR_NEW ) );

                    if( !( changeFlags & CHT_DONE ) )
                        board->Add( boardItem );

                    //ratsnest->Add( boardItem );       // TODO currently done by BOARD::Add()

                    if( boardItem->Type() == PCB_MODULE_T )
                    {
                        MODULE* mod = static_cast<MODULE*>( boardItem );
                        mod->RunOnChildren( std::bind( &KIGFX::VIEW::Add, view, _1 ) );
                    }
                }
                else
                {
                    // modules inside modules are not supported yet
                    assert( boardItem->Type() != PCB_MODULE_T );

                    if( !( changeFlags & CHT_DONE ) )
                        board->m_Modules->Add( boardItem );
                }

                view->Add( boardItem );
                break;
            }

            case CHT_REMOVE:
            {
                if( !m_editModules )
                {
                    undoList.PushItem( ITEM_PICKER( boardItem, UR_DELETED ) );
                }

                switch( boardItem->Type() )
                {
                // Module items
                case PCB_PAD_T:
                case PCB_MODULE_EDGE_T:
                case PCB_MODULE_TEXT_T:
                {
                    // Do not allow footprint text removal when not editing a module
                    if( !m_editModules )
                        break;

                    bool remove = true;

                    if( boardItem->Type() == PCB_MODULE_TEXT_T )
                    {
                        TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( boardItem );

                        switch( text->GetType() )
                        {
                            case TEXTE_MODULE::TEXT_is_REFERENCE:
                                //DisplayError( frame, _( "Cannot delete component reference." ) );
                                remove = false;
                                break;

                            case TEXTE_MODULE::TEXT_is_VALUE:
                                //DisplayError( frame, _( "Cannot delete component value." ) );
                                remove = false;
                                break;

                            case TEXTE_MODULE::TEXT_is_DIVERS:    // suppress warnings
                                break;

                            default:
                                assert( false );
                                break;
                        }
                    }

                    if( remove )
                    {
                        view->Remove( boardItem );

                        if( !( changeFlags & CHT_DONE ) )
                        {
                            MODULE* module = static_cast<MODULE*>( boardItem->GetParent() );
                            assert( module && module->Type() == PCB_MODULE_T );
                            module->Delete( boardItem );
                        }

                        board->m_Status_Pcb = 0; // it is done in the legacy view (ratsnest perhaps?)
                    }

                    break;
                }

                // Board items
                case PCB_LINE_T:                // a segment not on copper layers
                case PCB_TEXT_T:                // a text on a layer
                case PCB_TRACE_T:               // a track segment (segment on a copper layer)
                case PCB_VIA_T:                 // a via (like track segment on a copper layer)
                case PCB_DIMENSION_T:           // a dimension (graphic item)
                case PCB_TARGET_T:              // a target (graphic item)
                case PCB_MARKER_T:              // a marker used to show something
                case PCB_ZONE_T:                // SEG_ZONE items are now deprecated
                case PCB_ZONE_AREA_T:
                    view->Remove( boardItem );

                    if( !( changeFlags & CHT_DONE ) )
                        board->Remove( boardItem );

                    //ratsnest->Remove( boardItem );    // currently done by BOARD::Remove()
                    break;

                case PCB_MODULE_T:
                {
                    // There are no modules inside a module yet
                    assert( !m_editModules );

                    MODULE* module = static_cast<MODULE*>( boardItem );
                    module->ClearFlags();
                    module->RunOnChildren( std::bind( &KIGFX::VIEW::Remove, view, _1 ) );

                    view->Remove( module );

                    if( !( changeFlags & CHT_DONE ) )
                        board->Remove( module );

                    // Clear flags to indicate, that the ratsnest, list of nets & pads are not valid anymore
                    board->m_Status_Pcb = 0;
                }
                break;

                default:                        // other types do not need to (or should not) be handled
                    assert( false );
                    break;
                }
                break;
            }

            case CHT_MODIFY:
            {
                if( !m_editModules )
                {
                    ITEM_PICKER itemWrapper( boardItem, UR_CHANGED );
                    assert( ent.m_copy );
                    itemWrapper.SetLink( ent.m_copy );
                    undoList.PushItem( itemWrapper );
                }

                if ( boardItem->Type() == PCB_MODULE_T )
                {
                    MODULE* module = static_cast<MODULE*>( boardItem );
                    module->RunOnChildren( [&view] ( BOARD_ITEM *aItem ){ view->Update( aItem ); } );
                }

                view->Update ( boardItem );
                ratsnest->Update( boardItem );
                break;
            }

            default:
                assert( false );
                break;
        }
    }

    if( !m_editModules )
        frame->SaveCopyInUndoList( undoList, UR_UNSPECIFIED );

    if( TOOL_MANAGER* toolMgr = frame->GetToolManager() )
        toolMgr->PostEvent( { TC_MESSAGE, TA_MODEL_CHANGE, AS_GLOBAL } );

    ratsnest->Recalculate();
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
    RN_DATA* ratsnest = board->GetRatsnest();

    for( auto it = m_changes.rbegin(); it != m_changes.rend(); ++it )
    {
        COMMIT_LINE& ent = *it;
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( ent.m_item );
        BOARD_ITEM* copy = static_cast<BOARD_ITEM*>( ent.m_copy );

        switch( ent.m_type )
        {
        case CHT_ADD:
            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* oldModule = static_cast<MODULE*>( item );
                oldModule->RunOnChildren( std::bind( &KIGFX::VIEW::Remove, view, _1 ) );
            }

            view->Remove( item );
            ratsnest->Remove( item );
            break;

        case CHT_REMOVE:
            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* newModule = static_cast<MODULE*>( item );
                newModule->RunOnChildren( std::bind( &EDA_ITEM::ClearFlags, _1, SELECTED ) );
                newModule->RunOnChildren( std::bind( &KIGFX::VIEW::Add, view, _1 ) );
            }

            view->Add( item );
            ratsnest->Add( item );
            break;

        case CHT_MODIFY:
        {
            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* oldModule = static_cast<MODULE*>( item );
                oldModule->RunOnChildren( std::bind( &KIGFX::VIEW::Remove, view, _1 ) );
            }

            view->Remove( item );
            ratsnest->Remove( item );

            item->SwapData( copy );

            item->ClearFlags( SELECTED );

            // Update all pads/drawings/texts, as they become invalid
            // for the VIEW after SwapData() called for modules
            if( item->Type() == PCB_MODULE_T )
            {
                MODULE* newModule = static_cast<MODULE*>( item );
                newModule->RunOnChildren( std::bind( &EDA_ITEM::ClearFlags, _1, SELECTED ) );
                newModule->RunOnChildren( std::bind( &KIGFX::VIEW::Add, view, _1 ) );
            }

            view->Add( item );
            ratsnest->Add( item );
            delete copy;
            break;
        }

        default:
            assert( false );
            break;
        }
    }

    ratsnest->Recalculate();

    clear();
}
