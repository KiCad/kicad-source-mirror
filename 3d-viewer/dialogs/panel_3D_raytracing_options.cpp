/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <eda_3d_viewer_settings.h>
#include <base_units.h>
#include "panel_3D_raytracing_options.h"


PANEL_3D_RAYTRACING_OPTIONS::PANEL_3D_RAYTRACING_OPTIONS( wxWindow* aParent ) :
        PANEL_3D_RAYTRACING_OPTIONS_BASE( aParent )
{
}


void PANEL_3D_RAYTRACING_OPTIONS::loadSettings( EDA_3D_VIEWER_SETTINGS* aCfg )
{
    m_cbRaytracing_renderShadows->SetValue( aCfg->m_Render.raytrace_shadows );
    m_cbRaytracing_addFloor->SetValue( aCfg->m_Render.raytrace_backfloor );
    m_cbRaytracing_showRefractions->SetValue( aCfg->m_Render.raytrace_refractions );
    m_cbRaytracing_showReflections->SetValue( aCfg->m_Render.raytrace_reflections );
    m_cbRaytracing_postProcessing->SetValue( aCfg->m_Render.raytrace_post_processing );
    m_cbRaytracing_antiAliasing->SetValue( aCfg->m_Render.raytrace_anti_aliasing );
    m_cbRaytracing_proceduralTextures->SetValue( aCfg->m_Render.raytrace_procedural_textures );

    m_numSamples_Shadows->SetValue( aCfg->m_Render.raytrace_nrsamples_shadows );
    m_numSamples_Reflections->SetValue( aCfg->m_Render.raytrace_nrsamples_reflections );
    m_numSamples_Refractions->SetValue( aCfg->m_Render.raytrace_nrsamples_refractions );

    m_spreadFactor_Shadows->SetValue( EDA_UNIT_UTILS::UI::StringFromValue(
            pcbIUScale, EDA_UNITS::PERCENT, aCfg->m_Render.raytrace_spread_shadows * 100.0f ) );
    m_spreadFactor_Reflections->SetValue( EDA_UNIT_UTILS::UI::StringFromValue(
            pcbIUScale, EDA_UNITS::PERCENT, aCfg->m_Render.raytrace_spread_reflections * 100.0f ) );
    m_spreadFactor_Refractions->SetValue( EDA_UNIT_UTILS::UI::StringFromValue(
            pcbIUScale, EDA_UNITS::PERCENT, aCfg->m_Render.raytrace_spread_refractions * 100.0f ) );

    m_recursiveLevel_Reflections->SetValue( aCfg->m_Render.raytrace_recursivelevel_reflections );
    m_recursiveLevel_Refractions->SetValue( aCfg->m_Render.raytrace_recursivelevel_refractions );

    auto transfer_color =
            []( COLOR4D aColor, COLOR_SWATCH *aTarget )
            {
                aTarget->SetSupportsOpacity( false );
                aTarget->SetDefaultColor( KIGFX::COLOR4D( 0.5, 0.5, 0.5, 1.0 ) );
                aTarget->SetSwatchColor( aColor, false );
            };

    auto transfer_value =
            []( wxTextCtrl* aCtrl, int aValue )
            {
        aCtrl->SetValue(
                EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, EDA_UNITS::UNSCALED, aValue ) );
            };

    transfer_color( aCfg->m_Render.raytrace_lightColorCamera, m_colourPickerCameraLight );
    transfer_color( aCfg->m_Render.raytrace_lightColorTop, m_colourPickerTopLight );
    transfer_color( aCfg->m_Render.raytrace_lightColorBottom, m_colourPickerBottomLight );

    transfer_color( aCfg->m_Render.raytrace_lightColor[0], m_colourPickerLight1 );
    transfer_color( aCfg->m_Render.raytrace_lightColor[1], m_colourPickerLight2 );
    transfer_color( aCfg->m_Render.raytrace_lightColor[2], m_colourPickerLight3 );
    transfer_color( aCfg->m_Render.raytrace_lightColor[3], m_colourPickerLight4 );

    transfer_color( aCfg->m_Render.raytrace_lightColor[4], m_colourPickerLight5 );
    transfer_color( aCfg->m_Render.raytrace_lightColor[5], m_colourPickerLight6 );
    transfer_color( aCfg->m_Render.raytrace_lightColor[6], m_colourPickerLight7 );
    transfer_color( aCfg->m_Render.raytrace_lightColor[7], m_colourPickerLight8 );

    transfer_value( m_lightElevation1, aCfg->m_Render.raytrace_lightElevation[0] );
    transfer_value( m_lightElevation2, aCfg->m_Render.raytrace_lightElevation[1] );
    transfer_value( m_lightElevation3, aCfg->m_Render.raytrace_lightElevation[2] );
    transfer_value( m_lightElevation4, aCfg->m_Render.raytrace_lightElevation[3] );
    transfer_value( m_lightElevation5, aCfg->m_Render.raytrace_lightElevation[4] );
    transfer_value( m_lightElevation6, aCfg->m_Render.raytrace_lightElevation[5] );
    transfer_value( m_lightElevation7, aCfg->m_Render.raytrace_lightElevation[6] );
    transfer_value( m_lightElevation8, aCfg->m_Render.raytrace_lightElevation[7] );

    transfer_value( m_lightAzimuth1, aCfg->m_Render.raytrace_lightAzimuth[0] );
    transfer_value( m_lightAzimuth2, aCfg->m_Render.raytrace_lightAzimuth[1] );
    transfer_value( m_lightAzimuth3, aCfg->m_Render.raytrace_lightAzimuth[2] );
    transfer_value( m_lightAzimuth4, aCfg->m_Render.raytrace_lightAzimuth[3] );
    transfer_value( m_lightAzimuth5, aCfg->m_Render.raytrace_lightAzimuth[4] );
    transfer_value( m_lightAzimuth6, aCfg->m_Render.raytrace_lightAzimuth[5] );
    transfer_value( m_lightAzimuth7, aCfg->m_Render.raytrace_lightAzimuth[6] );
    transfer_value( m_lightAzimuth8, aCfg->m_Render.raytrace_lightAzimuth[7] );
}


bool PANEL_3D_RAYTRACING_OPTIONS::TransferDataToWindow()
{
    loadSettings( GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) );
    return true;
}


bool PANEL_3D_RAYTRACING_OPTIONS::TransferDataFromWindow()
{
    if( EDA_3D_VIEWER_SETTINGS* cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" ) )
    {
        cfg->m_Render.raytrace_shadows = m_cbRaytracing_renderShadows->GetValue();
        cfg->m_Render.raytrace_backfloor = m_cbRaytracing_addFloor->GetValue();
        cfg->m_Render.raytrace_refractions = m_cbRaytracing_showRefractions->GetValue();
        cfg->m_Render.raytrace_reflections = m_cbRaytracing_showReflections->GetValue();
        cfg->m_Render.raytrace_post_processing = m_cbRaytracing_postProcessing->GetValue();
        cfg->m_Render.raytrace_anti_aliasing = m_cbRaytracing_antiAliasing->GetValue();
        cfg->m_Render.raytrace_procedural_textures = m_cbRaytracing_proceduralTextures->GetValue();

        cfg->m_Render.raytrace_nrsamples_shadows = m_numSamples_Shadows->GetValue();
        cfg->m_Render.raytrace_nrsamples_reflections = m_numSamples_Reflections->GetValue();
        cfg->m_Render.raytrace_nrsamples_refractions = m_numSamples_Refractions->GetValue();

        cfg->m_Render.raytrace_spread_shadows =
                EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::PERCENT,
                                                           m_spreadFactor_Shadows->GetValue() ) / 100.0f;
        cfg->m_Render.raytrace_spread_reflections =
                EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::PERCENT,
                                                           m_spreadFactor_Reflections->GetValue() ) / 100.0f;
        cfg->m_Render.raytrace_spread_refractions =
                EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::PERCENT,
                                                           m_spreadFactor_Refractions->GetValue() ) / 100.0f;

        cfg->m_Render.raytrace_recursivelevel_reflections = m_recursiveLevel_Reflections->GetValue();
        cfg->m_Render.raytrace_recursivelevel_refractions = m_recursiveLevel_Refractions->GetValue();

        cfg->m_Render.raytrace_lightColorCamera = m_colourPickerCameraLight->GetSwatchColor();
        cfg->m_Render.raytrace_lightColorTop = m_colourPickerTopLight->GetSwatchColor();
        cfg->m_Render.raytrace_lightColorBottom = m_colourPickerBottomLight->GetSwatchColor();

        cfg->m_Render.raytrace_lightColor[0] = m_colourPickerLight1->GetSwatchColor();
        cfg->m_Render.raytrace_lightColor[1] = m_colourPickerLight2->GetSwatchColor();
        cfg->m_Render.raytrace_lightColor[2] = m_colourPickerLight3->GetSwatchColor();
        cfg->m_Render.raytrace_lightColor[3] = m_colourPickerLight4->GetSwatchColor();
        cfg->m_Render.raytrace_lightColor[4] = m_colourPickerLight5->GetSwatchColor();
        cfg->m_Render.raytrace_lightColor[5] = m_colourPickerLight6->GetSwatchColor();
        cfg->m_Render.raytrace_lightColor[6] = m_colourPickerLight7->GetSwatchColor();
        cfg->m_Render.raytrace_lightColor[7] = m_colourPickerLight8->GetSwatchColor();

        auto get_value =
                []( wxTextCtrl* aCtrl )
                {
                    return EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::UNSCALED,
                                                                      aCtrl->GetValue() );
                };

        cfg->m_Render.raytrace_lightElevation[0] = get_value( m_lightElevation1 );
        cfg->m_Render.raytrace_lightElevation[1] = get_value( m_lightElevation2 );
        cfg->m_Render.raytrace_lightElevation[2] = get_value( m_lightElevation3 );
        cfg->m_Render.raytrace_lightElevation[3] = get_value( m_lightElevation4 );
        cfg->m_Render.raytrace_lightElevation[4] = get_value( m_lightElevation5 );
        cfg->m_Render.raytrace_lightElevation[5] = get_value( m_lightElevation6 );
        cfg->m_Render.raytrace_lightElevation[6] = get_value( m_lightElevation7 );
        cfg->m_Render.raytrace_lightElevation[7] = get_value( m_lightElevation8 );

        cfg->m_Render.raytrace_lightAzimuth[0] = get_value( m_lightAzimuth1 );
        cfg->m_Render.raytrace_lightAzimuth[1] = get_value( m_lightAzimuth2 );
        cfg->m_Render.raytrace_lightAzimuth[2] = get_value( m_lightAzimuth3 );
        cfg->m_Render.raytrace_lightAzimuth[3] = get_value( m_lightAzimuth4 );
        cfg->m_Render.raytrace_lightAzimuth[4] = get_value( m_lightAzimuth5 );
        cfg->m_Render.raytrace_lightAzimuth[5] = get_value( m_lightAzimuth6 );
        cfg->m_Render.raytrace_lightAzimuth[6] = get_value( m_lightAzimuth7 );
        cfg->m_Render.raytrace_lightAzimuth[7] = get_value( m_lightAzimuth8 );
    }

    return true;
}


void PANEL_3D_RAYTRACING_OPTIONS::ResetPanel()
{
    EDA_3D_VIEWER_SETTINGS cfg;
    cfg.Load();                     // Loading without a file will init to defaults

    loadSettings( &cfg );
}