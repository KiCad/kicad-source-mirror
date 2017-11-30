/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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


#include "pcb_tool.h"

#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <board_commit.h>

#include <class_module.h>
#include <pcb_draw_panel_gal.h>

#include "selection_tool.h"
#include "pcb_actions.h"
#include "tool_event_utils.h"

void PCB_TOOL::doInteractiveItemPlacement( INTERACTIVE_PLACER_BASE* aPlacer,
                                           const wxString& aCommitMessage,
                                           int aOptions )
{
    using namespace std::placeholders;
    std::unique_ptr<BOARD_ITEM> newItem;

    Activate();

    BOARD_COMMIT commit( frame() );

    GetManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    // do not capture or auto-pan until we start placing an item
    controls()->ShowCursor( true );
    controls()->SetSnapping( true );

    // Add a VIEW_GROUP that serves as a preview for the new item
    SELECTION preview;
    view()->Add( &preview );

    aPlacer->m_board = board();
    aPlacer->m_frame = frame();

    if( aOptions & IPO_SINGLE_CLICK )
    {
        VECTOR2I cursorPos = controls()->GetCursorPosition();

        newItem = aPlacer->CreateItem();
        newItem->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

        preview.Add( newItem.get() );
    }

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = controls()->GetCursorPosition();

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( newItem )
            {
                // Delete the old item and have another try
                newItem = nullptr;

                preview.Clear();

                if( aOptions & IPO_SINGLE_CLICK )
                    break;

                controls()->SetAutoPan( false );
                controls()->CaptureCursor( false );
                controls()->ShowCursor( true );
            }
            else
            {
                break;
            }

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !newItem )
            {
                // create the item if possible
                newItem = aPlacer->CreateItem();

                // no item created, so wait for another click
                if( !newItem  )
                    continue;

                controls()->CaptureCursor( true );
                controls()->SetAutoPan( true );

                newItem->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

                preview.Add( newItem.get() );

                if( newItem->Type() == PCB_MODULE_T )
                {
                    auto module = dyn_cast<MODULE*>( newItem.get() );

                    // modules have more drawable parts
                    module->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Add, &preview, _1 ) );
                }
            }
            else
            {
                newItem->ClearFlags();
                preview.Remove( newItem.get() );

                aPlacer->PlaceItem( newItem.get() );

                if( newItem->Type() == PCB_MODULE_T )
                {
                    auto module = dyn_cast<MODULE*>( newItem.get() );
                    module->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Remove, &preview, _1 ) );
                }

                commit.Add( newItem.release() );
                commit.Push( aCommitMessage );

                controls()->CaptureCursor( false );
                controls()->SetAutoPan( false );
                controls()->ShowCursor( true );

                if( !( aOptions & IPO_REPEAT ) )
                    break;

                if( aOptions & IPO_SINGLE_CLICK )
                {
                    VECTOR2I pos = controls()->GetCursorPosition();

                    newItem = aPlacer->CreateItem();
                    newItem->SetPosition( wxPoint( pos.x, pos.y ) );

                    preview.Add( newItem.get() );
                }
            }
        }

        else if( newItem && evt->Category() == TC_COMMAND )
        {
            /*
             * Handle any events that can affect the item as we move
             * it around, eg rotate and flip
             */

            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) && ( aOptions & IPO_ROTATE ) )
            {
                const auto rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle(
                        *frame(), *evt );
                newItem->Rotate( newItem->GetPosition(), rotationAngle );
                view()->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) && ( aOptions & IPO_FLIP ) )
            {
                newItem->Flip( newItem->GetPosition() );
                view()->Update( &preview );
            }
        }

        else if( newItem && evt->IsMotion() )
        {
            // track the cursor
            newItem->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            view()->Update( &preview );
        }
    }

    view()->Remove( &preview );
}

void PCB_TOOL::Reset( RESET_REASON aReason )
{

}

void PCB_TOOL::setTransitions()
{

}

PCB_DISPLAY_OPTIONS* PCB_TOOL::displayOptions() const
{
    return static_cast<PCB_DISPLAY_OPTIONS*>( frame()->GetDisplayOptions() );
}

PCB_DRAW_PANEL_GAL* PCB_TOOL::canvas() const
{
    return static_cast<PCB_DRAW_PANEL_GAL*>( frame()->GetGalCanvas() );
}

const SELECTION& PCB_TOOL::selection() const
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool->GetSelection();
    return selection;
}

SELECTION& PCB_TOOL::selection()
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    auto& selection = selTool->GetSelection();
    return selection;
}
