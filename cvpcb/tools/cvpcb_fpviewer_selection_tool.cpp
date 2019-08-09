/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <functional>
using namespace std::placeholders;

#include <bitmaps.h>
#include <preview_items/ruler_item.h>
#include <tool/tool_event.h>
#include <tools/cvpcb_actions.h>
#include <tools/cvpcb_fpviewer_selection_tool.h>
#include <view/view.h>
#include <view/view_controls.h>

CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL() :
        TOOL_INTERACTIVE( "cvpcb.FootprintViewerInteractiveSelection" ),
        m_frame( nullptr )
{
}


bool CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Init()
{
    getEditFrame<DISPLAY_FOOTPRINTS_FRAME>()->AddStandardSubMenus( m_menu );
    return true;
}


void CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<DISPLAY_FOOTPRINTS_FRAME>();
}


int CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( m_frame->ToolStackIsEmpty() )
            m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );

        // This is kind of hacky: activate RMB drag on any event.
        // There doesn't seem to be any other good way to tell when another tool
        // is canceled and control returns to the selection tool, except by the
        // fact that the selection tool starts to get events again.
        if( m_frame->IsCurrentTool( ACTIONS::selectionTool ) )
        {
            getViewControls()->SetAdditionalPanButtons( false, true );
        }

        // Disable RMB pan for other tools; they can re-enable if desired
        if( evt->IsActivate() )
        {
            getViewControls()->SetAdditionalPanButtons( false, false );
        }

        // single click? Select single object
        if( evt->IsClick( BUT_LEFT ) )
        {
            clearSelection();
        }

        // right click? if there is any object - show the context menu
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selection );
        }

        else if( evt->IsCancel() || evt->Action() == TA_UNDO_REDO_PRE )
        {
            clearSelection();
        }

        else
            evt->SetPassEvent();
    }

    // This tool is supposed to be active forever
    wxASSERT( false );

    return 0;
}


int CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::MeasureTool( const TOOL_EVENT& aEvent )
{
    auto& view = *getView();
    auto& controls = *getViewControls();
    auto  previous_settings = controls.GetSettings();

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;
    KIGFX::PREVIEW::RULER_ITEM                 ruler( twoPtMgr, m_frame->GetUserUnits() );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetSnapping( true );
    controls.SetAdditionalPanButtons( false, true );

    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );
        const VECTOR2I cursorPos = controls.GetCursorPosition();

        auto clearRuler = [&]() {
            view.SetVisible( &ruler, false );
            controls.SetAutoPan( false );
            controls.CaptureCursor( false );
            originSet = false;
        };

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
                clearRuler();
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }

        else if( evt->IsActivate() )
        {
            if( originSet )
                clearRuler();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }

        // click or drag starts
        else if( !originSet && ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            if( !evt->IsDrag( BUT_LEFT ) )
            {
                twoPtMgr.SetOrigin( cursorPos );
                twoPtMgr.SetEnd( cursorPos );
            }

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }

        else if( !originSet && evt->IsMotion() )
        {
            // make sure the origin is set before a drag starts
            // otherwise you can miss a step
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );
        }

        // second click or mouse up after drag ends
        else if( originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );

            view.SetVisible( &ruler, false );
        }

        // move or drag when origin set updates rules
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            twoPtMgr.SetAngleSnap( evt->Modifier( MD_CTRL ) );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }

        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_selection );
        }

        else
            evt->SetPassEvent();
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );
    controls.ApplySettings( previous_settings );
    return 0;
}

void CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::setTransitions()
{
    Go( &CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::Main,
            CVPCB_ACTIONS::selectionActivate.MakeEvent() );
    Go( &CVPCB_FOOTPRINT_VIEWER_SELECTION_TOOL::MeasureTool, ACTIONS::measureTool.MakeEvent() );
}
