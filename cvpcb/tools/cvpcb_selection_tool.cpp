/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <limits>

#include <functional>
using namespace std::placeholders;

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <hotkeys.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <preview_items/ruler_item.h>

#include <cvpcb_id.h>

#include "cvpcb_selection_tool.h"
#include "cvpcb_actions.h"

// Selection tool actions
TOOL_ACTION CVPCB_ACTIONS::selectionActivate( "cvpcb.InteractiveSelection",
        AS_GLOBAL, 0,
        "", "", NULL, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere


TOOL_ACTION CVPCB_ACTIONS::measureTool( "cvpcb.InteractiveSelection.measureTool",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MEASURE_TOOL ),
        _( "Measure Tool" ), _( "Interactively measure distance between points" ),
        nullptr, AF_ACTIVATE );


CVPCB_SELECTION_TOOL::CVPCB_SELECTION_TOOL() :
        TOOL_INTERACTIVE( "cvpcb.InteractiveSelection" ),
        m_frame( NULL ), m_menu( *this )
{
}


CVPCB_SELECTION_TOOL::~CVPCB_SELECTION_TOOL()
{
    getView()->Remove( &m_selection );
}


bool CVPCB_SELECTION_TOOL::Init()
{
    m_menu.AddStandardSubMenus( *getEditFrame<DISPLAY_FOOTPRINTS_FRAME>() );

    return true;
}


void CVPCB_SELECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<DISPLAY_FOOTPRINTS_FRAME>();
}


int CVPCB_SELECTION_TOOL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // This is kind of hacky: activate RMB drag on any event.
        // There doesn't seem to be any other good way to tell when another tool
        // is canceled and control returns to the selection tool, except by the
        // fact that the selection tool starts to get events again.
        if( m_frame->GetToolId() == ID_NO_TOOL_SELECTED)
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

        else if( evt->Action() == TA_CONTEXT_MENU_CLOSED )
        {
            m_menu.CloseContextMenu( evt );
        }
    }

    // This tool is supposed to be active forever
    wxASSERT( false );

    return 0;
}


SELECTION& CVPCB_SELECTION_TOOL::GetSelection()
{
    return m_selection;
}


void CVPCB_SELECTION_TOOL::setTransitions()
{
    Go( &CVPCB_SELECTION_TOOL::Main,             CVPCB_ACTIONS::selectionActivate.MakeEvent() );
    Go( &CVPCB_SELECTION_TOOL::MeasureTool,      CVPCB_ACTIONS::measureTool.MakeEvent() );
}

/*
void CVPCB_SELECTION_TOOL::zoomFitSelection( void )
{
    //Should recalculate the view to zoom in on the selection
    auto selectionBox = m_selection.ViewBBox();
    auto canvas = m_frame->GetGalCanvas();
    auto view = getView();

    VECTOR2D screenSize = view->ToWorld( canvas->GetClientSize(), false );

    if( !( selectionBox.GetWidth() == 0 ) || !( selectionBox.GetHeight() == 0 ) )
    {
        VECTOR2D vsize = selectionBox.GetSize();
        double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                fabs( vsize.y / screenSize.y ) );
        view->SetScale( scale );
        view->SetCenter( selectionBox.Centre() );
        view->Add( &m_selection );
    }

    m_frame->GetGalCanvas()->ForceRefresh();
}
*/

int CVPCB_SELECTION_TOOL::MeasureTool( const TOOL_EVENT& aEvent )
{
    auto& view = *getView();
    auto& controls = *getViewControls();
    auto previous_settings = controls.GetSettings();

    Activate();
    m_frame->SetToolID( ID_TB_MEASUREMENT_TOOL, wxCURSOR_PENCIL,
                        _( "Measure distance" ) );

    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr, m_frame->GetUserUnits() );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    bool originSet = false;

    controls.ShowCursor( true );
    controls.SetSnapping( true );
    controls.SetAdditionalPanButtons( false, true );

    while( auto evt = Wait() )
    {
        const VECTOR2I cursorPos = controls.GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            break;
        }

        // click or drag starts
        else if( !originSet &&
                ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
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
        else if( originSet &&
                ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );

            view.SetVisible( &ruler, false );
        }

        // move or drag when origin set updates rules
        else if( originSet &&
                ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
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
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );

    controls.ApplySettings( previous_settings );

    m_frame->SetNoToolSelected();

    return 0;
}

const BOX2I SELECTION::ViewBBox() const
{
    EDA_RECT eda_bbox;

    if( Size() == 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
    }
    else if( Size() > 1 )
    {
        eda_bbox = Front()->GetBoundingBox();
        auto i = m_items.begin();
        ++i;

        for( ; i != m_items.end(); ++i )
        {
            eda_bbox.Merge( (*i)->GetBoundingBox() );
        }
    }

    return BOX2I( eda_bbox.GetOrigin(), eda_bbox.GetSize() );
}


const KIGFX::VIEW_GROUP::ITEMS SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

    for( auto item : m_items )
        items.push_back( item );

    return items;
}
