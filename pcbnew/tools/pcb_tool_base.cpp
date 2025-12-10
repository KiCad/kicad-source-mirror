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

#include "pcb_tool_base.h"

#include <tool/tool_manager.h>
#include <board_commit.h>
#include <gal/graphics_abstraction_layer.h>
#include <footprint.h>
#include <pcb_draw_panel_gal.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_actions.h>
#include <tools/tool_event_utils.h>
#include <tools/zone_filler_tool.h>
#include <view/view_controls.h>


void PCB_TOOL_BASE::doInteractiveItemPlacement( const TOOL_EVENT&        aTool,
                                                INTERACTIVE_PLACER_BASE* aPlacer,
                                                const wxString& aCommitMessage, int aOptions )
{
    using namespace std::placeholders;
    std::unique_ptr<BOARD_ITEM> newItem;

    frame()->PushTool( aTool );

    BOARD_COMMIT commit( frame() );

    LEADER_MODE* angleSnapMode = nullptr;
    LEADER_MODE  savedAngleSnapMode = LEADER_MODE::DIRECT;
    bool         restoreAngleSnapMode = false;

    if( frame()->IsType( FRAME_PCB_EDITOR ) )
    {
        angleSnapMode = &GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" )->m_AngleSnapMode;
    }
    else if( frame()->IsType( FRAME_FOOTPRINT_EDITOR ) )
    {
        angleSnapMode = &GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" )->m_AngleSnapMode;
    }

    if( !angleSnapMode )
    {
        PCB_VIEWERS_SETTINGS_BASE* viewerSettings = frame()->GetViewerSettingsBase();

        if( viewerSettings )
            angleSnapMode = &viewerSettings->m_ViewersDisplay.m_AngleSnapMode;
    }

    if( angleSnapMode && *angleSnapMode != LEADER_MODE::DIRECT )
    {
        savedAngleSnapMode = *angleSnapMode;
        *angleSnapMode = LEADER_MODE::DIRECT;
        restoreAngleSnapMode = true;
        m_toolMgr->RunAction( PCB_ACTIONS::angleSnapModeChanged );
    }

    GetManager()->RunAction( ACTIONS::selectionClear );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls()->ShowCursor( true );
    controls()->ForceCursorPosition( false );
    // do not capture or auto-pan until we start placing an item

    PCB_GRID_HELPER grid( m_toolMgr, frame()->GetMagneticItemsSettings() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCB_SELECTION preview;
    view()->Add( &preview );

    aPlacer->m_board = board();
    aPlacer->m_frame = frame();
    aPlacer->m_modifiers = 0;

    auto makeNewItem =
            [&]( const VECTOR2I& aPosition )
            {
                if( frame()->GetModel() )
                    newItem = aPlacer->CreateItem();

                if( newItem )
                {
                    newItem->SetPosition( aPosition );
                    preview.Add( newItem.get() );

                    // footprints have more drawable parts
                    if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( newItem.get() ) )
                        fp->RunOnChildren( std::bind( &KIGFX::VIEW_GROUP::Add, &preview, _1 ),
                                           RECURSE_MODE::NO_RECURSE );
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

        grid.SetSnap( false ); // Interactive placement tools need to set their own item snaps
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = controls()->GetMousePosition();

        if( !evt->IsActivate() && !evt->IsCancelInteractive() )
        {
            cursorPos = grid.BestSnapAnchor( cursorPos, nullptr );
        }
        else
        {
            grid.FullReset();
        }

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
                    controls()->ForceCursorPosition( false );
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
                BOARD_ITEM*    newBoardItem = newItem.release();
                EDA_ITEM_FLAGS oldFlags = newBoardItem->GetFlags();

                newBoardItem->ClearFlags();

                if( !aPlacer->PlaceItem( newBoardItem, commit ) )
                {
                    newBoardItem->SetFlags( oldFlags );
                    newItem.reset( newBoardItem );
                    continue;
                }

                preview.Clear();
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
            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::trackViaSizeChanged ) )
        {
            m_toolMgr->PostAction( ACTIONS::refreshPreview );
        }
        else if( newItem && evt->Category() == TC_COMMAND )
        {
            /*
             * Handle any events that can affect the item as we move it around
             */
            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) && ( aOptions & IPO_ROTATE ) )
            {
                EDA_ANGLE rotationAngle = TOOL_EVT_UTILS::GetEventRotationAngle( *frame(), *evt );
                newItem->Rotate( newItem->GetPosition(), rotationAngle );
                view()->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) && ( aOptions & IPO_FLIP ) )
            {
                newItem->Flip( newItem->GetPosition(), frame()->GetPcbNewSettings()->m_FlipDirection );
                view()->Update( &preview );
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
                newItem.reset();

                makeNewItem( cursorPos );
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
            newItem->SetPosition( cursorPos );
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
    controls()->ForceCursorPosition( false );

    if( restoreAngleSnapMode )
    {
        *angleSnapMode = savedAngleSnapMode;
        m_toolMgr->RunAction( PCB_ACTIONS::angleSnapModeChanged );
    }
}


bool PCB_TOOL_BASE::Init()
{
    // A basic context manu.  Many (but not all) tools will choose to override this.
    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
    ctxMenu.AddSeparator( 1 );

    // Finally, add the standard zoom/grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( *m_menu.get() );

    return true;
}


void PCB_TOOL_BASE::Reset( RESET_REASON aReason )
{
}


void PCB_TOOL_BASE::setTransitions()
{
}


PCBNEW_SETTINGS::DISPLAY_OPTIONS& PCB_TOOL_BASE::displayOptions() const
{
    return frame<PCB_BASE_FRAME>()->GetPcbNewSettings()->m_Display;
}

PCB_DRAW_PANEL_GAL* PCB_TOOL_BASE::canvas() const
{
    return static_cast<PCB_DRAW_PANEL_GAL*>( frame<PCB_BASE_FRAME>()->GetCanvas() );
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
    return GetAngleSnapMode() != LEADER_MODE::DIRECT;
}

bool PCB_TOOL_BASE::Is90Limited() const
{
    return GetAngleSnapMode() == LEADER_MODE::DEG90;
}

LEADER_MODE PCB_TOOL_BASE::GetAngleSnapMode() const
{
    if( frame<PCB_BASE_FRAME>()->IsType( FRAME_PCB_EDITOR ) )
        return GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" )->m_AngleSnapMode;
    else
        return GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" )->m_AngleSnapMode;
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
