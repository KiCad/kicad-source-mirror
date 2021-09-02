/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_tool_base.h"

#include <tool/tool_manager.h>
#include <board_commit.h>
#include <footprint.h>
#include <pcb_draw_panel_gal.h>
#include <pcbnew_settings.h>
#include "pcb_selection_tool.h"
#include "pcb_actions.h"
#include "tool_event_utils.h"

void PCB_TOOL_BASE::doInteractiveItemPlacement( const std::string& aTool,
                                                INTERACTIVE_PLACER_BASE* aPlacer,
                                                const wxString& aCommitMessage, int aOptions )
{
    using namespace std::placeholders;
    std::unique_ptr<BOARD_ITEM> newItem;

    frame()->PushTool( aTool );
    Activate();

    BOARD_COMMIT commit( frame() );

    GetManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    // do not capture or auto-pan until we start placing an item
    controls()->ShowCursor( true );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCB_SELECTION preview;
    view()->Add( &preview );

    aPlacer->m_board = board();
    aPlacer->m_frame = frame();
    aPlacer->m_modifiers = 0;

    auto makeNewItem =
            [&]( VECTOR2I aPosition )
            {
                if( frame()->GetModel() )
                    newItem = aPlacer->CreateItem();

                if( newItem )
                {
                    newItem->SetPosition( (wxPoint) aPosition );
                    preview.Add( newItem.get() );

                    if( newItem->Type() == PCB_FOOTPRINT_T )
                    {
                        FOOTPRINT* fp = dyn_cast<FOOTPRINT*>( newItem.get() );

                        // footprints have more drawable parts
                        fp->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Add, &preview, _1 ) );
                    }
                }
            };

    if( aOptions & IPO_SINGLE_CLICK )
        makeNewItem( controls()->GetCursorPosition() );

    auto setCursor =
            [&]()
            {
                if( !newItem )
                    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
                else
                    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::PLACE );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        VECTOR2I cursorPos = controls()->GetCursorPosition();
        aPlacer->m_modifiers = evt->Modifier();

        auto cleanup =
                [&] ()
                {
                    newItem = nullptr;
                    preview.Clear();
                    view()->Update( &preview );
                    controls()->SetAutoPan( false );
                    controls()->CaptureCursor( false );
                    controls()->ShowCursor( true );
                };

        if( evt->IsCancelInteractive() )
        {
            if( aOptions & IPO_SINGLE_CLICK )
            {
                cleanup();
                frame()->PopTool( aTool );
                break;
            }
            else if( newItem )
            {
                cleanup();
            }
            else
            {
                frame()->PopTool( aTool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( newItem )
                cleanup();

            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame()->PopTool( aTool );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !newItem )
            {
                // create the item if possible
                makeNewItem( cursorPos );

                // no item created, so wait for another click
                if( !newItem  )
                    continue;

                controls()->CaptureCursor( true );
                controls()->SetAutoPan( true );
            }
            else
            {
                auto oldFlags = newItem->GetFlags();
                newItem->ClearFlags();

                if( !aPlacer->PlaceItem( newItem.get(), commit ) )
                {
                    newItem->SetFlags( oldFlags );
                    continue;
                }

                preview.Clear();
                newItem.release();
                commit.Push( aCommitMessage );

                controls()->CaptureCursor( false );
                controls()->SetAutoPan( false );
                controls()->ShowCursor( true );

                if( !( aOptions & IPO_REPEAT ) )
                    break;

                if( aOptions & IPO_SINGLE_CLICK )
                    makeNewItem( controls()->GetCursorPosition() );

                setCursor();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::trackViaSizeChanged ) )
        {
            m_toolMgr->RunAction( ACTIONS::refreshPreview );
        }
        else if( newItem && evt->Category() == TC_COMMAND )
        {
            /*
             * Handle any events that can affect the item as we move it around
             */
            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) && ( aOptions & IPO_ROTATE ) )
            {
                const int rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle( *frame(), *evt );
                newItem->Rotate( newItem->GetPosition(), rotationAngle );
                view()->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) && ( aOptions & IPO_FLIP ) )
            {
                newItem->Flip( newItem->GetPosition(), frame()->Settings().m_FlipLeftRight );
                view()->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::viaSizeInc )
                    || evt->IsAction( &PCB_ACTIONS::viaSizeDec ) )
            {
                // Refresh preview after event runs
                m_toolMgr->RunAction( ACTIONS::refreshPreview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::properties ) )
            {
                frame()->OnEditItemRequest( newItem.get() );

                // Notify other tools of the changes
                m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );
            }
            else if( evt->IsAction( &ACTIONS::refreshPreview ) )
            {
                preview.Clear();
                newItem.release();

                makeNewItem( (wxPoint) cursorPos );
                aPlacer->SnapItem( newItem.get() );
                view()->Update( &preview );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( newItem && evt->IsMotion() )
        {
            // track the cursor
            newItem->SetPosition( (wxPoint) cursorPos );
            aPlacer->SnapItem( newItem.get() );

            // Show a preview of the item
            view()->Update( &preview );
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    view()->Remove( &preview );
    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls()->SetAutoPan( false );
    controls()->CaptureCursor( false );
}


bool PCB_TOOL_BASE::Init()
{
    // A basic context manu.  Many (but not all) tools will choose to override this.
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( 1 );

    // Finally, add the standard zoom/grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( m_menu );

    return true;
}


void PCB_TOOL_BASE::Reset( RESET_REASON aReason )
{
}


void PCB_TOOL_BASE::setTransitions()
{
}


const PCB_DISPLAY_OPTIONS& PCB_TOOL_BASE::displayOptions() const
{
    return frame()->GetDisplayOptions();
}

PCB_DRAW_PANEL_GAL* PCB_TOOL_BASE::canvas() const
{
    return static_cast<PCB_DRAW_PANEL_GAL*>( frame()->GetCanvas() );
}


const PCB_SELECTION& PCB_TOOL_BASE::selection() const
{
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    return selTool->GetSelection();
}


PCB_SELECTION& PCB_TOOL_BASE::selection()
{
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    return selTool->GetSelection();
}


bool PCB_TOOL_BASE::Is45Limited() const
{
    return frame()->Settings().m_Use45DegreeLimit;
}


void INTERACTIVE_PLACER_BASE::SnapItem( BOARD_ITEM *aItem )
{
    // Base implementation performs no snapping
}


bool INTERACTIVE_PLACER_BASE::PlaceItem( BOARD_ITEM *aItem, BOARD_COMMIT& aCommit )
{
    aCommit.Add( aItem );
    return true;
}

