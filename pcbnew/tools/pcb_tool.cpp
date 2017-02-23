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

#include "selection_tool.h"
#include "pcb_actions.h"
#include "tool_event_utils.h"


void PCB_TOOL::doInteractiveItemPlacement( ITEM_CREATOR aItemCreator,
                                           const wxString& aCommitMessage )
{
    using namespace std::placeholders;

    KIGFX::VIEW& view = *getView();
    KIGFX::VIEW_CONTROLS& controls = *getViewControls();
    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    std::unique_ptr<BOARD_ITEM> newItem;

    Activate();

    BOARD_COMMIT commit( &frame );

    GetManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    // do not capture or auto-pan until we start placing an item
    controls.ShowCursor( true );
    controls.SetSnapping( true );

    // Add a VIEW_GROUP that serves as a preview for the new item
    SELECTION preview;
    view.Add( &preview );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = controls.GetCursorPosition();

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( newItem )
            {
                // Delete the old item and have another try
                newItem = nullptr;

                preview.Clear();

                controls.SetAutoPan( false );
                controls.CaptureCursor( false );
                controls.ShowCursor( true );
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
                newItem = aItemCreator( *evt );

                // no item created, so wait for another click
                if( !newItem  )
                    continue;

                controls.CaptureCursor( true );
                controls.SetAutoPan( true );

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

                if( newItem->Type() == PCB_MODULE_T )
                {
                    auto module = dyn_cast<MODULE*>( newItem.get() );
                    module->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Remove, &preview, _1 ) );
                }

                commit.Add( newItem.release() );
                commit.Push( aCommitMessage );

                controls.CaptureCursor( false );
                controls.SetAutoPan( false );
                controls.ShowCursor( true );
            }
        }

        else if( newItem && evt->Category() == TC_COMMAND )
        {
            /*
             * Handle any events that can affect the item as we move
             * it around, eg rotate and flip
             */

            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) )
            {
                const auto rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle(
                        frame, *evt );
                newItem->Rotate( newItem->GetPosition(), rotationAngle );
                view.Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) )
            {
                newItem->Flip( newItem->GetPosition() );
                view.Update( &preview );
            }
        }

        else if( newItem && evt->IsMotion() )
        {
            // track the cursor
            newItem->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            view.Update( &preview );
        }
    }

    view.Remove( &preview );
}
