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

#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <eda_3d_viewer.h>
#include <id.h>
#include <kiface_i.h>
#include <3d_viewer_id.h>
#include "3d_viewer_control.h"
#include "3d_actions.h"


bool EDA_3D_VIEWER_CONTROL::Init()
{
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    ctxMenu.AddItem( ACTIONS::zoomIn,             SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::zoomOut,            SELECTION_CONDITIONS::ShowAlways );

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
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveLeft,    SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveRight,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveUp,      SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( EDA_3D_ACTIONS::moveDown,    SELECTION_CONDITIONS::ShowAlways );

    return true;
}


void EDA_3D_VIEWER_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_3D_VIEWER>();
}


int EDA_3D_VIEWER_CONTROL::UpdateMenu( const TOOL_EVENT& aEvent )
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


int EDA_3D_VIEWER_CONTROL::Main( const TOOL_EVENT& aEvent )
{
    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( evt->IsClick( BUT_RIGHT ) )
            m_menu.ShowContextMenu();
        else
            evt->SetPassEvent();
    }

    return 0;
}


int EDA_3D_VIEWER_CONTROL::ViewControl( const TOOL_EVENT& aEvent )
{
    m_frame->GetCanvas()->SetView3D( aEvent.Parameter<intptr_t>() );

    return 0;
}


int EDA_3D_VIEWER_CONTROL::PanControl( const TOOL_EVENT& aEvent )
{
    switch( aEvent.Parameter<intptr_t>() )
    {
    case ACTIONS::CURSOR_UP:    m_frame->GetCanvas()->SetView3D( WXK_UP );    break;
    case ACTIONS::CURSOR_DOWN:  m_frame->GetCanvas()->SetView3D( WXK_DOWN );  break;
    case ACTIONS::CURSOR_LEFT:  m_frame->GetCanvas()->SetView3D( WXK_LEFT );  break;
    case ACTIONS::CURSOR_RIGHT: m_frame->GetCanvas()->SetView3D( WXK_RIGHT ); break;
    default:                    wxFAIL;                                       break;
    }

    return 0;
}


#define ROT_ANGLE 10.0

int EDA_3D_VIEWER_CONTROL::RotateView( const TOOL_EVENT& aEvent )
{
    CINFO3D_VISU& settings = m_frame->GetSettings();

    switch( aEvent.Parameter<intptr_t>() )
    {
    case ID_ROTATE3D_X_NEG: settings.CameraGet().RotateX( -glm::radians( ROT_ANGLE ) ); break;
    case ID_ROTATE3D_X_POS: settings.CameraGet().RotateX(  glm::radians( ROT_ANGLE ) ); break;
    case ID_ROTATE3D_Y_NEG: settings.CameraGet().RotateY( -glm::radians( ROT_ANGLE ) ); break;
    case ID_ROTATE3D_Y_POS: settings.CameraGet().RotateY(  glm::radians( ROT_ANGLE ) ); break;
    case ID_ROTATE3D_Z_NEG: settings.CameraGet().RotateZ( -glm::radians( ROT_ANGLE ) ); break;
    case ID_ROTATE3D_Z_POS: settings.CameraGet().RotateZ(  glm::radians( ROT_ANGLE ) ); break;
    default:                wxFAIL;                                                     break;
    }

    if( settings.RenderEngineGet() == RENDER_ENGINE::OPENGL_LEGACY )
        m_frame->GetCanvas()->Request_refresh();
    else
        m_frame->GetCanvas()->RenderRaytracingRequest();

    return 0;
}


int EDA_3D_VIEWER_CONTROL::ToggleOrtho( const TOOL_EVENT& aEvent )
{
    m_frame->GetSettings().CameraGet().ToggleProjection();

    if( m_frame->GetSettings().RenderEngineGet() == RENDER_ENGINE::OPENGL_LEGACY )
        m_frame->GetCanvas()->Request_refresh();
    else
        m_frame->GetCanvas()->RenderRaytracingRequest();

    return 0;
}


int EDA_3D_VIEWER_CONTROL::ToggleVisibility( const TOOL_EVENT& aEvent )
{
    DISPLAY3D_FLG flag = aEvent.Parameter<DISPLAY3D_FLG>();
    CINFO3D_VISU& settings = m_frame->GetSettings();

    settings.SetFlag( flag, !settings.GetFlag( flag ) );

    switch( flag )
    {
    case FL_RENDER_OPENGL_SHOW_MODEL_BBOX:
    case FL_RENDER_RAYTRACING_SHADOWS:
    case FL_RENDER_RAYTRACING_REFRACTIONS:
    case FL_RENDER_RAYTRACING_REFLECTIONS:
    case FL_RENDER_RAYTRACING_ANTI_ALIASING:
    case FL_AXIS:
        m_frame->GetCanvas()->Request_refresh();
        break;
    default:
        m_frame->NewDisplay( true );
        break;
    }

    return 0;
}


int EDA_3D_VIEWER_CONTROL::On3DGridSelection( const TOOL_EVENT& aEvent )
{
    GRID3D_TYPE grid = aEvent.Parameter<GRID3D_TYPE>();
    m_frame->GetSettings().GridSet( grid );

    if( m_frame->GetCanvas() )
        m_frame->GetCanvas()->Request_refresh();

    return 0;
}


int EDA_3D_VIEWER_CONTROL::ZoomRedraw( const TOOL_EVENT& aEvent )
{
    m_frame->GetCanvas()->Request_refresh();
    return 0;
}


int EDA_3D_VIEWER_CONTROL::ZoomInOut( const TOOL_EVENT& aEvent )
{
    bool direction = aEvent.IsAction( &ACTIONS::zoomIn );
    return doZoomInOut( direction, true );
}


int EDA_3D_VIEWER_CONTROL::ZoomInOutCenter( const TOOL_EVENT& aEvent )
{
    bool direction = aEvent.IsAction( &ACTIONS::zoomInCenter );
    return doZoomInOut( direction, false );
}


int EDA_3D_VIEWER_CONTROL::doZoomInOut( bool aDirection, bool aCenterOnCursor )
{
    if( m_frame->GetCanvas() )
    {
        m_frame->GetCanvas()->SetView3D( aDirection ? WXK_F1 : WXK_F2 );
        m_frame->GetCanvas()->DisplayStatus();
    }

    return 0;
}


int EDA_3D_VIEWER_CONTROL::ZoomFitScreen( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetCanvas() )
    {
        m_frame->GetCanvas()->SetView3D( WXK_HOME );
        m_frame->GetCanvas()->DisplayStatus();
    }

    return 0;
}


void EDA_3D_VIEWER_CONTROL::setTransitions()
{
    Go( &EDA_3D_VIEWER_CONTROL::Main,               EDA_3D_ACTIONS::controlActivate.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::UpdateMenu,         ACTIONS::updateMenu.MakeEvent() );

    // Pan control
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         ACTIONS::panUp.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         ACTIONS::panDown.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         ACTIONS::panLeft.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         ACTIONS::panRight.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         EDA_3D_ACTIONS::moveUp.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         EDA_3D_ACTIONS::moveDown.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         EDA_3D_ACTIONS::moveLeft.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::PanControl,         EDA_3D_ACTIONS::moveRight.MakeEvent() );

    // View rotation
    Go( &EDA_3D_VIEWER_CONTROL::ViewControl,        EDA_3D_ACTIONS::viewTop.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ViewControl,        EDA_3D_ACTIONS::viewBottom.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ViewControl,        EDA_3D_ACTIONS::viewLeft.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ViewControl,        EDA_3D_ACTIONS::viewRight.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ViewControl,        EDA_3D_ACTIONS::viewFront.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ViewControl,        EDA_3D_ACTIONS::viewBack.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::RotateView,         EDA_3D_ACTIONS::rotateXCW.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::RotateView,         EDA_3D_ACTIONS::rotateXCCW.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::RotateView,         EDA_3D_ACTIONS::rotateYCW.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::RotateView,         EDA_3D_ACTIONS::rotateYCCW.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::RotateView,         EDA_3D_ACTIONS::rotateZCW.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::RotateView,         EDA_3D_ACTIONS::rotateZCCW.MakeEvent() );

    // Zoom control
    Go( &EDA_3D_VIEWER_CONTROL::ZoomRedraw,         ACTIONS::zoomRedraw.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ZoomInOut,          ACTIONS::zoomIn.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ZoomInOut,          ACTIONS::zoomOut.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ZoomInOutCenter,    ACTIONS::zoomInCenter.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ZoomInOutCenter,    ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ZoomFitScreen,      ACTIONS::zoomFitScreen.MakeEvent() );

    // Grid
    Go( &EDA_3D_VIEWER_CONTROL::On3DGridSelection,  EDA_3D_ACTIONS::noGrid.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::On3DGridSelection,  EDA_3D_ACTIONS::show10mmGrid.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::On3DGridSelection,  EDA_3D_ACTIONS::show5mmGrid.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::On3DGridSelection,  EDA_3D_ACTIONS::show2_5mmGrid.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::On3DGridSelection,  EDA_3D_ACTIONS::show1mmGrid.MakeEvent() );

    // Visibility
    Go( &EDA_3D_VIEWER_CONTROL::ToggleOrtho,        EDA_3D_ACTIONS::toggleOrtho.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::attributesTHT.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::attributesSMD.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::attributesVirtual.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::showCopperThickness.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::showBoundingBoxes.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::renderShadows.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::proceduralTextures.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::addFloor.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::showRefractions.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::showReflections.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::antiAliasing.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::postProcessing.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleRealisticMode.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleBoardBody.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::showAxis.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleZones.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleAdhesive.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleSilk.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleSolderMask.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleSolderPaste.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleComments.MakeEvent() );
    Go( &EDA_3D_VIEWER_CONTROL::ToggleVisibility,   EDA_3D_ACTIONS::toggleECO.MakeEvent() );
}


