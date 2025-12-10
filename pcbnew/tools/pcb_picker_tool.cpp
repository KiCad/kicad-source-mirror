/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "pcb_picker_tool.h"

#include "pcb_actions.h"
#include "pcb_grid_helper.h"
#include <gal/graphics_abstraction_layer.h>
#include <kiplatform/ui.h>
#include <status_popup.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/zone_filler_tool.h>
#include <view/view_controls.h>


PCB_PICKER_TOOL::PCB_PICKER_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InteractivePicker" ),
        PICKER_TOOL_BASE() // calls reset()
{
}


bool PCB_PICKER_TOOL::Init()
{
    CONDITIONAL_MENU& menu = m_menu->GetMenu();

    auto snapIsSetToAllLayers =
            [this]( const SELECTION& aSel )
            {
                if( PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>() )
                {
                    if( frame->GetMagneticItemsSettings() )
                        return frame->GetMagneticItemsSettings()->allLayers;
                }

                return false;
            };

    // "Cancel" goes at the top of the context menu when a tool is active
    menu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );

    menu.AddSeparator( 1 );

    menu.AddItem( PCB_ACTIONS::magneticSnapAllLayers, !snapIsSetToAllLayers, 1 );
    menu.AddItem( PCB_ACTIONS::magneticSnapActiveLayer, snapIsSetToAllLayers, 1 );

    menu.AddSeparator( 1 );

    if( PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>() )
        frame->AddStandardSubMenus( *m_menu.get() );

    return true;
}


int PCB_PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    PCB_BASE_FRAME*       frame    = getEditFrame<PCB_BASE_FRAME>();
    PCB_GRID_HELPER       grid( m_toolMgr, frame->GetMagneticItemsSettings() );
    int                   finalize_state = WAIT_CANCEL;

    TOOL_EVENT sourceEvent;

    if( aEvent.IsAction( &ACTIONS::pickerTool ) )
    {
        wxCHECK_MSG( aEvent.Parameter<const TOOL_EVENT*>(), -1,
                     wxT( "PCB_PICKER_TOOL::Main() called without a source event" ) );

        sourceEvent = *aEvent.Parameter<const TOOL_EVENT*>();
        frame->PushTool( sourceEvent );
    }

    Activate();
    setControls();

    auto setCursor =
            [&]()
            {
                frame->GetCanvas()->SetCurrentCursor( m_cursor );
                controls->ShowCursor( true );
            };

    // Set initial cursor
    setCursor();
    VECTOR2D cursorPos;

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos =  controls->GetMousePosition();

        if( m_snap )
        {
            grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
            grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

            if( !evt->IsActivate() && !evt->IsCancelInteractive() )
            {
                // If we are switching, the canvas may not be valid any more
                cursorPos = grid.BestSnapAnchor( cursorPos, nullptr );
                controls->ForceCursorPosition( true, cursorPos );
            }
            else
            {
                grid.FullReset();
            }
        }

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& )
                {
                }
            }

            // Activating a new tool may have alternate finalization from canceling the current tool
            if( evt->IsActivate() )
                finalize_state = END_ACTIVATE;
            else
                finalize_state = EVT_CANCEL;

            break;
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            m_picked = cursorPos;

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& )
                {
                    finalize_state = EXCEPTION_CANCEL;
                    break;
                }
            }

            if( !getNext )
            {
                finalize_state = CLICK_CANCEL;
                break;
            }
            else
            {
                setControls();
            }
        }
        else if( evt->IsMotion() )
        {
            if( m_motionHandler )
            {
                try
                {
                    (*m_motionHandler)( cursorPos );
                }
                catch( std::exception& )
                {
                }
            }
        }
        else if( evt->IsDblClick( BUT_LEFT ) || evt->IsDrag( BUT_LEFT ) )
        {
            // Not currently used, but we don't want to pass them either
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            PCB_SELECTION dummy;
            m_menu->ShowContextMenu( dummy );
        }
        // TODO: It'd be nice to be able to say "don't allow any non-trivial editing actions",
        // but we don't at present have that, so we just knock out some of the egregious ones.
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    if( m_finalizeHandler )
    {
        try
        {
            (*m_finalizeHandler)( finalize_state );
        }
        catch( std::exception& )
        {
        }
    }

    reset();
    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );

    if( aEvent.IsAction( &ACTIONS::pickerTool ) )
        frame->PopTool( sourceEvent );

    return 0;
}


void PCB_PICKER_TOOL::reset()
{
    m_layerMask = LSET::AllLayersMask();
    PICKER_TOOL_BASE::reset();
}


void PCB_PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->CaptureCursor( false );
    controls->SetAutoPan( false );
}


int PCB_PICKER_TOOL::SelectPointInteractively( const TOOL_EVENT& aEvent )
{
    INTERACTIVE_PARAMS params = aEvent.Parameter<INTERACTIVE_PARAMS>();
    STATUS_TEXT_POPUP  statusPopup( frame() );
    bool               done = false;

    wxCHECK( params.m_Receiver, -1 );

    PCB_GRID_HELPER grid_helper( m_toolMgr, frame()->GetMagneticItemsSettings() );

    // By pushing this tool, we stop the Selection tool popping a disambiuation menu
    // in cases like returning to the Position Relative dialog after the selection.
    frame()->PushTool( aEvent );
    Activate();

    statusPopup.SetText( wxGetTranslation( params.m_Prompt ) );

    const auto sendPoint =
            [&]( const std::optional<VECTOR2I>& aPoint )
            {
                statusPopup.Hide();
                params.m_Receiver->UpdatePickedPoint( aPoint );
            };

    SetSnapping( true );
    SetCursor( KICURSOR::PLACE );
    ClearHandlers();

    SetClickHandler(
            [&]( const VECTOR2D& aPoint ) -> bool
            {
                std::optional<VECTOR2I> snapped = grid_helper.GetSnappedPoint();

                sendPoint( snapped ? *snapped : VECTOR2I( aPoint ) );

                return false; // got our item; don't need any more
            } );

    SetMotionHandler(
            [&]( const VECTOR2D& aPos )
            {
                grid_helper.SetSnap( !( CurrentModifiers() & MD_SHIFT ) );
                statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
            } );

    SetCancelHandler(
            [&]()
            {
                sendPoint( std::nullopt );
            } );

    SetFinalizeHandler(
            [&]( const int& aFinalState )
            {
                done = true;
            } );

    statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
    statusPopup.Popup();
    canvas()->SetStatusPopup( statusPopup.GetPanel() );

    // Drop into the main event loop
    Main( aEvent );

    ClearHandlers();
    canvas()->SetStatusPopup( nullptr );
    frame()->PopTool( aEvent );
    return 0;
}


int PCB_PICKER_TOOL::SelectItemInteractively( const TOOL_EVENT& aEvent )
{
    INTERACTIVE_PARAMS params = aEvent.Parameter<INTERACTIVE_PARAMS>();
    STATUS_TEXT_POPUP  statusPopup( frame() );
    bool               done = false;
    EDA_ITEM*          anchor_item = nullptr;

    PCB_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    frame()->PushTool( aEvent );
    Activate();

    statusPopup.SetText( wxGetTranslation( params.m_Prompt ) );

    const auto sendItem =
            [&]( const EDA_ITEM* aItem )
            {
                statusPopup.Hide();
                params.m_Receiver->UpdatePickedItem( aItem );
            };

    SetCursor( KICURSOR::BULLSEYE );
    SetSnapping( false );
    ClearHandlers();

    SetClickHandler(
            [&]( const VECTOR2D& aPoint ) -> bool
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                const PCB_SELECTION& sel = selectionTool->RequestSelection(
                        []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                        {
                        } );

                if( sel.Empty() )
                    return true; // still looking for an item

                anchor_item = sel.Front();

                if( params.m_ItemFilter && !params.m_ItemFilter( anchor_item ) )
                    return true;

                sendItem( sel.Front() );
                return false; // got our item; don't need any more
            } );

    SetMotionHandler(
            [&]( const VECTOR2D& aPos )
            {
                statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
            } );

    SetCancelHandler(
            [&]()
            {
                if( anchor_item && ( !params.m_ItemFilter || params.m_ItemFilter( anchor_item ) ) )
                    sendItem( anchor_item );
            } );

    SetFinalizeHandler(
            [&]( const int& aFinalState )
            {
                done = true;
            } );

    statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
    statusPopup.Popup();
    canvas()->SetStatusPopup( statusPopup.GetPanel() );

    // Drop into the main event loop
    Main( aEvent );

    ClearHandlers();
    canvas()->SetStatusPopup( nullptr );
    frame()->PopTool( aEvent );
    return 0;
}


void PCB_PICKER_TOOL::setTransitions()
{
    // clang-format off
    Go( &PCB_PICKER_TOOL::Main,                     ACTIONS::pickerTool.MakeEvent() );
    Go( &PCB_PICKER_TOOL::Main,                     ACTIONS::pickerSubTool.MakeEvent() );
    Go( &PCB_PICKER_TOOL::SelectItemInteractively,  PCB_ACTIONS::selectItemInteractively.MakeEvent() );
    Go( &PCB_PICKER_TOOL::SelectPointInteractively, PCB_ACTIONS::selectPointInteractively.MakeEvent() );
    // clang-format on
}
