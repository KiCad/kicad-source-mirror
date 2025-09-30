/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
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

#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <eda_3d_canvas.h>
#include <eda_3d_viewer_frame.h>
#include <id.h>
#include <kiface_base.h>
#include <tools/eda_3d_controller.h>
#include <tools/eda_3d_actions.h>
#include <dialogs/dialog_export_3d_image.h>
#include <dialogs/panel_preview_3d_model.h>
#include <dialogs/appearance_controls_3D.h>
#include <3d_rendering/opengl/render_3d_opengl.h>


bool EDA_3D_CONTROLLER::Init()
{
    std::shared_ptr<ACTION_MENU> rotateSubmenu = std::make_shared<ACTION_MENU>( true, this );
    rotateSubmenu->SetUntranslatedTitle( _HKI( "Rotate Board" ) );
    rotateSubmenu->SetIcon( BITMAPS::rotate_cw );
    m_menu->RegisterSubMenu( rotateSubmenu );

    rotateSubmenu->Add( EDA_3D_ACTIONS::rotateXCW );
    rotateSubmenu->Add( EDA_3D_ACTIONS::rotateXCCW );
    rotateSubmenu->AppendSeparator();
    rotateSubmenu->Add( EDA_3D_ACTIONS::rotateYCW );
    rotateSubmenu->Add( EDA_3D_ACTIONS::rotateYCCW );
    rotateSubmenu->AppendSeparator();
    rotateSubmenu->Add( EDA_3D_ACTIONS::rotateZCW );
    rotateSubmenu->Add( EDA_3D_ACTIONS::rotateZCCW );

    std::shared_ptr<ACTION_MENU> moveSubmenu = std::make_shared<ACTION_MENU>( true, this );
    moveSubmenu->SetUntranslatedTitle( _HKI( "Move Board" ) );
    moveSubmenu->SetIcon( BITMAPS::move );
    m_menu->RegisterSubMenu( moveSubmenu );

    moveSubmenu->Add( EDA_3D_ACTIONS::moveLeft );
    moveSubmenu->Add( EDA_3D_ACTIONS::moveRight );
    moveSubmenu->Add( EDA_3D_ACTIONS::moveUp );
    moveSubmenu->Add( EDA_3D_ACTIONS::moveDown );

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    ctxMenu.AddItem( ACTIONS::zoomInCenter,       SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::zoomOutCenter,      SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewTop,     SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewBottom,  SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewRight,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewLeft,    SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewFront,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::viewBack,    SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddMenu( rotateSubmenu.get(),         SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::flipView,    SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddMenu( moveSubmenu.get(),           SELECTION_CONDITIONS::ShowAlways );

    return true;
}


void EDA_3D_CONTROLLER::Reset( RESET_REASON aReason )
{
    m_canvas = nullptr;
    m_boardAdapter = nullptr;
    m_camera = nullptr;

    TOOLS_HOLDER* holder = m_toolMgr->GetToolHolder();

    wxCHECK( holder, /* void */ );
    wxCHECK( holder->GetToolCanvas()->GetId() == EDA_3D_CANVAS_ID, /* void */ );

    m_canvas = static_cast<EDA_3D_CANVAS*>( holder->GetToolCanvas() );

    if( EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( holder ) )
    {
        wxCHECK( frame->GetFrameType() == FRAME_PCB_DISPLAY3D, /* void */ );

        m_boardAdapter = &static_cast<EDA_3D_VIEWER_FRAME*>( frame )->GetAdapter();
        m_camera = &static_cast<EDA_3D_VIEWER_FRAME*>( frame )->GetCurrentCamera();
    }
    else if( wxWindow* previewWindow = dynamic_cast<wxWindow*>( holder ) )
    {
        wxCHECK( previewWindow->GetId() == PANEL_PREVIEW_3D_MODEL_ID, /* void */ );

        m_boardAdapter = &static_cast<PANEL_PREVIEW_3D_MODEL*>( holder )->GetAdapter();
        m_camera = &static_cast<PANEL_PREVIEW_3D_MODEL*>( holder )->GetCurrentCamera();
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
            wxWindow*     canvas = m_toolMgr->GetToolHolder()->GetToolCanvas();
            KIWAY_HOLDER* parent = dynamic_cast<KIWAY_HOLDER*>( wxGetTopLevelParent( canvas ) );

            if( parent && parent->GetType() == KIWAY_HOLDER::DIALOG )
            {
                DIALOG_SHIM* dialog = static_cast<DIALOG_SHIM*>( parent );

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
        else if( evt->IsMouseDown() )
        {
        }
        else if( evt->IsClick() && ( evt->Buttons() & BUT_RIGHT ) )
        {

            if( !m_canvas->m_mouse_was_moved )
                m_menu->ShowContextMenu();
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
    default:                    wxFAIL;                                               break;
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
    default:                  wxFAIL;                             break;
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

    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_toolMgr->GetToolHolder() );

    if( frame && frame->GetFrameType() == FRAME_PCB_DISPLAY3D )
        static_cast<EDA_3D_VIEWER_FRAME*>( frame )->NewDisplay( true );
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
    std::bitset<LAYER_3D_END> visibilityFlags = m_boardAdapter->GetVisibleLayers();
    APPEARANCE_CONTROLS_3D*   appearanceManager = nullptr;

    auto flipLayer =
            [&]( int layer )
            {
                appearanceManager->OnLayerVisibilityChanged( layer, !visibilityFlags.test( layer ) );
            };

    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_toolMgr->GetToolHolder() );

    if( frame && frame->GetFrameType() == FRAME_PCB_DISPLAY3D )
        appearanceManager = static_cast<EDA_3D_VIEWER_FRAME*>( frame )->GetAppearanceManager();

    if( appearanceManager )
    {
        if(      aEvent.IsAction( &EDA_3D_ACTIONS::showTHT ) )          flipLayer( LAYER_3D_TH_MODELS );
        else if( aEvent.IsAction( &EDA_3D_ACTIONS::showSMD ) )          flipLayer( LAYER_3D_SMD_MODELS );
        else if( aEvent.IsAction( &EDA_3D_ACTIONS::showVirtual ) )      flipLayer( LAYER_3D_VIRTUAL_MODELS );
        else if( aEvent.IsAction( &EDA_3D_ACTIONS::showNotInPosFile ) ) flipLayer( LAYER_3D_MODELS_NOT_IN_POS );
        else if( aEvent.IsAction( &EDA_3D_ACTIONS::showDNP ) )          flipLayer( LAYER_3D_MODELS_MARKED_DNP );
        else if( aEvent.IsAction( &EDA_3D_ACTIONS::showNavigator ) )    flipLayer( LAYER_3D_NAVIGATOR );
        else if( aEvent.IsAction( &EDA_3D_ACTIONS::showBBoxes ) )       flipLayer( LAYER_3D_BOUNDING_BOXES );
    }

    return 0;
}


int EDA_3D_CONTROLLER::ToggleLayersManager( const TOOL_EVENT& aEvent )
{
    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_toolMgr->GetToolHolder() );

    if( frame && frame->GetFrameType() == FRAME_PCB_DISPLAY3D )
        static_cast<EDA_3D_VIEWER_FRAME*>( frame )->ToggleAppearanceManager();

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


int EDA_3D_CONTROLLER::ReloadBoard( const TOOL_EVENT& aEvent )
{
    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_toolMgr->GetToolHolder() );

    if( frame && frame->GetFrameType() == FRAME_PCB_DISPLAY3D )
        static_cast<EDA_3D_VIEWER_FRAME*>( frame )->NewDisplay( true );

    return 0;
}


int EDA_3D_CONTROLLER::ToggleRaytracing( const TOOL_EVENT& aEvent )
{
    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_toolMgr->GetToolHolder() );

    if( frame && frame->GetFrameType() == FRAME_PCB_DISPLAY3D )
    {
        EDA_3D_VIEWER_FRAME* frame3d = static_cast<EDA_3D_VIEWER_FRAME*>( frame );

        RENDER_ENGINE& engine = frame3d->GetAdapter().m_Cfg->m_Render.engine;
        RENDER_ENGINE  old_engine = engine;

        if( old_engine == RENDER_ENGINE::OPENGL )
            engine = RENDER_ENGINE::RAYTRACING;
        else
            engine = RENDER_ENGINE::OPENGL;

        // Directly tell the canvas the rendering engine changed
        if( old_engine != engine )
            frame3d->GetCanvas()->RenderEngineChanged();
    }

    return 0;
}


int EDA_3D_CONTROLLER::ExportImage( const TOOL_EVENT& aEvent )
{
    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_toolMgr->GetToolHolder() );

    if( !frame || frame->GetFrameType() != FRAME_PCB_DISPLAY3D )
        return 0;

    EDA_3D_VIEWER_FRAME* viewer = static_cast<EDA_3D_VIEWER_FRAME*>( frame );
    EDA_3D_VIEWER_EXPORT_FORMAT fmt = aEvent.Parameter<EDA_3D_VIEWER_EXPORT_FORMAT>();

    wxSize currentSize = viewer->GetCanvas()->GetClientSize();

    if( fmt == EDA_3D_VIEWER_EXPORT_FORMAT::CLIPBOARD )
    {
        viewer->ExportImage( fmt, currentSize );
        return 0;
    }

    static wxSize lastSize( viewer->GetCanvas()->GetClientSize() );
    static EDA_3D_VIEWER_EXPORT_FORMAT lastFormat = EDA_3D_VIEWER_EXPORT_FORMAT::PNG;
    DIALOG_EXPORT_3D_IMAGE dlg( viewer, currentSize );

    if( dlg.ShowModal() == wxID_OK )
        viewer->ExportImage( lastFormat, dlg.GetSize() );

    return 0;
}


void EDA_3D_CONTROLLER::setTransitions()
{
    Go( &EDA_3D_CONTROLLER::Main,               EDA_3D_ACTIONS::controlActivate.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::UpdateMenu,         ACTIONS::updateMenu.MakeEvent() );

    // Miscellaneous control
    Go( &EDA_3D_CONTROLLER::ReloadBoard,        EDA_3D_ACTIONS::reloadBoard.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ToggleRaytracing,   EDA_3D_ACTIONS::toggleRaytacing.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ExportImage,        EDA_3D_ACTIONS::copyToClipboard.MakeEvent() );
    Go( &EDA_3D_CONTROLLER::ExportImage,        EDA_3D_ACTIONS::exportImage.MakeEvent() );

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
    Go( &EDA_3D_CONTROLLER::ToggleLayersManager,EDA_3D_ACTIONS::showLayersManager.MakeEvent() );
}


