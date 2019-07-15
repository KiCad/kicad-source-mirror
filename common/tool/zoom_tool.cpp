/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_draw_panel_gal.h>
#include <eda_draw_frame.h>
#include <id.h>
#include <preview_items/selection_area.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <view/view.h>
#include <view/view_controls.h>


ZOOM_TOOL::ZOOM_TOOL() :
        TOOL_INTERACTIVE( "common.Control.zoomTool" )
{
    m_frame = NULL;
}


ZOOM_TOOL::~ZOOM_TOOL() {}


void ZOOM_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
}


int ZOOM_TOOL::Main( const TOOL_EVENT& aEvent )
{
    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );

    while( TOOL_EVENT* evt = Wait() )
    {
        m_frame->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
            break;

        else if( evt->IsDrag( BUT_LEFT ) || evt->IsDrag( BUT_RIGHT ) )
        {
            if( selectRegion() )
                break;
        }

        else
            evt->SetPassEvent();
    }

    // Exit zoom tool
    m_frame->PopTool( tool );
    return 0;
}


bool ZOOM_TOOL::selectRegion()
{
    bool                cancelled = false;
    KIGFX::VIEW*        view = getView();
    EDA_DRAW_PANEL_GAL* canvas = m_frame->GetCanvas();

    getViewControls()->SetAutoPan( true );

    KIGFX::PREVIEW::SELECTION_AREA area;
    view->Add( &area );

    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            cancelled = true;
            break;
        }

        if( evt->IsDrag( BUT_LEFT ) || evt->IsDrag( BUT_RIGHT ) )
        {
            area.SetOrigin( evt->DragOrigin() );
            area.SetEnd( evt->Position() );
            view->SetVisible( &area, true );
            view->Update( &area, KIGFX::GEOMETRY );
        }

        if( evt->IsMouseUp( BUT_LEFT ) || evt->IsMouseUp( BUT_RIGHT ) )
        {
            view->SetVisible( &area, false );
            auto selectionBox = area.ViewBBox();

            if( selectionBox.GetWidth() == 0 || selectionBox.GetHeight() == 0 )
            {
                break;
            }
            else
            {
                VECTOR2D sSize = view->ToWorld( canvas->GetClientSize(), false );
                VECTOR2D vSize = selectionBox.GetSize();
                double scale;
                double ratio = std::max( fabs( vSize.x / sSize.x ), fabs( vSize.y / sSize.y ) );

                if( evt->IsMouseUp( BUT_LEFT ) )
                    scale = view->GetScale() / ratio;
                else
                    scale = view->GetScale() * ratio;

                view->SetScale( scale );
                view->SetCenter( selectionBox.Centre() );

                break;
            }
        }
    }

    view->SetVisible( &area, false );
    view->Remove( &area );
    getViewControls()->SetAutoPan( false );

    return cancelled;
}


void ZOOM_TOOL::setTransitions()
{
    Go( &ZOOM_TOOL::Main, ACTIONS::zoomTool.MakeEvent() );
}
