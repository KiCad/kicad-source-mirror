/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_3D_raytracing_options.h"
#include <3d_canvas/board_adapter.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <3d_viewer/tools/eda_3d_controller.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>


PANEL_3D_RAYTRACING_OPTIONS::PANEL_3D_RAYTRACING_OPTIONS( EDA_3D_VIEWER_FRAME* aFrame,
                                                          wxWindow* aParent ) :
        PANEL_3D_RAYTRACING_OPTIONS_BASE( aParent ),
        m_settings( aFrame->GetAdapter() ),
        m_canvas( aFrame->GetCanvas() )
{
}


void PANEL_3D_RAYTRACING_OPTIONS::ResetPanel()
{
    m_settings.m_RtCameraLightColor = SFVEC3F( 0.2f );
    m_settings.m_RtLightColorTop = SFVEC3F( 0.247f );
    m_settings.m_RtLightColorBottom = SFVEC3F( 0.247f );

    const std::vector<int> default_elevation =
    {
        67,  67,  67,  67, -67, -67, -67, -67,
    };

    const std::vector<int> default_azimuth =
    {
        45, 135, 225, 315, 45, 135, 225, 315,
    };

    for( size_t i = 0; i < m_settings.m_RtLightSphericalCoords.size(); ++i )
    {
        m_settings.m_RtLightColor[i] = SFVEC3F( 0.168f );

        m_settings.m_RtLightSphericalCoords[i].x =
                ( (float) default_elevation[i] + 90.0f ) / 180.0f;

        m_settings.m_RtLightSphericalCoords[i].y = (float) default_azimuth[i] / 180.0f;
    }

    TransferColorDataToWindow();
}


void PANEL_3D_RAYTRACING_OPTIONS::TransferColorDataToWindow()
{
    auto transfer_color = [] ( const SFVEC3F& aSource, COLOR_SWATCH *aTarget )
    {
        aTarget->SetSupportsOpacity( false );
        aTarget->SetDefaultColor( KIGFX::COLOR4D( 0.5, 0.5, 0.5, 1.0 ) );
        aTarget->SetSwatchColor( COLOR4D( aSource.r, aSource.g, aSource.b, 1.0 ), false );
    };

    transfer_color( m_settings.m_RtCameraLightColor, m_colourPickerCameraLight );
    transfer_color( m_settings.m_RtLightColorTop, m_colourPickerTopLight );
    transfer_color( m_settings.m_RtLightColorBottom, m_colourPickerBottomLight );

    transfer_color( m_settings.m_RtLightColor[0], m_colourPickerLight1 );
    transfer_color( m_settings.m_RtLightColor[1], m_colourPickerLight2 );
    transfer_color( m_settings.m_RtLightColor[2], m_colourPickerLight3 );
    transfer_color( m_settings.m_RtLightColor[3], m_colourPickerLight4 );

    transfer_color( m_settings.m_RtLightColor[4], m_colourPickerLight5 );
    transfer_color( m_settings.m_RtLightColor[5], m_colourPickerLight6 );
    transfer_color( m_settings.m_RtLightColor[6], m_colourPickerLight7 );
    transfer_color( m_settings.m_RtLightColor[7], m_colourPickerLight8 );

    m_spinCtrlLightElevation1->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[0].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation2->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[1].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation3->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[2].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation4->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[3].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation5->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[4].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation6->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[5].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation7->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[6].x * 180.0f - 90.0f ) );
    m_spinCtrlLightElevation8->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[7].x * 180.0f - 90.0f ) );

    m_spinCtrlLightAzimuth1->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[0].y * 180.0f ) );
    m_spinCtrlLightAzimuth2->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[1].y * 180.0f ) );
    m_spinCtrlLightAzimuth3->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[2].y * 180.0f ) );
    m_spinCtrlLightAzimuth4->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[3].y * 180.0f ) );
    m_spinCtrlLightAzimuth5->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[4].y * 180.0f ) );
    m_spinCtrlLightAzimuth6->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[5].y * 180.0f ) );
    m_spinCtrlLightAzimuth7->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[6].y * 180.0f ) );
    m_spinCtrlLightAzimuth8->SetValue(
            (int)( m_settings.m_RtLightSphericalCoords[7].y * 180.0f ) );
}


bool PANEL_3D_RAYTRACING_OPTIONS::TransferDataToWindow()
{
    m_checkBoxRaytracing_renderShadows->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_SHADOWS ) );
    m_checkBoxRaytracing_addFloor->SetValue( m_settings.GetFlag( FL_RENDER_RAYTRACING_BACKFLOOR ) );
    m_checkBoxRaytracing_showRefractions->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_REFRACTIONS ) );
    m_checkBoxRaytracing_showReflections->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_REFLECTIONS ) );
    m_checkBoxRaytracing_postProcessing->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING ) );
    m_checkBoxRaytracing_antiAliasing->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING ) );
    m_checkBoxRaytracing_proceduralTextures->SetValue(
            m_settings.GetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES ) );

    m_spinCtrl_NrSamples_Shadows->SetValue( m_settings.m_RtShadowSampleCount );
    m_spinCtrl_NrSamples_Reflections->SetValue( m_settings.m_RtReflectionSampleCount );
    m_spinCtrl_NrSamples_Refractions->SetValue( m_settings.m_RtRefractionSampleCount );

    m_spinCtrlDouble_SpreadFactor_Shadows->SetValue( m_settings.m_RtSpreadShadows * 100.0f );
    m_spinCtrlDouble_SpreadFactor_Reflections->SetValue(
            m_settings.m_RtSpreadReflections * 100.0f );
    m_spinCtrlDouble_SpreadFactor_Refractions->SetValue(
            m_settings.m_RtSpreadRefractions * 100.0f );

    m_spinCtrlRecursiveLevel_Reflections->SetValue( m_settings.m_RtRecursiveReflectionCount );
    m_spinCtrlRecursiveLevel_Refractions->SetValue( m_settings.m_RtRecursiveRefractionCount );

    TransferColorDataToWindow();

    return true;
}


bool PANEL_3D_RAYTRACING_OPTIONS::TransferDataFromWindow()
{
    m_settings.SetFlag( FL_RENDER_RAYTRACING_SHADOWS,
                        m_checkBoxRaytracing_renderShadows->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_BACKFLOOR,
                        m_checkBoxRaytracing_addFloor->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_REFRACTIONS,
                        m_checkBoxRaytracing_showRefractions->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_REFLECTIONS,
                        m_checkBoxRaytracing_showReflections->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_POST_PROCESSING,
                        m_checkBoxRaytracing_postProcessing->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_ANTI_ALIASING,
                        m_checkBoxRaytracing_antiAliasing->GetValue() );
    m_settings.SetFlag( FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES,
                        m_checkBoxRaytracing_proceduralTextures->GetValue() );

    m_settings.m_RtShadowSampleCount = m_spinCtrl_NrSamples_Shadows->GetValue();
    m_settings.m_RtReflectionSampleCount = m_spinCtrl_NrSamples_Reflections->GetValue();
    m_settings.m_RtRefractionSampleCount= m_spinCtrl_NrSamples_Refractions->GetValue();

    m_settings.m_RtSpreadShadows =
            static_cast<float>( m_spinCtrlDouble_SpreadFactor_Shadows->GetValue() ) / 100.0f;
    m_settings.m_RtSpreadReflections =
            static_cast<float>( m_spinCtrlDouble_SpreadFactor_Reflections->GetValue() ) / 100.0f;
    m_settings.m_RtSpreadRefractions =
            static_cast<float>( m_spinCtrlDouble_SpreadFactor_Refractions->GetValue() ) / 100.0f;

    m_settings.m_RtRecursiveReflectionCount = m_spinCtrlRecursiveLevel_Reflections->GetValue();
    m_settings.m_RtRecursiveRefractionCount = m_spinCtrlRecursiveLevel_Refractions->GetValue();

    auto transfer_color = [] ( SFVEC3F& aTarget, COLOR_SWATCH *aSource )
    {
        const COLOR4D color = aSource->GetSwatchColor();

        aTarget = SFVEC3F( color.r, color.g, color.b );
    };

    transfer_color( m_settings.m_RtCameraLightColor, m_colourPickerCameraLight );
    transfer_color( m_settings.m_RtLightColorTop, m_colourPickerTopLight );
    transfer_color( m_settings.m_RtLightColorBottom, m_colourPickerBottomLight );

    transfer_color( m_settings.m_RtLightColor[0], m_colourPickerLight1 );
    transfer_color( m_settings.m_RtLightColor[1], m_colourPickerLight2 );
    transfer_color( m_settings.m_RtLightColor[2], m_colourPickerLight3 );
    transfer_color( m_settings.m_RtLightColor[3], m_colourPickerLight4 );
    transfer_color( m_settings.m_RtLightColor[4], m_colourPickerLight5 );
    transfer_color( m_settings.m_RtLightColor[5], m_colourPickerLight6 );
    transfer_color( m_settings.m_RtLightColor[6], m_colourPickerLight7 );
    transfer_color( m_settings.m_RtLightColor[7], m_colourPickerLight8 );

    m_settings.m_RtLightSphericalCoords[0].x =
            ( m_spinCtrlLightElevation1->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[1].x =
            ( m_spinCtrlLightElevation2->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[2].x =
            ( m_spinCtrlLightElevation3->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[3].x =
            ( m_spinCtrlLightElevation4->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[4].x =
            ( m_spinCtrlLightElevation5->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[5].x =
            ( m_spinCtrlLightElevation6->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[6].x =
            ( m_spinCtrlLightElevation7->GetValue() + 90.0f ) / 180.0f;
    m_settings.m_RtLightSphericalCoords[7].x =
            ( m_spinCtrlLightElevation8->GetValue() + 90.0f ) / 180.0f;

    m_settings.m_RtLightSphericalCoords[0].y = m_spinCtrlLightAzimuth1->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[1].y = m_spinCtrlLightAzimuth2->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[2].y = m_spinCtrlLightAzimuth3->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[3].y = m_spinCtrlLightAzimuth4->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[4].y = m_spinCtrlLightAzimuth5->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[5].y = m_spinCtrlLightAzimuth6->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[6].y = m_spinCtrlLightAzimuth7->GetValue() / 180.0f;
    m_settings.m_RtLightSphericalCoords[7].y = m_spinCtrlLightAzimuth8->GetValue() / 180.0f;

    for( size_t i = 0; i < m_settings.m_RtLightSphericalCoords.size(); ++i )
    {
        m_settings.m_RtLightSphericalCoords[i].x =
                glm::clamp( m_settings.m_RtLightSphericalCoords[i].x, 0.0f, 1.0f );

        m_settings.m_RtLightSphericalCoords[i].y =
                glm::clamp( m_settings.m_RtLightSphericalCoords[i].y, 0.0f, 2.0f );
    }

    return true;
}
