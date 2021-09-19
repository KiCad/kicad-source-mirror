/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <3d_viewer/eda_3d_viewer_frame.h>
#include <kiplatform/ui.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <preview_items/ruler_item.h>
#include <tool/actions.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_viewer_tools.h>
#include <view/view_controls.h>
#include <wx/debug.h>


bool PCB_VIEWER_TOOLS::Init()
{
    // Populate the context menu displayed during the tool (primarily the measure tool)
    auto activeToolCondition =
        [ this ] ( const SELECTION& aSel )
        {
            return !frame()->ToolStackIsEmpty();
        };

    auto& ctxMenu = m_menu.GetMenu();

    // "Cancel" goes at the top of the context menu when a tool is active
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolCondition, 1 );
    ctxMenu.AddSeparator( 1 );

    frame()->AddStandardSubMenus( m_menu );

    return true;
}


void PCB_VIEWER_TOOLS::Reset( RESET_REASON aReason )
{
}


int PCB_VIEWER_TOOLS::Show3DViewer( const TOOL_EVENT& aEvent )
{
    EDA_3D_VIEWER_FRAME* draw3DFrame = frame()->CreateAndShow3D_Frame();

    if( frame()->IsType( FRAME_FOOTPRINT_VIEWER )
     || frame()->IsType( FRAME_FOOTPRINT_VIEWER_MODAL )
     || frame()->IsType( FRAME_FOOTPRINT_WIZARD ) )
    {
        // A stronger version of Raise() which promotes the window to its parent's level.
        KIPLATFORM::UI::ReparentQuasiModal( draw3DFrame );
    }

    // And load or update the current board
    frame()->Update3DView( true, true );

    return 0;
}


template<class T> void Flip( T& aValue )
{
    aValue = !aValue;
}


int PCB_VIEWER_TOOLS::ShowPadNumbers( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts.m_DisplayPadNum );
    frame()->SetDisplayOptions( opts );

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
            view()->Update( pad, KIGFX::REPAINT );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_VIEWER_TOOLS::PadDisplayMode( const TOOL_EVENT& aEvent )
{
    auto opts = displayOptions();

    Flip( opts.m_DisplayPadFill );
    frame()->SetDisplayOptions( opts );

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
            view()->Update( pad, KIGFX::REPAINT );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_VIEWER_TOOLS::GraphicOutlines( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = displayOptions();

    Flip( opts.m_DisplayGraphicsFill );
    frame()->SetDisplayOptions( opts );

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_FP_SHAPE_T )
                view()->Update( item, KIGFX::REPAINT );
        }
    }

    for( BOARD_ITEM* item : board()->Drawings() )
    {
        KICAD_T t = item->Type();

        if( t == PCB_SHAPE_T || BaseType( t ) == PCB_DIMENSION_T || t == PCB_TARGET_T )
            view()->Update( item, KIGFX::REPAINT );
    }

    canvas()->Refresh();

    return 0;
}


int PCB_VIEWER_TOOLS::TextOutlines( const TOOL_EVENT& aEvent )
{
    PCB_DISPLAY_OPTIONS opts = displayOptions();

    Flip( opts.m_DisplayTextFill );
    frame()->SetDisplayOptions( opts );

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        view()->Update( &fp->Reference(), KIGFX::REPAINT );
        view()->Update( &fp->Value(), KIGFX::REPAINT );

        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_FP_TEXT_T )
                view()->Update( item, KIGFX::REPAINT );
        }
    }

    for( BOARD_ITEM* item : board()->Drawings() )
    {
        KICAD_T t = item->Type();

        if( t == PCB_TEXT_T || BaseType( t ) == PCB_DIMENSION_T )
            view()->Update( item, KIGFX::REPAINT );
    }

    canvas()->Refresh();

    return 0;
}


using KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER;


int PCB_VIEWER_TOOLS::MeasureTool( const TOOL_EVENT& aEvent )
{
    if( IsFootprintFrame() && !frame()->GetModel() )
        return 0;

    if( frame()->IsCurrentTool( ACTIONS::measureTool ) )
        return 0;

    auto& view     = *getView();
    auto& controls = *getViewControls();

    std::string tool = aEvent.GetCommandStr().get();
    frame()->PushTool( tool );

    TWO_POINT_GEOMETRY_MANAGER twoPtMgr;
    PCB_GRID_HELPER            grid( m_toolMgr, frame()->GetMagneticItemsSettings() );
    bool                       originSet = false;
    EDA_UNITS                  units = frame()->GetUserUnits();
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr, units );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    auto setCursor =
            [&]()
            {
                frame()->GetCanvas()->SetCurrentCursor( KICURSOR::MEASURE );
            };

    auto cleanup =
            [&] ()
            {
                view.SetVisible( &ruler, false );
                controls.SetAutoPan( false );
                controls.CaptureCursor( false );
                originSet = false;
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls.ShowCursor( true );
    controls.SetAutoPan( false );
    controls.CaptureCursor( false );
    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( view.GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        const VECTOR2I cursorPos = grid.BestSnapAnchor( controls.GetMousePosition(), nullptr );
        controls.ForceCursorPosition(true, cursorPos );

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
            {
                cleanup();
            }
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( originSet )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                frame()->PopTool( tool );
                break;
            }
        }
        // click or drag starts
        else if( !originSet && ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }
        // second click or mouse up after drag ends
        else if( originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );
        }
        // move or drag when origin set updates rules
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            twoPtMgr.SetAngleSnap( frame()->Settings().m_Use45DegreeLimit );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            if( frame()->GetUserUnits() != units )
            {
                units = frame()->GetUserUnits();
                ruler.SwitchUnits( units );
                view.Update( &ruler, KIGFX::GEOMETRY );
                canvas()->Refresh();
            }
            evt->SetPassEvent();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );

    frame()->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls.SetAutoPan( false );
    controls.CaptureCursor( false );
    return 0;
}


void PCB_VIEWER_TOOLS::setTransitions()
{
    Go( &PCB_VIEWER_TOOLS::Show3DViewer,      ACTIONS::show3DViewer.MakeEvent() );

    // Display modes
    Go( &PCB_VIEWER_TOOLS::ShowPadNumbers,    PCB_ACTIONS::showPadNumbers.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::PadDisplayMode,    PCB_ACTIONS::padDisplayMode.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::GraphicOutlines,   PCB_ACTIONS::graphicsOutlines.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::TextOutlines,      PCB_ACTIONS::textOutlines.MakeEvent() );

    Go( &PCB_VIEWER_TOOLS::MeasureTool,       ACTIONS::measureTool.MakeEvent() );
}
