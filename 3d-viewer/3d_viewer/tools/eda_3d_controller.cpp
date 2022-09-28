/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <eda_3d_canvas.h>
#include <eda_3d_viewer_frame.h>
#include <id.h>
#include <kiface_base.h>
#include <tools/eda_3d_controller.h>
#include "eda_3d_actions.h"
#include "dialogs/panel_preview_3d_model.h"
#include <3d_rendering/opengl/render_3d_opengl.h>


bool EDA_3D_CONTROLLER::Init()
{
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    ctxMenu.AddItem( ACTIONS::zoomInCenter,       SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::zoomOutCenter,      SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewTop,     SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewBottom,  SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewRight,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewLeft,    SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewFront,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewBack,    SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( EDA_3D_ACTIONS::flipView,    SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveLeft,    SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveRight,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveUp,      SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveDown,    SELECTION_CONDITIONS::ShowAlways );

    return true;
}


void EDA_3D_CONTROLLER::Reset( RESET_REASON aReason )
{
    TOOLS_HOLDER* holder = m_toolMgr->GetToolHolder();

    wxASSERT( holder );

    m_canvas = nullptr;
    m_boardAdapter = nullptr;
    m_camera = nullptr;

    if( holder )
    {
        m_canvas = dynamic_cast<EDA_3D_CANVAS*>( holder->GetToolCanvas() );

        EDA_3D_BOARD_HOLDER* holder3d =
                dynamic_cast<EDA_3D_BOARD_HOLDER*>( holder );

        wxASSERT( holder3d );

        if( holder3d )
        {
            m_boardAdapter = &holder3d->GetAdapter();
            m_camera = &holder3d->GetCurrentCamera();
        }
    }
}


int EDA_3D_CONTROLLER::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );
    SELECTION         dummySel;

    if( conditionalMenu )
        conditionalMenu->Evaluate( dummySel );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


int EDA_3D_CONTROLLER::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->IsCancelInteractive() )
        {
            wxWindow* canvas = m_toolMgr->GetToolHolder()->GetToolCanvas();
            wxWindow* topLevelParent = canvas->GetParent();

            while( topLevelParent && !topLevelParent->IsTopLevel() )
                topLevelParent = topLevelParent->GetParent();

            if( topLevelParent && dynamic_cast<DIALOG_SHIM*>( topLevelParent ) )
            {
                DIALOG_SHIM* dialog = static_cast<DIALOG_SHIM*>( topLevelParent );

                if( dialog->IsQuasiModal() )
                    dialog->EndQuasiModal( wxID_CANCEL );
                else
                    dialog->EndModal( wxID_CANCEL );
            }
            else
            {
                evt->SetPassEvent();
            }
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

    return 0;
}


int EDA_3D_CONTROLLER::ViewControl( const TOOL_EVENT& aEvent )
{
    m_canvas->SetView3D( aEvent.Parameter<VIEW3D_TYPE>() );

    return 0;
}


int EDA_3D_CONTROLLER::PanControl( const TOOL_EVENT& aEvent )
{
    switch( aEvent.Parameter<ACTIONS::CURSOR_EVENT_TYPE>() )
    {
    case ACTIONS::CURSOR_UP:    m_canvas->SetView3D( VIEW3D_TYPE::VIEW3D_PAN_UP );    break;
    case ACTIONS::CURSOR_DOWN:  m_canvas->SetView3D( VIEW3D_TYPE::VIEW3D_PAN_DOWN );  break;
    case ACTIONS::CURSOR_LEFT:  m_canvas->SetView3D( VIEW3D_TYPE::VIEW3D_PAN_LEFT );  break;
    case ACTIONS::CURSOR_RIGHT: m_canvas->SetView3D( VIEW3D_TYPE::VIEW3D_PAN_RIGHT ); break;
    default:                    wxFAIL;                           break;
    }

    return 0;
}


int EDA_3D_CONTROLLER::RotateView( const TOOL_EVENT& aEvent )
{
    double rotIncrement = glm::radians( m_rotationIncrement );

    switch( aEvent.Parameter<ROTATION_DIR>() )
    {
    case ROTATION_DIR::X_CW:  m_camera->RotateX( -rotIncrement ); break;
    case ROTATION_DIR::X_CCW: m_camera->RotateX( rotIncrement );  break;
    /// Y rotations are backward b/c the RHR has Y pointing into the screen
    case ROTATION_DIR::Y_CW:  m_camera->RotateY( rotIncrement );  break;
    case ROTATION_DIR::Y_CCW: m_camera->RotateY( -rotIncrement ); break;
    case ROTATION_DIR::Z_CW:  m_camera->RotateZ( -rotIncrement ); break;
    case ROTATION_DIR::Z_CCW: m_camera->RotateZ( rotIncrement );  break;
    default:                wxFAIL;                               break;
    }

    if( m_boardAdapter->m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_canvas->Request_refresh();
    else
        m_canvas->RenderRaytracingRequest();

    return 0;
}


int EDA_3D_CONTROLLER::SetMaterial( const TOOL_EVENT& aEvent )
{
    m_boardAdapter->m_Cfg->m_Render.material_mode = aEvent.Parameter<MATERIAL_MODE>();

    if( auto* viewer = dynamic_cast<EDA_3D_VIEWER_FRAME*>( m_toolMgr->GetToolHolder() ) )
        viewer->NewDisplay( true );
    else
        m_canvas->Request_refresh();

    return 0;
}


int EDA_3D_CONTROLLER::ToggleOrtho( const TOOL_EVENT& aEvent )
{
    m_camera->ToggleProjection();

    if( m_boardAdapter->m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        m_canvas->Request_refresh();
    else
        m_canvas->RenderRaytracingRequest();

    return 0;
}

int EDA_3D_CONTROLLER::ToggleVisibility( const TOOL_EVENT& aEvent )
{
    bool reload = false;

#define FLIP( x ) x = !x

    if( aEvent.IsAction( &EDA_3D_ACTIONS::showTHT ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_footprints_normal );
        reload = true;
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::showSMD ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_footprints_insert );
        reload = true;
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::showVirtual ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_footprints_virtual );
        reload = true;
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::showNotInPosFile ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_footprints_not_in_posfile );
        reload = true;
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::showDNP ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_footprints_dnp );
        reload = true;
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::showBBoxes ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.opengl_show_model_bbox );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleRealisticMode ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.realistic );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleBoardBody ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_board_body );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::showAxis ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_axis );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleZones ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_zones );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleAdhesive ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_adhesive );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleSilk ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_silkscreen );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleSolderMask ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_soldermask );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleSolderPaste ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_solderpaste );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleComments ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_comments );
    }
    else if( aEvent.IsAction( &EDA_3D_ACTIONS::toggleECO ) )
    {
        FLIP( m_boardAdapter->m_Cfg->m_Render.show_eco );
    }

    if( reload )
    {
        if( m_boardAdapter->m_Cfg->m_Render.engine == RENDER_ENGINE::OPENGL )
        {
            auto* renderer = static_cast<RENDER_3D_OPENGL*>( m_canvas->GetCurrentRender() );
            renderer->Load3dModelsIfNeeded();
            m_canvas->Request_refresh();
        }
        else
        {
            m_canvas->RenderRaytracingRequest();
        }
    }
    else
    {
        if( auto viewer = dynamic_cast<EDA_3D_VIEWER_FRAME*>( m_toolMgr->GetToolHolder() ) )
            viewer->NewDisplay( true );
        else
            m_canvas->Request_refresh();
    }

    return 0;
}


int EDA_3D_CONTROLLER::On3DGridSelection( const TOOL_EVENT& aEvent )
{
    m_boardAdapter->m_Cfg->m_Render.grid_type = aEvent.Parameter<GRID3D_TYPE>();

    if( m_canvas )
        m_canvas->Request_refresh();

    return 0;
}


int EDA_3D_CONTROLLER::ZoomRedraw( const TOOL_EVENT& aEvent )
{
    m_canvas->Request_refresh();
    return 0;
}


int EDA_3D_CONTROLLER::ZoomInOut( const TOOL_EVENT& aEvent )
{
    bool direction = aEvent.IsAction( &ACTIONS::zoomIn );
    return doZoomInOut( direction, true );
}


int EDA_3D_CONTROLLER::ZoomInOutCenter( const TOOL_EVENT& aEvent )
{
    bool direction = aEvent.IsAction( &ACTIONS::zoomInCenter );
    return doZoomInOut( direction, false );
}


int EDA_3D_CONTROLLER::doZoomInOut( bool aDirection, bool aCenterOnCursor )
{
    if( m_canvas )
    {
        m_canvas->SetView3D( aDirection ? VIEW3D_TYPE::VIEW3D_ZOOM_IN : VIEW3D_TYPE::VIEW3D_ZOOM_OUT );
        m_canvas->DisplayStatus();
    }

    return 0;
}


int EDA_3D_CONTROLLER::ZoomFitScreen( const TOOL_EVENT& aEvent )
{
    if( m_canvas )
    {
        m_canvas->SetView3D( VIEW3D_TYPE::VIEW3D_FIT_SCREEN );
        m_canvas->DisplayStatus();
    }

    return 0;
}


void EDA_3D_CONTROLLER::setTransitions()
{
    Go( &EDA_3D_CONTROLLER::Main,               EDA_3D_ACTIONS::controlActivate.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::UpdateMenu,         ACTIONS::updateMenu.MakeEvent() );

    // Pan control
    Go( &EDA_3D_CONTROLLER::PanControl,         ACTIONS::panUp.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::PanControl,         ACTIONS::panDown.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::PanControl,         ACTIONS::panLeft.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::PanControl,         ACTIONS::panRight.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::PanControl,         EDA_3D_ACTIONS::moveUp.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::PanControl,         EDA_3D_ACTIONS::moveDown.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::PanControl,         EDA_3D_ACTIONS::moveLeft.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::PanControl,         EDA_3D_ACTIONS::moveRight.MakeEvent() );

    // View rotation
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::viewTop.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::viewBottom.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::viewLeft.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::viewRight.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::viewFront.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::viewBack.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::pivotCenter.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::homeView.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::resetView.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ViewControl,        EDA_3D_ACTIONS::flipView.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::RotateView,         EDA_3D_ACTIONS::rotateXCW.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::RotateView,         EDA_3D_ACTIONS::rotateXCCW.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::RotateView,         EDA_3D_ACTIONS::rotateYCW.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::RotateView,         EDA_3D_ACTIONS::rotateYCCW.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::RotateView,         EDA_3D_ACTIONS::rotateZCW.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::RotateView,         EDA_3D_ACTIONS::rotateZCCW.MakeEvent() );

    // Zoom control
    Go( &EDA_3D_CONTROLLER::ZoomRedraw,         ACTIONS::zoomRedraw.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ZoomInOutCenter,    ACTIONS::zoomInCenter.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ZoomInOutCenter,    ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ZoomFitScreen,      ACTIONS::zoomFitScreen.MakeEvent() );
    // zoom in/out at cursor does not exist in 3D viewer but because F1 and F2 keys generate
    // a zoomIn/zoomOut event, these events must be captured to use these hot keys. The actual
    // zoom is the same as ZoomInOutCenter
    Go( &EDA_3D_CONTROLLER::ZoomInOut,          ACTIONS::zoomIn.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ZoomInOut,          ACTIONS::zoomOut.MakeEvent() );

    // Grid
    Go( &EDA_3D_CONTROLLER::On3DGridSelection,  EDA_3D_ACTIONS::noGrid.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::On3DGridSelection,  EDA_3D_ACTIONS::show10mmGrid.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::On3DGridSelection,  EDA_3D_ACTIONS::show5mmGrid.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::On3DGridSelection,  EDA_3D_ACTIONS::show2_5mmGrid.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::On3DGridSelection,  EDA_3D_ACTIONS::show1mmGrid.MakeEvent() );

    // Material
    Go( &EDA_3D_CONTROLLER::SetMaterial,        EDA_3D_ACTIONS::materialNormal.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::SetMaterial,        EDA_3D_ACTIONS::materialDiffuse.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::SetMaterial,        EDA_3D_ACTIONS::materialCAD.MakeEvent() );

    // Visibility
    Go( &EDA_3D_CONTROLLER::ToggleOrtho,        EDA_3D_ACTIONS::toggleOrtho.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showTHT.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showSMD.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showVirtual.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showNotInPosFile.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showDNP.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showVirtual.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showBBoxes.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleRealisticMode.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleBoardBody.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::showAxis.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleZones.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleAdhesive.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleSilk.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleSolderMask.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleSolderPaste.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleComments.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleVisibility,   EDA_3D_ACTIONS::toggleECO.MakeEvent() );
}


