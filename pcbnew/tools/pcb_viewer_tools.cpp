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

#include "tools/pcb_viewer_tools.h"

#include <wx/clipbrd.h>

#include <3d_viewer/eda_3d_viewer_frame.h>
#include <footprint_editor_settings.h>
#include <gal/graphics_abstraction_layer.h>
#include <kiplatform/ui.h>
#include <pcb_base_frame.h>
#include <preview_items/ruler_item.h>
#include <preview_items/two_point_geom_manager.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_actions.h>


bool PCB_VIEWER_TOOLS::Init()
{
    // Populate the context menu displayed during the tool (primarily the measure tool)
    auto activeToolCondition =
            [this] ( const SELECTION& aSel )
            {
                return !frame()->ToolStackIsEmpty();
            };

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    // "Cancel" goes at the top of the context menu when a tool is active
    if( !m_isDefaultTool )
    {
        ctxMenu.AddItem( ACTIONS::cancelInteractive,     activeToolCondition, 1 );
        ctxMenu.AddSeparator( 1 );
    }

    ctxMenu.AddSeparator(                                activeToolCondition, 2 );

    ctxMenu.AddItem( ACTIONS::copy,                      activeToolCondition, 3 );
    ctxMenu.AddSeparator(                                activeToolCondition, 3 );

    frame()->AddStandardSubMenus( *m_menu.get() );

    return true;
}


void PCB_VIEWER_TOOLS::Reset( RESET_REASON aReason )
{
}


int PCB_VIEWER_TOOLS::Show3DViewer( const TOOL_EVENT& aEvent )
{
    bool do_reload_board = true;    // reload board flag

    // At EDA_3D_VIEWER_FRAME creation, the current board is loaded, so disable loading
    // the current board if the 3D frame is not yet created
    if( frame()->Get3DViewerFrame() == nullptr )
        do_reload_board = false;

    EDA_3D_VIEWER_FRAME* draw3DFrame = frame()->CreateAndShow3D_Frame();

    if( frame()->IsType( FRAME_FOOTPRINT_VIEWER )
     || frame()->IsType( FRAME_FOOTPRINT_WIZARD ) )
    {
        // A stronger version of Raise() which promotes the window to its parent's level.
        KIPLATFORM::UI::ReparentModal( draw3DFrame );
    }

    // And load or update the current board (if needed)
    if( do_reload_board )
        frame()->Update3DView( true, true );

    return 0;
}


template<class T> void Flip( T& aValue )
{
    aValue = !aValue;
}


int PCB_VIEWER_TOOLS::NextLineMode( const TOOL_EVENT& toolEvent )
{
    if( frame()->IsType( FRAME_PCB_EDITOR ) )
    {
        PCBNEW_SETTINGS* settings = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

        switch( settings->m_AngleSnapMode )
        {
        case LEADER_MODE::DIRECT: settings->m_AngleSnapMode = LEADER_MODE::DEG45; break;
        case LEADER_MODE::DEG45:  settings->m_AngleSnapMode = LEADER_MODE::DEG90; break;
        default:                  settings->m_AngleSnapMode = LEADER_MODE::DIRECT; break;
        }
    }
    else if( frame()->IsType( FRAME_FOOTPRINT_EDITOR ) )
    {
        FOOTPRINT_EDITOR_SETTINGS* settings = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );

        switch( settings->m_AngleSnapMode )
        {
        case LEADER_MODE::DIRECT: settings->m_AngleSnapMode = LEADER_MODE::DEG45; break;
        case LEADER_MODE::DEG45:  settings->m_AngleSnapMode = LEADER_MODE::DEG90; break;
        default:                  settings->m_AngleSnapMode = LEADER_MODE::DIRECT; break;
        }
    }
    else
    {
        LEADER_MODE& mode = frame()->GetViewerSettingsBase()->m_ViewersDisplay.m_AngleSnapMode;

        switch( mode )
        {
        case LEADER_MODE::DIRECT: mode = LEADER_MODE::DEG45; break;
        case LEADER_MODE::DEG45:  mode = LEADER_MODE::DEG90; break;
        default:                  mode = LEADER_MODE::DIRECT; break;
        }
    }

    frame()->UpdateStatusBar();

    // Notify other tools/UI (toolbars) that the angle snap mode has changed
    m_toolMgr->RunAction( PCB_ACTIONS::angleSnapModeChanged );

    return 0;
}


int PCB_VIEWER_TOOLS::ShowPadNumbers( const TOOL_EVENT& aEvent )
{
    PCB_VIEWERS_SETTINGS_BASE* cfg = frame()->GetViewerSettingsBase();
    Flip( cfg->m_ViewersDisplay.m_DisplayPadNumbers );

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
    PCB_VIEWERS_SETTINGS_BASE* cfg = frame()->GetViewerSettingsBase();
    Flip( cfg->m_ViewersDisplay.m_DisplayPadFill );

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
    PCB_VIEWERS_SETTINGS_BASE* cfg = frame()->GetViewerSettingsBase();
    Flip( cfg->m_ViewersDisplay.m_DisplayGraphicsFill );

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            KICAD_T t = item->Type();

            if( t == PCB_SHAPE_T || BaseType( t ) == PCB_DIMENSION_T )
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
    PCB_VIEWERS_SETTINGS_BASE* cfg = frame()->GetViewerSettingsBase();
    Flip( cfg->m_ViewersDisplay.m_DisplayTextFill );

    for( FOOTPRINT* fp : board()->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
        {
            wxCHECK2( field, continue );

            view()->Update( field, KIGFX::REPAINT );
        }

        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( item->Type() == PCB_TEXT_T )
                view()->Update( item, KIGFX::REPAINT );
        }
    }

    for( BOARD_ITEM* item : board()->Drawings() )
    {
        KICAD_T t = item->Type();

        if( t == PCB_TEXT_T || t == PCB_TEXTBOX_T || BaseType( t ) == PCB_DIMENSION_T )
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

    frame()->PushTool( aEvent );

    bool invertXAxis = displayOptions().m_DisplayInvertXAxis;
    bool invertYAxis = displayOptions().m_DisplayInvertYAxis;

    if( IsFootprintFrame() )
    {
        invertXAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertXAxis;
        invertYAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertYAxis;
    }

    TWO_POINT_GEOMETRY_MANAGER twoPtMgr;
    PCB_GRID_HELPER            grid( m_toolMgr, frame()->GetMagneticItemsSettings() );
    bool                       originSet = false;
    EDA_UNITS                  units = frame()->GetUserUnits();
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr, pcbIUScale, units, invertXAxis, invertYAxis );

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
                controls.ForceCursorPosition( false );
                originSet = false;
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls.ShowCursor( true );
    controls.SetAutoPan( false );
    controls.CaptureCursor( false );
    controls.ForceCursorPosition( false );

    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( view.GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = evt->HasPosition() ? evt->Position() : controls.GetMousePosition();

        if( !evt->IsActivate() && !evt->IsCancelInteractive() )
        {
            // If we are switching, the canvas may not be valid any more
            cursorPos = grid.BestSnapAnchor( cursorPos, nullptr );
            controls.ForceCursorPosition( true, cursorPos );
        }
        else
        {
            grid.FullReset();
        }

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
            {
                cleanup();
            }
            else if( m_isDefaultTool )
            {
                view.SetVisible( &ruler, false );
            }
            else
            {
                frame()->PopTool( aEvent );
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
                frame()->PopTool( aEvent );
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
            auto snap = LEADER_MODE::DIRECT;

            if( frame()->IsType( FRAME_PCB_EDITOR ) )
            {
                snap = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" )->m_AngleSnapMode;
            }
            else if( frame()->IsType( FRAME_FOOTPRINT_EDITOR ) )
            {
                snap = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" )->m_AngleSnapMode;
            }
            else
            {
                snap = frame()->GetViewerSettingsBase()->m_ViewersDisplay.m_AngleSnapMode;
            }

            twoPtMgr.SetAngleSnap( snap );
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
        else if( evt->IsAction( &ACTIONS::updatePreferences ) )
        {
            invertXAxis = displayOptions().m_DisplayInvertXAxis;
            invertYAxis = displayOptions().m_DisplayInvertYAxis;

            if( IsFootprintFrame() )
            {
                invertXAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertXAxis;
                invertYAxis = frame()->GetFootprintEditorSettings()->m_DisplayInvertYAxis;
            }

            ruler.UpdateDir( invertXAxis, invertYAxis );

            view.Update( &ruler, KIGFX::GEOMETRY );
            canvas()->Refresh();
            evt->SetPassEvent();
        }
        else if( evt->IsAction( &ACTIONS::copy ) )
        {
            if( originSet )
            {
                wxArrayString cursorStrings = ruler.GetDimensionStrings();
                wxString      text = wxJoin( cursorStrings, '\n' );

                if( wxTheClipboard->Open() )
                {
                    wxTheClipboard->SetData( new wxTextDataObject( text ) );
                    wxTheClipboard->Close();
                }
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu();
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
    controls.ForceCursorPosition( false );
    return 0;
}


int PCB_VIEWER_TOOLS::FootprintAutoZoom( const TOOL_EVENT& aEvent )
{
    PCB_VIEWERS_SETTINGS_BASE* cfg = dynamic_cast<PCB_VIEWERS_SETTINGS_BASE*>( frame()->config() );

    // Toggle the setting
    if( cfg )
        cfg->m_FootprintViewerAutoZoomOnSelect = !cfg->m_FootprintViewerAutoZoomOnSelect;

    return 0;
}


void PCB_VIEWER_TOOLS::setTransitions()
{
    // clang-format off
    Go( &PCB_VIEWER_TOOLS::Show3DViewer,      ACTIONS::show3DViewer.MakeEvent() );

    Go( &PCB_VIEWER_TOOLS::NextLineMode,      PCB_ACTIONS::lineModeNext.MakeEvent() );

    // Display modes
    Go( &PCB_VIEWER_TOOLS::ShowPadNumbers,    PCB_ACTIONS::showPadNumbers.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::PadDisplayMode,    PCB_ACTIONS::padDisplayMode.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::GraphicOutlines,   PCB_ACTIONS::graphicsOutlines.MakeEvent() );
    Go( &PCB_VIEWER_TOOLS::TextOutlines,      PCB_ACTIONS::textOutlines.MakeEvent() );

    Go( &PCB_VIEWER_TOOLS::FootprintAutoZoom,  PCB_ACTIONS::fpAutoZoom.MakeEvent() );

    Go( &PCB_VIEWER_TOOLS::MeasureTool,       ACTIONS::measureTool.MakeEvent() );
    // clang-format on
}
